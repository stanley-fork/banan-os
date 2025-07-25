#!/bin/bash

if [ -z $BANAN_DISK_IMAGE_PATH ]; then
	echo  "You must set the BANAN_DISK_IMAGE_PATH environment variable" >&2
	exit 1
fi

if [ -z $BANAN_SYSROOT_TAR ]; then
	echo  "You must set the BANAN_SYSROOT_TAR environment variable" >&2
	exit 1
fi

if [ -z $BANAN_BUILD_DIR ]; then
	echo  "You must set the BANAN_BUILD_DIR environment variable" >&2
	exit 1
fi

if [ "$1" == "full" ] || [ ! -f $BANAN_DISK_IMAGE_PATH ]; then
	$BANAN_SCRIPT_DIR/image-create.sh || exit 1
fi

set -u

MOUNT_DIR="$BANAN_BUILD_DIR/mount"
mkdir -p $MOUNT_DIR

LOOP_DEV="$(sudo losetup --show -Pf $BANAN_DISK_IMAGE_PATH || exit 1)"
ROOT_PARTITION="${LOOP_DEV}p2"
if [ ! -b $ROOT_PARTITION ]; then
	echo "Failed to probe partitions for banan disk image." >&2
	sudo losetup -d $LOOP_DEV
	exit 1
fi

if sudo mount $ROOT_PARTITION $MOUNT_DIR; then
	if (($BANAN_INITRD)); then
		sudo mkdir -p $MOUNT_DIR/boot
		sudo cp $BANAN_BUILD_DIR/kernel/banan-os.kernel $MOUNT_DIR/boot/banan-os.kernel
		sudo cp $BANAN_SYSROOT_TAR $MOUNT_DIR/boot/banan-os.initrd
	else
		cd $MOUNT_DIR
		sudo tar xf $BANAN_SYSROOT_TAR
		cd
	fi

	sudo umount $MOUNT_DIR
fi

sudo losetup -d $LOOP_DEV
