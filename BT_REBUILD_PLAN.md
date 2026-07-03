 > **SUPERSEDED (2026-07-03).** The baked-link-key architecture described here
> has been replaced by in-app pairing over the BlueZ D-Bus API — see
> `BLUETOOTH.md`. The hardware analysis below (BCM4345C0 deep sleep, LPM
> disable) is still accurate and is why `aeroscan-bt-init` exists.

# Bluetooth Auto-Connect Rebuild Plan
## Apple Magic Keyboard B8:F6:B1:04:29:A6 on RPi4 BCM4345C0

---

## Hardware Context

- **SBC**: Raspberry Pi 4 (BCM2711)
- **BT chip**: BCM43455 / BCM4345C0 combo (WiFi+BT)
- **BT transport**: PL011 UART (ttyAMA0) at 3 Mbps after firmware load
- **Kernel attach**: `dtparam=krnbt=on` in config.txt — no userspace btattach needed
- **UART console**: mini-UART (ttyS0) — PL011 is reserved for BT exclusively
- **Adapter MAC**: E4:5F:01:07:16:65
- **Keyboard MAC**: B8:F6:B1:04:29:A6
- **BlueZ version**: 5.72

---

## Root Cause (BCM4345C0 Deep Sleep)

BCM4345C0 enters **UART deep sleep within 1–2 seconds of UART idle**. The RPi4 device
tree has no `device-wakeup-gpios` entry, so the kernel cannot assert BT_DEV_WAKE to
wake the chip. Once asleep, all HCI commands time out with `ETIMEDOUT (-110)`.

The vendor command `HCI_VSC_Write_Sleep_Mode` (OGF=0x3f, OCF=0x27) disables deep sleep
for the session. It must be sent within 1–2 s of the adapter becoming UP — before the
chip's first sleep cycle.

---

## Phase Status

| Phase | Description | Status |
|---|---|---|
| BT-1 | Diagnostic pass | ✅ COMPLETE |
| BT-2 | Collapse to one init script | ✅ COMPLETE |
| BT-3 | Verify LPM disable | ✅ COMPLETE |
| BT-4 | Passive reconnect | ❌ FAILED — keyboard did not connect |
| BT-4b | Diagnose reconnect failure | ✅ COMPLETE — keyboard in fresh-pair mode, key cleared |
| BT-5 | Re-pair keyboard | ✅ COMPLETE — new key 7766FC3... overlaid |
| BT-6 | Boot verify auto-reconnect | ← CURRENT |

---

## What Is Confirmed Working

- `hcitool` and `hciconfig` present in image (`BR2_PACKAGE_BLUEZ5_UTILS_TOOLS=y` +
  `BR2_PACKAGE_BLUEZ5_UTILS_DEPRECATED=y`)
- `aeroscan-bt-init` runs at boot, completes in ~3 s, exits 0
- LPM disable: HCI VSC command sent, firmware acknowledged with status `0x00` ✅
  ```
  < HCI Command: ogf 0x3f, ocf 0x0027, plen 12
    00 00 00 00 00 00 00 00 00 00 00 00
  > HCI Event: 0x0e plen 4
    01 27 FC 00        ← status 0x00 = accepted
  ```
- Page scan confirmed: `hciconfig -a` shows `UP RUNNING PSCAN ISCAN` ✅
- Bonding info file in place at `/var/lib/bluetooth/E4:5F:01:07:16:65/B8:F6:B1:04:29:A6/info` ✅
- `bluetoothctl show` shows adapter powered, connectable, discoverable ✅

---

## What Failed — Phase BT-4 Post-Mortem (logs 1–5)

Initial logs showed `devices Paired` returning nothing and scan finding only unrelated
BLE devices. At that point it was unclear whether BlueZ had loaded the device at all.

## New Findings — 7th Diagnostic Log

The 7th log changes the picture significantly:

**1. `devices Paired` NOW returns the keyboard** ✅
`bluetoothctl devices Paired` returned `Device B8:F6:B1:04:29:A6 digits's Keyboard`.
BlueZ has the keyboard in its runtime device list and considers it paired.

**2. Keyboard appeared during `scan on`**
`bluetoothctl devices` showed the keyboard during active inquiry scan. This is the
critical clue:
- A keyboard in **reconnect mode (double-flash)** pages the Pi directly. It may also
  respond to inquiry if it's doing ISCAN. Appearing in scan does NOT rule out reconnect
  mode — Apple keyboards sometimes do both.
- A keyboard in **fresh-pair mode (single-flash)** does inquiry scan and will appear
  here but CANNOT connect because the Pi has `Pairable: no`.
- `Pairable: no` in the adapter state means new pairings are rejected. This does NOT
  affect reconnections from known devices (which only need PSCAN + matching link key).

**3. The journal from the scan window was missing** ← root cause of diagnostic gap
The old `aeroscan-bt-diag` captured the journal BEFORE the BT SCAN section ran. Any
BlueZ logs from when the keyboard appeared (~21:56:40) were not captured. This has
been fixed: the script now adds a **POST-SCAN JOURNAL** section after the scan.

**4. `bluetoothctl info B8:F6:B1:04:29:A6` was never called** ← added
The script now calls this PRE-SCAN so we can see Connected/Paired/Trusted flags and
any profile state before the keyboard is powered on.

---

## Phase BT-4b — Diagnostic Fix Applied

`aeroscan-bt-diag` has been updated with two additions:

