#pragma once

#include <kernel/FS/Inode.h>

namespace Kernel
{

	class Socket : public Inode
	{
	public:
		enum class Domain
		{
			INET,
			INET6,
			UNIX,
		};

		enum class Type
		{
			STREAM,
			DGRAM,
			SEQPACKET,
		};

		struct Info
		{
			mode_t mode;
			uid_t uid;
			gid_t gid;
		};

	public:
		const FileSystem* filesystem() const final override { return nullptr; }

	protected:
		Socket(const Info& info)
		{
			m_mode = info.mode;
			m_uid = info.uid;
			m_gid = info.gid;
		}

	private:
		BAN::ErrorOr<void> sync_inode(SyncType) final override { return {}; }
		BAN::ErrorOr<void> sync_data() final override { return {}; }
	};

}
