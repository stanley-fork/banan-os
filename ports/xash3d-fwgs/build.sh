#!/bin/bash ../install.sh

NAME='xash3d-fwgs'
VERSION='git'
DOWNLOAD_URL="https://github.com/FWGS/xash3d-fwgs.git#da1b9ad80d76156a5cbd54d3ce87edb32634ea87"
DEPENDENCIES=('SDL2' 'freetype' 'bzip2' 'libvorbis')

configure() {
	git submodule update --init --recursive || exit 1

	pushd 3rdparty/mainui || exit 1
	for patch in ../../../patches/mainui/*; do
		git apply $patch
	done
	popd

	./waf configure -T release || exit 1
}

build() {
	./waf build || exit 1
}

install() {
	./waf install --destdir="$DESTDIR/usr/share/games/halflife" || exit 1

	cat > "$DESTDIR/home/user/halflife/start.sh" << EOF
#!/bin/Shell
export LD_LIBRARY_PATH=/home/user/halflife
./xash3d -console
EOF
	chmod +x $DESTDIR/home/user/halflife/start.sh
}
