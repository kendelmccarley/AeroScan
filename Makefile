# AeroScan top-level Makefile
#
# Usage:
#   make download-buildroot     — fetch Buildroot 2024.02 (one-time)
#   make rpi2                   — configure + full build for RPi 2 (ARMv7, linuxfb)
#   make rpi4                   — configure + full build for RPi 4 (AArch64, eglfs)
#   make menuconfig-rpi2        — interactive Buildroot config (RPi 2)
#   make menuconfig-rpi4        — interactive Buildroot config (RPi 4)
#   make linux-menuconfig-rpi2  — interactive kernel config (RPi 2)
#   make linux-menuconfig-rpi4  — interactive kernel config (RPi 4)
#   make savedefconfig-rpi2     — write current config back to defconfig (RPi 2)
#   make savedefconfig-rpi4     — write current config back to defconfig (RPi 4)
#   make clean-rpi2             — remove RPi 2 build output
#   make clean-rpi4             — remove RPi 4 build output
#
# Override Buildroot location:
#   make rpi2 BUILDROOT_DIR=/path/to/buildroot
#   make rpi4 BUILDROOT_DIR=/path/to/buildroot

BUILDROOT_VERSION ?= 2024.02.10
BUILDROOT_DIR     ?= $(CURDIR)/buildroot
BR2_EXTERNAL      := $(CURDIR)/buildroot-external
OUTPUT_RPI2       := $(CURDIR)/output/rpi2
OUTPUT_RPI4       := $(CURDIR)/output/rpi4

MAKEFLAGS += --no-print-directory

.PHONY: all rpi2 rpi4 \
        menuconfig-rpi2 menuconfig-rpi4 \
        linux-menuconfig-rpi2 linux-menuconfig-rpi4 \
        savedefconfig-rpi2 savedefconfig-rpi4 \
        clean-rpi2 clean-rpi4 \
        download-buildroot \
        %-reinstall %-rebuild %-reconfigure

all: rpi2

# ─── Buildroot download ───────────────────────────────────────────────────────

download-buildroot:
	@if [ -d "$(BUILDROOT_DIR)" ]; then \
		echo "Buildroot already present at $(BUILDROOT_DIR)"; \
	else \
		echo "Downloading Buildroot $(BUILDROOT_VERSION) ..."; \
		wget -q --show-progress \
			"https://buildroot.org/downloads/buildroot-$(BUILDROOT_VERSION).tar.xz" \
			-O /tmp/buildroot-$(BUILDROOT_VERSION).tar.xz; \
		tar -xf /tmp/buildroot-$(BUILDROOT_VERSION).tar.xz; \
		mv "buildroot-$(BUILDROOT_VERSION)" "$(BUILDROOT_DIR)"; \
		rm /tmp/buildroot-$(BUILDROOT_VERSION).tar.xz; \
		echo "Buildroot ready at $(BUILDROOT_DIR)"; \
	fi

$(BUILDROOT_DIR)/Makefile:
	@echo "Buildroot not found at $(BUILDROOT_DIR)."
	@echo "Run:  make download-buildroot"
	@exit 1

# ─── RPi 2 targets ────────────────────────────────────────────────────────────

_br2-rpi2 = $(MAKE) -C "$(BUILDROOT_DIR)" O="$(OUTPUT_RPI2)" BR2_EXTERNAL="$(BR2_EXTERNAL)"

rpi2: | $(BUILDROOT_DIR)/Makefile
	mkdir -p "$(OUTPUT_RPI2)"
	$(_br2-rpi2) aeroscan_rpi2_defconfig
	$(_br2-rpi2)

menuconfig-rpi2: | $(BUILDROOT_DIR)/Makefile
	mkdir -p "$(OUTPUT_RPI2)"
	$(_br2-rpi2) aeroscan_rpi2_defconfig
	$(_br2-rpi2) menuconfig

linux-menuconfig-rpi2: | $(BUILDROOT_DIR)/Makefile
	$(_br2-rpi2) linux-menuconfig

savedefconfig-rpi2: | $(BUILDROOT_DIR)/Makefile
	$(_br2-rpi2) \
		BR2_DEFCONFIG="$(BR2_EXTERNAL)/configs/aeroscan_rpi2_defconfig" \
		savedefconfig

clean-rpi2:
	rm -rf "$(OUTPUT_RPI2)"

# ─── RPi 4 targets (AArch64, eglfs/KMS) ──────────────────────────────────────

_br2-rpi4 = $(MAKE) -C "$(BUILDROOT_DIR)" O="$(OUTPUT_RPI4)" BR2_EXTERNAL="$(BR2_EXTERNAL)"

rpi4: | $(BUILDROOT_DIR)/Makefile
	mkdir -p "$(OUTPUT_RPI4)"
	$(_br2-rpi4) aeroscan_rpi4_defconfig
	$(_br2-rpi4)

menuconfig-rpi4: | $(BUILDROOT_DIR)/Makefile
	mkdir -p "$(OUTPUT_RPI4)"
	$(_br2-rpi4) aeroscan_rpi4_defconfig
	$(_br2-rpi4) menuconfig

linux-menuconfig-rpi4: | $(BUILDROOT_DIR)/Makefile
	$(_br2-rpi4) linux-menuconfig

savedefconfig-rpi4: | $(BUILDROOT_DIR)/Makefile
	$(_br2-rpi4) \
		BR2_DEFCONFIG="$(BR2_EXTERNAL)/configs/aeroscan_rpi4_defconfig" \
		savedefconfig

clean-rpi4:
	rm -rf "$(OUTPUT_RPI4)"

# ─── Buildroot package pass-through ──────────────────────────────────────────
# Use rpi2- or rpi4- prefix to target the right output directory:
#   make rpi2-aeroscan-gui-rebuild
#   make rpi4-rpi4-wifi-firmware-reinstall
rpi2-%: | $(BUILDROOT_DIR)/Makefile
	$(_br2-rpi2) $*

rpi4-%: | $(BUILDROOT_DIR)/Makefile
	$(_br2-rpi4) $*
