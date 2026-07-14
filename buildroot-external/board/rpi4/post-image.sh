#!/bin/sh
# Runs after all images are built; assembles the flashable SD card image.
set -e

BOARD_DIR="$(dirname "$0")"
GENIMAGE_CFG="${BOARD_DIR}/genimage.cfg"
GENIMAGE_TMP="${BUILD_DIR}/genimage.tmp"

# Always stamp the latest config.txt, cmdline variants, and display selection
# files into the firmware directory so genimage picks them up even when the
# rpi-firmware package stamp is current.
install -D -m 0644 "${BOARD_DIR}/config.txt"       "${BINARIES_DIR}/rpi-firmware/config.txt"
install -D -m 0644 "${BOARD_DIR}/cmdline.txt"      "${BINARIES_DIR}/rpi-firmware/cmdline.txt"
install -D -m 0644 "${BOARD_DIR}/cmdline-hdmi.txt" "${BINARIES_DIR}/rpi-firmware/cmdline-hdmi.txt"
install -D -m 0644 "${BOARD_DIR}/display-dsi.txt"  "${BINARIES_DIR}/rpi-firmware/display-dsi.txt"
install -D -m 0644 "${BOARD_DIR}/display-hdmi.txt" "${BINARIES_DIR}/rpi-firmware/display-hdmi.txt"

# genimage needs a clean tmpdir each run.
rm -rf "${GENIMAGE_TMP}"

genimage \
    --rootpath "${TARGET_DIR}" \
    --tmppath  "${GENIMAGE_TMP}" \
    --inputpath "${BINARIES_DIR}" \
    --outputpath "${BINARIES_DIR}" \
    --config "${GENIMAGE_CFG}"

echo ""
echo "─────────────────────────────────────────────────────────"
echo "  SD card image ready: ${BINARIES_DIR}/sdcard.img"
echo ""
echo "  Flash command (replace sdX with your card device):"
echo "  sudo dd if=${BINARIES_DIR}/sdcard.img of=/dev/sdX bs=4M oflag=direct conv=fsync status=progress"
echo "─────────────────────────────────────────────────────────"
