# Qt5 runtime environment — eglfs (KMS/DRM + Mesa V3D) + XPT2046 touch
export QT_FONT_DPI=84
export QT_QPA_PLATFORM=eglfs

# eglfs_kms backend config: device, HDMI output, separateEglDevice.
# Rotation is NOT done via the KMS JSON "rotation" field (VC4 primary plane
# does not support 90/270 DRM rotation with imported V3D DMA-BUFs).
# Instead QT_QPA_EGLFS_ROTATION=90 applies the rotation in the V3D OpenGL
# compositor (QOpenGLCompositor::setRotation) — pure software, always works.
export QT_QPA_EGLFS_KMS_CONFIG=/etc/qt-kms.json
export QT_QPA_EGLFS_ROTATION=90

# Touch: Qt's eglfs-builtin evdevtouch handler. The XPT2046 panel axes are
# swapped AND both inverted vs the landscape view (EGLFS_ROTATION rotates
# rendering only, never input). Inversions apply before rotation on
# normalized coords, hence rotate=270:invertx:inverty (verified 2026-07-03).
export QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS=/dev/input/touchscreen0:rotate=270:invertx:inverty

# aeroscan-display-init (run by aeroscan-gui.service) writes the detected
# display config; it overrides the static HDMI defaults above when present.
if [ -f /run/aeroscan/display.env ]; then
    set -a
    . /run/aeroscan/display.env
    set +a
fi

# tslib env for the ts_* diagnostic tools only (Qt does not use tslib on rpi4)
export TSLIB_TSDEVICE=/dev/input/touchscreen0
export TSLIB_CONFFILE=/etc/ts.conf
export TSLIB_CALIBFILE=/etc/pointercal
