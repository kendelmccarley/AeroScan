# AeroScan Release Notes — July 2026 development snapshot

## Developer hardware configuration

| Component | In use |
|---|---|
| Board | Raspberry Pi 4 Model B |
| Display | Hosyond 5" DSI, 800×480 landscape, FT5406 capacitive touch (official-RPi-7" clone; requires the bundled legacy-ATTiny kernel patch). Waveshare 4" HDMI + XPT2046 remains selectable via `config.txt` include. |
| SDR | RTL2832U / R820T USB stick, single-dongle configuration (two different sticks tested) |
| ADS-B antenna | General-purpose whip — **under suspicion, see RF status** |
| GPS | u-blox 7 USB receiver |
| WiFi / BT | Onboard BCM43455 (5 GHz WiFi in use; BT runs the Raspberry Pi-tuned BCM4345C0 firmware after the idle-wedge fix) |
| Input | Panel touch + USB or Bluetooth keyboard; F12/PrintScreen captures screenshots |

## RF reception status — panel-generated jamming and antenna limitations

ADS-B reception at the development site is currently poor and inconsistent.
The interferer has been identified — **the DSI display panel jams the
receiver** — and the practical mitigations are a USB extension cable to
remote the SDR stick away from the panel/Pi, plus antenna placement
experiments:

- The receiver noise floor swings between about **−42 dBFS (quiet, frames
  decode)** and **−30 dBFS (jammed, nothing decodes)** on a timescale of
  minutes. During jammed periods the demodulator logs hundreds of thousands
  of noise-triggered preambles with zero valid CRCs.
- **Source identified: the DSI display panel.** The panel/ribbon generates
  broadband interference that raises the 1090 MHz noise floor when the SDR
  dongle is close to it. Mitigation: remote the receiver on a USB extension
  cable and experiment with antenna placement away from the electronics
  (watch the instrument rail's NF meter while moving things).
- One SDR stick exhibited an effectively **open antenna input** (−41 dBFS
  floor, pure noise) — believed to be an antenna connector mismatch
  (MCX vs SMA). Antenna/stick pairing matters.
- Even in quiet periods, position (CPR) decode rate is low — single-frame
  fields (callsign, altitude, velocity) decode while paired-frame positions
  often don't, so aircraft can appear in the Flight List with dashed
  distance and no plot on the scopes. A tuned 1090 MHz antenna with sky
  view is the expected fix.
- `dump1090` error correction is now enabled (`--no-fix` removed) — this
  was a genuine decode regression, verified ~10× message-rate improvement.
- The new on-screen SDR meters (right rail: NF / MSG / POS / DROP) exist
  precisely to make these experiments measurable at a glance.

## New in this snapshot

- **ADS-B**: dump1090 self-starts with the GUI (manual launches included);
  error correction enabled; live decoder statistics published to
  `/run/dump1090` and shown on the instrument rail.
- **Display**: Hosyond DSI support with one-line `config.txt` panel selector
  and clone-panel kernel patch; Bluetooth idle-wedge firmware fix.
- **Layout**: new 100 / 600 / 100 screen plan — Parhelia-branded left touch
  column (zones in the bottom half), 600 px main display (legacy screens
  auto-centered, MapScope and Media Player full-bleed), instrument rail
  right: GPS/ADSB indicators, SDR stats, Pi vitals (CPU/freq/load/RAM/temp
  with throttle flag/net/disk/uptime), date, and GPS-timezone 24 h clock.
- **Scopes**: aircraft icons and labels doubled; aircraft dim to 35 % after
  30 s without an update; radar sweep smoothed (4.5°/50 ms, one atomic
  frame per paint — no more double-line stutter); MapScope shows ~25 % more
  map and tolerates missing edge tiles.
- **Flight List**: Total Aircraft is now cumulative since app start.
- **Settings**: Known Networks shows the live WiFi IP; "Device Info" title;
  credits updated.
- **Status**: three-state ADS-B icon (red no-feed / amber feed / green
  aircraft heard).
- **Tooling**: `flash-sd.sh` verified flashing, `patch-sd-dsi.sh` SD
  patching, `aeroscan-dsidiag`, screenshot hotkey, `AEROSCAN_ADSB_DEBUG` /
  `AEROSCAN_SHOT_PERIOD` debug hooks.

## Still under investigation

- Mitigation of the panel-generated RF interference (USB-extension remoting
  of the receiver, antenna selection and placement — measure with the
  on-screen NF/MSG/POS meters); quantifying achievable range once quiet.
- Whether the ADS-B indicator's green state should require a *plottable*
  aircraft (position decoded) rather than merely one heard — proposed, not
  yet decided.

## Implemented but not yet proven solid

- Stale-aircraft dimming (implemented and deployed; not yet visually
  confirmed with live traffic).
- Dual-dongle operation (ADS-B + tuner simultaneously) — untested since the
  recent dongle-handoff changes.
- Radio tuner UI on the new 600 px-era layout — needs a regression pass.
- Waveshare HDMI panel path since the display-selector rework — untested.
- Bluetooth keyboard pairing long-run stability (firmware fix verified at
  the minutes scale only).
- GPS-derived timezone uses approximate CONUS zone boundaries — correct in
  zone interiors, approximate near borders; solar time outside the US.
- Raspberry Pi 2 target — not exercised recently; consider it stale.

## Not yet implemented

- README gallery screenshots showing live traffic on the scopes (the current
  set was captured with an empty sky — see RF status).
- Hardware: USB extension for the receiver, tuned 1090 MHz antenna, and
  permanent siting away from the display panel.
- Possible indicator refinements above; dim-threshold tuning against real
  position-update rates.
- Remaining roadmap items per `DEVELOPMENT_PLAN.md`.
