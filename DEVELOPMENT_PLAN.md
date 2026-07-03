# AeroScan — Development Plan

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

**RTL-SDR dongle dual use:** The single RTL-SDR dongle is time-shared between dump1090 (ADS-B)
and the software radio receiver. The Radio Tuner stops dump1090 before opening the dongle and
restarts it on exit. A second dongle eliminates the handoff but is not required.

### Future Target (Production)
| Component | Part |
|---|---|
| SBC | Raspberry Pi 4 Model B (BCM2711, quad-core Cortex-A72, ARMv8, 2GB+ RAM) |
| Display (HDMI) | Any 800x480 HDMI touchscreen |
| Display (DSI) | OSOYOO 3.5" DSI V3.0 (800x480, capacitive, ribbon connector) |
| All other hardware | Same as initial target |

Both display types appear to Qt5 as an 800x480 framebuffer. RPi 4 firmware auto-detects:
DSI panel is preferred when connected, HDMI is the fallback. No application-level display
switching logic is needed.

**Since both hardware targets use 800x480, a single UI layout serves both. The RPi 4
migration is a Buildroot and backend change, not a layout change.**

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
- **Initial target**: `raspberrypi2_defconfig` (ARMv7 32-bit)
- **Future target**: `raspberrypi4_64_defconfig` (ARMv8 64-bit)
- **Init system**: systemd
- **Root filesystem**: Read-only with writable `/var` overlay
- **Qt5 backend (RPi 2)**: `linuxfb` (HDMI framebuffer `/dev/fb0`)
- **Qt5 backend (RPi 4)**: `eglfs` (DRM/KMS)

### Display Summary
| Hardware | Resolution | Display Bus | Touch Bus | Qt5 Backend |
|---|---|---|---|---|
| RPi 2 + Waveshare XPT2046 | 800x480 | HDMI | SPI (XPT2046, resistive) | linuxfb |
| RPi 4 + HDMI touchscreen | 800x480 | HDMI | USB HID | eglfs |
| RPi 4 + OSOYOO DSI | 800x480 | DSI ribbon | I2C (capacitive) | eglfs |

All three configurations present Qt5 with an 800x480 surface. Touch input appears as
an evdev device regardless of the touch bus. One UI layout target serves all three.

---

## UI Layout — 800x480 (All Targets)

**Layout** — 480x480 content area centered in the display, with 160px columns on each
side available for status icons or future use.

```
┌──────────────┬──────────────────────────────────┬──────────────┐
│              │                                  │              │
│              │     480 x 480 content area        │              │
│  (status /   │     (radar, map, lists,           │  (status /   │
│   future)    │      clock, settings, etc.)       │   future)    │
│              │                                  │              │
│              │  Touch quadrant mapping:          │              │
│              │    upper-left  = up               │              │
│              │    lower-left  = down             │              │
│              │    upper-right = select           │              │
│              │    lower-right = back             │              │
└──────────────┴──────────────────────────────────┴──────────────┘
    160 px                   480 px                    160 px
```

Touch events on the 800x480 surface are intercepted by `WingletGUI::eventFilter()`,
mapped to a screen quadrant, and synthesized into `QKeyEvent`s delivered to the focused
widget. The display is rotated 90° (`display_rotate=3`) so physical touch coordinates are
swapped relative to logical screen axes; the quadrant mapping accounts for this.

| Physical corner | Logical quadrant | Qt key | Action |
|---|---|---|---|
| Upper-right | Upper-left | Key_A | Select / confirm |
| Upper-left | Lower-left | Key_Up | Scroll up / navigate up |
| Lower-right | Upper-right | Key_B | Back / cancel |
| Lower-left | Lower-right | Key_Down | Scroll down / navigate down |

Key_B is routed directly to WingletGUI rather than the focused widget to ensure back
navigation always works regardless of which sub-screen has focus.

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

### Phase 6 — Software Radio Receiver
**Goal:** Radio Tuner screen receives FM broadcast, aviation airband AM, and 2m amateur
NFM using the same RTL-SDR dongle as ADS-B, with clean ADS-B handoff on open/close.

#### 6a — System prerequisites
- `BR2_PACKAGE_RTL_SDR=y` — provides `rtl_fm` command-line tool (separate from
  `librtlsdr`; includes `rtl_test`, `rtl_eeprom`, etc.)
- `BR2_PACKAGE_ALSA_UTILS=y`, `BR2_PACKAGE_ALSA_LIB=y` — provides `aplay`
- Kernel audio config fragment additions:
  ```
  CONFIG_SND=y
  CONFIG_SND_BCM2835=m      # RPi 2 onboard audio (3.5mm jack + HDMI)
  CONFIG_SND_USB_AUDIO=m    # USB audio dongle support
  ```
- dump1090.service `Restart=on-failure` (already set in Phase 4) ensures `systemctl stop`
  works cleanly from the Radio Tuner

#### 6b — Frequency database
Preset frequencies are loaded from a JSON file bundled in the image, not hard-coded.
This allows updates without reflashing.

