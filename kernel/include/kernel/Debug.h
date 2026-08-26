#pragma once

#undef __debug_print
#define __debug_print(_prefix, _suffix, _format, ...) \
	do {                                              \
		Kernel::SpinLockGuard _(Debug::s_debug_lock); \
		Debug::print_prefix(__FILE__, __LINE__);      \
		BAN::Formatter::print(Debug::putchar,         \
			_prefix _format _suffix                   \
			__VA_OPT__(,) __VA_ARGS__);               \
	} while(false)

#include <BAN/Debug.h>
#include <BAN/Formatter.h>
#include <kernel/Lock/SpinLock.h>

#define BOCHS_BREAK() asm volatile("xchgw %bx, %bx")

#define DEBUG_VTTY 1

#define DEBUG_DEVFS 0

#define DEBUG_PCI 0
#define DEBUG_SCHEDULER 0
#define DEBUG_PS2 1

#define DEBUG_ARP 0
#define DEBUG_IPV4 0
#define DEBUG_ETHERTYPE 0
#define DEBUG_TCP 0
#define DEBUG_E1000 0

#define DEBUG_DISK_SYNC 0
#define DEBUG_NVMe 0

#define DEBUG_XHCI 0
#define DEBUG_USB 0
#define DEBUG_USB_HID 0
#define DEBUG_USB_HUB 0
#define DEBUG_USB_KEYBOARD 0
#define DEBUG_USB_MOUSE 0
#define DEBUG_USB_MASS_STORAGE 0

#define DEBUG_HDAUDIO 0


namespace Debug
{
	void dump_stack_trace();
	void dump_stack_trace(uintptr_t ip, uintptr_t bp);
	void dump_qr_code();

	void putchar(char);
	void print_prefix(const char*, int);

	extern Kernel::RecursiveSpinLock s_debug_lock;
}
