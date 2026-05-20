#pragma once

#include <kernel/FS/Inode.h>
#include <kernel/Lock/Mutex.h>
#include <kernel/Memory/ByteRingBuffer.h>
#include <kernel/ThreadBlocker.h>

#include <sys/stat.h>

namespace Kernel
{

	class Pipe final : public Inode, public BAN::Weakable<Pipe>
	{
	public:
		static BAN::ErrorOr<BAN::RefPtr<Inode>> open(BAN::RefPtr<Inode>, int status_flags);
		static BAN::ErrorOr<BAN::RefPtr<Inode>> create(uid_t, gid_t);
		~Pipe();

		void on_close(int status_flags) override;
		void on_clone(int status_flags) override;

		virtual const FileSystem* filesystem() const override { return nullptr; }

	private:
		virtual BAN::ErrorOr<void> sync_inode(SyncType) override;
		virtual BAN::ErrorOr<void> sync_data() override;

		virtual BAN::ErrorOr<size_t> read_impl(off_t, BAN::ByteSpan) override;
		virtual BAN::ErrorOr<size_t> write_impl(off_t, BAN::ConstByteSpan) override;
		virtual BAN::ErrorOr<void> truncate_impl(size_t) override;

		virtual bool can_read_impl() const override { return !m_buffer->empty(); }
		virtual bool can_write_impl() const override { return true; }
		virtual bool has_error_impl() const override { return m_reading_count == 0; }
		virtual bool has_hungup_impl() const override { return m_writing_count == 0; }

	private:
		Pipe(const struct stat&);

	private:
		Mutex m_mutex;
		ThreadBlocker m_thread_blocker;

		BAN::UniqPtr<ByteRingBuffer> m_buffer;

		BAN::Atomic<uint32_t> m_writing_count { 0 };
		BAN::Atomic<uint32_t> m_reading_count { 0 };

		BAN::RefPtr<Inode> m_named_inode;
	};

}
