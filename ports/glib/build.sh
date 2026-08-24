#!/bin/bash ../install.sh

NAME='glib'
VERSION='2.85.2'
DOWNLOAD_URL="https://download.gnome.org/sources/glib/${VERSION%.*}/glib-$VERSION.tar.xz#833b97c0f0a1bfdba1d0fbfc36cd368b855c5afd9f02b8ffb24129114ad051b2"
DEPENDENCIES=('pcre2' 'libffi' 'zlib' 'libiconv')
CONFIGURE_OPTIONS=(
	'-Dprefix=/usr'
	'-Dbuildtype=release'
	'-Dxattr=false'
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
	cat > "$DESTDIR/etc/init.d/glib-compile-schemas" << EOF
#!/bin/Shell
exec glib-compile-schemas /usr/share/glib-2.0/schemas
EOF
	chmod +x "$DESTDIR/etc/init.d/glib-compile-schemas"
}
