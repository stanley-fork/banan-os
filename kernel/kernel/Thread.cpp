#include <BAN/Errors.h>
#include <BAN/ScopeGuard.h>
#include <kernel/GDT.h>
#include <kernel/InterruptController.h>
#include <kernel/InterruptStack.h>
#include <kernel/Memory/kmalloc.h>
#include <kernel/Process.h>
#include <kernel/Scheduler.h>
#include <kernel/Thread.h>
#include <kernel/Timer/Timer.h>

namespace Kernel
{

	extern "C" [[noreturn]] void start_kernel_thread();
	extern "C" [[noreturn]] void start_userspace_thread();

	extern "C" void signal_trampoline();

	template<typename T>
	static void write_to_stack(uintptr_t& rsp, const T& value) requires(sizeof(T) <= sizeof(uintptr_t))
	{
		rsp -= sizeof(uintptr_t);
		*(uintptr_t*)rsp = (uintptr_t)value;
	}

	extern "C" uintptr_t get_thread_start_sp()
	{
		return Thread::current().interrupt_stack().sp;
	}

	extern "C" uintptr_t get_userspace_thread_stack_top()
	{
		return Thread::current().userspace_stack_top() - 4 * sizeof(uintptr_t);
	}

	extern "C" void load_thread_sse()
	{
		Thread::current().load_sse();
	}

	static pid_t s_next_tid = 1;

	alignas(16) static uint8_t s_default_sse_storage[512];
	static bool s_default_sse_storage_initialized = false;

	static void initialize_default_sse_storage()
	{
		const uint32_t mxcsr = 0x1F80;
		asm volatile(
			"finit;"
			"ldmxcsr %[mxcsr];"
			"fxsave %[storage];"
			: [storage]"=m"(s_default_sse_storage)
			: [mxcsr]"m"(mxcsr)
		);
	}

	BAN::ErrorOr<Thread*> Thread::create_kernel(entry_t entry, void* data, Process* process)
	{
		// Create the thread object
		Thread* thread = new Thread(s_next_tid++, process);
		if (thread == nullptr)
			return BAN::Error::from_errno(ENOMEM);
		BAN::ScopeGuard thread_deleter([thread] { delete thread; });

		// Initialize stack and registers
		thread->m_kernel_stack = TRY(VirtualRange::create_to_vaddr_range(
			PageTable::kernel(),
			KERNEL_OFFSET,
			~(uintptr_t)0,
			kernel_stack_size,
			PageTable::Flags::ReadWrite | PageTable::Flags::Present,
			true
		));

		// Initialize stack for returning
		uintptr_t sp = thread->kernel_stack_top();
		write_to_stack(sp, thread);
		write_to_stack(sp, &Thread::on_exit_trampoline);
		write_to_stack(sp, data);
		write_to_stack(sp, entry);

		thread->m_interrupt_stack.ip = reinterpret_cast<vaddr_t>(start_kernel_thread);
		thread->m_interrupt_stack.cs = 0x08;
		thread->m_interrupt_stack.flags = 0x002;
		thread->m_interrupt_stack.sp = sp;
		thread->m_interrupt_stack.ss = 0x10;

		memset(&thread->m_interrupt_registers, 0, sizeof(InterruptRegisters));

		thread_deleter.disable();

		return thread;
	}

	BAN::ErrorOr<Thread*> Thread::create_userspace(Process* process, PageTable& page_table)
	{
		ASSERT(process);

		// Create the thread object
		Thread* thread = new Thread(s_next_tid++, process);
		if (thread == nullptr)
			return BAN::Error::from_errno(ENOMEM);
		BAN::ScopeGuard thread_deleter([thread] { delete thread; });

		thread->m_is_userspace = true;

		thread->m_kernel_stack = TRY(VirtualRange::create_to_vaddr_range(
			page_table,
			0x200000, KERNEL_OFFSET,
			kernel_stack_size,
			PageTable::Flags::ReadWrite | PageTable::Flags::Present,
			true
		));

		thread->m_userspace_stack = TRY(VirtualRange::create_to_vaddr_range(
			page_table,
			0x200000, KERNEL_OFFSET,
			userspace_stack_size,
			PageTable::Flags::UserSupervisor | PageTable::Flags::ReadWrite | PageTable::Flags::Present,
			true
		));

		thread_deleter.disable();

		return thread;
	}

	Thread::Thread(pid_t tid, Process* process)
		: m_tid(tid), m_process(process)
	{
		if (!s_default_sse_storage_initialized)
		{
			initialize_default_sse_storage();
			s_default_sse_storage_initialized = true;
		}
		memcpy(m_sse_storage, s_default_sse_storage, sizeof(m_sse_storage));
	}

