#include <BAN/StringView.h>
#include <kernel/CriticalScope.h>
#include <kernel/FS/VirtualFileSystem.h>
#include <kernel/LockGuard.h>
#include <kernel/Memory/Heap.h>
#include <kernel/Memory/MMUScope.h>
#include <kernel/Process.h>
#include <kernel/Scheduler.h>
#include <LibELF/ELF.h>
#include <LibELF/Values.h>

#include <fcntl.h>

namespace Kernel
{

	static BAN::Vector<Process*> s_processes;
	static SpinLock s_process_lock;

	Process* Process::create_process()
	{
		static pid_t s_next_pid = 1;
		auto* process = new Process(s_next_pid++);
		ASSERT(process);
		return process;
	}

	void Process::register_process(Process* process)
	{
		s_process_lock.lock();
		MUST(s_processes.push_back(process));
		s_process_lock.unlock();
		
		for (auto* thread : process->m_threads)
			MUST(Scheduler::get().add_thread(thread));
	}

	Process* Process::create_kernel(entry_t entry, void* data)
	{
		auto* process = create_process();
		MUST(process->m_working_directory.push_back('/'));
		auto* thread = MUST(Thread::create(entry, data, process));
		process->add_thread(thread);
		register_process(process);
		return process;
	}

	BAN::ErrorOr<Process*> Process::create_userspace(BAN::StringView path)
	{
		auto* elf = TRY(LibELF::ELF::load_from_file(path));	
		if (!elf->is_native())
		{
			derrorln("ELF has invalid architecture");
			return BAN::Error::from_errno(EINVAL);
		}

		auto* process = create_process();
		MUST(process->m_working_directory.push_back('/'));
		MUST(process->init_stdio());
		process->m_mmu = new MMU();
		ASSERT(process->m_mmu);
		
		auto& elf_file_header = elf->file_header_native();
		for (size_t i = 0; i < elf_file_header.e_phnum; i++)
		{
			auto& elf_program_header = elf->program_header_native(i);

			switch (elf_program_header.p_type)
			{
			case LibELF::PT_NULL:
				break;
			case LibELF::PT_LOAD:
			{
				// TODO: Do some relocations or map kernel to higher half?
				ASSERT(process->mmu().is_range_free(elf_program_header.p_vaddr, elf_program_header.p_memsz));

				MMU::flags_t flags = MMU::Flags::UserSupervisor | MMU::Flags::Present;
				if (elf_program_header.p_flags & LibELF::PF_W)
					flags |= MMU::Flags::ReadWrite;
				size_t page_start = elf_program_header.p_vaddr / PAGE_SIZE;
				size_t page_end = BAN::Math::div_round_up<size_t>(elf_program_header.p_vaddr + elf_program_header.p_memsz, PAGE_SIZE);
				MUST(process->m_allocated_pages.reserve(page_end - page_start + 1));
				for (size_t page = page_start; page <= page_end; page++)
				{
					auto paddr = Heap::get().take_free_page();
					MUST(process->m_allocated_pages.push_back(paddr));
					process->mmu().map_page_at(paddr, page * PAGE_SIZE, flags);
				}

				{
					MMUScope _(process->mmu());
					memcpy((void*)elf_program_header.p_vaddr, elf->data() + elf_program_header.p_offset, elf_program_header.p_filesz);
					memset((void*)(elf_program_header.p_vaddr + elf_program_header.p_filesz), 0, elf_program_header.p_memsz - elf_program_header.p_filesz);
				}
				break;
			}
			default:
				ASSERT_NOT_REACHED();
			}
		}

		char** argv = nullptr;
		{
			MMUScope _(process->mmu());
			argv = (char**)MUST(process->allocate(sizeof(char**) * 1));
			argv[0] = (char*)MUST(process->allocate(path.size() + 1));
			memcpy(argv[0], path.data(), path.size());
			argv[0][path.size()] = '\0';
		}

		auto* thread = MUST(Thread::create_userspace(elf_file_header.e_entry, process, 1, argv));
		process->add_thread(thread);

		delete elf;

		register_process(process);
		return process;
	}

	Process::Process(pid_t pid)
		: m_pid(pid)
		, m_tty(TTY::current())
	{ }

	Process::~Process()
	{
		ASSERT(m_threads.empty());
		ASSERT(m_fixed_width_allocators.empty());
		ASSERT(m_general_allocator == nullptr);
		if (m_mmu)
		{
			MMU::get().load();
			delete m_mmu;
		}
		for (auto paddr : m_allocated_pages)
			Heap::get().release_page(paddr);
	}

	void Process::add_thread(Thread* thread)
	{
		LockGuard _(m_lock);
		MUST(m_threads.push_back(thread));
	}

	void Process::on_thread_exit(Thread& thread)
	{
		LockGuard _(m_lock);
		for (size_t i = 0; i < m_threads.size(); i++)
			if (m_threads[i] == &thread)
				m_threads.remove(i);
		if (m_threads.empty())
			exit();
	}