**PRE-SCAN KEYBOARD INFO** — runs before the scan:
```
sep "BLUETOOTHCTL INFO - KEYBOARD (PRE-SCAN)"
echo "info B8:F6:B1:04:29:A6" | bluetoothctl
```
Look for: `Paired: yes`, `Trusted: yes`, `Connected: no/yes`, any profile entries.
If `Device not available` → BlueZ hasn't loaded it (structural problem, see below).

**POST-SCAN JOURNAL** — runs after the 15-second scan window:
```
sep "JOURNAL - BLUETOOTH (POST-SCAN)"
journalctl -u bluetooth --boot=0 --no-pager -n 500
```
This captures whatever BlueZ logged during the keyboard connection attempt.

**How to run:** Flash the updated image, boot, run `aeroscan-bt-diag`, turn the keyboard
on during the `BT SCAN - 15 SECONDS` window. The log will now capture the full picture.

**What to look for in the POST-SCAN JOURNAL:**

**Reconnect mode (double-flash) + link key matches → CONNECTED:**
```
bluetoothd: profiles/input/device.c:... Connected
bluetoothd: src/device.c:device_connect_cb() bdaddr B8:F6:B1:04:29:A6
```

**Reconnect mode (double-flash) + link key mismatch → AUTH FAILURE:**
```
bluetoothd: error "Authentication Failed" (52)
bluetoothd: src/device.c:bonding_attempt_cb() bdaddr B8:F6:B1:04:29:A6 status 0x05
```
→ Proceed to Phase BT-5 (re-pair).

**Fresh-pair mode (single-flash) → NO CONNECTION ATTEMPT:**
Journal will show the inquiry response (device found) but no page/connection event.
`Post-scan keyboard info` section will show `Connected: no`.
→ Proceed to Phase BT-5 (re-pair). The Pi's `Pairable: no` is intentional for
  security; re-pair must be done manually once then the overlay key is updated.

**`bluetoothctl info` shows "Device not available" (PRE-SCAN section):**
BlueZ didn't load the device despite the info file being on disk.
```sh
systemctl restart bluetooth
sleep 2
bluetoothctl info B8:F6:B1:04:29:A6
```
If restart fixes it, replace the power-cycle in `aeroscan-bt-init` with
`systemctl restart bluetooth` (more reliable for forcing device cache reload).

---

## Phase BT-5 — Re-pair Keyboard (auto-launch, headless)

**Status**: CURRENT — keyboard cleared its stored key when put into pairing mode.

**Tool**: `aeroscan-bt-pair.service` — auto-launched at boot after `aeroscan-bt-init`.

### Normal boot path (keyboard already paired, correct link key)
`aeroscan-bt-pair` waits 30 s for the keyboard to reconnect on its own. If it connects
within that window, the service exits silently and the GUI is never interrupted.

### Pairing path (no connection after 30 s)
1. `aeroscan-gui` is stopped so the display is free
2. tty1 text console becomes visible on the HDMI panel
3. Instructions appear: put keyboard into FRESH-PAIR MODE (rapid single flash)
4. Script scans for up to 90 s; pairs when keyboard appears
5. **No PIN needed** — Apple Magic Keyboard uses BR/EDR legacy pairing (LegacyPairing=yes,
   Type=0, PINLength=0). BlueZ negotiates a zero-length PIN automatically. No agent or
   user input is required. Pairing completes silently.
6. On success:
   - New bonding info saved to `/boot/aeroscan-bt-pair-result.txt` (FAT32, readable from laptop)
   - GUI restarts automatically
8. On your dev machine: copy the content of that file into the overlay and rebuild:
   ```
   buildroot-external/board/rpi4/overlay/etc/bluetooth/pre-bonded/B8:F6:B1:04:29:A6/info
   make rpi4
   ```

### If pairing fails
The keyboard must be in RAPID SINGLE FLASH mode (not double-flash). Reboot and
put the keyboard into fresh-pair mode early (before the 90-second scan window closes).

---

## Keyboard LED Reference

| LED pattern | Meaning | Action |
|---|---|---|
| Double-flash (pause, repeat) | Has stored link key, paging host | Phase BT-4b — watch journal |
| Rapid single flash | Fresh-pair mode (key cleared) | Phase BT-5 — re-pair |
| Solid | Connected | Done |
| Off | Powered off or dead battery | Charge keyboard |

---

## Target Behavior (Definition of Done)

1. Pi boots with keyboard off
2. Keyboard is switched on — double-flashes (has stored key, trying to reconnect)
3. Within 10 seconds, keyboard LED goes solid — connected
4. Keystrokes appear in aeroscan-gui
5. If keyboard is switched off and back on, it reconnects automatically without any
   manual intervention

No `bluetoothctl` commands, no pairing mode, no manual steps.

---

## Remaining Open Questions

| Question | Status |
|---|---|
| Does `bluetoothctl info B8:F6:B1:04:29:A6` show device in runtime? | ❓ Will be in next log (PRE-SCAN section) |
| What is keyboard LED state when powered on? | ❓ User must note: double-flash = reconnect, single-flash = fresh-pair |
| Does journal show connection events when keyboard powers on? | ❓ Will be in next log (POST-SCAN JOURNAL section) |
| Is link key `8DF1…` still current on keyboard? | ❓ Auth failure in post-scan journal = stale key → Phase BT-5 |
| Does `systemctl restart bluetooth` load device into runtime better than power cycle? | ❓ Check if PRE-SCAN info shows "Device not available" |
