# AeroScan — Development Plan

> **Status snapshot (v0.2a, July 2026):** the Raspberry Pi 4 is the primary and
> actively developed target (Phase 13 complete and substantially extended);
> the Pi 2 target is stale. The UI has evolved past the original 160/480/160
> layout to a 100/600/100 instrument-panel plan (see UI Layout). Current
> hardware configuration, verified features, open RF/antenna issues, and the
> not-yet-solid list live in [`RELEASE_NOTES.md`](RELEASE_NOTES.md).

## Project Overview

AeroScan is a port and extension of the avBadge 2024 (Winglet) project targeting Raspberry Pi
single-board computers. It preserves the minimal embedded Linux philosophy of the original
(Buildroot-based, no Raspbian) while replacing the custom Allwinner T113S hardware with
commodity RPi hardware and adding drone Remote ID detection alongside the existing ADS-B
aircraft tracking.

Source project: `~/Vibe/avBadge_2024-2.0`

---

## Hardware Targets

### Initial Target (Development & Validation)
| Component | Part |
|---|---|
| SBC | Raspberry Pi 2 Model B v1.1 (BCM2836, quad-core Cortex-A7 @ 900MHz, ARMv7, 1GB RAM) |
| Display | Waveshare 800x480 HDMI LCD with XPT2046 resistive touch (SPI touch interface) |
| ADS-B / Radio | RTL-SDR USB dongle (shared: ADS-B 1090 MHz and software radio receiver) |
| GPS | u-blox NEO-6M via USB-UART adapter |
| WiFi | USB 802.11n dongle (e.g. Ralink RT5370) |
| Bluetooth | USB BT 4.0 dongle (for drone Remote ID BLE, Phase 12) |
| Drone WiFi | Alfa AWUS036N USB adapter (RT3070, monitor mode, Phase 12) |
| Input | XPT2046 resistive touch (quadrant-based tap: upper-left=up, lower-left=down, upper-right=select, lower-right=back) |

**Display interface:** HDMI carries the video signal. XPT2046 connects separately via SPI
(GPIO 9/10/11 for MOSI/MISO/CLK, plus CS and IRQ pins). The SPI bus is used only for touch.
All navigation is touch-based — no physical buttons or rotary encoder are used.

**ARMv7 advantage:** The RPi 2 v1.1 uses the same ARM Cortex-A7 instruction set as the
original T113S. Qt5 compiled for ARMv7 applies directly with no architecture change.

**No onboard wireless:** The RPi 2 Model B has no onboard WiFi or Bluetooth. USB dongles are
required for both. A powered USB hub is recommended when all USB peripherals are attached.

**RTL-SDR dongles:** both modes shipped (see Phase 6b): with two dongles, device 0
is dump1090 (ADS-B) and device 1 the radio tuner — no coverage gap; with a single
dongle the tuner hands off (stops dump1090, takes device 0, dump1090 restarts on
exit). Zero dongles shows a "No RTL-SDR dongle detected" message.

### Primary Target — AS BUILT (v0.2a development configuration)
| Component | Part |
|---|---|
| SBC | Raspberry Pi 4 Model B (BCM2711, quad-core Cortex-A72, ARMv8, 2GB+ RAM) |
| Display (DSI, default) | Hosyond 5" DSI 800x480 capacitive (official-RPi-7" clone; requires the bundled legacy-ATTiny kernel patch) |
| Display (HDMI, alternate) | Waveshare 4" 480x800 HDMI + XPT2046 resistive touch |
| SDR | RTL2832U/R820T USB stick (single-dongle handoff mode in daily use) |
| GPS | u-blox 7 USB receiver |
| WiFi / BT | Onboard BCM43455 (BT runs the RPi-tuned BCM4345C0.hcd — see `BCM4345C0.hcd.README`) |

Display selection is **not** firmware auto-detect (that failed for the Hosyond
clone): the active panel is chosen by an `include display-{dsi,hdmi}.txt` line
in `config.txt` on the boot partition, which also swaps the matching kernel
cmdline. Runtime pieces (`aeroscan-display-init`, Qt rotation, touch mapping)
adapt automatically. The original OSOYOO DSI panel was never used.

**Both panels are 800x480-class, so a single UI layout serves both.** The
RPi 4 is the development machine; the RPi 2 configuration below is retained
for reference but is stale (not exercised since the Pi 4 port).

---

## Software Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                        AeroScan Qt5 Application                     │
│  ┌─────────────┐  ┌──────────────┐  ┌──────────────┐  ┌─────────┐  │
│  │  RadarScope  │  │  FlightBoard │  │  MapScope    │  │  Clock  │  │
│  │  MapScope    │  │  DroneBoard* │  │  GPSTracker  │  │  etc.   │  │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  └────┬────┘  │
│         │                 │                  │               │       │
│  ┌──────▼─────────────────▼──────────────────▼───────────────▼────┐  │
│  │                    Worker Threads                               │  │
│  │  ADSBReceiver   GPSReceiver   DroneReceiver*   WifiMonitor      │  │
│  │  (TCP:30003)    (gpsd:2947)   (UDP:9999)       BattMonitor      │  │
│  └──────┬──────────────┬──────────────┬──────────────────────────┘  │
└─────────│──────────────│──────────────│──────────────────────────────┘
          │              │              │
┌─────────▼──────────────▼──────────────▼──────────────────────────────┐
│                     System Services (systemd)                        │
│   dump1090        gpsd           droneaware-ble*   droneaware-wifi*  │
│   (ADS-B)         (GPS)          (BLE scanner)     (WiFi monitor)    │
└──────────────────────────────────────────────────────────────────────┘

* = stub in initial build, active in Phase 12
```

### OS Layer
- **Build system**: Buildroot 2024
- **Primary target**: `aeroscan_rpi4_defconfig` (ARMv8 64-bit, custom external-tree defconfig)
- **Legacy target**: `aeroscan_rpi2_defconfig` (ARMv7 32-bit, stale)
- **Init system**: systemd
- **Root filesystem**: Read-only with writable `/var` overlay
- **Qt5 backend (RPi 2)**: `linuxfb` (HDMI framebuffer `/dev/fb0`)
- **Qt5 backend (RPi 4)**: `eglfs` (DRM/KMS)

### Display Summary
| Hardware | Resolution | Display Bus | Touch Bus | Qt5 Backend |
|---|---|---|---|---|
| RPi 2 + Waveshare XPT2046 | 800x480 | HDMI | SPI (XPT2046, resistive) | linuxfb |
| RPi 4 + HDMI touchscreen | 800x480 | HDMI | USB HID | eglfs |
| RPi 4 + Hosyond 5" DSI (default) | 800x480 | DSI ribbon | I2C (FT5406 capacitive) | eglfs |

All three configurations present Qt5 with an 800x480 surface. Touch input appears as
an evdev device regardless of the touch bus. One UI layout target serves all three.

---

## UI Layout — 800x480 (AS BUILT, v0.2a instrument-panel plan)

The original 160/480/160 quadrant-touch layout evolved into a three-column
instrument panel:

```
┌───────────┬──────────────────────────────────────────┬───────────┐
│ PARHELIA  │                                          │ GPS bar   │
│ (logo,    │        600 x 480 main display            │ ADSB bar  │
│  rotated  │                                          │ SDR stats │
│  90° CCW) │  Legacy 480x480 screens are centered     │ (msg/pos/ │
├───────────┤  via QStackedWidget contents margins;    │  sig/NF/  │
│ ▲ UP      │  screens with the "fullBleedScreen"      │  drop)    │
├───────────┤  property (MapScope, Media Player) get   │ Pi vitals │
│ ▼ DOWN    │  the full 600px.                         │ (cpu/load/│
├───────────┤                                          │ ram/temp/ │
│ ● SEL     │  Center taps in the display area         │ net/disk) │
├───────────┤  synthesize Key_A (select).              │ Date      │
│ ◀ BACK    │                                          │ Clock     │
└───────────┴──────────────────────────────────────────┴───────────┘
   100 px                     600 px                       100 px
