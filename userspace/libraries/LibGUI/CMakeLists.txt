set(LIBGUI_SOURCES
	Texture.cpp
	Window.cpp
)

add_library(libgui ${LIBGUI_SOURCES})
banan_include_headers(libgui ban)
banan_include_headers(libgui libfont)
banan_include_headers(libgui libinput)
banan_link_library(libgui libc)

banan_install_headers(libgui)
install(TARGETS libgui OPTIONAL)
