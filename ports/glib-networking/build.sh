#!/bin/bash ../install.sh

NAME='glib-networking'
VERSION='2.80.1'
DOWNLOAD_URL="https://download.gnome.org/sources/glib-networking/${VERSION%.*}/glib-networking-$VERSION.tar.xz#b80e2874157cd55071f1b6710fa0b911d5ac5de106a9ee2a4c9c7bee61782f8e"
DEPENDENCIES=('glib' 'openssl')
CONFIGURE_OPTIONS=(
	'-Dprefix=/usr'
	'-Dbuildtype=release'
	'-Dlibproxy=disabled'
	'-Dgnome_proxy=disabled'
	'-Dgnutls=disabled'
	'-Dopenssl=enabled'
	'-Dc_link_args=-lgcc_s' # something is messed up with my toolchain
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
