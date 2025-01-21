#!/bin/bash

# Define variables
IMAGE_NAME="disk_image.qcow2"
IMAGE_SIZE="10G"
EFI_PARTITION_SIZE="200M"
FAT32_PARTITION_SIZE="9.8G"
MOUNT_DIR="/mnt"

# Check for root privileges
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

# Create qcow2 image
qemu-img create -f qcow2 "$IMAGE_NAME" "$IMAGE_SIZE"

# Expose qcow2 image as a block device
modprobe nbd max_part=16
qemu-nbd --connect=/dev/nbd0 "$IMAGE_NAME"

# Partition the block device
echo -e "o\nn\np\n1\n\n+${EFI_PARTITION_SIZE}\nt\n1\n1\nn\np\n2\n\n\n\nw" | fdisk /dev/nbd0

# Format partitions
mkfs.vfat -F32 -n EFI /dev/nbd0p1
mkfs.vfat -F32 -n FAT32 /dev/nbd0p2

# Mount partitions
mkdir -p "$MOUNT_DIR/efi"
mkdir -p "$MOUNT_DIR/fat32"
mount /dev/nbd0p1 "$MOUNT_DIR/efi"
mount /dev/nbd0p2 "$MOUNT_DIR/fat32"

# Create folders and files
mkdir -p "$MOUNT_DIR/efi/efi_folder/sub_folder"
mkdir -p "$MOUNT_DIR/fat32/fat32_folder/sub_folder"
echo "File inside EFI partition" > "$MOUNT_DIR/efi/efi_folder/sub_folder/file_in_efi.txt"
echo "File inside FAT32 partition" > "$MOUNT_DIR/fat32/fat32_folder/sub_folder/file_in_fat32.txt"

# Unmount partitions
umount "$MOUNT_DIR/efi"
umount "$MOUNT_DIR/fat32"

# Disconnect qcow2 image from block device
qemu-nbd --disconnect /dev/nbd0

echo "Disk image created: $IMAGE_NAME"
