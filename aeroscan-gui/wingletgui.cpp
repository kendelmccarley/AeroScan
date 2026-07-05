#include "wingletgui.h"
#include "winglet-ui/theme.h"
#include "winglet-ui/windowcore/mainmenu.h"
#include "winglet-ui/windowcore/messagebox.h"
#include <QKeyEvent>
#include <QMouseEvent>

WingletGUI* WingletGUI::inst = NULL;

WingletGUI::WingletGUI(QWidget *parent)
    : QMainWindow{parent}, settings(this),
      centralContainer(new QWidget(this)),
      appStack(new QStackedWidget(centralContainer)),
      leftOverlay(new WingletUI::TouchButtonOverlay(WingletUI::TouchButtonOverlay::Left, centralContainer)),
      rightOverlay(new WingletUI::TouchButtonOverlay(WingletUI::TouchButtonOverlay::Right, centralContainer)),
      gpioControl(new WingletUI::GPIOControl(this))
{
    if (inst != NULL) {
        qWarning("WingletGUI: Second instance created, not setting global!");
    } else {
        inst = this;
    }

    setObjectName("WingletGUI");

    connect(&settings, SIGNAL(darkModeChanged(bool)), WingletUI::activeTheme, SLOT(setColorModePalette(bool)));
    WingletUI::activeTheme->setColorModePalette(settings.darkMode());
    connect(WingletUI::activeTheme, SIGNAL(colorPaletteChanged()), this, SLOT(colorPaletteUpdated()));

    // Pre-register socket meta type before worker threads start to avoid race
    dummySocket = new QTcpSocket(this);

    // Worker threads
    qRegisterMetaType<WingletUI::WifiScanResult>();
    wifiMonThread = new WingletUI::WorkerThread<WingletUI::WifiMonitor>();
    wifiMonThread->start();
    wifiMon = wifiMonThread->getWorkerObj();

    qRegisterMetaType<WingletUI::BtDeviceInfo>();
    btMonThread = new WingletUI::WorkerThread<WingletUI::BluetoothMonitor>();
    btMonThread->start();
    btMon = btMonThread->getWorkerObj();

    battMonThread = new WingletUI::WorkerThread<WingletUI::BattMonitor>();
    battMonThread->start();
    battMon = battMonThread->getWorkerObj();

    qRegisterMetaType<WingletUI::GPSReading>();
    gpsReceiverThread = new WingletUI::WorkerThread<WingletUI::GPSReceiver>();
    gpsReceiverThread->start();
    gpsReceiver = gpsReceiverThread->getWorkerObj();
    connect(gpsReceiver, SIGNAL(gpsUpdated(WingletUI::GPSReading)), this, SLOT(gpsUpdated(WingletUI::GPSReading)));

    adsbReceiverThread = new WingletUI::WorkerThread<WingletUI::ADSBReceiver>();
    adsbReceiverThread->start();
    adsbReceiver = adsbReceiverThread->getWorkerObj();
    connect(gpsReceiver, SIGNAL(gpsUpdated(WingletUI::GPSReading)), adsbReceiver, SLOT(gpsUpdated(WingletUI::GPSReading)));

    // Right overlay status indicators — initialize from current state then track changes
    rightOverlay->setGpsState(gpsReceiver->state());
    rightOverlay->setAdsbConnected(adsbReceiver->connected());
    connect(gpsReceiver,   SIGNAL(stateUpdated(int)),              rightOverlay, SLOT(setGpsState(int)));
    connect(adsbReceiver,  SIGNAL(connectionStateChanged(bool)),    rightOverlay, SLOT(setAdsbConnected(bool)));
    connect(adsbReceiver,  SIGNAL(aircraftCountChanged(int)),       rightOverlay, SLOT(setAdsbAircraftCount(int)));

    droneReceiverThread = new WingletUI::WorkerThread<WingletUI::DroneReceiver>();
    droneReceiverThread->start();
    droneReceiver = droneReceiverThread->getWorkerObj();

    rtlFm   = new WingletUI::RtlFmWorker(this);
    nasr    = new WingletUI::NASRDatabase(this);
    presets = new WingletUI::RadioPresets(this);

    // Keep the ALSA default device in sync with the audio output setting
    // (also on startup, in case of a fresh flash with a saved setting).
    // At boot the Bluetooth route may not be resolvable yet (BlueZ enumerates
    // devices asynchronously) — the pairedDevicesChanged hook below converges
    // the config once the paired headphones show up.
    connect(&settings, SIGNAL(audioOutputChanged(int)), this, SLOT(applyAudioOutput(int)));
    connect(btMon, SIGNAL(pairedDevicesChanged()), this, SLOT(btPairedDevicesChanged()));
    lastGoodAudioOutput = settings.audioOutput();
    writeAudioOutputConf(settings.audioOutput());

    // 800x480 total window; 480x480 content centered at x=160
    resize(800, 480);
    setPalette(WingletUI::activeTheme->palette);

    setCentralWidget(centralContainer);

    appStack->setGeometry(160, 0, 480, 480);
    appStack->setObjectName("AppStack");
    appStack->addWidget(new WingletUI::MainMenu(this));
    appStack->currentWidget()->setObjectName("MainMenu");

    leftOverlay->setGeometry(0, 0, 160, 480);
    leftOverlay->setFocusPolicy(Qt::NoFocus);
    connect(leftOverlay, SIGNAL(zonePressed(int)), this, SLOT(overlayZonePressed(int)));
    rightOverlay->setGeometry(640, 0, 160, 480);

#ifdef NO_HARDWARE
    QPixmap ringBackground(":/images/dev_ring.png");
    devRingLabel = new QLabel(centralContainer);
    devRingLabel->setGeometry(160, 0, 480, 480);
    devRingLabel->setPixmap(ringBackground);
    devRingLabel->setFocusPolicy(Qt::NoFocus);
    devRingLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    devRingLabel->raise();
    devRingLabel->setObjectName("DevRingLabel");
#endif

    appStack->currentWidget()->setFocus();
}