	Thread& Thread::current()
	{
		return Processor::scheduler().current_thread();
	}

	pid_t Thread::current_tid()
	{
		return Processor::scheduler().current_tid();
	}

	Process& Thread::process()
	{
		ASSERT(m_process);
		return *m_process;
	}

	const Process& Thread::process() const
	{
		ASSERT(m_process);
		return *m_process;
	}

	Thread::~Thread()
	{
		if (m_delete_process)
		{
			ASSERT(m_process);
			delete m_process;
		}
	}

	BAN::ErrorOr<Thread*> Thread::pthread_create(entry_t entry, void* arg)
	{
		auto* thread = TRY(create_userspace(m_process, m_process->page_table()));

		save_sse();
		memcpy(thread->m_sse_storage, m_sse_storage, sizeof(m_sse_storage));

		thread->setup_exec_impl(
			reinterpret_cast<uintptr_t>(entry),
			reinterpret_cast<uintptr_t>(arg),
			0, 0, 0
		);

		return thread;
	}

	BAN::ErrorOr<Thread*> Thread::clone(Process* new_process, uintptr_t sp, uintptr_t ip)
	{
		ASSERT(m_is_userspace);
		ASSERT(m_state == State::Executing);

		Thread* thread = new Thread(s_next_tid++, new_process);
		if (thread == nullptr)
			return BAN::Error::from_errno(ENOMEM);
		BAN::ScopeGuard thread_deleter([thread] { delete thread; });

		thread->m_is_userspace = true;

		thread->m_kernel_stack = TRY(m_kernel_stack->clone(new_process->page_table()));
		thread->m_userspace_stack = TRY(m_userspace_stack->clone(new_process->page_table()));

		thread->m_state = State::NotStarted;

		thread->m_interrupt_stack.ip = ip;
		thread->m_interrupt_stack.cs = 0x08;
		thread->m_interrupt_stack.flags = 0x002;
		thread->m_interrupt_stack.sp = sp;
		thread->m_interrupt_stack.ss = 0x10;

		save_sse();
		memcpy(thread->m_sse_storage, m_sse_storage, sizeof(m_sse_storage));

#if ARCH(x86_64)
		thread->m_interrupt_registers.rax = 0;
#elif ARCH(i686)
		thread->m_interrupt_registers.eax = 0;
#endif

		thread_deleter.disable();

		return thread;
	}

	void Thread::setup_exec()
	{
		const auto& userspace_info = process().userspace_info();
		ASSERT(userspace_info.entry);

		setup_exec_impl(
			userspace_info.entry,
			userspace_info.argc,
			reinterpret_cast<uintptr_t>(userspace_info.argv),
			reinterpret_cast<uintptr_t>(userspace_info.envp),
			userspace_info.file_fd
		);
	}

	void Thread::setup_exec_impl(uintptr_t entry, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3)
	{
		ASSERT(is_userspace());
		m_state = State::NotStarted;

		// Signal mask is inherited

		// Initialize stack for returning
		PageTable::with_fast_page(process().page_table().physical_address_of(kernel_stack_top() - PAGE_SIZE), [&] {
			uintptr_t sp = PageTable::fast_page() + PAGE_SIZE;
			write_to_stack(sp, entry);
			write_to_stack(sp, arg3);
			write_to_stack(sp, arg2);
			write_to_stack(sp, arg1);
			write_to_stack(sp, arg0);
		});

		m_interrupt_stack.ip = reinterpret_cast<vaddr_t>(start_userspace_thread);
		m_interrupt_stack.cs = 0x08;
		m_interrupt_stack.flags = 0x002;
		m_interrupt_stack.sp = kernel_stack_top() - 5 * sizeof(uintptr_t);
		m_interrupt_stack.ss = 0x10;

		memset(&m_interrupt_registers, 0, sizeof(InterruptRegisters));
	}

