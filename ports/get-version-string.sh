#!/bin/bash

REVISION=1

source 'build.sh'

[[ "$VERSION" == 'git' ]] && VERSION=0.0

echo "${NAME}-${VERSION//[_-]/.}_${REVISION}.${BANAN_ARCH}"
