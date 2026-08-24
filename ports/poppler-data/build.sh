#!/bin/bash ../install.sh

NAME='poppler-data'
VERSION='0.4.12'
DOWNLOAD_URL="https://poppler.freedesktop.org/poppler-data-$VERSION.tar.gz#c835b640a40ce357e1b83666aabd95edffa24ddddd49b8daff63adb851cdab74"

configure() {
	cmake --fresh -B build -S . -G Ninja \
		--toolchain="$BANAN_TOOLCHAIN_DIR/Toolchain.txt" \
		-DCMAKE_INSTALL_PREFIX='/usr' \
		-DCMAKE_BUILD_TYPE=Release \
		 || exit 1
}

build() {
	cmake --build build ||exit 1
}

install() {
	DESTDIR="$DESTDIR" cmake --install build ||exit 1
}
