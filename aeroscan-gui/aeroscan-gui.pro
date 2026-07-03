QT += core gui network dbus

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# Pass CONFIG+=no_hardware on the qmake command line to stub all hardware I/O.
# Phase 2: always stubbed. Remove this block when real hardware drivers land.
CONFIG(no_hardware) {
    DEFINES += NO_HARDWARE=1
} else {
    LIBS += -lwpa_client -lgpiod
}

CONFIG += c++11
QMAKE_CXXFLAGS_RELEASE += -O3

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    winglet-ui/hardware/gpiocontrol.cpp \
    winglet-ui/model/appmenumodel.cpp \
    winglet-ui/model/appsettingsenumselectionmodel.cpp \
    winglet-ui/model/btscanmodel.cpp \
    winglet-ui/model/knownnetworksmodel.cpp \
    winglet-ui/model/pairedbtdevicesmodel.cpp \
    winglet-ui/model/settingsmenumodel.cpp \
    winglet-ui/model/timezonesettingmodel.cpp \
    winglet-ui/model/wifiscanmodel.cpp \
    winglet-ui/settings/appsettings.cpp \
    winglet-ui/settings/appsettingspropentry.cpp \
    winglet-ui/settings/rootpasswordsetting.cpp \
    winglet-ui/settings/timezonesetting.cpp \
    winglet-ui/theme.cpp \
    winglet-ui/widget/elidedlabel.cpp \
    winglet-ui/widget/menuitemwidget.cpp \
    winglet-ui/widget/scrollablemenu.cpp \
    winglet-ui/widget/statusbar.cpp \
    winglet-ui/widget/touchbuttonoverlay.cpp \
    winglet-ui/window/clock.cpp \
    winglet-ui/window/compass.cpp \
    winglet-ui/window/credits.cpp \
    winglet-ui/window/droneboard.cpp \
    winglet-ui/window/flightboard.cpp \
    winglet-ui/window/gpsboard.cpp \
    winglet-ui/window/gpstracker.cpp \
    winglet-ui/window/mapscope.cpp \
    winglet-ui/window/canardboard.cpp \
    winglet-ui/window/oscope.cpp \
    winglet-ui/window/radarscope.cpp \
    winglet-ui/window/scrollarea.cpp \
    winglet-ui/window/settingsmenu.cpp \
    winglet-ui/window/simplemediaplayer.cpp \
    winglet-ui/windowcore/circularkeyboard.cpp \
    winglet-ui/windowcore/infoviewer.cpp \
    winglet-ui/windowcore/mainmenu.cpp \
    winglet-ui/windowcore/messagebox.cpp \
    winglet-ui/windowcore/selectorbox.cpp \
    winglet-ui/worker/abstractsocketworker.cpp \
    winglet-ui/worker/adsbreceiver.cpp \
    winglet-ui/worker/battmonitor.cpp \
    winglet-ui/worker/bluetoothmonitor.cpp \
    winglet-ui/worker/dronereceiver.cpp \
    winglet-ui/worker/gpsreceiver.cpp \
    winglet-ui/worker/nasrdatabase.cpp \
    winglet-ui/worker/rtlfmworker.cpp \
    winglet-ui/worker/wifimonitor.cpp \
    wingletgui.cpp

HEADERS += \
    winglet-ui/hardware/gpiocontrol.h \
    winglet-ui/model/appmenumodel.h \
    winglet-ui/model/appsettingsenumselectionmodel.h \
    winglet-ui/model/btscanmodel.h \
    winglet-ui/model/knownnetworksmodel.h \
    winglet-ui/model/pairedbtdevicesmodel.h \
    winglet-ui/model/settingsmenumodel.h \
    winglet-ui/model/timezonesettingmodel.h \
    winglet-ui/model/wifiscanmodel.h \
    winglet-ui/settings/abstractsettingsentry.h \
    winglet-ui/settings/appsettings.h \
    winglet-ui/settings/appsettingspropentry.h \
    winglet-ui/settings/rootpasswordsetting.h \
    winglet-ui/settings/timezonesetting.h \
    winglet-ui/theme.h \
    winglet-ui/widget/elidedlabel.h \
    winglet-ui/widget/menuitemwidget.h \
    winglet-ui/widget/scrollablemenu.h \
    winglet-ui/widget/statusbar.h \
    winglet-ui/widget/touchbuttonoverlay.h \
    winglet-ui/window/clock.h \
    winglet-ui/window/compass.h \
    winglet-ui/window/credits.h \
    winglet-ui/window/droneboard.h \
    winglet-ui/window/flightboard.h \
    winglet-ui/window/gpsboard.h \
    winglet-ui/window/gpstracker.h \
    winglet-ui/window/mapscope.h \
    winglet-ui/window/canardboard.h \
    winglet-ui/window/oscope.h \
    winglet-ui/window/radarscope.h \
    winglet-ui/window/scrollarea.h \
    winglet-ui/window/settingsmenu.h \
    winglet-ui/window/simplemediaplayer.h \
    winglet-ui/windowcore/circularkeyboard.h \
    winglet-ui/windowcore/infoviewer.h \
    winglet-ui/windowcore/mainmenu.h \
    winglet-ui/windowcore/messagebox.h \
    winglet-ui/windowcore/selectorbox.h \
    winglet-ui/worker/abstractsocketworker.h \
    winglet-ui/worker/adsbreceiver.h \
    winglet-ui/worker/battmonitor.h \
    winglet-ui/worker/bluetoothmonitor.h \
    winglet-ui/worker/dronereceiver.h \
    winglet-ui/worker/gpsreceiver.h \
    winglet-ui/worker/nasrdatabase.h \
    winglet-ui/worker/rtlfmworker.h \
    winglet-ui/worker/wifimonitor.h \
    winglet-ui/worker/workerthread.h \
    wingletgui.h

FORMS += \
    winglet-ui/window/credits.ui

RESOURCES += \
    resources.qrc

unix:!android: target.path = /usr/bin
!isEmpty(target.path): INSTALLS += target
