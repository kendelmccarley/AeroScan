#ifndef WINGLETGUI_H
#define WINGLETGUI_H

#include <QDateTime>
#include <QMainWindow>
#include <QLabel>
#include <QStackedWidget>

#include "winglet-ui/hardware/gpiocontrol.h"
#include "winglet-ui/settings/appsettings.h"
#include "winglet-ui/worker/workerthread.h"
#include "winglet-ui/worker/wifimonitor.h"
#include "winglet-ui/worker/bluetoothmonitor.h"
#include "winglet-ui/worker/battmonitor.h"
#include "winglet-ui/worker/gpsreceiver.h"
#include "winglet-ui/worker/adsbreceiver.h"
#include "winglet-ui/worker/dronereceiver.h"
#include "winglet-ui/worker/rtlfmworker.h"
#include "winglet-ui/worker/nasrdatabase.h"
#include "winglet-ui/worker/radiopresets.h"
#include "winglet-ui/widget/touchbuttonoverlay.h"

#define RELEASE_NOTES_FLAG_FILE "/var/show_release_notes"
#define RELEASE_NOTES_LOCATION "/etc/release_notes"

extern const char* const WINGLET_GUI_VERSION;

class WingletGUI : public QMainWindow
{
    Q_OBJECT
public:
    explicit WingletGUI(QWidget *parent = nullptr);
    ~WingletGUI();

    void addWidgetOnTop(QWidget* widget);
    void replaceWidgetOnTop(QWidget* oldWidget, QWidget* newWidget);
    void removeWidgetOnTop(QWidget* widget);

    void showMessageBox(const QString& msg, const QString& title, const QString& buttonText = "Okay",
                        bool replaceCurrentWidget = false, Qt::Alignment alignment = Qt::AlignCenter);

    bool tryShowReleaseNotes(bool forceShow = false);

    WingletUI::AppSettings settings;
    WingletUI::WifiMonitor *wifiMon;
    WingletUI::BluetoothMonitor *btMon;
    WingletUI::BattMonitor *battMon;
    WingletUI::GPSReceiver *gpsReceiver;
    WingletUI::ADSBReceiver *adsbReceiver;
    WingletUI::DroneReceiver *droneReceiver;
    WingletUI::RtlFmWorker   *rtlFm;
    WingletUI::NASRDatabase  *nasr;
    WingletUI::RadioPresets  *presets;

    static WingletGUI* inst;

    // Wall-clock time in the timezone implied by the GPS position (CONUS
    // zone bands + AK/HI/AZ, DST from the zone database; solar-time fallback
    // elsewhere; system zone when there has never been a fix).
    QDateTime gpsLocalTime();

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void keyPressEvent(QKeyEvent *ev) override;

private slots:
    void gpsUpdated(WingletUI::GPSReading reading);
    void colorPaletteUpdated();
    void overlayZonePressed(int zone);
    void applyAudioOutput(int output);
    void btPairedDevicesChanged();

private:
    void startAutoScreenshots();
    void saveScreenshot();
    bool writeAudioOutputConf(int output);
    int lastGoodAudioOutput = 0;
    bool btAudioWasConnected = false;
    // Wired output (jack/HDMI) that was active before headphones auto-selected
    // themselves, restored when they disconnect; -1 = nothing to restore.
    int btAudioRevertOutput = -1;

    QWidget *centralContainer;
    QStackedWidget *appStack;
    WingletUI::TouchButtonOverlay *leftOverlay;
    WingletUI::TouchButtonOverlay *rightOverlay;


#ifdef NO_HARDWARE
    QLabel *devRingLabel;
#endif

    WingletUI::WorkerThread<WingletUI::WifiMonitor> *wifiMonThread;
    WingletUI::WorkerThread<WingletUI::BluetoothMonitor> *btMonThread;
    WingletUI::WorkerThread<WingletUI::BattMonitor> *battMonThread;
    WingletUI::WorkerThread<WingletUI::GPSReceiver> *gpsReceiverThread;
    WingletUI::WorkerThread<WingletUI::ADSBReceiver> *adsbReceiverThread;
    WingletUI::WorkerThread<WingletUI::DroneReceiver> *droneReceiverThread;

    WingletUI::GPIOControl *gpioControl;

    QTcpSocket *dummySocket;
};

#endif // WINGLETGUI_H
