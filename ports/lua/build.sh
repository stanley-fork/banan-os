#!/bin/bash ../install.sh

NAME='lua'
VERSION='5.4.7'
DOWNLOAD_URL="https://www.lua.org/ftp/lua-$VERSION.tar.gz#9fbf5e28ef86c69858f6d3d34eccc32e911c1a28b4120ff3e84aaa70cfbf1e30"

configure() {
	make clean
}

build() {
	make -j$(nproc) posix CC="$CC" MYCFLAGS='-fPIC' || exit 1
}

install() {
	make install \
		INSTALL_DATA='cp -d' \
		INSTALL_TOP="$DESTDIR/usr" \
		TO_LIB="liblua.so liblua.so.${VERSION%.*} liblua.so.$VERSION" \
		|| exit 1
}
