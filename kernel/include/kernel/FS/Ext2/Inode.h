#pragma once

#include <BAN/String.h>
#include <BAN/StringView.h>
#include <kernel/FS/Ext2/Definitions.h>
#include <kernel/FS/Inode.h>

namespace Kernel
{

	class Ext2FS;

	class Ext2Inode final : public Inode
	{
	public:
		~Ext2Inode();

		virtual ino_t ino() const override { return m_ino; };
		virtual Mode mode() const override { return { m_inode.mode }; }
		virtual nlink_t nlink() const override { return m_inode.links_count; }
		virtual uid_t uid() const override { return m_inode.uid; }
		virtual gid_t gid() const override { return m_inode.gid; }
		virtual off_t size() const override { return m_inode.size; }
		virtual timespec atime() const override { return timespec { .tv_sec = m_inode.atime, .tv_nsec = 0 }; }
		virtual timespec mtime() const override { return timespec { .tv_sec = m_inode.mtime, .tv_nsec = 0 }; }
		virtual timespec ctime() const override { return timespec { .tv_sec = m_inode.ctime, .tv_nsec = 0 }; }
		virtual blksize_t blksize() const override;
		virtual blkcnt_t blocks() const override;
		virtual dev_t dev() const override { return 0; }
		virtual dev_t rdev() const override { return 0; }

		virtual const FileSystem* filesystem() const override;

	protected:
		virtual BAN::ErrorOr<BAN::RefPtr<Inode>> find_inode_impl(BAN::StringView) override;
		virtual BAN::ErrorOr<size_t> list_next_inodes_impl(off_t, struct dirent*, size_t) override;
		virtual BAN::ErrorOr<void> create_file_impl(BAN::StringView, mode_t, uid_t, gid_t) override;
		virtual BAN::ErrorOr<void> create_directory_impl(BAN::StringView, mode_t, uid_t, gid_t) override;
		virtual BAN::ErrorOr<void> link_inode_impl(BAN::StringView, BAN::RefPtr<Inode>) override;
		virtual BAN::ErrorOr<void> unlink_impl(BAN::StringView) override;

		virtual BAN::ErrorOr<BAN::String> link_target_impl() override;
		virtual BAN::ErrorOr<void> set_link_target_impl(BAN::StringView) override;

		virtual BAN::ErrorOr<size_t> read_impl(off_t, BAN::ByteSpan) override;
		virtual BAN::ErrorOr<size_t> write_impl(off_t, BAN::ConstByteSpan) override;
		virtual BAN::ErrorOr<void> truncate_impl(size_t) override;
		virtual BAN::ErrorOr<void> chmod_impl(mode_t) override;
		virtual BAN::ErrorOr<void> chown_impl(uid_t, gid_t) override;
		virtual BAN::ErrorOr<void> utimens_impl(const timespec[2]) override;
		virtual BAN::ErrorOr<void> fsync_impl() override;

		virtual bool can_read_impl() const override { return true; }
		virtual bool can_write_impl() const override { return true; }
		virtual bool has_error_impl() const override { return false; }
		virtual bool has_hungup_impl() const override { return false; }

	private:
		// Returns maximum number of data blocks in use
		// NOTE: the inode might have more blocks than what this suggests if it has been shrinked
		uint32_t max_used_data_block_count() const { return size() / blksize(); }

		BAN::ErrorOr<BAN::Optional<uint32_t>> block_from_indirect_block(uint32_t block, uint32_t index, uint32_t depth);
		BAN::ErrorOr<BAN::Optional<uint32_t>> fs_block_of_data_block_index(uint32_t data_block_index);

		BAN::ErrorOr<void> link_inode_to_directory(Ext2Inode&, BAN::StringView name);
		BAN::ErrorOr<bool> is_directory_empty();

		BAN::ErrorOr<void> cleanup_indirect_block(uint32_t block, uint32_t depth);
		BAN::ErrorOr<void> cleanup_default_links();
		BAN::ErrorOr<void> cleanup_from_fs();

		BAN::ErrorOr<uint32_t> allocate_new_block_to_indirect_block(uint32_t& block, uint32_t index, uint32_t depth);
		BAN::ErrorOr<uint32_t> allocate_new_block(uint32_t data_block_index);
		BAN::ErrorOr<void> sync();

		uint32_t block_group() const;

	private:
		Ext2Inode(Ext2FS& fs, Ext2::Inode inode, uint32_t ino)
			: m_fs(fs)
			, m_inode(inode)
			, m_ino(ino)
		{}
		static BAN::ErrorOr<BAN::RefPtr<Ext2Inode>> create(Ext2FS&, uint32_t);

	private:
		Ext2FS& m_fs;
		Ext2::Inode m_inode;
		const uint32_t m_ino;

		friend class Ext2FS;
		friend class BAN::RefPtr<Ext2Inode>;
	};

}
