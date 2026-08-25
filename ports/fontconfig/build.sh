#!/bin/bash ../install.sh

NAME='fontconfig'
VERSION='2.17.1'
DOWNLOAD_URL="https://gitlab.freedesktop.org/fontconfig/fontconfig/-/archive/$VERSION/fontconfig-$VERSION.tar.gz#82e73b26adad651b236e5f5d4b3074daf8ff0910188808496326bd3449e5261d"
DEPENDENCIES=('freetype' 'expat' 'libiconv' 'dejavu-fonts-ttf')
CONFIGURE_OPTIONS=(
	'-Dprefix=/usr'
	'-Dbuildtype=release'
	'-Dtests=disabled'
	'-Dnls=disabled'
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
