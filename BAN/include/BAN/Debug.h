#pragma once

#ifndef __debug_print

#include <BAN/Formatter.h>
#include <stdio.h>

#define __debug_print(_prefix, _suffix, _format, ...) \
	do {                                              \
		flockfile(stddbg);                            \
		BAN::Formatter::print(                        \
			[](int c) { putc_unlocked(c, stddbg); },  \
			_prefix _format _suffix                   \
			__VA_OPT__(,) __VA_ARGS__);               \
		fflush(stddbg);                               \
		funlockfile(stddbg);                          \
	} while(false)

#endif

#define __debug_print_if(_func, _cond, ...) \
	do {                              \
		if constexpr(_cond)           \
			_func(__VA_ARGS__);       \
	} while(false)

#define dprintln(...) __debug_print("",           "\r\n", __VA_ARGS__)
#define dwarnln(...)  __debug_print("\e[33m", "\e[m\r\n", __VA_ARGS__)
#define derrorln(...) __debug_print("\e[31m", "\e[m\r\n", __VA_ARGS__)

#define dprintln_if(cond, ...) __debug_print_if(dprintln, cond, __VA_ARGS__)
#define dwarnln_if(cond, ...)  __debug_print_if(dwarnln,  cond, __VA_ARGS__)
#define derrorln_if(cond, ...) __debug_print_if(derrorln, cond, __VA_ARGS__)
