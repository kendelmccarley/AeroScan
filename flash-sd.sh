#!/bin/bash
# Safely flash the rpi4 sdcard.img to an SD card (default /dev/sda).
# Unmounts desktop auto-mounts first (writing the raw device while its
# partitions are mounted lets stale filesystem metadata flush over the fresh
# image — silent rootfs corruption), flashes with direct I/O, then verifies
# the written rootfs with a read-only fsck before declaring success.
#
# Usage: ./flash-sd.sh [/dev/sdX]
set -e

DEV="${1:-/dev/sda}"
IMG="$(dirname "$0")/output/rpi4/images/sdcard.img"

[ -f "$IMG" ] || { echo "ERROR: $IMG not found — run 'make rpi4' first."; exit 1; }
[ -b "$DEV" ] || { echo "ERROR: $DEV is not a block device."; exit 1; }

echo "Flashing $IMG -> $DEV"
lsblk -o NAME,SIZE,MOUNTPOINTS "$DEV"
read -r -p "This ERASES $DEV. Continue? [y/N] " ans
[ "$ans" = "y" ] || [ "$ans" = "Y" ] || exit 1

# Unmount every mounted partition of the target device.
for part in $(lsblk -lno NAME "$DEV" | tail -n +2); do
    sudo umount "/dev/$part" 2>/dev/null || true
done

sudo dd if="$IMG" of="$DEV" bs=4M oflag=direct conv=fsync status=progress
sync

# Re-read the partition table, then verify the written rootfs.
sudo partprobe "$DEV"
sleep 2
echo "Verifying written rootfs (read-only fsck)..."
if sudo e2fsck -fn "${DEV}2"; then
    echo "OK — flash verified. Safe to remove the card."
else
    echo "VERIFY FAILED — do not boot this card; reflash (or try another card)."
    exit 1
fi
