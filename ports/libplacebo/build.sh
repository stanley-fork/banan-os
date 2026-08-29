#!/bin/bash ../install.sh

NAME='libplacebo'
VERSION='git'
DOWNLOAD_URL="https://github.com/haasn/libplacebo.git#cee9b076f2c63104ccfd497fa79c39a867293ec4"
DEPENDENCIES=('lcms2')
CONFIGURE_OPTIONS=(
	'-Dprefix=/usr'
	'-Dbuildtype=release'
	'-Dvulkan=disabled'
	'-Dunwind=disabled'
)

pre_configure() {
	git submodule update --init || exit 1
}

configure() {
	meson setup \
		--reconfigure \
		--cross-file "$MESON_CROSS_FILE" \
		"${CONFIGURE_OPTIONS[@]}" \
		build || exit 1
}

build() {
	meson compile -C build || exit 1
}

install() {
	meson install --destdir="$DESTDIR" -C build || exit 1
}
