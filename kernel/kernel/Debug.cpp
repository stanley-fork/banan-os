#include <kernel/Debug.h>
#include <kernel/InterruptController.h>
#include <kernel/Lock/SpinLock.h>
#include <kernel/Memory/PageTable.h>
#include <kernel/Terminal/Serial.h>
#include <kernel/Terminal/TTY.h>
#include <kernel/Timer/Timer.h>

#include <ctype.h>

extern TerminalDriver* g_terminal_driver;

namespace Debug
{

	Kernel::RecursiveSpinLock s_debug_lock;

	void dump_stack_trace()
	{
		using namespace Kernel;

		struct stackframe
		{
			stackframe* rbp;
			uintptr_t rip;
		};

		stackframe* frame = (stackframe*)__builtin_frame_address(0);
		if (!frame)
		{
			dprintln("Could not get frame address");
			return;
		}
		uintptr_t first_rip = frame->rip;
		uintptr_t last_rip = 0;
		bool first = true;

		BAN::Formatter::print(Debug::putchar, "\e[36mStack trace:\r\n");
		while (frame)
		{
			if (PageTable::current().is_page_free((vaddr_t)frame & PAGE_ADDR_MASK))
			{
				derrorln("    {} not mapped", frame);
				break;
			}

			BAN::Formatter::print(Debug::putchar, "    {}\r\n", (void*)frame->rip);

			if (!first && frame->rip == first_rip)
			{
				derrorln("looping kernel panic :(");
				break;
			}
			else if (!first && frame->rip == last_rip)
			{
				derrorln("repeating stack trace");
				break;
			}

			last_rip = frame->rip;
			frame = frame->rbp;
			first = false;
		}
		BAN::Formatter::print(Debug::putchar, "\e[m");
	}

	void putchar(char ch)
	{
		if (Kernel::Serial::has_devices())
			return Kernel::Serial::putchar_any(ch);
		if (Kernel::TTY::is_initialized())
			return Kernel::TTY::putchar_current(ch);

		if (g_terminal_driver)
		{
			static uint32_t col = 0;
			static uint32_t row = 0;

			uint32_t row_copy = row;

			if (ch == '\n')
			{
				row++;
				col = 0;
			}
			else if (ch == '\r')
			{
				col = 0;
			}
			else
			{
				if (!isprint(ch))
					ch = '?';
				g_terminal_driver->putchar_at(ch, col, row, TerminalColor::BRIGHT_WHITE, TerminalColor::BLACK);

				col++;
				if (col >= g_terminal_driver->width())
				{
					row++;
					col = 0;
				}
			}

			if (row >= g_terminal_driver->height())
				row = 0;

			if (row != row_copy)
			{
				for (uint32_t i = col; i < g_terminal_driver->width(); i++)
				{
					g_terminal_driver->putchar_at(' ', i, row, TerminalColor::BRIGHT_WHITE, TerminalColor::BLACK);
					if (row + 1 < g_terminal_driver->height())
						g_terminal_driver->putchar_at(' ', i, row + 1, TerminalColor::BRIGHT_WHITE, TerminalColor::BLACK);
				}
			}
		}
	}

	void print_prefix(const char* file, int line)
	{
		auto ms_since_boot = Kernel::SystemTimer::is_initialized() ? Kernel::SystemTimer::get().ms_since_boot() : 0;
		BAN::Formatter::print(Debug::putchar, "[{5}.{3}] {}:{}: ", ms_since_boot / 1000, ms_since_boot % 1000, file, line);
	}

}
