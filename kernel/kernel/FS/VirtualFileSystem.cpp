#include <BAN/ScopeGuard.h>
#include <BAN/StringView.h>
#include <kernel/FS/DevFS/FileSystem.h>
#include <kernel/FS/ProcFS/FileSystem.h>
#include <kernel/FS/TmpFS/FileSystem.h>
#include <kernel/FS/USTARModule.h>
#include <kernel/FS/VirtualFileSystem.h>
#include <kernel/Lock/LockGuard.h>
#include <kernel/Storage/Partition.h>
#include <kernel/Timer/Timer.h>

#include <ctype.h>
#include <fcntl.h>

namespace Kernel
{

	static BAN::RefPtr<VirtualFileSystem> s_instance;

	static BAN::ErrorOr<BAN::RefPtr<BlockDevice>> find_partition_by_uuid(BAN::StringView uuid)
	{
		ASSERT(uuid.size() == 36);

		BAN::RefPtr<BlockDevice> result;
		DevFileSystem::get().for_each_inode(
			[&result, uuid](BAN::RefPtr<Inode> inode) -> BAN::Iteration
			{
				if (!inode->is_device())
					return BAN::Iteration::Continue;
				if (!static_cast<Device*>(inode.ptr())->is_partition())
					return BAN::Iteration::Continue;
				auto* partition = static_cast<Partition*>(inode.ptr());
				ASSERT(partition->uuid().size() == uuid.size());
				for (size_t i = 0; i < uuid.size(); i++)
					if (tolower(uuid[i]) != tolower(partition->uuid()[i]))
						return BAN::Iteration::Continue;
				result = partition;
				return BAN::Iteration::Break;
			}
		);

		if (!result)
			return BAN::Error::from_errno(ENOENT);
		return result;
	}

	static BAN::ErrorOr<BAN::RefPtr<BlockDevice>> find_block_device_by_name(BAN::StringView name)
	{
		auto device_inode = TRY(DevFileSystem::get().root_inode()->find_inode(name));
		if (!device_inode->mode().ifblk())
			return BAN::Error::from_errno(ENOTBLK);
		return BAN::RefPtr<BlockDevice>(static_cast<BlockDevice*>(device_inode.ptr()));
	}

	static BAN::RefPtr<FileSystem> load_fallback_root_filesystem()
	{
		if (g_boot_info.modules.empty())
			panic("No fallback boot modules given");

		auto filesystem_or_error = TmpFileSystem::create(-1, 0755, 0, 0);
		if (filesystem_or_error.is_error())
			panic("Failed to create fallback filesystem: {}", filesystem_or_error.error());

		dprintln("Trying to load fallback filesystem from {} modules", g_boot_info.modules.size());

		auto filesystem = BAN::RefPtr<FileSystem>::adopt(filesystem_or_error.release_value());

		bool loaded_initrd = false;
		for (const auto& module : g_boot_info.modules)
		{
			if (auto ret = unpack_boot_module_into_directory(filesystem->root_inode(), module); ret.is_error())
				dwarnln("Failed to unpack boot module: {}", ret.error());
			else
				loaded_initrd |= ret.value();
		}

		if (!loaded_initrd)
			panic("Could not load initrd from any boot module :(");

		return filesystem;
	}