**File:** `/usr/share/aeroscan-gui/frequencies.json`

```json
{
  "airband": [
    {"name": "Guard",       "freq_khz": 121500},
    {"name": "Local Tower", "freq_khz": 0},
    ...
  ],
  "fm": [
    {"name": "Local FM 1",  "freq_khz": 0},
    ...
  ],
  "ham_2m": [
    {"name": "2m Calling",  "freq_khz": 146520},
    {"name": "APRS",        "freq_khz": 144390},
    ...
  ]
}
```

Airband frequencies are sourced from the FAA NASR 28-day subscription data (free,
machine-readable CSV). FM and 2m presets are user-defined. The file is installed via
the Buildroot overlay and can be updated over WiFi in a future phase.

#### 6c — Dongle handoff
When the Radio Tuner opens:
1. `QtConcurrent::run` → `systemctl stop dump1090` (blocking, ~1–2 sec)
2. rtl_fm subprocess starts on released dongle
3. Status bar ADS-B icon goes dark (ADSBReceiver loses connection to port 30003,
   shows disconnected state automatically)

When the Radio Tuner closes (Back key or explicit stop):
1. rtl_fm and aplay subprocesses killed
2. `QtConcurrent::run` → `systemctl start dump1090`
3. ADSBReceiver reconnects automatically once port 30003 is available

#### 6d — Audio pipeline
Two `QProcess` objects piped together via `setStandardOutputProcess()`:

```
rtl_fm [opts] -  →  aplay -r 48000 -f S16_LE -t raw -c 1
```

| Band | Mode | rtl_fm flags |
|---|---|---|
| Airband AM (118–137 MHz) | `-M am`   | `-s 200k -r 48000` |
| FM broadcast (87.5–108 MHz) | `-M wbfm` | `-s 200k -r 48000` |
| 2m amateur (144–148 MHz) | `-M nfm`  | `-s 200k -r 48000` |

Squelch (`-l`) defaults: AM=60, NFM=50, WFM=0 (FM broadcast is always open).
Configurable from UI.

#### 6e — Qt UI: RadioTuner window
**New files:** `winglet-ui/window/radiotuner.h/.cpp`

State machine:
```
IDLE → STOPPING_ADSB → STARTING_RF → PLAYING → STOPPING_RF → STARTING_ADSB → IDLE
```

UI layout (480×480 content area):
```
┌──────────────────────────────────┐
│  [AM]  [FM]  [2m]               │  ← band selector, A key cycles
│                                  │
│        121.500 MHz               │  ← large frequency display
│                                  │
│  ▲  Guard         121.500        │
│  ►  Local Tower   118.300        │  ← preset list (from frequencies.json)
│     ATIS          133.000        │
│     Approach      119.100        │
│                                  │
│  [Starting...] / [Live] / [Err]  │  ← status line
└──────────────────────────────────┘
```

Key map:
| Key | Action |
|---|---|
| Up / Down | Scroll preset list |
| A | Tune to selected preset (cycles band if on header row) |
| B | Stop radio, restart dump1090, close window |

Persistent settings (already in `AppSettings`): `canardLastAirbandFreq`,
`canardLastFmFreq`, `canardLastAirbandPreset`, `canardLastFmPreset`. Add
`lastHam2mFreq` and `lastHam2mPreset` for 2m band.

#### Open questions (resolve on hardware)
- Which ALSA device does `aplay` default to? May need `-D hw:0,0` for 3.5mm jack or
  `-D hw:1,0` for HDMI audio. Verify with `aplay -l` on target.
- Optimal squelch levels for local RF environment — tune on site.
- If ADS-B gap during radio use is unacceptable, second RTL-SDR dongle eliminates
  the handoff entirely (dump1090 pinned to `--device-index 0`, rtl_fm to `-d 1`).

**Deliverable:** Radio Tuner screen receives FM broadcast, airband AM, and 2m amateur
NFM; presets load from JSON file; ADS-B restores cleanly on exit.

---

### Phase 7 — Drone Detection Stubs
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
`TouchButtonOverlay` 160x480 widget that hosts three touch buttons. Two instances are
created: left column (UP, DOWN, LEFT at x=0) and right column (RIGHT, A, B at x=640).
Each button press posts a `QKeyEvent` to the currently focused widget, making it
transparent to all existing screen navigation code.

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

### Phase 8 — WiFi Connectivity
**Goal:** WiFi connects automatically; managed from settings UI.

- Add USB WiFi driver to Buildroot kernel config (`rt2800usb` for RT5370 or equivalent)
- Verify `wpa_supplicant` starts at boot on `wlan0`
- Confirm `WifiMonitor` worker connects to `wpa_cli` socket (path may differ on RPi)
- Test network scan and connect from Settings → WiFi menu
- Verify dump1090 map tile download works over WiFi
- Verify `frequencies.json` can be updated over WiFi (Phase 6 dependency)

**Deliverable:** WiFi connects from settings UI; online map tiles load.

---

### Phase 9 — Power & Battery
**Goal:** Clean shutdown; battery status visible if UPS hardware is present.

