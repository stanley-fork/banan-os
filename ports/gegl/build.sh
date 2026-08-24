#!/bin/bash ../install.sh

NAME='gegl'
VERSION='0.4.70'
DOWNLOAD_URL="https://download.gimp.org/gegl/0.4/gegl-$VERSION.tar.xz#47f50d9c3aecd375deb48c11ebfead52d162e4fc162a4b3d44618277f1faec02"
DEPENDENCIES=('json-glib' 'babl' 'libjpeg-turbo' 'libpng')
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
