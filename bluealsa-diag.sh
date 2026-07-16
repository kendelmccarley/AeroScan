#!/bin/sh
# BlueALSA A2DP diagnostic. Run on the Pi as root.
# Determines why the paired headset connects but produces no audio (no A2DP
# transport / PCM): is the bluealsa daemon on the bus, has it registered an
# A2DP endpoint with BlueZ, and does a transport form when we connect?
MAC="${1:-20:22:06:11:04:17}"

sec() { printf '\n===== %s =====\n' "$1"; }

sec "bluealsa version"
bluealsa --version 2>&1 | head -1

sec "is org.bluealsa on the D-Bus system bus?"
dbus-send --system --print-reply --dest=org.freedesktop.DBus \
    /org/freedesktop/DBus org.freedesktop.DBus.ListNames 2>/dev/null \
    | grep -i alsa || echo "(org.bluealsa NOT on the bus — daemon wedged/not registered)"

sec "A2DP endpoints / transports known to BlueZ"
dbus-send --system --print-reply --dest=org.bluez / \
    org.freedesktop.DBus.ObjectManager.GetManagedObjects 2>/dev/null \
    | grep -iE 'MediaEndpoint|MediaTransport|/A2DP|UUID.*110[abd]' | head -40 \
    || echo "(no media endpoints/transports found)"

sec "headset link + resolved profiles"
bluetoothctl info "$MAC" | grep -iE 'Connected|Name|ServicesResolved|UUID'

sec "connect the audio profile explicitly"
bluetoothctl connect "$MAC"
sleep 3

sec "transports after connect"
dbus-send --system --print-reply --dest=org.bluez / \
    org.freedesktop.DBus.ObjectManager.GetManagedObjects 2>/dev/null \
    | grep -iE 'MediaTransport|State|/fd[0-9]|/A2DP' | head -20 \
    || echo "(still no transport)"

sec "bluealsa recent log"
journalctl -u bluealsa -n 15 --no-pager

printf '\n===== END =====\n'
