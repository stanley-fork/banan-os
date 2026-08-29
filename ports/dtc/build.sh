#!/bin/bash ../install.sh

NAME='dtc'
VERSION='1.8.1'
DOWNLOAD_URL="https://github.com/dgibson/dtc/archive/refs/tags/v$VERSION.tar.gz#74b50bb19134f6562490afea53e59953dd6c4afb17e5ccb60be32221262d3390"
CONFIGURE_OPTIONS=(
	'-Dprefix=/usr'
	'-Dbuildtype=release'
	'-Ddefault_library=shared'
	'-Dtests=false'
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
