# Bluetooth Architecture (current)

In-app bluetooth keyboard pairing over the BlueZ D-Bus API. Replaces the old
baked-link-key overlay approach (see superseded `BT_PHASE2.md` /
`BT_REBUILD_PLAN.md` for that history).

## Design

| Layer | Component | Role |
|---|---|---|
| Kernel | `board/rpi4/linux/bluetooth.fragment` + `modules-load.d/bluetooth.conf` | BT core, hci_uart, hidp modules |
| Boot | `aeroscan-bt-init` (+ `.service`) | Disables BCM4345C0 UART deep sleep (LPM) in the 1–2 s window after hci0 UP, enables page scan for reconnects |
| Daemon | bluetoothd (BlueZ 5.72) + `/etc/bluetooth/main.conf` | `AutoEnable=true`, `FastConnectable=true`; persists link keys in `/var/lib/bluetooth` (on SD card, survives reboots) |
| App worker | `aeroscan-gui/winglet-ui/worker/bluetoothmonitor.{h,cpp}` | QtDBus client of `org.bluez`: device tracking (ObjectManager), discovery, pair/trust/connect, remove. Registers an `org.bluez.Agent1` (capability `DisplayOnly`) so SSP keyboards can show a passkey on screen |
| App UI | Settings → **Bluetooth** (below WiFi) | **Pair Keyboard** (scan selector → pairing message box with passkey display), **Paired Devices** (Connect / Forget Device) |

## Pairing flows

- **Apple A1314 (legacy, zero PIN):** pairs silently, no prompt.
- **Legacy keyboards needing a PIN:** agent generates a PIN, the UI shows it,
  user types it on the keyboard + Enter.
- **Modern SSP keyboards:** bluetoothd calls `DisplayPasskey`; the UI shows the
  6-digit code, user types it on the keyboard + Enter.

After pairing the device is set `Trusted` so bluetoothd accepts its pages at
boot — power the keyboard on and it reconnects on its own (PSCAN is enabled by
`aeroscan-bt-init`).

## Key facts

- Link keys live only in `/var/lib/bluetooth/` at runtime. Nothing is baked
  into the image; reflashing the SD card just means re-pairing once on-device.
- `Pairable` is off except while an in-app pairing is running.
- Works unchanged with a USB BT dongle (RPi2 Phase 11) — nothing in the app is
  BCM4345C0-specific; only `aeroscan-bt-init` is, and it is an rpi4 overlay.

## Build notes

- `BR2_PACKAGE_QT5BASE_DBUS=y` (rpi4 defconfig) and `QT += dbus` (`.pro`).
- After changing the defconfig, qt5base must be explicitly reconfigured:

```
make rpi4-aeroscan_rpi4_defconfig     # refresh output/rpi4/.config
make rpi4-qt5base-reconfigure         # rebuild Qt with QtDBus
rm -rf output/rpi4/target/var/lib/bluetooth   # purge stale baked key from target dir
make rpi4-aeroscan-gui-reconfigure    # re-run qmake (QT += dbus) + rebuild GUI
make rpi4                             # assemble image
```

## On-device test matrix

1. Pair A1314 (fresh-pair mode, rapid single flash) → no prompt → connected.
2. Reboot → keyboard off/on → double-flash → solid within ~10 s → keys work.
3. Pair a modern SSP keyboard → passkey prompt → type code + Enter → connected.
4. Paired Devices → Forget Device → `/var/lib/bluetooth/<adapter>/<mac>` gone,
   keyboard no longer reconnects.