	void Thread::setup_process_cleanup()
	{
		ASSERT(Processor::get_interrupt_state() == InterruptState::Disabled);

		m_state = State::NotStarted;
		static entry_t entry(
			[](void* process_ptr)
			{
				auto* thread = &Thread::current();
				auto* process = static_cast<Process*>(process_ptr);

				ASSERT(thread->m_process == process);

				process->cleanup_function(thread);

				thread->m_delete_process = true;

				// will call on thread exit after return
			}
		);

		m_signal_pending_mask = 0;
		m_signal_block_mask = ~0ull;

		PageTable::with_fast_page(process().page_table().physical_address_of(kernel_stack_top() - PAGE_SIZE), [&] {
			uintptr_t sp = PageTable::fast_page() + PAGE_SIZE;
			write_to_stack(sp, this);
			write_to_stack(sp, &Thread::on_exit_trampoline);
			write_to_stack(sp, m_process);
			write_to_stack(sp, entry);
		});

		m_interrupt_stack.ip = reinterpret_cast<vaddr_t>(start_kernel_thread);
		m_interrupt_stack.cs = 0x08;
		m_interrupt_stack.flags = 0x002;
		m_interrupt_stack.sp = kernel_stack_top() - 4 * sizeof(uintptr_t);
		m_interrupt_stack.ss = 0x10;

		memset(&m_interrupt_registers, 0, sizeof(InterruptRegisters));
	}

	bool Thread::is_interrupted_by_signal() const
	{
		if (!is_userspace() || m_state != State::Executing)
			return false;
		auto& interrupt_stack = *reinterpret_cast<InterruptStack*>(kernel_stack_top() - sizeof(InterruptStack));
		if (!GDT::is_user_segment(interrupt_stack.cs))
			return false;

		uint64_t full_pending_mask = m_signal_pending_mask | process().signal_pending_mask();
		uint64_t signals = full_pending_mask & ~m_signal_block_mask;
		for (uint8_t i = 0; i < _SIGMAX; i++)
		{
			if (!(signals & ((uint64_t)1 << i)))
				continue;

			vaddr_t signal_handler;
			{
				SpinLockGuard _(m_process->m_signal_lock);
				ASSERT(!(m_process->m_signal_handlers[i].sa_flags & SA_SIGINFO));
				signal_handler = (vaddr_t)m_process->m_signal_handlers[i].sa_handler;
			}
			if (signal_handler == (vaddr_t)SIG_IGN)
				continue;
			if (signal_handler == (vaddr_t)SIG_DFL && (i == SIGCHLD || i == SIGURG))
				continue;
			return true;
		}
		return false;
	}

	bool Thread::can_add_signal_to_execute() const
	{
		return is_interrupted_by_signal() && m_mutex_count == 0;
	}

	bool Thread::will_execute_signal() const
	{
		if (!is_userspace() || m_state != State::Executing)
			return false;
		auto& interrupt_stack = *reinterpret_cast<InterruptStack*>(kernel_stack_top() - sizeof(InterruptStack));
		return interrupt_stack.ip == (uintptr_t)signal_trampoline;
	}

	void Thread::handle_signal(int signal)
	{
		ASSERT(&Thread::current() == this);
		ASSERT(is_userspace());

		SpinLockGuard _(m_signal_lock);

		auto& interrupt_stack = *reinterpret_cast<InterruptStack*>(kernel_stack_top() - sizeof(InterruptStack));
		ASSERT(GDT::is_user_segment(interrupt_stack.cs));

		if (signal == 0)
		{
			uint64_t full_pending_mask = m_signal_pending_mask | process().signal_pending_mask();
			for (signal = _SIGMIN; signal <= _SIGMAX; signal++)
			{
				uint64_t mask = 1ull << signal;
				if ((full_pending_mask & mask) && !(m_signal_block_mask & mask))
					break;
			}
			ASSERT(signal <= _SIGMAX);
		}
		else
		{
			ASSERT(signal >= _SIGMIN);
			ASSERT(signal <= _SIGMAX);
		}

		vaddr_t signal_handler;
		{
			SpinLockGuard _(m_process->m_signal_lock);
			ASSERT(!(m_process->m_signal_handlers[signal].sa_flags & SA_SIGINFO));
			signal_handler = (vaddr_t)m_process->m_signal_handlers[signal].sa_handler;
		}

		m_signal_pending_mask &= ~(1ull << signal);
		process().remove_pending_signal(signal);

		if (signal_handler == (vaddr_t)SIG_IGN)
			;
		else if (signal_handler != (vaddr_t)SIG_DFL)
		{
			// call userspace signal handlers
#if ARCH(x86_64)
			interrupt_stack.sp -= 128; // skip possible red-zone
#endif
			write_to_stack(interrupt_stack.sp, interrupt_stack.ip);
			write_to_stack(interrupt_stack.sp, signal);
			write_to_stack(interrupt_stack.sp, signal_handler);
			interrupt_stack.ip = (uintptr_t)signal_trampoline;
		}
		else
		{
			switch (signal)
			{
				// Abnormal termination of the process with additional actions.
				case SIGABRT:
				case SIGBUS:
				case SIGFPE:
				case SIGILL:
				case SIGQUIT:
				case SIGSEGV:
				case SIGSYS:
				case SIGTRAP:
				case SIGXCPU:
				case SIGXFSZ:
					process().exit(128 + signal, signal | 0x80);
					break;

				// Abnormal termination of the process
				case SIGALRM:
				case SIGHUP:
				case SIGINT:
				case SIGKILL:
				case SIGPIPE:
				case SIGTERM:
				case SIGUSR1:
				case SIGUSR2:
				case SIGPOLL:
				case SIGPROF:
				case SIGVTALRM:
					process().exit(128 + signal, signal);
					break;

				// Ignore the signal
				case SIGCHLD:
				case SIGURG:
					break;

				// Stop the process:
				case SIGTSTP:
				case SIGTTIN:
				case SIGTTOU:
					ASSERT_NOT_REACHED();

				// Continue the process, if it is stopped; otherwise, ignore the signal.
				case SIGCONT:
					ASSERT_NOT_REACHED();
			}
		}
	}

