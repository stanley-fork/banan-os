#!/bin/bash ../install.sh

NAME='butterscotch'
VERSION='git'
DOWNLOAD_URL="https://github.com/ButterscotchRunner/Butterscotch.git#b95f61c1aa84d9dd4a7b589b2a3331ab28dfa2ea"
DEPENDENCIES=('SDL2' 'openal-soft')

configure() {
	cmake --fresh -B build -S . -G Ninja \
		--toolchain="$BANAN_TOOLCHAIN_DIR/Toolchain.txt" \
		-DCMAKE_INSTALL_PREFIX=/usr \
		-DCMAKE_BUILD_TYPE=Release \
		-DPLATFORM='desktop' \
		-DAUDIO_BACKEND='openal' \
		-DDESKTOP_BACKEND='sdl2' \
		. || exit 1
}

build() {
	cmake --build build || exit 1
}

install() {
	mkdir -p "$DESTDIR/usr/bin" || exit 1
	cp -vf build/butterscotch "$DESTDIR/usr/bin/" || exit 1
}
