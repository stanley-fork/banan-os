#!/bin/bash ../install.sh

NAME='shared-mime-info'
VERSION='2.5.1'
DOWNLOAD_URL="https://gitlab.freedesktop.org/xdg/shared-mime-info/-/archive/$VERSION/shared-mime-info-$VERSION.tar.gz#ea248ea157b7fa0165f4fe282c84919fa84c3f175553642f229c8f1ab7539128"
DEPENDENCIES=('glib' 'libxml2')
CONFIGURE_OPTIONS=(
	'-Dprefix=/usr'
	'-Dbuildtype=release'
	'-Dbuild-spec=false'
	'-Dbuild-tools=false'
	'-Dbuild-tests=false'
	'-Dbuild-translations=false'
	'-Dupdate-mimedb=true'
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