WingletGUI::~WingletGUI()
{
    delete appStack;
#ifdef NO_HARDWARE
    delete devRingLabel;
#endif

    wifiMonThread->quit();
    btMonThread->quit();
    gpsReceiverThread->quit();
    adsbReceiverThread->quit();
    battMonThread->quit();
    droneReceiverThread->quit();

    wifiMonThread->wait();    delete wifiMonThread;
    btMonThread->wait();      delete btMonThread;
    battMonThread->wait();    delete battMonThread;
    gpsReceiverThread->wait(); delete gpsReceiverThread;
    adsbReceiverThread->wait(); delete adsbReceiverThread;
    droneReceiverThread->wait(); delete droneReceiverThread;

    delete gpioControl;
}

void WingletGUI::keyPressEvent(QKeyEvent *ev)
{
    if (ev->key() == Qt::Key::Key_B) {
        QWidget* widget = appStack->currentWidget();
        if (appStack->currentIndex() != appStack->count() - 1)
            qWarning("WingletGUI::keyPressEvent: Current widget is not at end?");
        // Never close the last widget — that would leave an empty appStack.
        if (widget && appStack->count() > 1 && widget->close()) {
            appStack->removeWidget(widget);
            delete widget;
        }
        widget = appStack->currentWidget();
        if (widget)
            widget->setFocus();
    }
}

void WingletGUI::addWidgetOnTop(QWidget* widget)
{
    if (appStack->indexOf(widget) >= 0) {
        qWarning("WingletGUI::addWidgetOnTop: widget already in stack");
        return;
    }
    appStack->addWidget(widget);
    appStack->setCurrentWidget(widget);
    widget->setFocus();
}

void WingletGUI::replaceWidgetOnTop(QWidget* oldWidget, QWidget* newWidget)
{
    if (appStack->indexOf(newWidget) >= 0) {
        qWarning("WingletGUI::replaceWidgetOnTop: newWidget already in stack");
        return;
    }
    if (appStack->currentWidget() != oldWidget || appStack->currentIndex() != appStack->count() - 1) {
        qWarning("WingletGUI::replaceWidgetOnTop: oldWidget is not top");
        return;
    }
    appStack->addWidget(newWidget);
    appStack->setCurrentWidget(newWidget);
    newWidget->setFocus();
    appStack->removeWidget(oldWidget);
    oldWidget->setParent(this);
    oldWidget->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);
    oldWidget->close();
}

void WingletGUI::removeWidgetOnTop(QWidget* widget)
{
    if (appStack->currentWidget() != widget || appStack->currentIndex() != appStack->count() - 1) {
        qWarning("WingletGUI::removeWidgetOnTop: widget is not top");
        return;
    }
    appStack->removeWidget(widget);
    widget->setParent(this);
    widget->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, true);
    widget->close();
    appStack->setCurrentIndex(appStack->count() - 1);
    if (appStack->count() > 0) {
        QWidget* newTop = appStack->widget(appStack->count() - 1);
        if (newTop)
            newTop->setFocus();
    }
}

