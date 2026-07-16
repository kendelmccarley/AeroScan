#!/bin/sh
# AeroScan audio / RTL-SDR diagnostic. Run on the Pi as root.
# Collects everything needed to fix the stuck "STARTING RTL" tuner and to
# figure out why the analog 3.5mm "Headphones" card isn't enumerating.
set -u

sec() { printf '\n===== %s =====\n' "$1"; }

sec "PLAYBACK CARDS (aplay -l)"
aplay -l 2>&1

sec "/proc/asound/cards"
cat /proc/asound/cards 2>&1

sec "LOADED SND MODULES"
lsmod 2>/dev/null | grep -iE 'snd|bcm2835' || echo "(none loaded)"

sec "snd-bcm2835 MODULE PRESENT IN IMAGE?"
find /lib/modules -iname '*bcm2835*' 2>/dev/null || echo "(no bcm2835 module file found)"

sec "DEPLOYED config.txt AUDIO LINES"
for f in /boot/config.txt /boot/firmware/config.txt; do
    [ -f "$f" ] && { echo "--- $f ---"; grep -niE 'audio|dtparam|dtoverlay|audremap' "$f"; }
done

sec "dmesg AUDIO / bcm2835"
dmesg 2>/dev/null | grep -iE 'bcm2835|audremap|snd_|vc4.*hdmi.*audio' | tail -25

sec "GENERATED /etc/asound.conf"
cat /etc/asound.conf 2>&1

sec "PAIRED / CONNECTED BLUETOOTH AUDIO"
for m in $(bluetoothctl devices 2>/dev/null | awk '{print $2}'); do
    echo "--- $m ---"
    bluetoothctl info "$m" 2>/dev/null | grep -iE 'Name|Connected'
done

sec "dump1090 STATE"
systemctl is-active dump1090.service 2>&1
systemctl is-enabled dump1090.service 2>&1

sec "RTL-SDR DONGLES"
for d in /sys/bus/usb/devices/*/idVendor; do
    v=$(cat "$d" 2>/dev/null); [ "$v" = "0bda" ] && \
        echo "$(dirname "$d") product=$(cat "$(dirname "$d")/idProduct" 2>/dev/null)"
done

printf '\n===== END =====\n'
