#include <kernel/Device/DebugDevice.h>
#include <kernel/FS/DevFS/FileSystem.h>
#include <kernel/Process.h>
#include <kernel/Timer/Timer.h>

namespace Kernel
{

	BAN::ErrorOr<BAN::RefPtr<DebugDevice>> DebugDevice::create(mode_t mode, uid_t uid, gid_t gid)
	{
		auto* result = new DebugDevice(mode, uid, gid, DevFileSystem::get().get_next_dev());
		if (result == nullptr)
			return BAN::Error::from_errno(ENOMEM);
		return BAN::RefPtr<DebugDevice>::adopt(result);
	}

	BAN::ErrorOr<size_t> DebugDevice::write_impl(off_t, BAN::ConstByteSpan buffer)
	{
		auto ms_since_boot = SystemTimer::get().ms_since_boot();
		Debug::DebugLock::lock();
		BAN::Formatter::print(Debug::putchar, "[{5}.{3}] {}: ",
			ms_since_boot / 1000,
			ms_since_boot % 1000,
			Kernel::Process::current().name()
		);
		for (size_t i = 0; i < buffer.size(); i++)
			Debug::putchar(buffer[i]);
		Debug::DebugLock::unlock();
		return buffer.size();
	}

}