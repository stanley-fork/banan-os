#!/bin/bash ../install.sh

NAME='lcms2'
VERSION='2.18'
DOWNLOAD_URL="https://github.com/mm2/Little-CMS/archive/refs/tags/lcms$VERSION.tar.gz#4f52a4459a93ac02b88e49b04dc0679e52fc92d36d3b722e5a1a44dbe8118236"
TAR_CONTENT="Little-CMS-lcms$VERSION"
DEPENDENCIES=('libtiff' 'libjpeg-turbo')
CONFIGURE_OPTIONS=(
	'-Dprefix=/usr'
	'-Dbuildtype=release'
	'-Dtests=disabled'
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
