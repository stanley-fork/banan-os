#pragma once

#include <kernel/Device/Device.h>

namespace Kernel
{

	class NullDevice : public CharacterDevice
	{
	public:
		static BAN::ErrorOr<BAN::RefPtr<NullDevice>> create(mode_t, uid_t, gid_t);

		virtual dev_t rdev() const override { return m_rdev; }

		virtual BAN::StringView name() const override { return "null"sv; }

	protected:
		NullDevice(mode_t mode, uid_t uid, gid_t gid, dev_t rdev)
			: CharacterDevice(mode, uid, gid)
			, m_rdev(rdev)
		{ }

		virtual BAN::ErrorOr<size_t> read_impl(off_t, BAN::ByteSpan) override { return 0; }
		virtual BAN::ErrorOr<size_t> write_impl(off_t, BAN::ConstByteSpan buffer) override { return buffer.size(); };

	private:
		const dev_t m_rdev;
	};

}