#!/bin/bash

if [[ -z $BANAN_ROOT_DIR ]]; then
	export BANAN_ROOT_DIR="$(realpath $(dirname $(realpath "$0"))/..)"
fi

source "$BANAN_ROOT_DIR/script/config.sh"

package_standalone() {
	pushd "$BANAN_PORT_DIR/$1" >/dev/null

	local version_string="$(../get-version-string.sh)"
	if [ ! -f "$BANAN_PORT_DIR/package/repo/${version_string}.xbps" ]; then
		local dependency
		for dependency in $(source build.sh; echo ${DEPENDENCIES[*]}); do
			package_standalone "$dependency"
		done

		find "$BANAN_SYSROOT/usr/lib" \( -type f -o -type l \) -delete
		PACKAGE=1 ./build.sh || echo "$port_name" >> ../failed-ports
	fi

	popd >/dev/null
}

cd "$BANAN_PORT_DIR"
for build_script in */build.sh; do
	port_name="${build_script%/*}"

	[[ "$port_name" == 'llvm' ]] && continue

	package_standalone "$port_name"
done
