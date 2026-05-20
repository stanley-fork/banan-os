#pragma once

#include <BAN/Errors.h>

namespace Kernel
{

	BAN::ErrorOr<void> read_from_user(const void* user_addr, void* out, size_t size);
	BAN::ErrorOr<void> read_string_from_user(const char* user_addr, char* out, size_t max_size);
	BAN::ErrorOr<void> write_to_user(void* user_addr, const void* in, size_t size);

};