```

- **Left column** (`TouchButtonOverlay`, Left): Parhelia wordmark in the upper
  half; UP / DOWN / SEL / BACK touch zones in the lower half (60px each).
  Zone taps synthesize `QKeyEvent`s; Key_B routes directly to `WingletGUI` so
  back navigation always works regardless of focus.
- **Right column** (`TouchButtonOverlay`, Right): live instrument rail on a
  2-second poll — GPS/ADS-B indicator bars, dump1090 decoder statistics (from
  `/run/dump1090/stats.json`), Pi vitals (with firmware throttle flag), date,
  and a 24-hour clock in the GPS-derived timezone.
- Compact status icons (WiFi/battery/date/GPS pin/three-state ADS-B airplane)
  remain inside each screen's content area (`StatusBar`).
- No display rotation is used on the DSI panel (landscape native); the HDMI
  panel path applies fbcon/Qt rotation via its cmdline/display-init settings.

---

## Development Phases

### Phase 0 — Toolchain & Buildroot Foundation ✓ COMPLETE
**Goal:** Cross-compiled ARMv7 image boots to shell on RPi 2.

- Set up Buildroot external tree under `~/Vibe/AeroScan/buildroot-external/`
- Start from `raspberrypi2_defconfig`
- Add packages: Qt5 (core, gui, widgets, network, multimedia, svg), GStreamer 1.0
  (base, good, bad), gpsd, wpa_supplicant, Python3, libgpiod, busybox, ca-certificates
- Boot to BusyBox shell over UART (115200 baud, GPIO 14/15 UART0)
- Verify SD card image generation and package build

**Deliverable:** RPi 2 boots to shell; UART console works.

---

### Phase 1 — Display & Touch Input ✓ COMPLETE
**Goal:** 800x480 HDMI framebuffer active; XPT2046 touch events register.

- Set HDMI resolution in `config.txt` (hdmi_group=2, hdmi_mode=87, hdmi_cvt=800 480 60)
- Enable `ads7846` kernel driver; device tree overlay for SPI wiring
- `post-build.sh` decompresses `.ko.xz` modules to `.ko` so kmod can load them without
  xz support; regenerates `modules.dep` with host depmod
- udev rule matches `ENV{ID_INPUT_TOUCHSCREEN}=="1"` → symlink to `/dev/input/touchscreen0`
- Calibrate with `ts_calibrate`; verify with `ts_test`
- Qt5 environment: `QT_QPA_PLATFORM=linuxfb:fb=/dev/fb0 QT_QPA_GENERIC_PLUGINS=tslib`

**Deliverable:** Qt5 window at 800x480 on HDMI display; touch events register.

---

### Phase 2 — Qt5 Application Compiles (Stub Mode) ✓ COMPLETE
**Goal:** `aeroscan-gui` builds for ARMv7 and launches to main menu on RPi 2.

- Fork `software/winglet-gui/` into `~/Vibe/AeroScan/aeroscan-gui/`
- Remove all T113S-specific hardware references: ledControl, saoMonitor, canardInterface
- Update main window geometry: `resize(800, 480)`, content stack at x=160 w=480
- Cross-compile as Buildroot package; deploy to RPi 2 SD card

**Deliverable:** Main menu visible on HDMI display at 800x480.

---

### Phase 3 — Touch Input ✓ COMPLETE
**Goal:** All navigation works via touch anywhere on the screen.

`WingletGUI::eventFilter()` intercepts all `QEvent::MouseButtonPress` events (Qt converts
XPT2046 touch to mouse events via tslib), maps the screen position to one of four quadrants,
and synthesizes the appropriate `QKeyEvent` press+release to the target widget. Key_B is
sent directly to WingletGUI to guarantee back navigation works from all sub-screens
regardless of which widget has focus.

Display is rotated 90° (`display_rotate=3`), so logical x/y axes are swapped relative to
physical touch coordinates. The quadrant thresholds (x < 400, y < 240 in logical
coordinates) are correct for this rotation.

**Deliverable:** Full navigation on all screens via touch; sub-screen back button works.

---

### Phase 4 — ADS-B (RTL-SDR + dump1090) ✓ COMPLETE
**Goal:** Live aircraft appear on FlightBoard and RadarScope.

- `BR2_PACKAGE_LIBRTLSDR=y`, `BR2_PACKAGE_DUMP1090=y` (mainline Buildroot v9.0,
  FlightAware fork)
- `/etc/modprobe.d/rtlsdr.conf` blacklists `dvb_usb_rtl28xxu`, `dvb_usb_v2`, `rtl2832`,
  `rtl2832_sdr` — prevents DVB kernel drivers from claiming the dongle before librtlsdr
- `dump1090.service` enabled at boot; serves SBS1 format on TCP port 30003
- `ADSBReceiver` worker connects to `localhost:30003`; parses MSG sentences
- `ADSBReceiver::gpsUpdated` slot added; connected to `GPSReceiver::gpsUpdated` signal
  so bearing/distance calculations update when a GPS fix is obtained (not just at startup)
- dump1090 service uses `Restart=on-failure` (not `always`) so the Radio Tuner can stop
  it without it immediately restarting

**Deliverable:** Live ADS-B aircraft visible on radar and flight list.

---

### Phase 5 — GPS ✓ COMPLETE
**Goal:** Own-ship position known; aircraft distance and bearing calculate correctly.

- `BR2_PACKAGE_GPSD=y` with `DEVICES="/dev/ttyUSB0"` and `BR2_PACKAGE_GPSD_UBX=y`
- `gpsd.socket` enabled via `sockets.target.wants` symlink in `post-build.sh`
  (socket-activated; starts gpsd on first client connection to port 2947)
- `GPSReceiver` worker connects to gpsd JSON protocol on `localhost:2947`; subscribes
  with `?WATCH={"enable":true,"nmea":true}`
- Last known position persisted to `/var/ui-settings.json` via `AppSettings`:
  saved on first fix, on position change > 0.01°, or every 60 seconds
- Default position (no prior fix): 32.231917°N, 110.951333°W (Tucson, AZ)
- GPS status icon in status bar: **green** when locked, **red** when no lock or
  disconnected. All windows (FlightBoard, RadarScope, MapScope, ADSBReceiver) initialize
  from `settings.lastLatitude/Longitude` and update live as fixes arrive

**Deliverable:** GPS position fix; aircraft distances correct on radar; persistent
last-known position survives reboots.

---

### Phase 6 — Software Radio Receiver ✓ COMPLETE (except 6k; regression pass pending)
**Goal:** Radio Tuner screen receives FM broadcast, aviation airband AM, and 2m amateur
NFM on a dedicated second RTL-SDR dongle, with presets, squelch, and a UI fully
operable from the four touch zones (no keyboard required).

**Status (v0.2a):** shipped and exercised on hardware — v2 touch-first UI, presets,
squelch, NASR airband data, and the single-dongle handoff (verified end-to-end during
the ADS-B debugging campaign: tuner open stops dump1090, close restarts it, the GUI
reconnects in ~8 s). Remaining: 6k (band scan, optional) and a UI regression pass on
the new 600 px-era layout (see RELEASE_NOTES.md).

| Sub-phase | Scope | Status |
|---|---|---|
| 6a | System prerequisites (rtl_fm, aplay, audio routing) | ✓ COMPLETE |
| 6b | Dongle architecture (2 dongles, no handoff) | ✓ COMPLETE |
| 6c | Audio pipeline (FM wbfm + airband AM) | ✓ COMPLETE |
| 6d | Airband frequency data (NASR nearby query) | ✓ COMPLETE |
| 6e | Tuner screen v1 (CanardBoard) | ✓ COMPLETE (superseded by 6i) |
| 6f | 2m amateur band (NFM) | ✓ COMPLETE |
| 6g | Squelch | ✓ COMPLETE (defaults may need site tuning) |
| 6h | User presets (favorites, JSON store) | ✓ COMPLETE |
| 6i | Tuner screen v2 — touch-first layout rework | ✓ COMPLETE (regression pass on 600px layout pending) |
| 6j | Preset browser + save-preset flow | ✓ COMPLETE |
| 6k | Band scan (optional stretch) | pending |

#### 6a — System prerequisites ✓ AS BUILT
- `rtl_fm` comes from `BR2_PACKAGE_LIBRTLSDR` (no separate rtl-sdr package needed).
- `BR2_PACKAGE_ALSA_UTILS` with `APLAY` + `AMIXER`.
- Audio output routing is owned by Settings → Audio (Headphone Jack / HDMI /
  Bluetooth), which writes `/etc/asound.conf`; `aplay` always opens the ALSA
  `default` device. The original open question "which ALSA device?" is resolved —
  the tuner never passes `-D`.
- `RtlFmWorker::restartAudio()` reopens the stream when the output route changes
  (called by `WingletGUI::writeAudioOutputConf` and on Bluetooth reconnect).

#### 6b — Dongle architecture ✓ AS BUILT (two-dongle concurrent + single-dongle handoff)
Two modes, chosen by dongle count (`RtlFmWorker::pollAvailability()` recounts
every 3 s):
- **Two+ dongles:** device 0 = dump1090 (ADS-B), device 1 = radio tuner
  (`rtl_fm -d 1`). Both run at once — no ADS-B coverage gap.
- **One dongle:** the tuner hands off. Opening it (`RtlFmWorker::startSession()`)
  stops `dump1090.service` and takes device 0 (`rtl_fm -d 0`); leaving the tuner
  (`stop()`) restarts dump1090. ADSBReceiver reconnects to dump1090's SBS port
  on its own once it is back. Hot-plugging the second dongle mid-session
  reconciles live (release device 0 back to ADS-B, move the tuner to device 1).

The main-menu Radio Tuner entry is always visible; selecting it with **zero**
dongles shows a "No RTL-SDR dongle detected" message instead of opening. The
tuner window still closes itself if the last dongle is pulled mid-session.

#### 6c — Audio pipeline ✓ AS BUILT (FM/AM; NFM added in 6f)
Two `QProcess` objects piped via `setStandardOutputProcess()`:
`rtl_fm -d 1 [mode opts] -` → `aplay -r 48000 -f S16_LE -t raw -c 1 -`

| Band | Freq encoding | rtl_fm flags (as built / planned) | Status |
|---|---|---|---|
| FM broadcast 87.9–107.9 MHz | int ×10 (1017 = 101.7) | `-M wbfm -s 170000 -r 48000 -g 20` | ✓ |
| Airband AM 118.000–136.975 MHz | kHz (121500) | `-M am -s 200000 -r 48000 -g 20` + `-l SQL` (6g) | ✓ (squelch pending) |
| 2m amateur 144.000–148.000 MHz | kHz (146520) | `-M fm -s 12000 -r 48000 -g 20 -l SQL` (`-E deemp` — verify on HW) | 6f |

Note: rtl_fm has no runtime control channel — every retune/squelch/mode change
restarts the pipeline. `tune()` already works this way; 6g adds a ~300 ms restart
debounce so held-down keys don't thrash the dongle.

#### 6d — Airband frequency data ✓ AS BUILT (replaces static frequencies.json for airband)
Airband presets are **live FAA NASR data**, not a baked file:
- Settings → Update Radio Data runs `/usr/libexec/aeroscan/nasr-update.py` →
  `/var/lib/aeroscan/apt_freq.csv` (+ `nasr_edition.txt`), refreshed on the FAA
  28-day cycle over WiFi.
- `NASRDatabase::queryNearby(lat, lon, 20 nm, max 30)` feeds the tuner using the
  GPS position (or the cached last-known position), sorted by distance, with
  ident + frequency type (TWR/CTAF/GND/ATIS…).
- FM and 2m have no equivalent source — they get user presets in 6h.

#### 6e — Tuner screen v1 ✓ AS BUILT — `winglet-ui/window/canardboard.{h,cpp}`
(Class name kept from the avBadge "Canard" RF add-on; this is the "RadioTuner"
of the original plan.) Digit-spinbox frequency entry, FM/Airband toggle, NASR
preset stepper, volume slider, Canard logo watermark, StatusBar.

**Known limitation motivating 6i:** v1 navigation depends on Left/Right (digit
cursor, mode switch) and Return (focus cycling, preset select). The touch overlay
only produces **Up / Down / A / B** (+ center-tap = A), so v1 is not fully operable
without a Bluetooth keyboard. v2 must be complete with those four keys.

#### 6f — 2m amateur band (NFM) ✓ CODE COMPLETE (2026-07-04, awaiting on-device verification)
- `RtlFmWorker`: add `MODE_HAM2M`; constants `HAM2M_MIN_FREQ = 144000`,
  `HAM2M_MAX_FREQ = 148000`, kHz encoding and 3 decimal digits like airband;
  last-digit tuning rounds to 5 kHz steps (same trick airband uses).
- Pipeline flags per the 6c table. Start without `-E deemp`; add if audio is
  shrill on-air.
- `AppSettings`: `canardLastHam2mFreq` (default 146520 — national simplex
  calling) and `canardLastHam2mPreset`, saved/restored by the tuner exactly like
  the FM/airband pairs.
- Band cycle order: FM → Airband → 2m.
- Receive-only; this is the substrate Phase 11 (APRS decode, 144.390) builds on.

#### 6g — Squelch ✓ CODE COMPLETE (2026-07-04, defaults need on-site tuning)
- `RtlFmWorker::setSquelch(int)` → rtl_fm `-l` (linear amplitude units, 0–~200,
  not dB). Defaults: AM = 60, NFM = 50, FM broadcast = 0 (always open — hide the
  control in FM mode).
- Persisted per mode: `canardSquelchAm`, `canardSquelchNfm` in `AppSettings`.
- Changing squelch restarts rtl_fm (see 6c debounce note).
- Optimal thresholds are site-dependent — tune on hardware, ship the defaults.

#### 6h — User presets (favorites) ✓ CODE COMPLETE (2026-07-05, awaiting on-device verification)
As built: `winglet-ui/worker/radiopresets.{h,cpp}` owned by `WingletGUI` (`presets`),
loads `/var/lib/aeroscan/radio_presets.json`, importing the seed
`/usr/share/aeroscan-gui/frequencies.json` (Buildroot common overlay) on first
run. `presetsForBand(mode)`, `add/remove/rename`, `presetsChanged()`; kHz stored
uniformly with `uiFreqToKhz()/khzToUiFreq()` at the boundary.

Original design below:
- Store: `/var/lib/aeroscan/radio_presets.json` — writable, survives reflash of
  `/usr`, created on first save. Seed defaults ship in the image at
  `/usr/share/aeroscan-gui/frequencies.json` (Buildroot overlay) and are imported
  on first run if the store doesn't exist:

```json
{
  "version": 1,
  "presets": [
    {"band": "airband", "name": "Guard",      "freq_khz": 121500},
    {"band": "ham2m",   "name": "2m Calling", "freq_khz": 146520},
    {"band": "ham2m",   "name": "APRS",       "freq_khz": 144390}
  ]
}
```

- New class `winglet-ui/worker/radiopresets.{h,cpp}`: GUI-thread store owned by
  `WingletGUI` (like `nasr`), lazy load, save-on-change, `presetsChanged()` signal.
  API: `presetsForBand(mode)`, `add(mode, name, freqKhz)`, `remove(id)`,
  `rename(id, name)`.
- FM encoding note: store `freq_khz` in the file (89100), convert to/from the
  ×10 UI encoding (891) at the store boundary so the file format is uniform.
- Airband keeps the NASR nearby list as its primary source; favorites supplement
  it (Guard, home-field frequencies that may be >20 nm away, etc.).

#### 6i — Tuner screen v2: touch-first layout rework (CanardBoard) ✓ CODE COMPLETE (2026-07-05, awaiting on-device verification)
As built: vertical control stack with Up/Down row navigation, A activate, and
per-row edit modes (frequency digit cursor, volume ±5, squelch ±5, live-tuning
preset stepper). Softvol volume wired through all three `writeAudioOutputConf`
variants (`Radio` control pinned to the Headphones card) + `amixer -c Headphones
sset Radio`. Two deliberate deviations from the sketch below: (1) presets are a
single live-tuning stepper row (favorites + NASR nearby) plus an "All Presets…"
row into the 6j browser, rather than several inline preset rows; (2) **B always
closes the tuner** — the platform routes touch-B straight to `WingletGUI`, so A
is the confirm/advance key for edit modes (there is no B-exits-edit).

Original design below:
**Interaction model** — matches the platform convention (ScrollableMenu/SelectorBox:
Up/Down navigate, A select, B back). The screen is a vertical control stack:

- **Up / Down** — move the selection highlight between rows.
- **A** — activate the selected row: cycle its value (Band), enter edit mode
  (Frequency, Volume, Squelch), tune it (preset rows), or open a screen
  (All Presets…, Save Preset).
- **In edit mode** — Up/Down adjust the value live; on Frequency, A advances the
  digit cursor left→right and exits edit after the last digit; on Volume/Squelch,
  A confirms. B exits edit mode without leaving the screen.
- **B** (outside edit mode) — close the tuner, stop the pipeline (existing behavior).

**Layout (480×480 centered content, circular display safe area, StatusBar on top):**

```
        ╭──────────────────────────────╮
        │         RADIO TUNER          │  title
        │                              │
        │    Band       ‹ AIRBAND ›    │  A cycles FM / AIRBAND / 2M
        │                              │
        │      ┌─┐┌─┐┌─┐ ┌─┐┌─┐┌─┐     │  frequency digit cells (large,
        │      │1││1││8│.│3││0││0│     │  reuse v1 spinbox styling);
        │      └─┘└─┘└─┘ └─┘└─┘└─┘     │  cursor under active digit
        │             MHz    ▲         │  in edit mode
        │                              │
        │    KTUS TWR       118.300    │  preset rows: favorites first,
        │    KTUS ATIS      123.800    │  then NASR nearby (airband);
        │    All Presets…              │  A on a row = tune it
        │                              │
        │    Volume   ▮▮▮▮▮▮▮▯▯▯  70%  │  A = edit, Up/Down = ±5
        │    Squelch  ▮▮▮▯▯▯▯▯▯▯  60   │  hidden in FM mode
        │    ★ Save Preset             │  opens 6j save flow
        │                              │
        │      ● LIVE      RTL #1      │  status: LIVE / STARTING /
        ╰──────────────────────────────╯  NO DONGLE / squelched
