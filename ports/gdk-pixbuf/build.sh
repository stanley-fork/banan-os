#!/bin/bash ../install.sh

NAME='gdk-pixbuf'
VERSION='2.44.4'
DOWNLOAD_URL="https://gitlab.gnome.org/GNOME/gdk-pixbuf/-/archive/$VERSION/gdk-pixbuf-$VERSION.tar.gz#6de2f77d992155b4121d20036e7e986dfe595a0e654381cdd0d7257f493c208a"
DEPENDENCIES=('glib' 'libpng' 'libjpeg-turbo' 'libtiff' 'shared-mime-info')
CONFIGURE_OPTIONS=(
	'-Dprefix=/usr'
	'-Dbuildtype=release'
	'-Dtests=false'
	'-Dman=false'
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

post_install() {
	mkdir -p "$DESTDIR/etc/init.d"
	cat > "$DESTDIR/etc/init.d/gdk-pixbuf-update-loaders" << EOF
#!/bin/Shell
exec gdk-pixbuf-query-loaders --update-cache
EOF
	chmod +x "$DESTDIR/etc/init.d/gdk-pixbuf-update-loaders"
}