void WingletGUI::showMessageBox(const QString& msg, const QString& title, const QString& buttonText, bool replaceCurrentWidget, Qt::Alignment alignment)
{
    auto msgbox = new WingletUI::MessageBox(this, alignment);
    msgbox->setMessageText(msg);
    msgbox->setTitleText(title);
    msgbox->setSingleButtonWithText(buttonText);
    if (replaceCurrentWidget)
        replaceWidgetOnTop(appStack->currentWidget(), msgbox);
    else
        addWidgetOnTop(msgbox);
}

void WingletGUI::gpsUpdated(WingletUI::GPSReading reading)
{
    settings.reportGPSReading(&reading);
}

void WingletGUI::colorPaletteUpdated()
{
    setPalette(WingletUI::activeTheme->palette);
}

void WingletGUI::applyAudioOutput(int output)
{
    // Interactive path (user changed Settings -> Audio)
    if (writeAudioOutputConf(output)) {
        lastGoodAudioOutput = output;
    }
    else {
        showMessageBox("No Bluetooth headphones are paired.\n"
                       "Pair them first in Settings -> Bluetooth -> Pair Headphones.",
                       "No Headphones");
        settings.setAudioOutput(lastGoodAudioOutput);
    }
}

void WingletGUI::btPairedDevicesChanged()
{
    bool connected = false;
    foreach (const WingletUI::BtDeviceInfo &dev, btMon->pairedDevices()) {
        if (dev.isAudio && dev.connected) {
            connected = true;
            break;
        }
    }

    bool newlyConnected = (connected && !btAudioWasConnected);
    bool newlyDisconnected = (!connected && btAudioWasConnected);
    btAudioWasConnected = connected;

    // A headset just connected: make it the active output, remembering the
    // wired output it displaced. Only on the connection edge, so the user can
    // still switch to the jack/HDMI afterwards while it stays connected.
    if (newlyConnected && settings.audioOutput() != 2) {
        btAudioRevertOutput = settings.audioOutput();
        settings.setAudioOutput(2);  // audioOutputChanged -> applyAudioOutput(2),
        return;                      // which writes asound.conf and reopens the stream
    }

    // The headset just disconnected: if it is still the selected output (the
    // user didn't manually switch away while it was connected), restore the
    // output that was active before it connected.
    if (newlyDisconnected) {
        if (settings.audioOutput() == 2 && btAudioRevertOutput >= 0)
            settings.setAudioOutput(btAudioRevertOutput);
        btAudioRevertOutput = -1;
        return;
    }

    if (settings.audioOutput() != 2)
        return;

    // Already routed to Bluetooth: re-resolve the MAC in asound.conf (no-op if
    // unchanged) — this also covers the boot race where the setting is applied
    // before BlueZ has enumerated the paired headphones.
    writeAudioOutputConf(2);

    // bluealsa's PCM only exists while the headphones are connected, so an
    // aplay started while they were away has already died; reopen the tuner
    // stream when they (re)appear.
    if (newlyConnected)
        rtlFm->restartAudio();
}

