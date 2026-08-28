#pragma once

#include <kernel/Arch.h>

#include <stdint.h>

namespace Kernel
{

	struct InterruptStack
	{
		uintptr_t ip;
		uintptr_t cs;
		uintptr_t flags;
		uintptr_t sp;
		uintptr_t ss;
	};

#if ARCH(x86_64)
	struct YieldRegisters
	{
		uintptr_t r15;
		uintptr_t r14;
		uintptr_t r13;
		uintptr_t r12;
		uintptr_t rbp;
		uintptr_t rbx;
		uintptr_t ret;
		uintptr_t sp;
		uintptr_t ip;
	};
#elif ARCH(i686)
	struct YieldRegisters
	{
		uintptr_t ebp;
		uintptr_t edi;
		uintptr_t esi;
		uintptr_t ebx;
		uintptr_t ret;
		uintptr_t sp;
		uintptr_t ip;
	};
#endif

}
