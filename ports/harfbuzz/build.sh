#!/bin/bash ../install.sh

NAME='harfbuzz'
VERSION='14.3.1'
DOWNLOAD_URL="https://github.com/harfbuzz/harfbuzz/releases/download/$VERSION/harfbuzz-$VERSION.tar.xz#9dae9538aae2ffdf70cec31f2c27bf68e2aaeeae3112688467697d5faf6194f7"
DEPENDENCIES=('glib' 'freetype' 'icu')
CONFIGURE_OPTIONS=(
	'-Dprefix=/usr'
	'-Dbuildtype=release'
	'-Dtests=disabled'
	'-Dcairo=disabled'
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