	void Process::exit()
	{
		m_lock.lock();
		m_threads.clear();
		for (auto& open_fd : m_open_files)
			open_fd.inode = nullptr;

		// NOTE: We must clear allocators while the mmu is still alive
		m_fixed_width_allocators.clear();
		if (m_general_allocator)
		{
			delete m_general_allocator;
			m_general_allocator = nullptr;
		}

		dprintln("process {} exit", pid());
		s_process_lock.lock();
		for (size_t i = 0; i < s_processes.size(); i++)
			if (s_processes[i] == this)
				s_processes.remove(i);
		s_process_lock.unlock();

		// FIXME: we can't assume this is the current process
		ASSERT(&Process::current() == this);
		Scheduler::get().set_current_process_done();
	}

	BAN::ErrorOr<void> Process::init_stdio()
	{
		ASSERT(m_open_files.empty());
		TRY(open("/dev/tty1", O_RDONLY)); // stdin
		TRY(open("/dev/tty1", O_WRONLY)); // stdout
		TRY(open("/dev/tty1", O_WRONLY)); // stderr
		return {};
	}

	BAN::ErrorOr<void> Process::set_termios(const termios& termios)
	{
		if (m_tty == nullptr)
			return BAN::Error::from_errno(ENOTTY);
		m_tty->set_termios(termios);
		return {};
	}

	BAN::ErrorOr<int> Process::open(BAN::StringView path, int flags)
	{
		if (flags & ~O_RDWR)
			return BAN::Error::from_errno(ENOTSUP);

		BAN::String absolute_path = TRY(absolute_path_of(path));

		auto file = TRY(VirtualFileSystem::get().file_from_absolute_path(absolute_path));

		LockGuard _(m_lock);
		int fd = TRY(get_free_fd());
		auto& open_file_description = m_open_files[fd];
		open_file_description.inode = file.inode;
		open_file_description.path = BAN::move(file.canonical_path);
		open_file_description.flags = flags;

		return fd;
	}

	BAN::ErrorOr<void> Process::close(int fd)
	{
		LockGuard _(m_lock);
		TRY(validate_fd(fd));
		auto& open_file_description = this->open_file_description(fd);
		open_file_description.inode = nullptr;
		return {};
	}

	BAN::ErrorOr<size_t> Process::read(int fd, void* buffer, size_t offset, size_t count)
	{
		OpenFileDescription open_fd_copy;

		{
			LockGuard _(m_lock);
			TRY(validate_fd(fd));
			open_fd_copy = open_file_description(fd);
		}

		if (!(open_fd_copy.flags & O_RDONLY))
			return BAN::Error::from_errno(EBADF);
		return TRY(open_fd_copy.inode->read(offset, buffer, count));
	}

	BAN::ErrorOr<size_t> Process::write(int fd, const void* buffer, size_t offset, size_t count)
	{
		OpenFileDescription open_fd_copy;

		{
			LockGuard _(m_lock);
			TRY(validate_fd(fd));
			open_fd_copy = open_file_description(fd);
		}

		if (!(open_fd_copy.flags & O_WRONLY))
			return BAN::Error::from_errno(EBADF);
		return TRY(open_fd_copy.inode->write(offset, buffer, count));
	}

	BAN::ErrorOr<void> Process::creat(BAN::StringView path, mode_t mode)
	{
		auto absolute_path = TRY(absolute_path_of(path));

		size_t index;
		for (index = absolute_path.size(); index > 0; index--)
			if (absolute_path[index - 1] == '/')
				break;

		auto directory = absolute_path.sv().substring(0, index);
		auto file_name = absolute_path.sv().substring(index);

		auto parent_file = TRY(VirtualFileSystem::get().file_from_absolute_path(directory));
		TRY(parent_file.inode->create_file(file_name, mode));

		return {};
	}

	BAN::ErrorOr<void> Process::mount(BAN::StringView source, BAN::StringView target)
	{
		auto absolute_source = TRY(absolute_path_of(source));
		auto absolute_target = TRY(absolute_path_of(target));
		TRY(VirtualFileSystem::get().mount(absolute_source, absolute_target));
		return {};
	}

	BAN::ErrorOr<void> Process::fstat(int fd, struct stat* out)
	{
		OpenFileDescription open_fd_copy;

		{
			LockGuard _(m_lock);
			TRY(validate_fd(fd));
			open_fd_copy = open_file_description(fd);
		}

		out->st_dev		= open_fd_copy.inode->dev();
		out->st_ino		= open_fd_copy.inode->ino();
		out->st_mode	= open_fd_copy.inode->mode().mode;
		out->st_nlink	= open_fd_copy.inode->nlink();
		out->st_uid		= open_fd_copy.inode->uid();
		out->st_gid		= open_fd_copy.inode->gid();
		out->st_rdev	= open_fd_copy.inode->rdev();
		out->st_size	= open_fd_copy.inode->size();
		out->st_atim	= open_fd_copy.inode->atime();
		out->st_mtim	= open_fd_copy.inode->mtime();
		out->st_ctim	= open_fd_copy.inode->ctime();
		out->st_blksize	= open_fd_copy.inode->blksize();
		out->st_blocks	= open_fd_copy.inode->blocks();

		return {};
	}

