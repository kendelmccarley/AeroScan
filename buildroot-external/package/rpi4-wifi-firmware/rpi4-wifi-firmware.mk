################################################################################
#
# rpi4-wifi-firmware
#
# Device-specific BCM43455 WiFi firmware for Raspberry Pi 4.
# Source: RPi-Distro/firmware-nonfree, bookworm branch.
#
################################################################################

RPI4_WIFI_FIRMWARE_VERSION = c91cd2804cf7463aab913e7247c176049f16bbd6
RPI4_WIFI_FIRMWARE_SITE = $(call github,RPi-Distro,firmware-nonfree,$(RPI4_WIFI_FIRMWARE_VERSION))
RPI4_WIFI_FIRMWARE_LICENSE = Redistributable firmware
RPI4_WIFI_FIRMWARE_LICENSE_FILES = debian/config/brcm80211/brcm/brcmfmac43455-sdio.raspberrypi,4-model-b.txt

# The .bin in the brcm/ directory is a symlink → ../cypress/cyfmac43455-sdio.bin,
# but that exact name does not exist in the cypress/ directory (only
# cyfmac43455-sdio-standard.bin and cyfmac43455-sdio-minimal.bin do).
# $(INSTALL) follows the symlink and silently fails on the missing target.
# Install the real source file (standard variant) directly under the name
# brcmfmac driver requests: brcmfmac43455-sdio.raspberrypi,4-model-b.bin.
define RPI4_WIFI_FIRMWARE_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/lib/firmware/brcm
	$(INSTALL) -m 0644 \
		$(@D)/debian/config/brcm80211/cypress/cyfmac43455-sdio-standard.bin \
		$(TARGET_DIR)/lib/firmware/brcm/brcmfmac43455-sdio.raspberrypi,4-model-b.bin
	$(INSTALL) -m 0644 \
		$(@D)/debian/config/brcm80211/brcm/brcmfmac43455-sdio.raspberrypi,4-model-b.clm_blob \
		$(TARGET_DIR)/lib/firmware/brcm/brcmfmac43455-sdio.raspberrypi,4-model-b.clm_blob
	$(INSTALL) -m 0644 \
		$(@D)/debian/config/brcm80211/brcm/brcmfmac43455-sdio.raspberrypi,4-model-b.txt \
		$(TARGET_DIR)/lib/firmware/brcm/brcmfmac43455-sdio.raspberrypi,4-model-b.txt
endef

$(eval $(generic-package))
