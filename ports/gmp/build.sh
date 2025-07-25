#!/bin/bash ../install.sh

NAME='gmp'
VERSION='6.3.0'
DOWNLOAD_URL="https://ftp.gnu.org/gnu/gmp/gmp-$VERSION.tar.xz#a3c2b80201b89e68616f4ad30bc66aee4927c3ce50e33929ca819d5c43538898"
CONFIG_SUB=('configfsf.sub')

# configure does not work with (default) c23
export CFLAGS="--std=c17 $CFLAGS"

post_install() {
	# remove libtool file
	rm -f $BANAN_SYSROOT/usr/lib/libgmp.la
}