```

- Preset rows show 2–3 entries and scroll within the row region as the selection
  moves; airband labels come from NASR (`ident + freqType`), FM/2m from favorites.
- The wheel (if present / BT keyboard arrows) keeps adjusting volume directly, as
  in v1. Return remains a shortcut for A. Nothing *requires* them.
- Status line states: `● LIVE` (pipeline up), `○ STARTING`, `✕ NO DONGLE`
  (rtl_fm exited / <2 dongles — window auto-closes as today), plus the tuned
  dongle index for debugging.

**Volume that works on every output:** v1's `setVolume` shells out to
`amixer sset Master/PCM/Headphone`, which only affects the 3.5 mm jack (vc4hdmi
has no volume control; Bluetooth ignores it too). Fix: wrap `pcm.!default` in an
ALSA `softvol` plugin (control name `Radio`, control card pinned to the always-
present `Headphones` card) in **all three** generated `/etc/asound.conf` variants
(`WingletGUI::writeAudioOutputConf`), and change `setVolume` to
`amixer sset Radio N%`. Uniform software volume on jack/HDMI/Bluetooth; headphone
AVRCP buttons still work on top of it.

#### 6j — Preset browser + save-preset flow ✓ CODE COMPLETE (2026-07-05, awaiting on-device verification)
As built: `winglet-ui/model/radiopresetsmodel.{h,cpp}` (two-level, PairedBt
pattern) drives a `SelectorBox` opened from the tuner's "All Presets…" row;
favorite rows offer Tune/Rename/Delete, NASR rows Tune/Add to Favorites. The
tuner reads the chosen action back on `showEvent` (return-state, like
`SettingsMenu`); Rename/Save use the CircularKeyboard, save ends in a brief
"Preset saved" MessageBox.

Original design below:
**Preset browser** — reuse `SelectorBox` with a new two-level model
`winglet-ui/model/radiopresetsmodel.{h,cpp}` (same pattern as
`PairedBtDevicesModel`): root rows are presets, child rows are actions.

```
              ╭  Presets — AIRBAND  ╮
              │ ★ Guard     121.500 │   favorites (user store, this band)
              │ ★ Home CTAF 122.800 │
              │ KTUS TWR    118.300 │   nearby (NASR, distance-sorted,
              │ KTUS ATIS   123.800 │   airband only, "2.1 nm" detail line)
              │ KRYN CTAF   125.250 │
              ╰      ...scroll      ╯
   selecting a row → child actions:
     [Tune]  [Add to Favorites]          (NASR rows)
     [Tune]  [Rename]  [Delete]          (favorite rows)
