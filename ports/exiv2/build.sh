#!/bin/bash ../install.sh

NAME='exiv2'
VERSION='0.28.8'
DOWNLOAD_URL="https://github.com/Exiv2/exiv2/archive/refs/tags/v$VERSION.tar.gz#ea51b0609f58a9afa063b60daa1539948b62247721e154f4fff0ad3aec9f9756"
DEPENDENCIES=('zlib' 'expat')

configure() {
	cmake --fresh -B build -S . -G Ninja  \
		--toolchain="$BANAN_TOOLCHAIN_DIR/Toolchain.txt" \
		-DCMAKE_INSTALL_PREFIX=/usr \
		-DCMAKE_BUILD_TYPE=Release \
		-DEXIV2_ENABLE_NLS=OFF \
		-DEXIV2_ENABLE_BROTLI=OFF \
		-DEXIV2_ENABLE_INIH=OFF \
		-DBUILD_WITH_STACK_PROTECTOR=OFF \
		|| exit 1
}

build() {
	cmake --build build || exit 1
}

install() {
	DESTDIR="$DESTDIR" cmake --install build || exit 1
}
