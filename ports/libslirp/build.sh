#!/bin/bash ../install.sh

NAME='libslirp'
VERSION='4.9.4'
DOWNLOAD_URL="https://gitlab.freedesktop.org/slirp/libslirp/-/archive/v$VERSION/libslirp-v$VERSION.tar.gz#3998863b020aeda34bddc567097c6efba55a78cdf6eeee6bcd42c11ef23967da"
DEPENDENCIES=('glib')
TAR_CONTENT="libslirp-v$VERSION"
CONFIGURE_OPTIONS=(
	'-Dprefix=/usr'
	'-Dbuildtype=release'
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
