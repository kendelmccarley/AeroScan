#!/bin/sh
# aeroscan-btdiag.sh — snapshot BT-audio / SDR / tuner state across phases.
#
# Usage on the Pi (run as root):
#   ./aeroscan-btdiag.sh reset          # start a fresh log
#   ./aeroscan-btdiag.sh before         # BEFORE launching aeroscan-gui
#   ./aeroscan-btdiag.sh during         # while on the tuner page (ideally AS it goes silent)
#   ./aeroscan-btdiag.sh after          # after backing out / closing the GUI
#
# Everything is appended to /tmp/aeroscan-btdiag.log AND printed to the screen.
# Optional 2nd arg overrides the headset MAC.

PHASE="${1:-unlabeled}"
MAC="${2:-20:22:06:11:04:17}"
LOG=/tmp/aeroscan-btdiag.log

if [ "$PHASE" = "reset" ]; then
    : > "$LOG"
    echo "cleared $LOG"
    exit 0
fi

sec() { printf '\n--- %s ---\n' "$1"; }

# jiffies of CPU (utime+stime) a pid has used, from /proc/<pid>/stat fields 14+15
cpu_jiffies() { awk '{print $14 + $15}' "/proc/$1/stat" 2>/dev/null; }
first_pid()   { pidof "$1" 2>/dev/null | awk '{print $1}'; }

run() {
    printf '\n==================================================================\n'
    printf 'PHASE=%s   %s   uptime=%ss\n' "$PHASE" "$(date '+%F %T')" "$(cut -d. -f1 /proc/uptime)"
    printf '==================================================================\n'

    sec "processes (pid / name / args)"
    ps -eo pid,comm,args 2>/dev/null | grep -iE 'aeroscan-gui|rtl_fm|aplay|bluealsa|dump1090' | grep -v grep \
        || echo "(none of the target processes running)"

    # Blocked-vs-active probe: sample rtl_fm & aplay CPU over 3s. While audio is
    # actually flowing both burn CPU; a ~0 delta means the process is blocked
    # (the "everything alive but silent" stall).
    sec "cpu progress over 3s  (~0 while 'playing' => BLOCKED/stalled)"
    rp=$(first_pid rtl_fm); ap=$(first_pid aplay)
    r1=$(cpu_jiffies "$rp"); a1=$(cpu_jiffies "$ap")
    sleep 3
    r2=$(cpu_jiffies "$rp"); a2=$(cpu_jiffies "$ap")
    [ -n "$rp" ] && echo "rtl_fm pid=$rp cpu_delta=$(( ${r2:-0} - ${r1:-0} ))" || echo "rtl_fm: not running"
    [ -n "$ap" ] && echo "aplay  pid=$ap cpu_delta=$(( ${a2:-0} - ${a1:-0} ))" || echo "aplay:  not running"

    sec "FIFO (created by the fixed GUI while the tuner is open)"
    ls -l /tmp/aeroscan-radio.pcm 2>&1

    sec "headset link"
    bluetoothctl info "$MAC" 2>/dev/null | grep -iE 'Name|Paired|Connected' || echo "(bluetoothctl/info failed)"

    sec "A2DP transport state (bluez MediaTransport1: active/pending/idle)"
    dbus-send --system --print-reply --dest=org.bluez / \
        org.freedesktop.DBus.ObjectManager.GetManagedObjects 2>/dev/null \
        | grep -iA25 'MediaTransport1' \
        | grep -iE 'MediaTransport1|"State"|"Volume"|"Codec"|active|pending|idle' | head -20 \
        || echo "(no MediaTransport — A2DP not streaming)"

    sec "bluealsa daemon on the bus?"
    dbus-send --system --print-reply --dest=org.freedesktop.DBus /org/freedesktop/DBus \
        org.freedesktop.DBus.ListNames 2>/dev/null | grep -i bluealsa || echo "(org.bluealsa ABSENT)"

    sec "softvol 'Radio' volume"
    amixer -c Headphones sget Radio 2>/dev/null | grep -iE '%|\[on\]|\[off\]' || echo "(no Radio control)"

    sec "recent Bluetooth kernel messages"
    dmesg 2>/dev/null | grep -iE 'Bluetooth: hci0' | tail -10

    sec "controller/link errors this boot (timeouts / disconnects)"
    dmesg 2>/dev/null | grep -iE 'hci0.*(tx timeout|hardware error|Disconnection|supervision|-110)' | tail -12 \
        || echo "(none)"

    sec "RTL-SDR dongles + dump1090"
    for d in /sys/bus/usb/devices/*/idVendor; do
        v=$(cat "$d" 2>/dev/null)
        [ "$v" = "0bda" ] && echo "dongle: $(dirname "$d") pid=$(cat "$(dirname "$d")/idProduct" 2>/dev/null)"
    done
    echo "dump1090: $(systemctl is-active dump1090.service 2>/dev/null)"

    sec "power / thermal (rule in/out undervoltage)"
    echo "throttled: $(vcgencmd get_throttled 2>/dev/null)"
    echo "temp:      $(vcgencmd measure_temp 2>/dev/null)"
    echo "volts:     $(vcgencmd measure_volts 2>/dev/null)"

    printf '\n===== END PHASE=%s =====\n' "$PHASE"
}

run 2>&1 | tee -a "$LOG"
