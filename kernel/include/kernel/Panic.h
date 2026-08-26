#pragma once

#include <kernel/Debug.h>

#define __panic_stringify_helper(s) #s
#define __panic_stringify(s) __panic_stringify_helper(s)

#define panic(...) panic_impl(__FILE__ ":" __panic_stringify(__LINE__), __VA_ARGS__)

namespace Kernel
{

	extern volatile bool g_paniced;

	template<typename... Args>
	__attribute__((__noreturn__))
	static void panic_impl(const char* location, const char* message, Args&&... args)
	{
		asm volatile("cli");

		const bool had_debug_lock = Debug::s_debug_lock.current_processor_has_lock();

		bool first_panic = false;

		{
			SpinLockGuard _(Debug::s_debug_lock);
			BAN::Formatter::print(Debug::putchar, "\e[31m");
			BAN::Formatter::print(Debug::putchar, "Kernel panic at {}\r\n", location);
			if (had_debug_lock)
				BAN::Formatter::print(Debug::putchar, "  while having debug lock...\r\n");
			BAN::Formatter::print(Debug::putchar, message, BAN::forward<Args>(args)...);
			BAN::Formatter::print(Debug::putchar, "\e[m\r\n");
			if (!g_paniced)
			{
				Debug::dump_stack_trace();
				g_paniced = true;
				first_panic = true;
			}
		}

		if (first_panic)
			Debug::dump_qr_code();

		asm volatile("ud2");
		__builtin_unreachable();
	}

}
