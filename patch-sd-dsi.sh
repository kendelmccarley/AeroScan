#!/bin/bash
# Patch the SD card with the DSI display fixes, no rebuild needed:
#   boot:   config.txt (forced vc4-kms-dsi-7inch overlay), cmdline.txt (rotate:0)
#   rootfs: aeroscan-drm-init (skip HDMI PHY workaround when DSI connected),
#           aeroscan-dsidiag (diagnostic, in case it's still black)
# Run from the host with the SD card at /dev/sdb (adjust below if different).
set -e

REPO=/home/kendel/Vibe/AeroScan
BOARD="$REPO/buildroot-external/board/rpi4"
BOOT_MNT=/media/kendel/aeroscan-boot
ROOT_MNT=/media/kendel/aeroscan-root

sudo mkdir -p "$BOOT_MNT" "$ROOT_MNT"

# Mount each partition at its expected path, reusing an existing mount if it
# is already there (e.g. left over from a previous run) and unmounting
# desktop auto-mounts elsewhere first.
ensure_mount() {
    dev=$1; target=$2
    if findmnt -rno TARGET "$dev" | grep -qx "$target"; then
        return 0
    fi
    sudo umount "$dev" 2>/dev/null || true
    sudo mount "$dev" "$target"
}
ensure_mount /dev/sda1 "$BOOT_MNT"
ensure_mount /dev/sda2 "$ROOT_MNT"

sudo cp "$BOARD/config.txt"       "$BOOT_MNT/config.txt"
sudo cp "$BOARD/cmdline.txt"      "$BOOT_MNT/cmdline.txt"
sudo cp "$BOARD/cmdline-hdmi.txt" "$BOOT_MNT/cmdline-hdmi.txt"
sudo cp "$BOARD/display-dsi.txt"  "$BOOT_MNT/display-dsi.txt"
sudo cp "$BOARD/display-hdmi.txt" "$BOOT_MNT/display-hdmi.txt"

sudo install -m 0755 "$BOARD/overlay/usr/sbin/aeroscan-drm-init" \
    "$ROOT_MNT/usr/sbin/aeroscan-drm-init"
sudo install -m 0644 "$BOARD/overlay/etc/profile.d/qt_env.sh" \
    "$ROOT_MNT/etc/profile.d/qt_env.sh"
sudo install -m 0755 "$REPO/aeroscan-dsidiag.sh" \
    "$ROOT_MNT/usr/sbin/aeroscan-dsidiag"

# Patched attiny regulator module (clone-panel REG_POWERON fix).
# Must be newer than the kernel patch, i.e. `make rpi4-linux-rebuild` has run.
MOD=lib/modules/6.1.61-v8/kernel/drivers/regulator/rpi-panel-attiny-regulator.ko
KPATCH="$BOARD/linux/patches/0001-rpi-panel-attiny-legacy-poweron-for-clones.patch"
if [ ! "$REPO/output/rpi4/target/$MOD" -nt "$KPATCH" ]; then
    echo "ERROR: $MOD is older than the kernel patch."
    echo "Run 'make rpi4-linux-rebuild' first, then re-run this script."
    sudo umount "$BOOT_MNT" "$ROOT_MNT"
    exit 1
fi
sudo install -m 0644 "$REPO/output/rpi4/target/$MOD" "$ROOT_MNT/$MOD"

sync
if ! sudo umount "$BOOT_MNT" "$ROOT_MNT"; then
    echo "Unmount was busy — data is already synced. Detach with:"
    echo "  sudo umount -l $BOOT_MNT $ROOT_MNT"
fi
echo "SD card patched — boot the Pi."