	static BAN::RefPtr<FileSystem> load_root_filesystem(BAN::StringView root_path)
	{
		if (root_path.empty())
			return load_fallback_root_filesystem();

		enum class RootType
		{
			PartitionUUID,
			BlockDeviceName,
		};

		BAN::StringView entry;
		RootType type;

		if (root_path.starts_with("PARTUUID="_sv))
		{
			entry = root_path.substring(9);
			if (entry.size() != 36)
			{
				derrorln("Invalid UUID '{}'", entry);
				return load_fallback_root_filesystem();
			}
			type = RootType::PartitionUUID;
		}
		else if (root_path.starts_with("/dev/"_sv))
		{
			entry = root_path.substring(5);
			if (entry.empty() || entry.contains('/'))
			{
				derrorln("Invalid root path '{}'", root_path);
				return load_fallback_root_filesystem();
			}
			type = RootType::BlockDeviceName;
		}
		else
		{
			derrorln("Unsupported root path format '{}'", root_path);
			return load_fallback_root_filesystem();
		}

		constexpr size_t timeout_ms = 10'000;
		constexpr size_t sleep_ms = 500;

		for (size_t i = 0; i < timeout_ms / sleep_ms; i++)
		{
			BAN::ErrorOr<BAN::RefPtr<BlockDevice>> ret = BAN::Error::from_errno(EINVAL);

			switch (type)
			{
				case RootType::PartitionUUID:
					ret = find_partition_by_uuid(entry);
					break;
				case RootType::BlockDeviceName:
					ret = find_block_device_by_name(entry);
					break;
			}

			if (!ret.is_error())
			{
				auto filesystem_or_error = FileSystem::from_block_device(ret.release_value());
				if (filesystem_or_error.is_error())
				{
					derrorln("Could not create filesystem from '{}': {}", root_path, filesystem_or_error.error());
					return load_fallback_root_filesystem();
				}
				return filesystem_or_error.release_value();;
			}

			if (ret.error().get_error_code() != ENOENT)
			{
				derrorln("Could not open root device '{}': {}", root_path, ret.error());
				return load_fallback_root_filesystem();
			}

			if (i == 4)
				dwarnln("Could not find specified root device, waiting for it to get loaded...");

			SystemTimer::get().sleep_ms(sleep_ms);
		}

		derrorln("Could not find root device '{}' after {} ms", root_path, timeout_ms);
		return load_fallback_root_filesystem();
	}

	void VirtualFileSystem::initialize(BAN::StringView root_path)
	{
		ASSERT(!s_instance);
		s_instance = MUST(BAN::RefPtr<VirtualFileSystem>::create());

		s_instance->m_root_fs = load_root_filesystem(root_path);
		if (!s_instance->m_root_fs)
			panic("Could not load root filesystem");

		Credentials root_creds { 0, 0, 0, 0 };
		MUST(s_instance->mount(root_creds, &DevFileSystem::get(), "/dev"_sv));

		MUST(s_instance->mount(root_creds, &ProcFileSystem::get(), "/proc"_sv));

		auto tmpfs = MUST(TmpFileSystem::create(-1, 01777, 0, 0));
		MUST(s_instance->mount(root_creds, tmpfs, "/tmp"_sv));
	}

	VirtualFileSystem& VirtualFileSystem::get()
	{
		ASSERT(s_instance);
		return *s_instance;
	}

	BAN::ErrorOr<void> VirtualFileSystem::mount(const Credentials& credentials, BAN::StringView block_device_path, BAN::StringView target)
	{
		// TODO: allow custom root
		auto block_device_file = TRY(file_from_absolute_path(root_inode(), credentials, block_device_path, true));
		if (!block_device_file.inode->is_device())
			return BAN::Error::from_errno(ENOTBLK);

		auto* device = static_cast<Device*>(block_device_file.inode.ptr());
		if (!device->mode().ifblk())
			return BAN::Error::from_errno(ENOTBLK);

		auto file_system = TRY(FileSystem::from_block_device(static_cast<BlockDevice*>(device)));
		return mount(credentials, file_system, target);
	}

	BAN::ErrorOr<void> VirtualFileSystem::mount(const Credentials& credentials, BAN::RefPtr<FileSystem> file_system, BAN::StringView path)
	{
		// TODO: allow custom root
		auto file = TRY(file_from_absolute_path(root_inode(), credentials, path, true));
		if (!file.inode->mode().ifdir())
			return BAN::Error::from_errno(ENOTDIR);

		LockGuard _(m_mount_point_lock);
		TRY(m_mount_points.emplace_back(file_system, BAN::move(file)));
		return {};
	}

	VirtualFileSystem::MountPoint* VirtualFileSystem::mount_from_host_inode(BAN::RefPtr<Inode> inode)
	{
		LockGuard _(m_mount_point_lock);
		for (MountPoint& mount : m_mount_points)
			if (*mount.host.inode == *inode)
				return &mount;
		return nullptr;
	}

