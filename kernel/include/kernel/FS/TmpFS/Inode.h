#pragma once

#include <BAN/Iteration.h>
#include <BAN/Optional.h>
#include <kernel/FS/Inode.h>
#include <kernel/FS/TmpFS/Definitions.h>

namespace Kernel
{

	namespace TmpFuncs
	{

		template<typename F>
		concept for_each_valid_entry_callback = requires(F func, TmpDirectoryEntry& entry)
		{
			requires BAN::is_same_v<decltype(func(entry)), BAN::Iteration>;
		};

	}

	class TmpFileSystem;

	class TmpInode : public Inode
	{
	public:
		virtual ino_t		ino()		const override final { return m_ino; }
		virtual Mode		mode()		const override final { return Mode(m_inode_info.mode); }
		virtual nlink_t		nlink()		const override final { return m_inode_info.nlink; }
		virtual uid_t		uid()		const override final { return m_inode_info.uid; }
		virtual gid_t		gid()		const override final { return m_inode_info.gid; }
		virtual off_t		size()		const override final { return m_inode_info.size; }
		virtual timespec	atime()		const override final { return m_inode_info.atime; }
		virtual timespec	mtime()		const override final { return m_inode_info.mtime; }
		virtual timespec	ctime()		const override final { return m_inode_info.ctime; }
		virtual blksize_t	blksize()	const override final { return PAGE_SIZE; }
		virtual blkcnt_t	blocks()	const override final { return m_inode_info.blocks; }
		virtual dev_t		dev()		const override final { return 0; } // TODO
		virtual dev_t		rdev()		const override final { return 0; } // TODO

	public:
		static BAN::ErrorOr<BAN::RefPtr<TmpInode>> create_from_existing(TmpFileSystem&, ino_t, const TmpInodeInfo&);
		~TmpInode();

	protected:
		TmpInode(TmpFileSystem&, ino_t, const TmpInodeInfo&);

		void sync();
		void free_all_blocks();
		virtual BAN::ErrorOr<void> prepare_unlink() { return {}; };

		BAN::Optional<size_t> block_index(size_t data_block_index);
		BAN::ErrorOr<size_t> block_index_with_allocation(size_t data_block_index);

	protected:
		TmpFileSystem& m_fs;
		TmpInodeInfo m_inode_info;
		const ino_t m_ino;

		// has to be able to increase link count
		friend class TmpDirectoryInode;
	};

	class TmpFileInode : public TmpInode
	{
	public:
		static BAN::ErrorOr<BAN::RefPtr<TmpFileInode>> create_new(TmpFileSystem&, mode_t, uid_t, gid_t);
		~TmpFileInode();

	protected:
		virtual BAN::ErrorOr<size_t> read_impl(off_t, BAN::ByteSpan) override;
		virtual BAN::ErrorOr<size_t> write_impl(off_t, BAN::ConstByteSpan) override;
		virtual BAN::ErrorOr<void> truncate_impl(size_t) override;
		virtual BAN::ErrorOr<void> chmod_impl(mode_t) override;
		virtual bool has_data_impl() const override { return true; }

	private:
		TmpFileInode(TmpFileSystem&, ino_t, const TmpInodeInfo&);

		friend class TmpInode;
	};

	class TmpSymlinkInode : public TmpInode
	{
	public:
		~TmpSymlinkInode();

	private:
		TmpSymlinkInode(TmpFileSystem&, ino_t, const TmpInodeInfo&, BAN::StringView target);
	};

	class TmpDirectoryInode : public TmpInode
	{
	public:
		static BAN::ErrorOr<BAN::RefPtr<TmpDirectoryInode>> create_root(TmpFileSystem&, mode_t, uid_t, gid_t);
		static BAN::ErrorOr<BAN::RefPtr<TmpDirectoryInode>> create_new(TmpFileSystem&, mode_t, uid_t, gid_t, TmpInode& parent);

		~TmpDirectoryInode();

	protected:
		virtual BAN::ErrorOr<void> prepare_unlink() override;

	protected:
		virtual BAN::ErrorOr<BAN::RefPtr<Inode>> find_inode_impl(BAN::StringView) override final;
		virtual BAN::ErrorOr<void> list_next_inodes_impl(off_t, DirectoryEntryList*, size_t) override final;
		virtual BAN::ErrorOr<void> create_file_impl(BAN::StringView, mode_t, uid_t, gid_t) override final;
		virtual BAN::ErrorOr<void> create_directory_impl(BAN::StringView, mode_t, uid_t, gid_t) override final;
		virtual BAN::ErrorOr<void> unlink_impl(BAN::StringView) override final;

	private:
		TmpDirectoryInode(TmpFileSystem&, ino_t, const TmpInodeInfo&);

		BAN::ErrorOr<void> link_inode(TmpInode&, BAN::StringView);

		template<TmpFuncs::for_each_valid_entry_callback F>
		void for_each_valid_entry(F callback);

		friend class TmpInode;
	};

}
