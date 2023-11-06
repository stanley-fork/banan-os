#!/bin/bash

if [[ -z $BANAN_DISK_IMAGE_PATH ]]; then
	echo  "You must set the BANAN_DISK_IMAGE_PATH environment variable" >&2
	exit 1
fi

if [[ -z $BANAN_SYSROOT ]]; then
	echo  "You must set the BANAN_SYSROOT environment variable" >&2
	exit 1
fi

if [[ -z $BANAN_TOOLCHAIN_PREFIX ]]; then
	echo  "You must set the BANAN_TOOLCHAIN_PREFIX environment variable" >&2
	exit 1
fi

if [[ -z $BANAN_ARCH ]]; then
	echo  "You must set the BANAN_ARCH environment variable" >&2
	exit 1
fi

DISK_SIZE=$[50 * 1024 * 1024]
MOUNT_DIR=/mnt

truncate -s 0 "$BANAN_DISK_IMAGE_PATH"
truncate -s $DISK_SIZE "$BANAN_DISK_IMAGE_PATH"

if [ "$BANAN_UEFI_BOOT" == "1" ]; then
	sed -e 's/\s*\([-\+[:alnum:]]*\).*/\1/' << EOF | fdisk "$BANAN_DISK_IMAGE_PATH" > /dev/null
	  g     # gpt
	  n     # new partition
	  1     # partition number 1
	        # default (from the beginning of the disk)
	  +16M  # efi system size
	  n     # new partition
	  2     # partition number 2
	        # default (right after efi system partition)
			# default (to the end of disk)
	  t     # set type
	  1     # ... of partition 1
	  1     # efi system
	  t     # set type
	  2     # ... of partition 2
	  20    # Linux filesystem
	  w     # write changes
EOF
else
	sed -e 's/\s*\([-\+[:alnum:]]*\).*/\1/' << EOF | fdisk "$BANAN_DISK_IMAGE_PATH" > /dev/null
	  g     # gpt
	  n     # new partition
	  1     # partition number 1
	        # default (from the beginning of the disk)
	  +1M   # bios boot partition size
	  n     # new partition
	  2     # partition number 2
	        # default (right after bios partition)
			# default (to the end of disk)
	  t     # set type
	  1     # ... of partition 1
	  4     # bios boot partition
	  t     # set type
	  2     # ... of partition 2
	  20    # Linux filesystem
	  w     # write changes
EOF
fi

LOOP_DEV=$(sudo losetup -f --show "$BANAN_DISK_IMAGE_PATH")
sudo partprobe $LOOP_DEV

PARTITION1=${LOOP_DEV}p1
PARTITION2=${LOOP_DEV}p2

sudo mkfs.ext2 -b 1024 -q $PARTITION2

if [[ "$BANAN_UEFI_BOOT" == "1" ]]; then
	sudo mkfs.fat $PARTITION1 > /dev/null
	sudo mount $PARTITION1 "$MOUNT_DIR"
	sudo mkdir -p "$MOUNT_DIR/EFI/BOOT"
	sudo "$BANAN_TOOLCHAIN_PREFIX/bin/grub-mkstandalone" -O "$BANAN_ARCH-efi" -o "$MOUNT_DIR/EFI/BOOT/BOOTX64.EFI" "boot/grub/grub.cfg=$BANAN_TOOLCHAIN_DIR/grub-memdisk.cfg"
	sudo umount "$MOUNT_DIR"

	sudo mount $PARTITION2 "$MOUNT_DIR"
	sudo mkdir -p "$MOUNT_DIR/boot/grub"
	sudo cp "$BANAN_TOOLCHAIN_DIR/grub-uefi.cfg" "$MOUNT_DIR/boot/grub/grub.cfg"
	sudo umount "$MOUNT_DIR"
else
	sudo mount $PARTITION2 "$MOUNT_DIR"
	sudo grub-install --no-floppy --target=i386-pc --modules="normal ext2 multiboot" --boot-directory="$MOUNT_DIR/boot" $LOOP_DEV
	sudo mkdir -p "$MOUNT_DIR/boot/grub"
	sudo cp "$BANAN_TOOLCHAIN_DIR/grub-legacy-boot.cfg" "$MOUNT_DIR/boot/grub/grub.cfg"
	sudo umount "$MOUNT_DIR"
fi

sudo losetup -d $LOOP_DEV