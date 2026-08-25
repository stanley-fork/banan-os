#!/bin/bash ../install.sh

NAME='sdl12-compat'
VERSION='1.2.76'
DOWNLOAD_URL="https://github.com/libsdl-org/sdl12-compat/archive/refs/tags/release-$VERSION.tar.gz#e889ac9c7e8a6bdfc31972bf1f1254b84882cb52931608bada62e8febbf0270b"
TAR_CONTENT="sdl12-compat-release-$VERSION"
DEPENDENCIES=('sdl2-compat')

configure() {
	cmake --fresh -S . -B build -G Ninja \
		--toolchain="$BANAN_TOOLCHAIN_DIR/Toolchain.txt" \
		-DCMAKE_INSTALL_PREFIX='/usr' \
		-DCMAKE_BUILD_TYPE=Release \
		 || exit 1
}

build() {
	cmake --build build || exit 1
}

install() {
	DESTDIR="$DESTDIR" cmake --install build || exit 1
	ln -svf sdl12_compat.pc "$DESTDIR"/usr/lib/pkgconfig/sdl.pc || exit 1
}
