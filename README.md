# AeroScan

AeroScan is a self-contained aircraft scanner for the Raspberry Pi: a radar-scope
style Qt GUI showing live ADS-B traffic (RTL-SDR + dump1090) with GPS positioning,
an aviation-chart map overlay, an airband radio tuner, and WiFi/Bluetooth device
management — all running on a minimal Buildroot Linux image that boots straight
into the application. It is a port of the [avBadge 2024 "Winglet"](https://github.com/AerospaceVillage/avBadge_2024)
badge firmware from its custom Allwinner T113S hardware to commodity Raspberry Pi
hardware.

## Hardware

Two build targets are provided: **Raspberry Pi 4** (AArch64, Qt eglfs on KMS/DRM)
and **Raspberry Pi 2** (ARMv7, Qt linuxfb).

**The Pi 4 is the recommended target.** Its USB ports can distribute power to the
display, GPS receiver, and RTL-SDR dongles directly. The Pi 2's onboard USB power
budget cannot handle these peripherals, so using the Pi 2 requires an external
powered USB hub — without one, peripheral power draw causes brownouts and
spontaneous reboots.

| Component | Notes |
|---|---|
| Raspberry Pi 4 (recommended) or Pi 2 Model B | Pi 2 needs a powered USB hub |
| Display | Pi 4: official 7" DSI or compatible clones (auto-detected), or HDMI. Pi 2: Waveshare 800×480 HDMI + XPT2046 resistive touch (SPI) |
| RTL-SDR USB dongle(s) | device 0 = ADS-B 1090 MHz (dump1090); optional device 1 = airband radio tuner |
| u-blox GPS receiver | NEO-6M or similar, USB serial |
| USB WiFi dongle | RT5370 supported out of the box |
| USB Bluetooth dongle (Pi 2) | Pi 4 uses onboard BT; in-app pairing over BlueZ D-Bus |

## Building

Requires a Linux host with the usual Buildroot dependencies (gcc, make, python3,
wget, etc.).

```bash
git clone <this-repo> AeroScan && cd AeroScan
make download-buildroot          # one-time: fetch Buildroot 2024.02

# Map tiles need a free OpenAIP API key (https://www.openaip.net → Account → API Keys)
echo YOUR_OPENAIP_KEY > .openaip-key

make rpi4                        # configure + full build (or: make rpi2)
```

Flash the result to an SD card:

```bash
dd if=output/rpi4/images/sdcard.img of=/dev/sdX bs=4M status=progress
```

Useful targets (see `Makefile` for the full list):

| Target | Purpose |
|---|---|
| `make rpi4` / `make rpi2` | Configure + full image build |
| `make menuconfig-rpi4` | Interactive Buildroot config |
| `make savedefconfig-rpi4` | Write config back to the defconfig |
| `make rpi4-<pkg>-rebuild` | Rebuild one package (e.g. `make rpi4-aeroscan-gui-rebuild && make rpi4` after GUI source changes) |
| `make clean-rpi4` | Remove build output |

## Map tiles

Aviation chart tiles (OpenAIP overlay: airspace, airports, NAVAIDs; CONUS,
zoom 7–10) are **not stored in this repository**. They are downloaded
automatically during the build:

- The board `post-build.sh` runs `tools/fetch-aviation-tiles.py`, which fills
  `maps-cache/` at the project root and installs the tiles into the image at
  `/opt/winglet-gui/maps`.
- `maps-cache/` is gitignored and survives `make clean`. The first build spends
  a few hours downloading (~120 MB, rate-limited to respect the OpenAIP free
  tier); later builds resume from the cache and finish instantly.
- The download requires an OpenAIP API key in `.openaip-key` (or the
  `OPENAIP_KEY` environment variable). Without a key the build still succeeds,
  just without map tiles.
- The key is baked into the image at `/etc/aeroscan/openaip.key` so tiles can be
  refreshed on the device later via `aeroscan-fetch-tiles` (or `aeroscan-setup`)
  without rebuilding.

## First boot

Boot lands on a tty1 shell (the GUI service is preset-disabled on the Pi 4
image while the port is under active development). Run `aeroscan-setup` on the
console or over SSH to configure WiFi, display, and hostname, then start the
GUI with `systemctl start aeroscan-gui` or run `aeroscan-gui` directly.

## Repository layout

```
Makefile                  Top-level build entry points (wraps Buildroot)
aeroscan-gui/             Qt5 application source (radar scope, map, settings, …)
buildroot-external/       Buildroot external tree (BR2_EXTERNAL)
  configs/                aeroscan_rpi2_defconfig, aeroscan_rpi4_defconfig
  board/rpi2/, rpi4/      config.txt, cmdline.txt, kernel fragments, rootfs
                          overlays, post-build/post-image scripts
  board/common/overlay/   Shared on-device tools (aeroscan-setup,
                          aeroscan-fetch-tiles, NASR updater)
  package/                aeroscan-gui and rpi4-wifi-firmware packages
tools/                    Host-side tile download scripts
maps-cache/               (gitignored) build-time tile cache
buildroot/, output/       (gitignored) Buildroot tree and build output
```

## Status

- **Pi 2**: display + touch, Qt GUI, ADS-B working; WiFi bring-up in progress.
- **Pi 4**: primary target. Display auto-detection (DSI preferred, HDMI
  fallback), touch input rotation, in-app Bluetooth pairing (BlueZ D-Bus,
  passkey shown on screen), audio output selection (headphone jack / HDMI),
  WiFi and SSH. Code complete; on-device verification ongoing.

Further design notes live in `DEVELOPMENT_PLAN.md` and `BLUETOOTH.md`.