	BAN::ErrorOr<void> Process::stat(BAN::StringView path, struct stat* out)
	{
		int fd = TRY(open(path, O_RDONLY));
		auto ret = fstat(fd, out);
		MUST(close(fd));
		return ret;
	}

	// FIXME: This whole API has to be rewritten
	BAN::ErrorOr<BAN::Vector<BAN::String>> Process::read_directory_entries(int fd)
	{
		OpenFileDescription open_fd_copy;

		{
			LockGuard _(m_lock);
			TRY(validate_fd(fd));
			open_fd_copy = open_file_description(fd);
		}

		return TRY(open_fd_copy.inode->read_directory_entries(0));
	}

	BAN::ErrorOr<BAN::String> Process::working_directory() const
	{
		BAN::String result;

		LockGuard _(m_lock);
		TRY(result.append(m_working_directory));
		
		return result;
	}

	BAN::ErrorOr<void> Process::set_working_directory(BAN::StringView path)
	{
		BAN::String absolute_path = TRY(absolute_path_of(path));

		auto file = TRY(VirtualFileSystem::get().file_from_absolute_path(absolute_path));
		if (!file.inode->mode().ifdir())
			return BAN::Error::from_errno(ENOTDIR);

		LockGuard _(m_lock);
		m_working_directory = BAN::move(file.canonical_path);

		return {};
	}

	static constexpr uint16_t next_power_of_two(uint16_t value)
	{
		value--;
		value |= value >> 1;
		value |= value >> 2;
		value |= value >> 4;
		value |= value >> 8;
		return value + 1;
	}

	BAN::ErrorOr<void*> Process::allocate(size_t bytes)
	{
		vaddr_t address = 0;

		if (bytes <= PAGE_SIZE)
		{
			// Do fixed width allocation
			size_t allocation_size = next_power_of_two(bytes);
			ASSERT(bytes <= allocation_size);

			LockGuard _(m_lock);

			bool needs_new_allocator { true };

			for (auto& allocator : m_fixed_width_allocators)
			{
				if (allocator.allocation_size() == allocation_size && allocator.allocations() < allocator.max_allocations())
				{
					address = allocator.allocate();
					needs_new_allocator = false;
				}
			}

			if (needs_new_allocator)
			{
				TRY(m_fixed_width_allocators.emplace_back(mmu(), allocation_size));
				address = m_fixed_width_allocators.back().allocate();
			}
		}
		else
		{
			LockGuard _(m_lock);

			if (!m_general_allocator)
			{
				m_general_allocator = new GeneralAllocator(mmu());
				if (m_general_allocator == nullptr)
					return BAN::Error::from_errno(ENOMEM);
			}

			address = m_general_allocator->allocate(bytes);
		}

		if (address == 0)
			return BAN::Error::from_errno(ENOMEM);
		return (void*)address;
	}

	void Process::free(void* ptr)
	{
		LockGuard _(m_lock);

		for (auto it = m_fixed_width_allocators.begin(); it != m_fixed_width_allocators.end(); it++)
		{
			if (it->deallocate((vaddr_t)ptr))
			{
				// TODO: This might be too much. Maybe we should only
				//       remove allocators when we have low memory... ?
				if (it->allocations() == 0)
					m_fixed_width_allocators.remove(it);
				return;
			}
		}

		if (m_general_allocator && m_general_allocator->deallocate((vaddr_t)ptr))
			return;

		dwarnln("free called on pointer that was not allocated");	
	}

	void Process::termid(char* buffer) const
	{
		if (m_tty == nullptr)
			buffer[0] = '\0';
		strcpy(buffer, "/dev/");
		strcpy(buffer + 5, m_tty->name().data());
	}

	BAN::ErrorOr<BAN::String> Process::absolute_path_of(BAN::StringView path) const
	{
		if (path.empty())
			return working_directory();
		BAN::String absolute_path;
		if (path.front() != '/')
		{
			LockGuard _(m_lock);
			TRY(absolute_path.append(m_working_directory));
		}
		if (!absolute_path.empty() && absolute_path.back() != '/')
			TRY(absolute_path.push_back('/'));
		TRY(absolute_path.append(path));
		return absolute_path;
	}

	BAN::ErrorOr<void> Process::validate_fd(int fd)
	{
		ASSERT(m_lock.is_locked());
		if (fd < 0 || m_open_files.size() <= (size_t)fd || !m_open_files[fd].inode)
			return BAN::Error::from_errno(EBADF);
		return {};
	}

	Process::OpenFileDescription& Process::open_file_description(int fd)
	{
		ASSERT(m_lock.is_locked());
		MUST(validate_fd(fd));
		return m_open_files[fd];
	}

	BAN::ErrorOr<int> Process::get_free_fd()
	{
		ASSERT(m_lock.is_locked());
		for (size_t fd = 0; fd < m_open_files.size(); fd++)
			if (!m_open_files[fd].inode)
				return fd;
		TRY(m_open_files.push_back({}));
		return m_open_files.size() - 1;
	}

}