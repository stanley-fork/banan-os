#!/bin/bash ../install.sh

NAME='babl'
VERSION='0.1.124'
DOWNLOAD_URL="https://download.gimp.org/pub/babl/0.1/babl-$VERSION.tar.xz#1b0d544ab6f409f2b1b5f677226272d1e8c6d373f2f453ee870bfc7e5dd4f1b1"
DEPENDENCIES=('lcms2')
CONFIGURE_OPTIONS=(
	'-Dprefix=/usr'
	'-Dbuildtype=release'
	'-Dwith-docs=false'
	'-Denable-gir=false'
	'-Denable-vapi=false'
	'-Dgi-docgen=disabled'
	'-Denable-avx2=false'
)

configure() {
	meson setup \
		--reconfigure \
		--cross-file "$MESON_CROSS_FILE" \
		"${CONFIGURE_OPTIONS[@]}" \
		build || exit 1
}

post_configure() {
	grep -q '<alloca.h>' build/config.h || echo '#include <alloca.h>' >> build/config.h
}

build() {
	meson compile -C build || exit 1
}

install() {
	meson install --destdir="$DESTDIR" -C build || exit 1
}