	bool Thread::add_signal(int signal)
	{
		SpinLockGuard _(m_signal_lock);
		if (m_process)
		{
			vaddr_t signal_handler;
			{
				SpinLockGuard _(m_process->m_signal_lock);
				ASSERT(!(m_process->m_signal_handlers[signal].sa_flags & SA_SIGINFO));
				signal_handler = (vaddr_t)m_process->m_signal_handlers[signal].sa_handler;
			}
			if (signal_handler == (vaddr_t)SIG_IGN)
				return false;
			if (signal_handler == (vaddr_t)SIG_DFL && (signal == SIGCHLD || signal == SIGURG))
				return false;
		}
		uint64_t mask = 1ull << signal;
		if (!(m_signal_block_mask & mask))
		{
			m_signal_pending_mask |= mask;
			if (this != &Thread::current())
				Processor::scheduler().unblock_thread(this);
			return true;
		}
		return false;
	}

	BAN::ErrorOr<void> Thread::sleep_or_eintr_ns(uint64_t ns)
	{
		if (is_interrupted_by_signal())
			return BAN::Error::from_errno(EINTR);
		SystemTimer::get().sleep_ns(ns);
		if (is_interrupted_by_signal())
			return BAN::Error::from_errno(EINTR);
		return {};
	}

	BAN::ErrorOr<void> Thread::block_or_eintr_indefinite(ThreadBlocker& thread_blocker)
	{
		if (is_interrupted_by_signal())
			return BAN::Error::from_errno(EINTR);
		thread_blocker.block_indefinite();
		if (is_interrupted_by_signal())
			return BAN::Error::from_errno(EINTR);
		return {};
	}

	BAN::ErrorOr<void> Thread::block_or_eintr_or_timeout_ns(ThreadBlocker& thread_blocker, uint64_t timeout_ns, bool etimedout)
	{
		const uint64_t wake_time_ns = SystemTimer::get().ns_since_boot() + timeout_ns;
		return block_or_eintr_or_waketime_ns(thread_blocker, wake_time_ns, etimedout);
	}

	BAN::ErrorOr<void> Thread::block_or_eintr_or_waketime_ns(ThreadBlocker& thread_blocker, uint64_t wake_time_ns, bool etimedout)
	{
		if (is_interrupted_by_signal())
			return BAN::Error::from_errno(EINTR);
		thread_blocker.block_with_timeout_ns(wake_time_ns);
		if (is_interrupted_by_signal())
			return BAN::Error::from_errno(EINTR);
		if (etimedout && SystemTimer::get().ms_since_boot() >= wake_time_ns)
			return BAN::Error::from_errno(ETIMEDOUT);
		return {};
	}

	void Thread::on_exit_trampoline(Thread* thread)
	{
		thread->on_exit();
	}

	void Thread::on_exit()
	{
		ASSERT(this == &Thread::current());
		if (!m_delete_process && has_process())
		{
			if (process().on_thread_exit(*this))
			{
				Processor::set_interrupt_state(InterruptState::Disabled);
				setup_process_cleanup();
				Processor::yield();
				ASSERT_NOT_REACHED();
			}
		}
		m_state = State::Terminated;
		Processor::yield();
		ASSERT_NOT_REACHED();
	}

	void Thread::save_sse()
	{
		asm volatile("fxsave %0" :: "m"(m_sse_storage));
	}

	void Thread::load_sse()
	{
		asm volatile("fxrstor %0" :: "m"(m_sse_storage));
	}

}
