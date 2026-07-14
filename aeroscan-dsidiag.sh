#!/bin/sh
# AeroScan DSI display diagnostic. Run on the Pi as root.
# Figures out why the Hosyond DSI panel shows the rainbow splash but goes
# black at the firmware -> kernel handoff.
#
# The three failure layers this distinguishes:
#   A. Firmware never applied the vc4-kms-dsi-7inch overlay
#      (panel nodes absent from /proc/device-tree)
#   B. Overlay applied but the panel/bridge/regulator drivers failed to
#      probe (nodes present, but no DSI-1 connector / probe errors in dmesg)
#   C. DSI-1 is up and scanning out but the backlight is off
#      (connector "connected"+enabled, backlight bl_power/brightness wrong)
set -u

sec() { printf '\n===== %s =====\n' "$1"; }

sec "FIRMWARE VERSION"
vcgencmd version 2>&1

sec "FIRMWARE CONFIG AS PARSED (display-related)"
vcgencmd get_config int 2>&1 | grep -iE 'display|lcd|dsi|hdmi|hotplug' || echo "(nothing matched)"

sec "OVERLAYS THE FIRMWARE APPLIED (/proc/device-tree/chosen)"
# Newer firmware records applied overlays under chosen; older doesn't — absence
# of this node alone is not proof the overlay didn't load.
if [ -d /proc/device-tree/chosen/overlays ]; then
    ls /proc/device-tree/chosen/overlays
else
    echo "(no /proc/device-tree/chosen/overlays node on this firmware)"
fi

sec "LAYER A: DSI PANEL NODES IN LIVE DEVICE TREE?"
echo "--- toshiba,tc358762 (DSI bridge) compatible anywhere: ---"
grep -rl "toshiba,tc358762" /proc/device-tree 2>/dev/null || echo "ABSENT — overlay did NOT apply"
echo "--- attiny regulator (raspberrypi,7inch-touchscreen-panel-regulator): ---"
grep -rl "7inch-touchscreen-panel-regulator" /proc/device-tree 2>/dev/null || echo "ABSENT — overlay did NOT apply"
echo "--- DSI controller node status: ---"
for n in /proc/device-tree/soc/dsi@7e700000/status /proc/device-tree/soc/dsi@7e209000/status; do
    [ -f "$n" ] && printf '%s = %s\n' "$n" "$(tr -d '\0' < "$n")"
done

sec "LAYER B: LOADED DISPLAY MODULES"
lsmod 2>/dev/null | grep -iE 'vc4|v3d|tc358762|attiny|panel|ft5x06|raspberrypi_ts|backlight' \
    || echo "(none matched)"

sec "LAYER B: DRM CONNECTORS"
for d in /sys/class/drm/card[0-9]-*; do
    [ -d "$d" ] || continue
    printf '%s: status=%s enabled=%s\n' "$d" \
        "$(cat "$d/status" 2>/dev/null)" "$(cat "$d/enabled" 2>/dev/null)"
    [ -s "$d/modes" ] && sed 's/^/    mode: /' "$d/modes"
done

sec "LAYER B: dmesg — DSI / bridge / regulator / panel / probe errors"
dmesg 2>/dev/null | grep -iE 'dsi|tc358762|attiny|panel|regulator@45|deferred|probe.*fail' | tail -40

sec "LAYER B: dmesg — vc4 / drm summary"
dmesg 2>/dev/null | grep -iE 'vc4|drm|v3d' | tail -30

sec "LAYER B: I2C BUSES (panel regulator lives on the CSI/DSI i2c, usually i2c-10)"
ls -l /dev/i2c-* 2>/dev/null || echo "(no i2c dev nodes)"
for b in /sys/bus/i2c/devices/*; do
    [ -f "$b/name" ] && printf '%s: %s\n' "$b" "$(cat "$b/name")"
done 2>/dev/null | grep -iE 'i2c-1[01]|45|attiny|regulator' || true

sec "LAYER C: BACKLIGHT"
for bl in /sys/class/backlight/*; do
    [ -d "$bl" ] || continue
    printf '%s: brightness=%s max=%s bl_power=%s\n' "$bl" \
        "$(cat "$bl/brightness" 2>/dev/null)" \
        "$(cat "$bl/max_brightness" 2>/dev/null)" \
        "$(cat "$bl/bl_power" 2>/dev/null)"
done
[ -d /sys/class/backlight ] && [ -z "$(ls -A /sys/class/backlight 2>/dev/null)" ] \
    && echo "(no backlight devices registered)"

sec "aeroscan-drm-init LOG (grabs DRM master at boot; check what it picked)"
cat /var/log/aeroscan-drm-init.log 2>&1 | tail -30

sec "DEPLOYED config.txt / cmdline.txt"
for f in /boot/config.txt /boot/firmware/config.txt; do
    [ -f "$f" ] && { echo "--- $f (display lines) ---"; grep -niE 'dtoverlay|display|hdmi|dsi|lcd' "$f"; }
done
for f in /boot/cmdline.txt /boot/firmware/cmdline.txt; do
    [ -f "$f" ] && { echo "--- $f ---"; cat "$f"; }
done

printf '\n===== DONE =====\n'
