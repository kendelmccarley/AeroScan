 > **SUPERSEDED (2026-07-03).** Keys are no longer baked into the overlay.
> Pairing is now done on-device from Settings → Bluetooth → Pair Keyboard,
> and BlueZ persists the link key in `/var/lib/bluetooth` on the SD card.
> See `BLUETOOTH.md`.

# BT Phase 2 — Re-pair Keyboard and Bake Key into Overlay

## Prerequisites

- Phase 1 is built and flashed (clean BT infrastructure, LPM disable working)
- Pi is booted and reachable via SSH
- Apple Magic Keyboard A1314, MAC: B8:F6:B1:04:29:A6
- The key in the overlay (7766FC3...) is STALE — keyboard cleared it during debugging

## Why This Is Needed

The keyboard was put into pairing mode multiple times during development, which
clears its stored link key. The overlay contains the old stale key. After Phase 1
is flashed, the Pi has LPM working so the adapter stays awake, but the first
connection attempt will fail with auth error (key mismatch). This phase re-pairs
once and bakes the new key into the overlay permanently.

## Step 1 — Verify LPM is working after boot

SSH in and check that aeroscan-bt-init ran successfully:

    systemctl status aeroscan-bt-init

Expected: active (exited), exit code 0.
Also check hciconfig shows PSCAN:

    hciconfig hci0

Expected: UP RUNNING PSCAN (page scan enabled).

If aeroscan-bt-init failed, check the journal:

    journalctl -u aeroscan-bt-init --no-pager

## Step 2 — Put keyboard into fresh-pair mode

Hold the keyboard power button until the LED blinks RAPIDLY (single fast flash).
This is fresh-pair mode — the keyboard has cleared its stored key and is
advertising for a new pairing. Do NOT release until you see rapid blinking.

## Step 3 — Pair from the Pi via SSH

Run these one at a time:

    bluetoothctl pairable on
    bluetoothctl scan on

Wait for the keyboard to appear. You will see:
    [NEW] Device B8:F6:B1:04:29:A6 digits's Keyboard

Then (replace MAC if different):

    bluetoothctl pair B8:F6:B1:04:29:A6
    bluetoothctl trust B8:F6:B1:04:29:A6
    bluetoothctl scan off
    bluetoothctl pairable off

Pairing should complete silently (Apple A1314 uses zero-PIN legacy pairing —
no passkey prompt, no user interaction needed).

## Step 4 — Capture the new link key

    cat /var/lib/bluetooth/E4:5F:01:07:16:65/B8:F6:B1:04:29:A6/info

Copy the entire output.

## Step 5 — Update the overlay on the dev machine

Replace the contents of:

    buildroot-external/board/rpi4/overlay/var/lib/bluetooth/E4:5F:01:07:16:65/B8:F6:B1:04:29:A6/info

with the output from Step 4.

## Step 6 — Rebuild

    make rpi4

The new image has the correct key baked in. Future reflashes will not require
re-pairing.

## Step 7 — Verify auto-reconnect

1. Boot the Pi (from the newly flashed image or the existing one — key is already
   live on disk from Step 3)
2. Wait for boot to complete
3. Switch the keyboard OFF then ON
4. LED should double-flash (reconnect mode) then go solid within ~10 seconds
5. Type something — keystrokes should appear on screen

## Notes

- The key in the overlay is only needed for reflashes. If you never reflash,
  the live key on the SD card from Step 3 persists.
- If the keyboard is put into fresh-pair mode again (rapid single flash), the
  key is cleared and this phase must be repeated from Step 2.
- The keyboard MAC B8:F6:B1:04:29:A6 is hardcoded in the overlay path.
  If you ever use a different keyboard, create a new directory with its MAC.
