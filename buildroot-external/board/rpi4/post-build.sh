#!/bin/sh
# Runs after the rootfs is assembled, before image creation.
set -e

if [ -z "${TARGET_DIR}" ]; then
    echo "ERROR: post-build.sh must be run as part of Buildroot (TARGET_DIR not set)"
    exit 1
fi

# ─── OS release ───────────────────────────────────────────────────────────────
OS_RELEASE="${TARGET_DIR}/usr/lib/os-release"
if [ -f "${OS_RELEASE}" ]; then
    sed -i 's/PRETTY_NAME=.*/PRETTY_NAME="AeroScan OS"/g' "${OS_RELEASE}"
    cat >> "${OS_RELEASE}" <<EOF
IMAGE_ID=aeroscan-os
IMAGE_VERSION=$(date '+%Y%m%dT%H%M%S')
VENDOR_NAME="AeroScan Project"
EOF
fi

# ─── Framebuffer console login (tty1) — prompt only, no autologin ───────────
mkdir -p "${TARGET_DIR}/etc/systemd/system/getty.target.wants"
ln -snf /usr/lib/systemd/system/getty@.service \
    "${TARGET_DIR}/etc/systemd/system/getty.target.wants/getty@tty1.service"
# Remove autologin drop-in if a previous build left it in TARGET_DIR.
# The overlay no longer contains autologin.conf, but overlay rsync only adds
# files; it does not delete ones removed from the overlay.
rm -f "${TARGET_DIR}/etc/systemd/system/getty@tty1.service.d/autologin.conf"

# ─── Decompress kernel modules ────────────────────────────────────────────────
# kms.fragment sets MODULE_COMPRESS_NONE so this is normally a no-op, but kept
# as a safety net in case the kernel defconfig overrides the fragment.
find "${TARGET_DIR}/lib/modules" -name "*.ko.xz" | while read -r f; do
    xz --decompress --keep "$f"
    rm -f "$f"
done
KERNEL_VER=$(ls "${TARGET_DIR}/lib/modules/" 2>/dev/null | head -1)
if [ -n "${KERNEL_VER}" ]; then
    "${HOST_DIR}/sbin/depmod" -a -b "${TARGET_DIR}" "${KERNEL_VER}"
fi

# ─── SSH daemon ───────────────────────────────────────────────────────────────
if [ -f "${TARGET_DIR}/usr/lib/systemd/system/sshd.service" ]; then
    mkdir -p "${TARGET_DIR}/etc/systemd/system/multi-user.target.wants"
    ln -snf /usr/lib/systemd/system/sshd.service \
        "${TARGET_DIR}/etc/systemd/system/multi-user.target.wants/sshd.service"
fi

# ─── Bluetooth service ────────────────────────────────────────────────────────
if [ -f "${TARGET_DIR}/usr/lib/systemd/system/bluetooth.service" ]; then
    mkdir -p "${TARGET_DIR}/etc/systemd/system/multi-user.target.wants"
    ln -snf /usr/lib/systemd/system/bluetooth.service \
        "${TARGET_DIR}/etc/systemd/system/multi-user.target.wants/bluetooth.service"
fi

# ─── BlueALSA (Bluetooth headphone audio, service file from overlay) ──────────
if [ -f "${TARGET_DIR}/usr/bin/bluealsa" ]; then
    mkdir -p "${TARGET_DIR}/etc/systemd/system/multi-user.target.wants"
    ln -snf /usr/lib/systemd/system/bluealsa.service \
        "${TARGET_DIR}/etc/systemd/system/multi-user.target.wants/bluealsa.service"
fi

chmod +x "${TARGET_DIR}/usr/sbin/aeroscan-bt-init"       2>/dev/null || true
chmod +x "${TARGET_DIR}/usr/sbin/aeroscan-drm-init"      2>/dev/null || true
chmod +x "${TARGET_DIR}/usr/sbin/aeroscan-display-init"  2>/dev/null || true

# aeroscan-gui and aeroscan-bt-init are enabled via the systemd preset file
# overlay/etc/systemd/system-preset/70-aeroscan-disable.preset.
# That preset is read by systemctl preset-all (which runs in the fakeroot step,
# after this script), so do not create/remove those symlinks here.

# ─── Purge retired bluetooth debug/pairing tooling ────────────────────────────
# Bluetooth pairing moved in-app (BlueZ D-Bus). These files
# were removed from the overlay, but overlay rsync never deletes from
# TARGET_DIR — stale copies from older builds kept shipping and hijacking boot
# (aeroscan-bt-pair stopped the GUI and took over tty1).
for tool in aeroscan-bt-connect aeroscan-bt-diag aeroscan-bt-keepalive \
            aeroscan-bt-lpm-disable aeroscan-bt-pair aeroscan-bt-restore; do
    rm -f "${TARGET_DIR}/usr/sbin/${tool}" \
          "${TARGET_DIR}/usr/lib/systemd/system/${tool}.service" \
          "${TARGET_DIR}/etc/systemd/system/bluetooth.service.wants/${tool}.service" \
          "${TARGET_DIR}/etc/systemd/system/multi-user.target.wants/${tool}.service"
done
rm -rf "${TARGET_DIR}/etc/bluetooth/pre-bonded"
# Login-shell debug hook (started GUI + ran aeroscan-bt-diag on every login)
rm -f "${TARGET_DIR}/etc/profile.d/aeroscan-touch-diag.sh"

# ─── DRM HDMI init service ────────────────────────────────────────────────────
if [ -f "${TARGET_DIR}/usr/lib/systemd/system/aeroscan-drm-hdmi-init.service" ]; then
    mkdir -p "${TARGET_DIR}/etc/systemd/system/multi-user.target.wants"
    ln -snf /usr/lib/systemd/system/aeroscan-drm-hdmi-init.service \
        "${TARGET_DIR}/etc/systemd/system/multi-user.target.wants/aeroscan-drm-hdmi-init.service"
