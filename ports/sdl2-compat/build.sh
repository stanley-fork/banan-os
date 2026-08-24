#!/bin/bash ../install.sh

NAME='sdl2-compat'
VERSION='2.32.70'
DOWNLOAD_URL="https://github.com/libsdl-org/sdl2-compat/releases/download/release-$VERSION/sdl2-compat-$VERSION.tar.gz#998fa62557eb46ffe7e5c3e2c123bc332f7df9d9f593b3ceed88ed1158428a44"
DEPENDENCIES=('SDL3')

configure() {
	cmake --fresh -S . -B build -G Ninja \
		--toolchain="$BANAN_TOOLCHAIN_DIR/Toolchain.txt" \
		-DCMAKE_INSTALL_PREFIX='/usr' \
		-DCMAKE_BUILD_TYPE=Release \
		-DSDL2COMPAT_X11=OFF \
		 || exit 1
}

build() {
	cmake --build build || exit 1
}

install() {
	DESTDIR="$DESTDIR" cmake --install build || exit 1
	ln -sf sdl2-compat.pc "$DESTDIR"/usr/lib/pkgconfig/sdl2.pc || exit 1
}