```

- Opened from the tuner's "All Presets…" row; band follows the tuner's band.
- `Tune` closes the browser and retunes (tuner reads the result on `showEvent`,
  same return-state pattern `SettingsMenu` uses for the BT selector).
- `Rename` uses the CircularKeyboard; `Delete` prompts nothing (single child
  action, consistent with WiFi "Forget Network").

**Save-preset flow** — from the tuner's `★ Save Preset` row:

```
   CircularKeyboard (existing widget)
   Title:  "Save Preset"
   Prompt: "Name for 118.300 MHz (Airband):"
   Prefill: "118.300"        max length ~16 (menu column width)
   entryComplete → RadioPresets::add() → brief "Preset saved" MessageBox
```

#### 6k — Band scan (optional stretch)
rtl_fm natively scans multiple frequencies with squelch: pass repeated `-f` args
(all favorites + nearby) or a range (`-f 118M:137M:25k -l 60`) and it parks on
the first active carrier. Limitation: rtl_fm does not report *which* frequency
broke squelch (audio-only stdout), so the UI can only show `SCANNING…` without a
live readout; any key stops the scan and returns to the last manual frequency.
Airband + 2m only (needs squelch). Defer unless it earns its keep.

#### New / modified files (6f–6k)
| File | Change |
|---|---|
| `winglet-ui/worker/rtlfmworker.{h,cpp}` | `MODE_HAM2M`, squelch, per-mode flags, restart debounce |
| `winglet-ui/worker/radiopresets.{h,cpp}` | **new** — user preset JSON store |
| `winglet-ui/model/radiopresetsmodel.{h,cpp}` | **new** — two-level browser model |
| `winglet-ui/window/canardboard.{h,cpp}` | v2 control-stack layout + interaction |
| `winglet-ui/settings/appsettings.h` | `canardLastHam2mFreq/Preset`, `canardSquelchAm/Nfm` |
| `wingletgui.{h,cpp}` | own `RadioPresets`; softvol wrapper in `writeAudioOutputConf` |
| `aeroscan-gui.pro` | add new sources |
| `board/*/overlay/usr/share/aeroscan-gui/frequencies.json` | **new** — seed presets |

#### Verification checklist (on hardware)
- FM: local broadcast station, clean audio, volume row works on jack, HDMI, and
  Bluetooth headphones (softvol).
- Airband: local TWR/ATIS audible; squelch 60 silences noise floor but passes
  voice; NASR rows show correct ident/type/distance.
- 2m: 146.520 receives a test carrier from an HT; squelch 50 behaves; APRS
  packets audible on 144.390 (raw bursts — decode is Phase 11).
- Presets: save from tuner, appears in browser + tuner rows, tunes, renames,
  deletes; survives reboot; seed file imports on first boot.
- Whole flow with **touch only** (no BT keyboard): band change, digit edit,
  preset tune, volume, squelch, save.
- Dongle pulled mid-play: window closes, no orphan rtl_fm/aplay (check `ps`).
- No NASR data: tuner shows "Settings > Update Radio Data" hint (existing v1
  behavior preserved).

**Deliverable:** Radio Tuner receives FM broadcast, airband AM, and 2m NFM with
per-band squelch; favorites persist in JSON; NASR nearby presets for airband;
fully operable from the four touch zones; volume works on all audio outputs.

---

### Phase 7 — Drone Detection Stubs ✓ COMPLETE
**Goal:** Stub infrastructure compiles cleanly; all hooks exist in relevant screens;
no functional drone detection yet — data flows when Phase 12 services are running.

#### New files created in this phase:

**`aeroscan-gui/winglet-ui/worker/dronereceiver.h`**
Defines the `Drone` data struct (mac, id, radio, lat, lon, altM, speedMs, hdg, rssi,
timestamp, distance, bearing) and the `DroneReceiver` worker class. The worker binds a
`QUdpSocket` on port 9999 and parses incoming DroneAware JSON lines. Mirrors the pattern
of `ADSBReceiver` but uses UDP instead of TCP.

**`aeroscan-gui/winglet-ui/worker/dronereceiver.cpp`**
Full implementation: socket bind, JSON parse per datagram, distance/bearing calculation
from own-ship GPS position, stale-drone cleanup timer (30-second timeout). The worker
is active from startup but port 9999 will be silent until Phase 12 services run.

**`aeroscan-gui/winglet-ui/window/droneboard.h/.cpp`**
`DroneBoard` tabular drone list screen modeled on `FlightBoard`. Columns: ID/MAC,
distance (nm), altitude (m), heading, detection method (BLE/WiFi), RSSI. Registered in
the main menu. Shows "No drones detected" until Phase 12 services are active.

**`aeroscan-gui/winglet-ui/widget/touchbuttonoverlay.h/.cpp`**
`TouchButtonOverlay` — much evolved since this phase: now the 100px-wide left
touch column (Parhelia logo + UP/DOWN/SEL/BACK zones) and the right instrument
rail (GPS/ADS-B bars, SDR stats, Pi vitals, date/clock). Zone presses still post
`QKeyEvent`s to the focused widget, transparent to all screen navigation code.

#### Modified files in this phase:

**`wingletgui.h/.cpp`**
Add `DroneReceiver` worker thread (started at boot; port 9999 silent until Phase 12).

**`radarscope.h`**
Add `QMap<QString, Drone> droneSpace` public member. Add `drawDrones(QPainter*)` private
declaration (stub no-op).

**`radarscope.cpp`**
`getAirTraffic()` slot fetches `droneSpace` from `WingletGUI::inst->droneReceiver`
(returns empty map until Phase 12). `paintEvent` calls `drawDrones()` (no-op stub).

**`winglet-gui.pro`**
All new `.h` and `.cpp` files added to `HEADERS` and `SOURCES`.

**Deliverable:** Project compiles with all drone hooks in place. `DroneBoard` accessible
from main menu. `DroneReceiver` thread running. No drone data visible yet.

---

### Phase 8 — WiFi Connectivity ✓ COMPLETE (rpi4)
**Goal:** WiFi connects automatically; managed from settings UI.

- Add USB WiFi driver to Buildroot kernel config (`rt2800usb` for RT5370 or equivalent)
- Verify `wpa_supplicant` starts at boot on `wlan0`
- Confirm `WifiMonitor` worker connects to `wpa_cli` socket (path may differ on RPi)
- Test network scan and connect from Settings → WiFi menu
- Verify dump1090 map tile download works over WiFi
- Verify Settings → Update Radio Data (NASR download) works over WiFi (Phase 6 dependency)

As built on rpi4: onboard BCM43455 (WPA3-capable wpa_supplicant), in-app scan/join
with on-screen keyboard, saved-network management with reconnect dialog, live IP
shown under the connected network, first-connect reliability fix, SSH access used
throughout development.

**Deliverable:** WiFi connects from settings UI; online map tiles load.

---

### Phase 9 — Power & Battery ◐ PARTIAL
**Goal:** Clean shutdown; battery status visible if UPS hardware is present.

- **USB-C power only (no battery):** Disable `BattMonitor` worker; hide battery icon
  in status bar
- **With UPS hat** (e.g. Waveshare UPS HAT, IP5328 via I2C): Rewrite `BattMonitor`
  to read from UPS hat I2C registers in place of the original AXP2585
- Implement low-battery warning and `poweroff` trigger at configurable threshold
- Implement clean shutdown on B-button hold or power key event

As built so far: Power menu (poweroff / reboot / restart UI / exit to terminal)
works; battery icon shows `?` (no telemetry — BattMonitor is effectively stubbed
on USB-C power). UPS-hat support, low-battery handling, and hiding the icon when
no battery hardware exists remain open.

**Deliverable:** Clean shutdown; battery icon reflects charge level or is hidden.

---

### Phase 10 — UI Layout Pass (800x480) ✓ COMPLETE (superseded by the v0.2a instrument-panel layout)
**Goal:** All screens visually correct in the 480x480 content area at 800x480 total window.

- Audit every screen: content must fit within the 480px-wide centered content area (x=160)
- Circular screens (RadarScope, Compass, Oscilloscope, GPSTracker): center (240, 240),
  radius 240px — unchanged from original; verify no clipping at 480px width
- FlightBoard: verify table at (75, 75, 310, 330) fits correctly; confirm status bar
  icons position correctly in the 480px content area
- ScrollableMenu (Main Menu, Settings): verify arc layout renders correctly at 480px width
- Status bar: confirm all icons remain within the 480px content area (not in sidebars)
- Touch button columns: verify 160px width is sufficient for comfortable tap targets;
  button labels legible; visual style matches application theme; consider using remaining
  space in each column for status icons or telemetry readouts

Superseded: the audit happened as part of the v0.2a 100/600/100 rework (see UI
Layout above) — legacy screens are auto-centered by stack margins, MapScope and
Media Player use the full 600 px, the left column got the Parhelia brand + 4
compact touch zones, and the right column became the live instrument rail. The
"remaining space for status icons or telemetry readouts" idea shipped for real.

**Deliverable:** All screens correct at 800x480; sidebar buttons usable.

---

### Phase 11 — APRS Decoding (Optional Extension of Phase 6)
**Goal:** APRS stations on 144.390 MHz decoded and shown as position markers on MapScope.

This phase extends Phase 6 by adding actual packet decoding on top of the receive-only
2m NFM audio. It is optional and independent of Phase 12.

- Add `direwolf` to Buildroot (TNC/APRS decoder; moderate build complexity)
- When Radio Tuner is in 2m mode and tuned to 144.390 MHz, pipe `rtl_fm` audio to
  `direwolf` instead of (or in addition to) `aplay`
- `direwolf` outputs decoded APRS packets; a new worker thread parses position reports
- MapScope extended with APRS station overlay (similar to drone overlay in Phase 12)
- Station list screen (optional): callsign, grid square, last heard, distance

**Deliverable:** APRS position reports visible on map when tuned to 144.390 MHz.

---

### Phase 12 — Drone Remote ID Detection
**Goal:** Nearby drones appear on radar alongside aircraft; DroneBoard shows live data.

**Prerequisite:** Phase 7 stubs must be in place.

#### 12a — DroneAware Services in Buildroot
- Add Python3 dependencies to Buildroot: `bluepy` or `bleak`, `scapy`, `pyroute2`
- Package `ble_feeder.py` and `wifi_feeder.py` from DroneAware-Node-Releases as
  Buildroot overlay files under `/opt/droneaware/`
- Add systemd units `droneaware-ble.service` and `droneaware-wifi.service` to overlay
- Add `rt2800usb` kernel module for Alfa AWUS036N
- Smoke test: `nc -luk 9999` should show JSON lines when drone transmissions are present

#### 12b — RadarScope Drone Rendering
- Implement `drawDrones(QPainter*)` in `radarscope.cpp`:
  - BLE-detected drones: hollow red diamond icon
  - WiFi-detected drones: solid orange diamond icon
  - Show drone ID or last 8 characters of MAC as label
  - Same polar coordinate math already used for aircraft
- Activate the `droneSpace` fetch in `getAirTraffic()` (remove stub comment)

#### 12c — DroneBoard Live Data
- `refreshTable()` already calls `droneReceiver->droneSpace()` — data now populated
- Replace "No drones detected" placeholder with live table
- Add RSSI column and detection method (BLE/WiFi) to table

#### 12d — MapScope Drone Overlay (optional)
- Extend `mapscope.cpp` with same drone icon rendering as RadarScope

**USB load on RPi 2 with full drone stack:**
```
4x USB 2.0 ports:
├── RTL-SDR dongle #0     (ADS-B 1090 MHz, dump1090)
├── RTL-SDR dongle #1     (software radio, rtl_fm — Phase 6)
├── Alfa AWUS036N         (WiFi monitor mode, drone Remote ID)
└── USB-UART adapter      (u-blox NEO-6M GPS)
```
Powered USB hub required. USB Bluetooth dongle (BLE drone Remote ID) and WiFi
dongle move to the hub.

**Deliverable:** Nearby drones visible on radar with correct icons distinct from aircraft;
DroneBoard shows live list with distance, altitude, heading, method, and RSSI.

---

### Phase 13 — RPi 4 Port ✓ COMPLETE (as-built differs — see below)
**Goal:** Same application and UI runs on RPi 4 with HDMI or DSI at 800x480.

Since both hardware targets share the 800x480 resolution and the centered 480x480 layout, this phase
is primarily a Buildroot configuration change — not a layout change.

- Add `raspberrypi4_64_defconfig` as second Buildroot target in the external tree
- Switch Qt5 backend from `linuxfb` to `eglfs` for RPi 4 (DRM/KMS)
- Update `config.txt` for RPi 4 dual-display support:
  ```
  # Force HDMI to 800x480 when no DSI panel is connected
  hdmi_group=2
  hdmi_mode=87
  hdmi_cvt=800 480 60 6 0 0 0
  hdmi_force_hotplug=1
  # DSI panel overlay (auto-preferred by firmware when panel is connected)
  dtoverlay=<osoyoo-dsi-panel-overlay>
  ```
- Verify HDMI fallback when DSI panel is absent (and vice versa)
- Update touch input path: OSOYOO DSI touch appears as I2C evdev device; USB HID
  touch on HDMI displays appears as USB evdev device — both handled transparently
  by Qt5's evdev plugin
- Update `adsbReceiverThread` and other threads for aarch64 if any 32-bit assumptions exist
- Rebuild and flash RPi 4 image

As built (differs from the sketch above): dedicated `aeroscan_rpi4_defconfig`
(64-bit, eglfs/KMS, kernel 6.1 RPi tree); display is **not** firmware
auto-detected — the Hosyond DSI clone needed a forced overlay plus a
legacy-ATTiny-protocol kernel patch (`board/rpi4/linux/patches/`), and panel
choice is a one-line `config.txt` include (`display-dsi.txt` /
`display-hdmi.txt`) that also swaps the kernel cmdline. Onboard BT required the
RPi-tuned BCM4345C0.hcd (generic blob wedges into unwakeable UART sleep).
Note: the RPi 4 port happened **before** Phase 12, out of dependency-graph
order — drone detection now depends on it rather than the reverse.

**Deliverable:** RPi 4 boots with all features. ✓

---

### Phase 14 — Buildroot Image Finalization ◐ PARTIAL
**Goal:** Single reproducible SD card image per hardware target.

- Write `post-build.sh`: set permissions, symlinks, default configs
- Configure read-only rootfs with writable `/var` overlay
- Add user data partition `/userdata` for maps, custom media, settings persistence
- Configure auto-login and auto-start of `aeroscan-gui` via systemd service
- Produce:
  - `aeroscan-rpi2.img.gz` — RPi 2, Waveshare XPT2046 HDMI 800x480
  - `aeroscan-rpi4.img.gz` — RPi 4, HDMI/DSI auto-detect 800x480
- Target: boot-to-GUI in under 20 seconds on RPi 2; under 15 seconds on RPi 4

Done so far: reproducible `sdcard.img` per target with post-image assembly,
first-boot rootfs auto-expansion, verified flashing via `flash-sd.sh`, release
notes shipped to `/etc/release_notes`. Still open: read-only rootfs with
writable overlay, GUI auto-start (deliberately preset-disabled during
development — boot lands on tty1), boot-time targets unmeasured.

**Deliverable:** Flashable images for both hardware targets.

---

### Phase 15 — Integration Testing ◐ PARTIAL
**Goal:** All subsystems stable under sustained concurrent load.

- Full stack on RPi 2: dump1090 + gpsd + wpa_supplicant + Qt5 GUI
- Full stack with drones: add droneaware-ble + droneaware-wifi
- Stress test: RadarScope with 20+ aircraft + active drone detections + GPS fix
- Radio Tuner: verify clean ADS-B handoff on open/close; verify audio output quality
- Memory ceiling: < 700MB RSS on RPi 2 (1GB total)
- Thermal: 30-minute sustained run; verify no CPU throttling (heatsink recommended on RPi 2)
- SPI touch: verify XPT2046 touch coordinates remain calibrated after thermal soak
- Display: verify no framebuffer tearing on HDMI display under radar sweep animation

Progress: the ADS-B pipeline is verified end-to-end on rpi4 (including a
synthetic SBS feed driving a moving target across the scopes); the tuner
handoff cycle is verified; multi-hour GUI sessions ran through the v0.2a
debugging campaign without crashes. Blocked/open: real-traffic soak testing is
limited by site RF interference and antenna (see RELEASE_NOTES.md); the 20+
aircraft stress test, thermal soak, and Pi 2 runs have not happened.

**Deliverable:** Stable, no crashes under normal operating conditions on both targets.

---

### Phase 16 — Aircraft Emitter Category (dump1090-fa JSON API)
**Goal:** Decode and display aircraft type (helicopter, heavy jet, light piston, UAV, etc.)
sourced from the dump1090-fa JSON aircraft feed alongside the existing SBS-1 data.

**Why a second data source is required:** The SBS-1 BaseStation format served on TCP port
30003 does not include the ADS-B emitter category field. The field is present in the raw
DF17 ADS-B frame (ES Identification & Category message, type code 1–4), and dump1090-fa
decodes it and exposes it in its JSON aircraft feed at `http://localhost:8080/data/aircraft.json`.
A periodic HTTP poll is the simplest way to obtain it without parsing raw ADS-B frames.

#### Emitter category codes (from ADS-B spec)

| Code | Meaning |
|---|---|
| A1 | Light (< 15,500 lbs) — piston / small turboprop |
| A2 | Small (15,500–75,000 lbs) — regional jets |
| A3 | Large (75,000–300,000 lbs) — airliners |
| A4 | High vortex large (e.g. 757) |
| A5 | Heavy (> 300,000 lbs) — 747, A380 |
| A6 | High performance (> 5g, > 400 kt) — military fast jet |
| A7 | Rotorcraft (helicopter) |
| B1 | Glider / sailplane |
| B2 | Lighter-than-air (balloon, blimp) |
| B3 | Parachutist / skydiver |
| B4 | Ultralight / hang glider / paraglider |
| B6 | UAV / drone |

Many aircraft transmit no category (code A0 / not set); the field must be treated as optional.

#### Implementation approach

**New worker: `ADSBCategoryPoller`** (or extend `ADSBReceiver` with a `QNetworkAccessManager`)

- Polls `http://localhost:8080/data/aircraft.json` every 5 seconds via `QNetworkAccessManager`
- Parses the `aircraft` array; each entry contains `hex` (ICAO24) and `category` (string, e.g. `"A3"`)
- Acquires `ADSBReceiver::state_mutex` and merges `category` into the matching `m_airspace` entry
  by ICAO24 key — no new data structure required

**`Aircraft` struct additions:**
```cpp
QString category = "";       // ADS-B emitter category string (e.g. "A3", "A7"), empty if unknown
bool categoryValid = false;
```

**UI uses:**
- `FlightBoard`: add a `Type` column showing a short label derived from the category code
  (e.g. "Heavy", "Helo", "UAV", "Light") — replaces or supplements the callsign color coding
- `RadarScope` / `MapScope`: optionally use a different icon or color per category
  (e.g. hollow diamond for helicopters, standard dot for fixed-wing)

#### Prerequisites
- dump1090-fa must be used (not vanilla dump1090); the JSON endpoint and category field
  are dump1090-fa extensions
- `BR2_PACKAGE_QT5NETWORK=y` must be set in the Buildroot defconfig (likely already present
  since `QT += network` is in `aeroscan-gui.pro`)
- Port 8080 must be accessible on localhost (dump1090-fa default; configure with
  `--net-http-port`) — **or simpler, as of v0.2a**: the service already runs with
  `--write-json /run/dump1090`, so `aircraft.json` (which includes `category`) can
  be read directly from `/run/dump1090/aircraft.json` with no HTTP at all; the
  instrument rail's stats poller is the pattern to copy

**Dependency:** Phase 4 (dump1090 running). Independent of all other phases.

**Deliverable:** Aircraft type visible in FlightBoard `Type` column; rotorcraft distinguished
from fixed-wing on radar; unknown/unset category displays gracefully as blank or `—`.

---

## Radar Scope — Visual Enhancement Backlog

Ideas to consider for a future styling pass on `radarscope.cpp`. Shipped in
v0.2a from this list's spirit: smooth atomic sweep (4.5°/50 ms — fixes the
double-line stutter), doubled aircraft icons/labels, and stale-track dimming
(30 s at 35 % opacity). The items below remain open, roughly ordered by
visual impact.

1. **Phosphor sweep trail** — Draw the last ~15° of sweep arc as a series of
   semi-transparent filled wedges fading from ~60% alpha to 0. The single biggest
   "real radar" visual cue; cheap to implement (store current angle, iterate back).

2. **Target trails** — Store the last 3–5 positions per aircraft and draw them as fading
   dots behind each blip. Conveys direction and relative speed without any additional text.

3. **Ownship marker at center** — A small amber `+` or diamond at (240,240). Currently
   there is no visual indication that the scope center represents own-ship position.

4. **Cardinal direction ticks** — Short N/S/E/W tick marks and a small "N" label at the
   top of the outermost range ring. Essential for orientation; trivially cheap to draw.

5. **Multiple range rings** — Currently one ring. Two additional inner rings at 1/3 and
   2/3 of max range, drawn at 40% and 20% alpha respectively, add depth without clutter.

6. **Sweep line glow** — Draw the sweep line three times: full width at full opacity,
   then 2× wider at 30% alpha, then 4× wider at 10% alpha. Soft cathode-ray glow with
   three `drawLine` calls.

7. **Unified aircraft color scheme** — Replace the per-airline rainbow (orange SWA, blue
   DAL, brown UPS, etc.) with neon green for all targets, amber for the closest target
   within ~5 nm, and white for targets on the ground. Matches scope aesthetic; faster
   to read at a glance.

8. **Edge vignette** — A `QRadialGradient` overlay, transparent at center and ~25% black
   at the rim, gives the classic CRT-tube curvature feel. One `drawEllipse` call.

---

## Phase Dependency Graph

```
Phase 0  (Buildroot)
  └── Phase 1  (Display + Touch)
        └── Phase 2  (Qt5 GUI stub, 800x480)
              ├── Phase 3  (Touch input)
              ├── Phase 4  (ADS-B / dump1090) ──────────────────┐
              ├── Phase 5  (GPS)                                 │
              ├── Phase 6  (Software radio) ◄────────────────────┘
              │     └── Phase 11 (APRS decoding, optional)
              └── Phase 7  (Drone stubs) ─────────────────────────┐
                    ├── Phase 8  (WiFi)                           │
                    ├── Phase 9  (Power)                          │
                    ├── Phase 10 (UI layout 800x480)              │
                    └── Phase 12 (Drone detection) ◄──────────────┘
                          └── Phase 13 (RPi 4 port)
                                └── Phase 14 (Image finalization)
                                      └── Phase 15 (Integration testing)
```

Phases 3–7 are independent of each other once Phase 2 is complete and can be worked
in parallel. Phase 6 depends on Phase 4 (RTL-SDR infrastructure). Phase 12 requires
Phase 7 stubs. Phase 13 requires Phase 12.

---

## USB Peripheral Summary

| Peripheral | RPi 2 port | Notes |
|---|---|---|
| RTL-SDR dongle #0 | USB 2.0 | ADS-B 1090 MHz (dump1090, device index 0) |
| RTL-SDR dongle #1 | USB 2.0 | Software radio (rtl_fm -d 1, Phase 6); tuner grayed out if absent |
| Alfa AWUS036N | USB 2.0 | WiFi monitor mode for drone detection (Phase 12) |
| USB-UART (GPS) | USB 2.0 | u-blox NEO-6M GPS receiver |
| USB BT dongle | USB hub | BLE drone Remote ID (Phase 12) |
| USB WiFi dongle | USB hub | General connectivity (wlan0); moves to hub at Phase 12 |

The RPi 2 has 4x USB 2.0 ports. Full Phase 12 load (6 USB devices) requires a powered hub.
Phases 0–11 (pre-drone) need at most 4 USB devices and fit without a hub.

**Audio output:** RPi 2 BCM2835 onboard audio via 3.5mm jack (headphones). HDMI audio
is also available. ALSA device selection for the Radio Tuner must be verified on target
hardware (`aplay -l` to enumerate devices).

---

## Frequency Database

**Airband (as built):** live FAA NASR 28-day subscription data (free, machine-readable
CSV), downloaded on-device via Settings → Update Radio Data to
`/var/lib/aeroscan/apt_freq.csv`. Fields used: facility identifier, frequency, use
(tower, CTAF, ground, ATIS, etc.). The tuner queries airports within 20 nm of the
GPS position (`NASRDatabase::queryNearby`).

**User presets (Phase 6h):** stored in `/var/lib/aeroscan/radio_presets.json`
(writable, survives reflash). The image ships seed defaults at
`/usr/share/aeroscan-gui/frequencies.json` (Buildroot overlay), imported on first
run: Guard 121.500, 2m Calling 146.520, APRS 144.390.

**2m amateur repeaters (future):** RepeaterBook API (free, public JSON) for local
repeaters could populate presets automatically; national simplex and APRS
frequencies are shipped as seed constants.

**FM broadcast:** User-defined presets only. No authoritative machine-readable
source for local FM stations; save from the tuner's Save Preset flow.

---

## Key Reference Links

- Source project: `~/Vibe/avBadge_2024-2.0`
- DroneAware Node: https://github.com/fduflyer/DroneAware-Node-Releases
  - Output format: `{t, mac, radio, rssi, type, lat, lon, alt, speed, hdg, id}` on UDP 9999
- Waveshare 3.5" RPi LCD (A) spec: https://www.waveshare.com/3.5inch-rpi-lcd-a.htm
- OSOYOO 3.5" DSI V3.0: https://osoyoo.store/products/3-5-dsi-touchscreen-v3
- FAA NASR data: https://www.faa.gov/air_traffic/flight_info/aeronav/Aero_Data/

---

## Notes

**Resolution consistency:** Both the RPi 2 (Waveshare XPT2046 HDMI) and RPi 4 (HDMI or DSI)
targets use 800x480. A single UI layout and a single set of geometry constants serve both.
There is no per-target UI variant.

**ADS-B:** The original T113S DSP-based receiver cannot be ported. All ADS-B reception uses
an RTL-SDR USB dongle with dump1090. The `ADSBReceiver` Qt5 worker already targets
`localhost:30003` (SBS1 TCP) — this is the correct interface and requires no code change
from the original.

**RTL-SDR dongle sharing:** dump1090 uses `Restart=on-failure`. The Radio Tuner calls
`systemctl stop dump1090` before opening the dongle and `systemctl start dump1090` on close.
The ADSBReceiver worker detects the port 30003 drop automatically and shows the disconnected
state in the status bar without any explicit notification. It reconnects automatically once
dump1090 restarts.

**Software radio bands:** Three bands supported by a single `rtl_fm` invocation change:
AM (airband, 118–137 MHz), WFM (FM broadcast, 87.5–108 MHz), NFM (2m amateur, 144–148 MHz).
The RTL-SDR hardware covers all three with no physical change. Transmit is not possible
(receive-only hardware); amateur radio licensing is not required for receive-only operation.

**Drone detection stubs:** The `DroneReceiver` worker and `DroneBoard` UI are fully stubbed
in Phase 7. The worker binds UDP 9999 from startup. The DroneBoard is in the main menu.
No drone data flows until Phase 12 when the DroneAware Python services are in the Buildroot
image and the USB hardware (Alfa AWUS036N, USB BT dongle) is connected.

**Touch input:** All navigation uses the XPT2046 resistive touchscreen divided into four
quadrants. `WingletGUI::eventFilter()` intercepts mouse events (Qt converts touch via
tslib), maps screen position to a quadrant, and synthesizes Qt key events. Key_B is
routed directly to WingletGUI to guarantee back navigation from all sub-screens. The
display 90° rotation (`display_rotate=3`) means physical and logical touch coordinates
have swapped axes; the quadrant mapping is calibrated for this.

**ARMv7 / ARMv8:** The RPi 2 build uses `raspberrypi2_defconfig` (ARMv7). The RPi 4 build
uses `raspberrypi4_64_defconfig` (ARMv8/aarch64). Both build from the same application
source tree. The only expected ARMv7→ARMv8 porting concern is confirming that no
`armhf`-only binary assets or 32-bit integer assumptions are present in the codebase.
