#!/bin/bash ../install.sh

NAME='AppStream'
VERSION='1.1.2'
DOWNLOAD_URL="https://www.freedesktop.org/software/appstream/releases/AppStream-$VERSION.tar.xz#46b4257100e25a6468ceed7b3ab82441f47b119da3398d30aea6d7b91174b586"
DEPENDENCIES=('glib' 'curl' 'libxml2' 'libxmlb' 'libfyaml')
CONFIGURE_OPTIONS=(
	'-Dprefix=/usr'
	'-Dbuildtype=release'
	'-Dbash-completion=false'
	'-Dgir=false'
	'-Dman=false'
	'-Dstemming=false'
	'-Dsystemd=false'
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
