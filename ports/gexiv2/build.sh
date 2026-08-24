#!/bin/bash ../install.sh

NAME='gexiv2'
VERSION='0.14.6' # gimp requires 0.14.x
DOWNLOAD_URL="https://github.com/GNOME/gexiv2/archive/refs/tags/gexiv2-$VERSION.tar.gz#9083050df329b630f71f6f71aafc193c26e46548854ae21037d0a679ef62ae50"
TAR_CONTENT="gexiv2-gexiv2-$VERSION"
DEPENDENCIES=('exiv2' 'glib')
CONFIGURE_OPTIONS=(
	'-Dprefix=/usr'
	'-Dbuildtype=release'
	'-Dintrospection=false'
	'-Dvapi=false'
	'-Dpython3=false'
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
