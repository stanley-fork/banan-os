#!/bin/bash ../install.sh

NAME='libass'
VERSION='0.17.5'
DOWNLOAD_URL="https://github.com/libass/libass/releases/download/$VERSION/libass-$VERSION.tar.xz#2dca25c0e0c837ddf00b52011b3f82cac1e4ddd3ad018227806b0c2288864acc"
DEPENDENCIES=('fontconfig' 'fribidi' 'harfbuzz')
CONFIGURE_OPTIONS=(
	'-Dprefix=/usr'
	'-Dbuildtype=release'
	'-Ddefault_library=shared'
)

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
