#!/bin/bash ../install.sh

NAME='freetype'
VERSION='2.14.3'
DOWNLOAD_URL="https://download.savannah.gnu.org/releases/freetype/freetype-$VERSION.tar.xz#36bc4f1cc413335368ee656c42afca65c5a3987e8768cc28cf11ba775e785a5f"
DEPENDENCIES=('zlib' 'libpng')
CONFIGURE_OPTIONS=(
	'-Dprefix=/usr'
	'-Dbuildtype=release'
	'-Dharfbuzz=dynamic'
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
