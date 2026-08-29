#!/bin/bash ../install.sh

NAME='libxmlb'
VERSION='0.3.29'
DOWNLOAD_URL="https://github.com/hughsie/libxmlb/releases/download/$VERSION/libxmlb-$VERSION.tar.xz#448294be33bfae62f00fa66e506f1cae80237ce71b7ab6530aefa75005eeb08a"
DEPENDENCIES=('glib' 'zstd')
CONFIGURE_OPTIONS=(
	'-Dprefix=/usr'
	'-Dbuildtype=release'
	'-Dintrospection=false'
	'-Dgtkdoc=false'
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
