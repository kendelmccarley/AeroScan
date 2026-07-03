#!/bin/bash
# Patch the mounted SD card rootfs with the DRM HDMI init fix.
# Run from the host with the SD card mounted at /media/kendel/aeroscan-root/.

sudo mkdir -p /media/kendel/aeroscan-root
sudo mount /dev/sdb2 /media/kendel/aeroscan-root

sudo cp /home/kendel/Vibe/AeroScan/buildroot-external/board/rpi4/overlay/usr/sbin/aeroscan-drm-init \
    /media/kendel/aeroscan-root/usr/sbin/aeroscan-drm-init && \
    sudo chmod +x /media/kendel/aeroscan-root/usr/sbin/aeroscan-drm-init

sudo cp /home/kendel/Vibe/AeroScan/buildroot-external/board/rpi4/overlay/usr/lib/systemd/system/aeroscan-drm-hdmi-init.service \
    /media/kendel/aeroscan-root/usr/lib/systemd/system/aeroscan-drm-hdmi-init.service

sudo ln -snf /usr/lib/systemd/system/aeroscan-drm-hdmi-init.service \
    /media/kendel/aeroscan-root/etc/systemd/system/multi-user.target.wants/aeroscan-drm-hdmi-init.service