- **USB-C power only (no battery):** Disable `BattMonitor` worker; hide battery icon
  in status bar
- **With UPS hat** (e.g. Waveshare UPS HAT, IP5328 via I2C): Rewrite `BattMonitor`
  to read from UPS hat I2C registers in place of the original AXP2585
- Implement low-battery warning and `poweroff` trigger at configurable threshold
- Implement clean shutdown on B-button hold or power key event

**Deliverable:** Clean shutdown; battery icon reflects charge level or is hidden.

---

### Phase 10 — UI Layout Pass (800x480)
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
├── RTL-SDR dongle        (ADS-B 1090 MHz / software radio, time-shared)
├── Alfa AWUS036N         (WiFi monitor mode, drone Remote ID)
├── USB-UART adapter      (u-blox NEO-6M GPS)
└── USB Bluetooth dongle  (BLE drone Remote ID)
```
Powered USB hub required. WiFi dongle moves to hub port 5.

**Deliverable:** Nearby drones visible on radar with correct icons distinct from aircraft;
DroneBoard shows live list with distance, altitude, heading, method, and RSSI.

---

### Phase 13 — RPi 4 Port
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

**Deliverable:** RPi 4 boots with all features; HDMI and DSI auto-detected; same
800x480 layout and behavior as RPi 2 target.

---

### Phase 14 — Buildroot Image Finalization
**Goal:** Single reproducible SD card image per hardware target.

- Write `post-build.sh`: set permissions, symlinks, default configs
- Configure read-only rootfs with writable `/var` overlay
- Add user data partition `/userdata` for maps, custom media, settings persistence
- Configure auto-login and auto-start of `aeroscan-gui` via systemd service
- Produce:
  - `aeroscan-rpi2.img.gz` — RPi 2, Waveshare XPT2046 HDMI 800x480
  - `aeroscan-rpi4.img.gz` — RPi 4, HDMI/DSI auto-detect 800x480
- Target: boot-to-GUI in under 20 seconds on RPi 2; under 15 seconds on RPi 4

**Deliverable:** Flashable images for both hardware targets.

---

### Phase 15 — Integration Testing
**Goal:** All subsystems stable under sustained concurrent load.

- Full stack on RPi 2: dump1090 + gpsd + wpa_supplicant + Qt5 GUI
- Full stack with drones: add droneaware-ble + droneaware-wifi
- Stress test: RadarScope with 20+ aircraft + active drone detections + GPS fix
- Radio Tuner: verify clean ADS-B handoff on open/close; verify audio output quality
- Memory ceiling: < 700MB RSS on RPi 2 (1GB total)
- Thermal: 30-minute sustained run; verify no CPU throttling (heatsink recommended on RPi 2)
- SPI touch: verify XPT2046 touch coordinates remain calibrated after thermal soak
- Display: verify no framebuffer tearing on HDMI display under radar sweep animation

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
- Port 8080 must be accessible on localhost (dump1090-fa default; configure with `--net-http-port`)

**Dependency:** Phase 4 (dump1090 running). Independent of all other phases.

**Deliverable:** Aircraft type visible in FlightBoard `Type` column; rotorcraft distinguished
from fixed-wing on radar; unknown/unset category displays gracefully as blank or `—`.

---

## Radar Scope — Visual Enhancement Backlog

Ideas to consider for a future styling pass on `radarscope.cpp`. None are committed to a
phase yet. Roughly ordered by visual impact.

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
| RTL-SDR dongle | USB 2.0 | ADS-B 1090 MHz; time-shared with software radio (Phase 6) |
| Alfa AWUS036N | USB 2.0 | WiFi monitor mode for drone detection (Phase 12) |
| USB-UART (GPS) | USB 2.0 | u-blox NEO-6M GPS receiver |
| USB BT dongle | USB 2.0 | BLE drone Remote ID (Phase 12) |
| USB WiFi dongle | USB hub | General connectivity (wlan0); moves to hub at Phase 12 |

The RPi 2 has 4x USB 2.0 ports. Full Phase 12 load (5 USB devices) requires a powered hub.
Phases 0–11 (pre-drone) require at most 3 USB devices and fit without a hub.

**Audio output:** RPi 2 BCM2835 onboard audio via 3.5mm jack (headphones). HDMI audio
is also available. ALSA device selection for the Radio Tuner must be verified on target
hardware (`aplay -l` to enumerate devices).

---

## Frequency Database

Preset frequencies for the Radio Tuner are stored in
`/usr/share/aeroscan-gui/frequencies.json`, installed via the Buildroot overlay and
updatable over WiFi without reflashing.

**Airband source:** FAA NASR 28-day subscription data (free, machine-readable CSV).
Fields used: facility identifier, frequency, use (tower, ATIS, approach, departure, etc.).

**2m amateur source:** RepeaterBook API (free, public JSON) for local repeaters;
national simplex and APRS frequencies are constants.

**FM broadcast:** User-defined. No authoritative machine-readable source for all
local FM stations; populate manually or leave blank for user entry.

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
