#!/bin/bash ../install.sh

NAME='libfyaml'
VERSION='0.9.6'
DOWNLOAD_URL="https://github.com/pantoniou/libfyaml/releases/download/v$VERSION/libfyaml-$VERSION.tar.gz#a59cc3331e2eb903ec36933ad52a45888041cac31e44f553a00511131242c483"

configure() {
	cmake --fresh -B build -S . -G Ninja  \
		--toolchain="$BANAN_TOOLCHAIN_DIR/Toolchain.txt" \
		-DCMAKE_INSTALL_PREFIX=/usr \
		-DCMAKE_BUILD_TYPE=Release \
		|| exit 1
}

build() {
	cmake --build build || exit 1
}

install() {
	DESTDIR="$DESTDIR" cmake --install build || exit 1
}
