#!/bin/bash ../install.sh

NAME='harfbuzz'
VERSION='12.2.0'
DOWNLOAD_URL="https://github.com/harfbuzz/harfbuzz/releases/download/$VERSION/harfbuzz-$VERSION.tar.xz#ecb603aa426a8b24665718667bda64a84c1504db7454ee4cadbd362eea64e545"
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

post_install() {
	if [ -n "$HARFBUZZ_CIRCULAR" ]; then
		return
	fi

	export HARFBUZZ_CIRCULAR=1

	pushd "$BANAN_PORT_DIR/freetype" >/dev/null || exit 1
	rm -f .compile_hash
	./build.sh || exit 1
	popd >/dev/null

	if grep -q cairo "$BANAN_PORT_DIR/.installed_ports"; then
		pushd "$BANAN_PORT_DIR/cairo" >/dev/null || exit 1
		rm -f .compile_hash
		./build.sh || exit 1
		popd >/dev/null
	fi
}
