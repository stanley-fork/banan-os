#!/bin/bash ../install.sh

NAME='json-c'
VERSION='0.18-20240915'
DOWNLOAD_URL="https://github.com/json-c/json-c/archive/refs/tags/json-c-$VERSION.tar.gz#3112c1f25d39eca661fe3fc663431e130cc6e2f900c081738317fba49d29e298"
TAR_CONTENT="json-c-json-c-$VERSION"

configure() {
	cmake --fresh -B build -S . -G Ninja  \
		--toolchain="$BANAN_TOOLCHAIN_DIR/Toolchain.txt" \
		-DCMAKE_INSTALL_PREFIX=/usr \
		-DCMAKE_BUILD_TYPE=Release \
		-DBUILD_STATIC_LIBS=OFF \
		|| exit 1
}

build() {
	cmake --build build || exit 1
}

install() {
	DESTDIR="$DESTDIR" cmake --install build || exit 1
}
