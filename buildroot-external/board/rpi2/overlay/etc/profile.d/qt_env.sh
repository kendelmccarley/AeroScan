# Qt5 runtime environment — 800x480 linuxfb + XPT2046 touch via tslib
export QT_FONT_DPI=84
export QT_QPA_PLATFORM=linuxfb:fb=/dev/fb0

# tslib calibrated touch. Run ts_calibrate once after first boot to populate
# /etc/pointercal. The symlink /dev/input/touchscreen0 is created by udev.
export TSLIB_TSDEVICE=/dev/input/touchscreen0
export TSLIB_CONFFILE=/etc/ts.conf
export TSLIB_CALIBFILE=/etc/pointercal
export QT_QPA_GENERIC_PLUGINS=tslib:/dev/input/touchscreen0
