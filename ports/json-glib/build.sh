#!/bin/bash ../install.sh

NAME='json-glib'
VERSION='1.10.8'
DOWNLOAD_URL="https://github.com/GNOME/json-glib/archive/refs/tags/$VERSION.tar.gz#7a114bdac0b2611a7207e981c37fa9b1e70d9cb642470cd9e967b135428cec52"
DEPENDENCIES=('glib' 'json-c')
CONFIGURE_OPTIONS=(
	'-Dprefix=/usr'
	'-Dbuildtype=release'
	'-Dintrospection=disabled'
	'-Dnls=disabled'
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