bool WingletGUI::writeAudioOutputConf(int output)
{
    // Route the ALSA "default" device (used by aplay and any future audio) to
    // the selected output. Card ids: bcm2835 analog = "Headphones" (needs
    // dtparam=audio=on), vc4 HDMI0 = "vc4hdmi0" (needs hdmi_drive:0=2).
    // Bluetooth goes through the bluez-alsa daemon's ALSA plugin instead of a
    // sound card; "plug" adapts the tuner's 48k mono S16 to the A2DP stream.
    // Every route is wrapped in a softvol plugin named "Radio" so the tuner's
    // volume slider works uniformly: the 3.5mm jack has a hardware mixer but
    // vc4hdmi and Bluetooth do not. The softvol control is pinned to the
    // always-present "Headphones" card so `amixer sset Radio N%` resolves the
    // same control regardless of the selected output.
    const char *SOFTVOL_CTL =
        "    control { name \"Radio\"; card \"Headphones\" }\n"
        "    min_dB -51.0\n"
        "    max_dB 0.0\n";

    QString conf;
    if (output == 2) {
        // Prefer the connected headphones, else the first paired audio device
        QString addr;
        foreach (const WingletUI::BtDeviceInfo &dev, btMon->pairedDevices()) {
            if (!dev.isAudio)
                continue;
            if (addr.isEmpty() || dev.connected)
                addr = dev.address;
            if (dev.connected)
                break;
        }
        if (addr.isEmpty())
            return false;  // Nothing paired to route to; keep the current conf

        conf = QString("# Generated by aeroscan-gui (Settings -> Audio) - do not edit\n"
                       "pcm.!default {\n"
                       "    type plug\n"
                       "    slave.pcm {\n"
                       "        type softvol\n"
                       "        slave.pcm {\n"
                       "            type bluealsa\n"
                       "            device \"%1\"\n"
                       "            profile \"a2dp\"\n"
                       "        }\n"
                       "%2"
                       "    }\n"
                       "}\n"
                       "ctl.!default {\n"
                       "    type bluealsa\n"
                       "    device \"%1\"\n"
                       "}\n").arg(addr, SOFTVOL_CTL);
    }
    else {
        const char *card = (output == 1) ? "vc4hdmi0" : "Headphones";
        conf = QString("# Generated by aeroscan-gui (Settings -> Audio) - do not edit\n"
                       "pcm.!default {\n"
                       "    type plug\n"
                       "    slave.pcm {\n"
                       "        type softvol\n"
                       "        slave.pcm { type hw; card \"%1\" }\n"
                       "%2"
                       "    }\n"
                       "}\n"
                       "ctl.!default { type hw; card \"Headphones\" }\n").arg(card, SOFTVOL_CTL);
    }

    QFile confFile("/etc/asound.conf");
    if (confFile.open(QFile::ReadOnly)) {
        // Don't rewrite (and don't bounce the tuner stream) if nothing changed
        bool unchanged = (confFile.readAll() == conf.toUtf8());
        confFile.close();
        if (unchanged)
            return true;
    }

    if (confFile.open(QFile::WriteOnly | QFile::Truncate)) {
        confFile.write(conf.toUtf8());
        confFile.close();
    }
    else {
        qWarning("Failed to write /etc/asound.conf");
        return true;  // I/O failure, not a routing error — don't revert the setting
    }

    // aplay holds the old device; reopen the stream if the tuner is playing
    rtlFm->restartAudio();
    return true;
}

bool WingletGUI::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key::Key_PowerOff && keyEvent->isAutoRepeat()) {
            QApplication::exit(WingletUI::EXIT_CODE_POWEROFF);
            return true;
        }
    }

    // Convert a tap on the center app-stack area into Key_A (select).
    // Widgets in the stack are keyboard-driven and don't handle raw mouse events.
    if (event->type() == QEvent::MouseButtonPress) {
        QWidget *w = qobject_cast<QWidget *>(object);
        if (w && appStack->isAncestorOf(w)) {
            QWidget *target = QApplication::focusWidget();
            if (!target) target = appStack->currentWidget();
            if (target) {
                QKeyEvent kp(QEvent::KeyPress,   Qt::Key_A, Qt::NoModifier);
                QKeyEvent kr(QEvent::KeyRelease, Qt::Key_A, Qt::NoModifier);
                QApplication::sendEvent(target, &kp);
                QApplication::sendEvent(target, &kr);
            }
            return true;
        }
    }

    return false;
}

void WingletGUI::overlayZonePressed(int zone)
{
    static const Qt::Key zoneKeys[] = {
        Qt::Key_Up, Qt::Key_Down, Qt::Key_A, Qt::Key_B
    };
    Qt::Key key = zoneKeys[zone];

    // Key_B goes directly to WingletGUI — sub-screen keyPressEvent handlers
    // accept-without-ignoring unhandled keys, so Key_B never propagates up
    // to WingletGUI::keyPressEvent otherwise.
    QWidget *target = (key == Qt::Key_B) ? this : QApplication::focusWidget();
    if (!target) target = appStack->currentWidget();
    if (target) {
        QKeyEvent kpress(QEvent::KeyPress, key, Qt::NoModifier);
        QKeyEvent krelease(QEvent::KeyRelease, key, Qt::NoModifier);
        QApplication::sendEvent(target, &kpress);
        QApplication::sendEvent(target, &krelease);
    }
}

bool WingletGUI::tryShowReleaseNotes(bool forceShow)
{
    if (!forceShow && !QFile::exists(RELEASE_NOTES_FLAG_FILE))
        return false;
    QFile file(RELEASE_NOTES_LOCATION);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QTextStream in(&file);
    QString title = in.readLine();
    QString body = in.readAll();
    file.close();
    if (title.isEmpty() || body.isEmpty())
        return false;
    WingletGUI::inst->showMessageBox(body, title, "Okay", false, Qt::AlignLeft | Qt::AlignVCenter);
    return true;
}
