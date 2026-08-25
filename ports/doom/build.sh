#!/bin/bash ../install.sh

NAME='doom'
VERSION='git'
DOWNLOAD_URL="https://github.com/ozkl/doomgeneric.git#dcb7a8dbc7a16ce3dda29382ac9aae9d77d21284"
DEPENDENCIES=('sdl2-compat' 'SDL2_mixer' 'timidity')

configure() {
	rm -rf doomgeneric/build
}

build() {
	if [ ! -f ../doom1.wad ]; then
		wget 'https://bananymous.com/files/doom1.wad' -O ../doom1.wad || exit 1
	fi

	make -C doomgeneric -f Makefile.sdl CC="$CC" SDL_PATH="$BANAN_SYSROOT/usr/bin/" || exit 1
}

install() {
	mkdir -p "$DESTDIR/usr/bin"
	cp -vf doomgeneric/doomgeneric "$DESTDIR/usr/bin/doom"

	mkdir -p "$DESTDIR/usr/share/games/doom"
	cp -vf ../doom1.wad "$DESTDIR/usr/share/games/doom/"
}