	VirtualFileSystem::MountPoint* VirtualFileSystem::mount_from_root_inode(BAN::RefPtr<Inode> inode)
	{
		LockGuard _(m_mount_point_lock);
		for (MountPoint& mount : m_mount_points)
			if (*mount.target->root_inode() == *inode)
				return &mount;
		return nullptr;
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstack-usage="
	BAN::ErrorOr<VirtualFileSystem::File> VirtualFileSystem::file_from_relative_path(BAN::RefPtr<Inode> root_inode, const File& parent, const Credentials& credentials, BAN::StringView path, int flags)
	{
		auto inode = parent.inode;
		ASSERT(inode);

		char canonical_buf[PATH_MAX];
		size_t canonical_len = parent.canonical_path.size();
		if (!parent.canonical_path.empty() && parent.canonical_path.back() == '/')
			canonical_len--;
		memcpy(canonical_buf, parent.canonical_path.data(), canonical_len);
		ASSERT(canonical_len == 0 || canonical_buf[canonical_len - 1] != '/');

		char path_storage[PATH_MAX];

		size_t link_depth = 0;
		for (;;)
		{
			const auto path_part = [&path] {
				size_t off = 0;
				while (off < path.size() && path[off] == '/')
					off++;

				size_t len = 0;
				while (off + len < path.size() && path[off + len] != '/')
					len++;

				const auto result = path.substring(off, len);
				path = path.substring(off + len);
				return result;
			}();

			if (path_part.empty())
				break;
			if (path_part == "."_sv)
				continue;

			auto orig_inode = inode;

			// resolve file name
			{
				if (!(inode == root_inode && path_part == ".."_sv))
				{
					auto parent_inode = inode;
					if (path_part == ".."_sv)
						if (auto* mount_point = mount_from_root_inode(inode))
							parent_inode = mount_point->host.inode;
					if (!parent_inode->can_access(credentials, O_SEARCH))
						return BAN::Error::from_errno(EACCES);
					inode = TRY(parent_inode->find_inode(path_part));
				}

				if (path_part == ".."_sv)
				{
					while (canonical_len && canonical_buf[canonical_len - 1] != '/')
						canonical_len--;
					if (canonical_len)
						canonical_len--;
				}
				else
				{
					if (auto* mount_point = mount_from_host_inode(inode))
						inode = mount_point->target->root_inode();
					if (canonical_len + 1 + path_part.size() > sizeof(canonical_buf))
						return BAN::Error::from_errno(ENAMETOOLONG);
					canonical_buf[canonical_len++] = '/';
					memcpy(canonical_buf + canonical_len, path_part.data(), path_part.size());
					canonical_len += path_part.size();
				}
			}

			if (!inode->mode().iflnk())
				continue;
			if ((flags & O_NOFOLLOW) && !path.find([](char ch) { return ch != '/'; }).has_value())
				continue;

			// resolve symbolic links
			{
				auto link_target = TRY(inode->link_target());
				if (link_target.empty())
					return BAN::Error::from_errno(ENOENT);

				if (link_target.front() == '/')
				{
					inode = root_inode;
					canonical_len = 0;
				}
				else
				{
					inode = orig_inode;

					while (canonical_len && canonical_buf[canonical_len - 1] != '/')
						canonical_len--;
					if (canonical_len)
						canonical_len--;
				}

				if (link_target.size() + path.size() > sizeof(path_storage))
					return BAN::Error::from_errno(ENAMETOOLONG);
				memcpy(path_storage, link_target.data(), link_target.size());
				memcpy(path_storage + link_target.size(), path.data(), path.size());
				path = BAN::StringView(path_storage, link_target.size() + path.size());

				link_depth++;
				if (link_depth > SYMLOOP_MAX)
					return BAN::Error::from_errno(ELOOP);
			}
		}

		if (!inode->can_access(credentials, flags))
			return BAN::Error::from_errno(EACCES);

		if (canonical_len == 0)
			canonical_buf[canonical_len++] = '/';

		BAN::String canonical_path;
		TRY(canonical_path.append(BAN::StringView(canonical_buf, canonical_len)));

		return File(BAN::move(inode), BAN::move(canonical_path));
	}
#pragma GCC diagnostic pop

}