fi

# ─── dump1090 service ─────────────────────────────────────────────────────────
if [ -f "${TARGET_DIR}/usr/lib/systemd/system/dump1090.service" ]; then
    mkdir -p "${TARGET_DIR}/etc/systemd/system/multi-user.target.wants"
    ln -snf /usr/lib/systemd/system/dump1090.service \
        "${TARGET_DIR}/etc/systemd/system/multi-user.target.wants/dump1090.service"
fi

# ─── gpsd socket activation ───────────────────────────────────────────────────
if [ -f "${TARGET_DIR}/usr/lib/systemd/system/gpsd.socket" ]; then
    mkdir -p "${TARGET_DIR}/etc/systemd/system/sockets.target.wants"
    ln -snf /usr/lib/systemd/system/gpsd.socket \
        "${TARGET_DIR}/etc/systemd/system/sockets.target.wants/gpsd.socket"
fi

# ─── WiFi: wpa_supplicant + networkd + timesyncd ─────────────────────────────
mkdir -p "${TARGET_DIR}/etc/systemd/system/multi-user.target.wants"
mkdir -p "${TARGET_DIR}/etc/systemd/system/sysinit.target.wants"

ln -snf /usr/lib/systemd/system/aeroscan-wifi.service \
    "${TARGET_DIR}/etc/systemd/system/multi-user.target.wants/aeroscan-wifi.service"

if [ -f "${TARGET_DIR}/usr/lib/systemd/system/systemd-networkd.service" ]; then
    ln -snf /usr/lib/systemd/system/systemd-networkd.service \
        "${TARGET_DIR}/etc/systemd/system/multi-user.target.wants/systemd-networkd.service"
fi

if [ -f "${TARGET_DIR}/usr/lib/systemd/system/systemd-resolved.service" ]; then
    ln -snf /usr/lib/systemd/system/systemd-resolved.service \
        "${TARGET_DIR}/etc/systemd/system/multi-user.target.wants/systemd-resolved.service"
    rm -f "${TARGET_DIR}/etc/resolv.conf"
    ln -sf /run/systemd/resolve/resolv.conf "${TARGET_DIR}/etc/resolv.conf"
fi

if [ -f "${TARGET_DIR}/usr/lib/systemd/system/systemd-timesyncd.service" ]; then
    ln -snf /usr/lib/systemd/system/systemd-timesyncd.service \
        "${TARGET_DIR}/etc/systemd/system/sysinit.target.wants/systemd-timesyncd.service"
fi

# ─── Writable runtime directories ─────────────────────────────────────────────
sed -i 's#:/root#:/var/root#g' "${TARGET_DIR}/etc/passwd"
mkdir -p "${TARGET_DIR}/var/root"
mkdir -p "${TARGET_DIR}/boot"   # FAT32 mount point (see fstab)
mkdir -p "${TARGET_DIR}/var/lib/wpa_supplicant"
mkdir -p "${TARGET_DIR}/var/run"

# ─── Aviation map tiles (OpenAIP overlay, CONUS z7-10) ────────────────────────
# The OpenAIP API key is not stored in the repo. Provide it either via the
# OPENAIP_KEY environment variable or a .openaip-key file at the project root
# (free key: https://www.openaip.net → Account → API Keys).
TILE_SCRIPT="${BR2_EXTERNAL_AEROSCAN_PATH}/../tools/fetch-aviation-tiles.py"
TILE_CACHE="${BR2_EXTERNAL_AEROSCAN_PATH}/../maps-cache"
TILE_DEST="${TARGET_DIR}/opt/winglet-gui/maps"
OPENAIP_KEY_FILE="${BR2_EXTERNAL_AEROSCAN_PATH}/../.openaip-key"
if [ -z "${OPENAIP_KEY:-}" ] && [ -f "${OPENAIP_KEY_FILE}" ]; then
    OPENAIP_KEY="$(cat "${OPENAIP_KEY_FILE}")"
fi

if [ -n "${OPENAIP_KEY:-}" ]; then
    # Bake the key into the image so the on-device tools (aeroscan-setup,
    # aeroscan-fetch-tiles) can refresh tiles without a rebuild.
    mkdir -p "${TARGET_DIR}/etc/aeroscan"
    printf '%s\n' "${OPENAIP_KEY}" > "${TARGET_DIR}/etc/aeroscan/openaip.key"
    chmod 644 "${TARGET_DIR}/etc/aeroscan/openaip.key"
fi

if [ -z "${OPENAIP_KEY:-}" ]; then
    echo "WARNING: no OpenAIP API key (set OPENAIP_KEY or create .openaip-key) — skipping aviation tile install"
elif [ ! -f "${TILE_SCRIPT}" ]; then
    echo "WARNING: ${TILE_SCRIPT} not found — skipping aviation tile install"
elif ! command -v python3 >/dev/null 2>&1; then
    echo "WARNING: python3 not found on host — skipping aviation tile install"
else
    echo "─── Aviation tiles: syncing to cache (--resume skips existing) ───"
    python3 "${TILE_SCRIPT}" \
        --key   "${OPENAIP_KEY}" \
        --bbox  24.0 -125.0 49.5 -66.0 \
        --zooms 7-10 \
        --out   "${TILE_CACHE}" \
        --resume

    echo "─── Aviation tiles: installing into rootfs ───"
    mkdir -p "${TILE_DEST}"
    cp -a "${TILE_CACHE}/." "${TILE_DEST}/"
    echo "    $(find "${TILE_DEST}" -name '*.png' | wc -l) tiles installed to ${TILE_DEST}"
fi
