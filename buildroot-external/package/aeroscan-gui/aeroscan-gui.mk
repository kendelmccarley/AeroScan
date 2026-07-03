################################################################################
#
# aeroscan-gui
#
################################################################################

AEROSCAN_GUI_VERSION = local
AEROSCAN_GUI_SITE = $(BR2_EXTERNAL_AEROSCAN_PATH)/../aeroscan-gui
AEROSCAN_GUI_SITE_METHOD = local
AEROSCAN_GUI_DEPENDENCIES = qt5base qt5multimedia qt5svg wpa_supplicant libgpiod

define AEROSCAN_GUI_CONFIGURE_CMDS
	(cd $(@D) && $(HOST_DIR)/bin/qmake \
		INCLUDEPATH+=$(@D) \
		$(AEROSCAN_GUI_SITE)/aeroscan-gui.pro)
endef

define AEROSCAN_GUI_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D)
endef

define AEROSCAN_GUI_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 755 $(@D)/aeroscan-gui \
		$(TARGET_DIR)/usr/bin/aeroscan-gui
	cp -r $(AEROSCAN_GUI_SITE)/assets \
		$(TARGET_DIR)/usr/share/aeroscan-gui/
endef

$(eval $(generic-package))
