#!/bin/bash ../install.sh

NAME='gimp'
VERSION='3.2.4'
DOWNLOAD_URL="https://download.gimp.org/gimp/v3.2/gimp-$VERSION.tar.xz#7312bc53e9c6d2d0056ca7b93f1c6b98707946dd934f714c21b8746ecb601588"
DEPENDENCIES=('gtk3' 'gegl' 'gexiv2' 'glib-networking' 'librsvg' 'lcms2' 'libmypaint' 'mypaint-brushes' 'AppStream' 'libarchive' 'libXmu' 'poppler')
CONFIGURE_OPTIONS=(
	'-Dprefix=/usr'
	'-Dbuildtype=release'
	'-Dshmem-type=sysv'
	'-Dgi-docgen=disabled'
)

pre_configure() {
	# allow native pkgconfig lookup
	unset PKG_CONFIG_DIR PKG_CONFIG_SYSROOT_DIR PKG_CONFIG_LIBDIR PKG_CONFIG_PATH
}

configure() {
	meson setup \
		--reconfigure \
		--cross-file "$MESON_CROSS_FILE" \
		"${CONFIGURE_OPTIONS[@]}" \
		_build || exit 1
}

build() {
	meson compile -C _build || exit 1
}

install() {
	meson install --destdir="$DESTDIR" -C _build || exit 1
}
