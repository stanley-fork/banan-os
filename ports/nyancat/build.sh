#!/bin/bash ../install.sh

NAME='nyancat'
VERSION='git'
DOWNLOAD_URL="https://github.com/klange/nyancat.git#1.5.2"

configure() {
	:
}

install() {
	mkdir -p "$DESTDIR/usr/bin"
	cp src/nyancat "$DESTDIR/usr/bin/"
}
