#pragma once
#include <BAN/Vector.h>
#include <BAN/StringView.h>
typedef struct Banos_Symbol Banos_Symbol;
namespace Banos {
	void* resolve_symbol(const char* name);
	void import_symbols(Banos_Symbol* symbols, size_t count);
	void initialize_initial_drivers(void);
	BAN::ErrorOr<size_t> load_driver_from_image(const char* u_image);
}
