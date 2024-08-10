#pragma once

#include <sys/types.h>

namespace Kernel
{

	enum class DeviceNumber : dev_t
	{
		Framebuffer = 1,
		TTY,
		PTSMaster,
		Null,
		Zero,
		Debug,
		Keyboard,
		Mouse,
		SCSI,
		NVMeController,
		NVMeNamespace,
		Ethernet,
		TmpFS,
	};

}
