#!/bin/bash ../install.sh

NAME='poppler'
VERSION='26.03.0'
DOWNLOAD_URL="https://poppler.freedesktop.org/poppler-$VERSION.tar.xz#8b3c5e2a9f2ab4c3ec5029f28af1b433c6b71f0d1e7b3997aa561cf1c0ca4ebe"
DEPENDENCIES=('gtk3' 'cairo' 'boost' 'curl' 'lcms2' 'libtiff' 'poppler-data')

configure() {
	cmake --fresh -B build -S . -G Ninja \
		--toolchain="$BANAN_TOOLCHAIN_DIR/Toolchain.txt" \
		-DCMAKE_INSTALL_PREFIX=/usr \
		-DCMAKE_BUILD_TYPE=Release \
		-DBUILD_SHARED_LIBS=ON \
		-DENABLE_NSS3=OFF \
		-DENABLE_GPGME=OFF \
		-DENABLE_QT5=OFF \
		-DENABLE_QT6=OFF \
		-DENABLE_LIBOPENJPEG=none \
		 || exit 1
}

build() {
	cmake --build build ||exit 1
}

install() {
	DESTDIR="$DESTDIR" cmake --install build ||exit 1
}
