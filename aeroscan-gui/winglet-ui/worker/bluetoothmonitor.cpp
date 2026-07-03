#include "bluetoothmonitor.h"

#include <QDBusServiceWatcher>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusMetaType>
#include <QDBusVariant>
#include <QRandomGenerator>

namespace WingletUI {

#define BLUEZ_SERVICE "org.bluez"
#define ADAPTER_IFACE "org.bluez.Adapter1"
#define DEVICE_IFACE "org.bluez.Device1"
#define PROPERTIES_IFACE "org.freedesktop.DBus.Properties"
#define OBJMGR_IFACE "org.freedesktop.DBus.ObjectManager"
#define AGENT_PATH "/com/aeroscan/btagent"

// Bluetooth SIG service UUIDs: classic HID and HID-over-GATT (BLE keyboards)
#define HID_UUID "00001124-0000-1000-8000-00805f9b34fb"
#define HOG_UUID "00001812-0000-1000-8000-00805f9b34fb"

#define PAIR_TIMEOUT_MS 90000
#define CONNECT_TIMEOUT_MS 30000

// ---------------------------------------------------------------------------
// BtPairingAgent — org.bluez.Agent1
// ---------------------------------------------------------------------------

BtPairingAgent::BtPairingAgent(BluetoothMonitor *monitor)
    : QObject(monitor), monitor(monitor) {}

void BtPairingAgent::Release()
{
    monitor->agentRegistered = false;
}

QString BtPairingAgent::RequestPinCode(const QDBusObjectPath &device)
{
    // Legacy (pre-SSP) pairing: we pick a PIN, the user types it on the
    // keyboard and presses enter. (Zero-PIN devices like the Apple A1314
    // never reach this callback — bluetoothd handles them silently.)
    (void) device;
    QString pin = QString::asprintf("%04u", QRandomGenerator::global()->bounded(10000u));
    emit monitor->pairingDisplayCode(pin);
    return pin;
}

void BtPairingAgent::DisplayPinCode(const QDBusObjectPath &device, const QString &pincode)
{
    (void) device;
    emit monitor->pairingDisplayCode(pincode);
}

quint32 BtPairingAgent::RequestPasskey(const QDBusObjectPath &device)
{
    // Only called when the remote side has a display and we'd need a numeric
    // keypad. Not possible on this device, reject the pairing.
    (void) device;
    sendErrorReply("org.bluez.Error.Rejected", "No input available on this device");
    return 0;
}

void BtPairingAgent::DisplayPasskey(const QDBusObjectPath &device, quint32 passkey, quint16 entered)
{
    // SSP passkey entry: show 6 digits, user types them on the keyboard.
    // bluetoothd re-invokes this as digits are entered; only show it once.
    (void) device;
    if (entered == 0)
        emit monitor->pairingDisplayCode(QString::asprintf("%06u", passkey));
}

void BtPairingAgent::RequestConfirmation(const QDBusObjectPath &device, quint32 passkey)
{
    // Numeric comparison. The user already picked this device on screen, so
    // accept — a keyboard can't display the passkey for comparison anyway.
    (void) device;
    (void) passkey;
}

void BtPairingAgent::RequestAuthorization(const QDBusObjectPath &device)
{
    (void) device;
}

void BtPairingAgent::AuthorizeService(const QDBusObjectPath &device, const QString &uuid)
{
    (void) device;
    (void) uuid;
}

void BtPairingAgent::Cancel()
{
    // Pairing cancelled by bluetoothd; the pending Pair() reply carries the error
}

// ---------------------------------------------------------------------------
// BluetoothMonitor
// ---------------------------------------------------------------------------

BluetoothMonitor::BluetoothMonitor(QThread *ownerThread)
{
    moveToThread(ownerThread);

    qDBusRegisterMetaType<InterfaceMap>();
    qDBusRegisterMetaType<ManagedObjectMap>();

    agent = new BtPairingAgent(this);

    connect(this, SIGNAL(doStartScan()), this, SLOT(startScanInThread()));
    connect(this, SIGNAL(doStopScan()), this, SLOT(stopScanInThread()));
    connect(this, SIGNAL(doPairDevice(QString)), this, SLOT(pairDeviceInThread(QString)));
    connect(this, SIGNAL(doCancelPairing()), this, SLOT(cancelPairingInThread()));
    connect(this, SIGNAL(doConnectDevice(QString)), this, SLOT(connectDeviceInThread(QString)));
    connect(this, SIGNAL(doRemoveDevice(QString)), this, SLOT(removeDeviceInThread(QString)));

    QDBusConnection bus = QDBusConnection::systemBus();
    if (!bus.isConnected()) {
        qWarning("BluetoothMonitor: system D-Bus unavailable, bluetooth disabled");
        return;
    }

    bluezWatcher = new QDBusServiceWatcher(BLUEZ_SERVICE, bus,
                                           QDBusServiceWatcher::WatchForRegistration |
                                           QDBusServiceWatcher::WatchForUnregistration,
                                           this);
    connect(bluezWatcher, SIGNAL(serviceRegistered(QString)), this, SLOT(bluezRegistered()));
    connect(bluezWatcher, SIGNAL(serviceUnregistered(QString)), this, SLOT(bluezUnregistered()));

    if (bus.interface()->isServiceRegistered(BLUEZ_SERVICE))
        setupBluez();
}

BluetoothMonitor::~BluetoothMonitor()
{
    if (agentObjectRegistered)
        QDBusConnection::systemBus().unregisterObject(AGENT_PATH);
}

void BluetoothMonitor::bluezRegistered()
{
    setupBluez();
}

void BluetoothMonitor::bluezUnregistered()
{
    teardownBluez();
}

void BluetoothMonitor::setupBluez()
{
    if (bluezAvailable)
        return;
    bluezAvailable = true;

    QDBusConnection bus = QDBusConnection::systemBus();
    bus.connect(BLUEZ_SERVICE, "/", OBJMGR_IFACE, "InterfacesAdded",
                this, SLOT(interfacesAdded(QDBusObjectPath,InterfaceMap)));
    bus.connect(BLUEZ_SERVICE, "/", OBJMGR_IFACE, "InterfacesRemoved",
                this, SLOT(interfacesRemoved(QDBusObjectPath,QStringList)));
    // Empty path = match PropertiesChanged from every BlueZ object (adapter + devices)
    bus.connect(BLUEZ_SERVICE, "", PROPERTIES_IFACE, "PropertiesChanged",
                this, SLOT(propertiesChanged(QString,QVariantMap,QStringList,QDBusMessage)));

    registerAgent();

    QDBusMessage msg = QDBusMessage::createMethodCall(BLUEZ_SERVICE, "/", OBJMGR_IFACE,
                                                      "GetManagedObjects");
    auto *watcher = new QDBusPendingCallWatcher(bus.asyncCall(msg), this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(managedObjectsReady(QDBusPendingCallWatcher*)));
}

void BluetoothMonitor::teardownBluez()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    bus.disconnect(BLUEZ_SERVICE, "/", OBJMGR_IFACE, "InterfacesAdded",
                   this, SLOT(interfacesAdded(QDBusObjectPath,InterfaceMap)));
    bus.disconnect(BLUEZ_SERVICE, "/", OBJMGR_IFACE, "InterfacesRemoved",
                   this, SLOT(interfacesRemoved(QDBusObjectPath,QStringList)));
    bus.disconnect(BLUEZ_SERVICE, "", PROPERTIES_IFACE, "PropertiesChanged",
                   this, SLOT(propertiesChanged(QString,QVariantMap,QStringList,QDBusMessage)));

    bluezAvailable = false;
    agentRegistered = false;
    adapterPath.clear();
    adapterPowered = false;
    adapterDiscovering = false;
    deviceProps.clear();

    if (!pairingDevicePath.isEmpty())
        reportPairingResult(false, "Bluetooth service stopped");

    publishDevices();
    recomputeState();
}

void BluetoothMonitor::registerAgent()
{
    QDBusConnection bus = QDBusConnection::systemBus();

    if (!agentObjectRegistered) {
        agentObjectRegistered = bus.registerObject(AGENT_PATH, agent,
                                                   QDBusConnection::ExportAllSlots);
        if (!agentObjectRegistered) {
            qWarning("BluetoothMonitor: failed to register pairing agent object");
            return;
        }
    }

    if (agentRegistered)
        return;

    // DisplayOnly: we can show a passkey on screen; the keyboard types it back
    QDBusMessage reg = QDBusMessage::createMethodCall(BLUEZ_SERVICE, "/org/bluez",
                                                      "org.bluez.AgentManager1", "RegisterAgent");
    reg << QVariant::fromValue(QDBusObjectPath(AGENT_PATH)) << QString("DisplayOnly");
    QDBusMessage regReply = bus.call(reg);
    if (regReply.type() == QDBusMessage::ErrorMessage) {
        qWarning("BluetoothMonitor: RegisterAgent failed: %s",
                 qUtf8Printable(regReply.errorMessage()));
        return;
    }

    QDBusMessage def = QDBusMessage::createMethodCall(BLUEZ_SERVICE, "/org/bluez",
                                                      "org.bluez.AgentManager1", "RequestDefaultAgent");
    def << QVariant::fromValue(QDBusObjectPath(AGENT_PATH));
    bus.asyncCall(def);

    agentRegistered = true;
}

void BluetoothMonitor::managedObjectsReady(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<ManagedObjectMap> reply = *watcher;
    watcher->deleteLater();

    if (reply.isError()) {
        qWarning("BluetoothMonitor: GetManagedObjects failed: %s",
                 qUtf8Printable(reply.error().message()));
        return;
    }

    ManagedObjectMap objects = reply.value();
    for (auto itr = objects.constBegin(); itr != objects.constEnd(); itr++)
        ingestObject(itr.key().path(), itr.value());

    publishDevices();
    recomputeState();
}

void BluetoothMonitor::interfacesAdded(const QDBusObjectPath &path, InterfaceMap interfaces)
{
    ingestObject(path.path(), interfaces);
    publishDevices();
    recomputeState();
}

void BluetoothMonitor::interfacesRemoved(const QDBusObjectPath &path, const QStringList &interfaces)
{
    if (interfaces.contains(DEVICE_IFACE))
        deviceProps.remove(path.path());

    if (interfaces.contains(ADAPTER_IFACE) && path.path() == adapterPath) {
        adapterPath.clear();
        adapterPowered = false;
        adapterDiscovering = false;
        if (!pairingDevicePath.isEmpty())
            reportPairingResult(false, "Bluetooth adapter removed");
    }

    publishDevices();
    recomputeState();
}

void BluetoothMonitor::ingestObject(const QString &path, const InterfaceMap &interfaces)
{
    auto adapterItr = interfaces.constFind(ADAPTER_IFACE);
    if (adapterItr != interfaces.constEnd() && adapterPath.isEmpty()) {
        adapterPath = path;
        applyAdapterProperties(*adapterItr);
    }

    auto deviceItr = interfaces.constFind(DEVICE_IFACE);
    if (deviceItr != interfaces.constEnd())
        applyDeviceProperties(path, *deviceItr);
}

void BluetoothMonitor::applyDeviceProperties(const QString &path, const QVariantMap &props)
{
    QVariantMap &cache = deviceProps[path];
    for (auto itr = props.constBegin(); itr != props.constEnd(); itr++)
        cache.insert(itr.key(), itr.value());
}

void BluetoothMonitor::applyAdapterProperties(const QVariantMap &props)
{
    auto itr = props.constFind("Powered");
    if (itr != props.constEnd())
        adapterPowered = itr->toBool();

    itr = props.constFind("Discovering");
    if (itr != props.constEnd())
        adapterDiscovering = itr->toBool();
}

void BluetoothMonitor::propertiesChanged(const QString &interface, const QVariantMap &changed,
                                         const QStringList &invalidated, const QDBusMessage &msg)
{
    if (interface == ADAPTER_IFACE) {
        if (msg.path() != adapterPath)
            return;
        applyAdapterProperties(changed);
    }
    else if (interface == DEVICE_IFACE) {
        auto itr = deviceProps.find(msg.path());
        if (itr == deviceProps.end())
            return;
        for (auto changedItr = changed.constBegin(); changedItr != changed.constEnd(); changedItr++)
            itr->insert(changedItr.key(), changedItr.value());
        foreach (const QString &key, invalidated)
            itr->remove(key);
    }
    else {
        return;
    }

    publishDevices();
    recomputeState();
}

// --- Requests proxied from the GUI thread ---

void BluetoothMonitor::startScan() { emit doStartScan(QPrivateSignal()); }
void BluetoothMonitor::stopScan() { emit doStopScan(QPrivateSignal()); }
void BluetoothMonitor::pairDevice(const QString &devicePath) { emit doPairDevice(devicePath, QPrivateSignal()); }
void BluetoothMonitor::cancelPairing() { emit doCancelPairing(QPrivateSignal()); }
void BluetoothMonitor::connectDevice(const QString &devicePath) { emit doConnectDevice(devicePath, QPrivateSignal()); }
void BluetoothMonitor::removeDevice(const QString &devicePath) { emit doRemoveDevice(devicePath, QPrivateSignal()); }

void BluetoothMonitor::startScanInThread()
{
    if (!bluezAvailable || adapterPath.isEmpty())
        return;

    if (!adapterPowered)
        setAdapterProperty("Powered", true);

    if (adapterDiscovering)
        return;

    QDBusMessage msg = QDBusMessage::createMethodCall(BLUEZ_SERVICE, adapterPath,
                                                      ADAPTER_IFACE, "StartDiscovery");
    QDBusConnection::systemBus().asyncCall(msg);
}

void BluetoothMonitor::stopScanInThread()
{
    if (!bluezAvailable || adapterPath.isEmpty() || !adapterDiscovering)
        return;

    QDBusMessage msg = QDBusMessage::createMethodCall(BLUEZ_SERVICE, adapterPath,
                                                      ADAPTER_IFACE, "StopDiscovery");
    QDBusConnection::systemBus().asyncCall(msg);
}

void BluetoothMonitor::pairDeviceInThread(QString devicePath)
{
    if (!bluezAvailable || adapterPath.isEmpty()) {
        emit pairingComplete(false, "Bluetooth is not available");
        return;
    }
    if (!pairingDevicePath.isEmpty())
        return;  // Pairing already in progress
    if (!deviceProps.contains(devicePath)) {
        emit pairingComplete(false, "Device is no longer visible");
        return;
    }

    pairingDevicePath = devicePath;
    recomputeState();

    if (deviceProps[devicePath].value("Paired").toBool()) {
        // Already bonded (e.g. re-selected from scan) — just trust + connect
        setDeviceProperty(devicePath, "Trusted", true);
        auto *watcher = asyncDeviceCall(devicePath, "Connect", CONNECT_TIMEOUT_MS);
        connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                this, SLOT(connectReplyReady(QDBusPendingCallWatcher*)));
        return;
    }

    setAdapterProperty("Pairable", true);
    auto *watcher = asyncDeviceCall(devicePath, "Pair", PAIR_TIMEOUT_MS);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(pairReplyReady(QDBusPendingCallWatcher*)));
}

void BluetoothMonitor::pairReplyReady(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<> reply = *watcher;
    watcher->deleteLater();

    if (pairingDevicePath.isEmpty())
        return;  // Cancelled while the call was in flight

    if (reply.isError() && reply.error().name() != "org.bluez.Error.AlreadyExists") {
        setAdapterProperty("Pairable", false);
        reportPairingResult(false, reply.error().message());
        return;
    }

    // Bonded. Trust it so bluetoothd accepts its reconnects at boot, then
    // bring up the HID connection.
    setDeviceProperty(pairingDevicePath, "Trusted", true);
    auto *connectWatcher = asyncDeviceCall(pairingDevicePath, "Connect", CONNECT_TIMEOUT_MS);
    connect(connectWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(connectReplyReady(QDBusPendingCallWatcher*)));
}

void BluetoothMonitor::connectReplyReady(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<> reply = *watcher;
    watcher->deleteLater();

    if (pairingDevicePath.isEmpty())
        return;  // Cancelled while the call was in flight

    setAdapterProperty("Pairable", false);

    if (reply.isError() && reply.error().name() != "org.bluez.Error.AlreadyConnected")
        reportPairingResult(false, reply.error().message());
    else
        reportPairingResult(true, "");
}

void BluetoothMonitor::cancelPairingInThread()
{
    if (pairingDevicePath.isEmpty())
        return;

    QDBusMessage msg = QDBusMessage::createMethodCall(BLUEZ_SERVICE, pairingDevicePath,
                                                      DEVICE_IFACE, "CancelPairing");
    QDBusConnection::systemBus().asyncCall(msg);

    setAdapterProperty("Pairable", false);
    reportPairingResult(false, "Pairing cancelled");
}

void BluetoothMonitor::connectDeviceInThread(QString devicePath)
{
    if (!bluezAvailable || !deviceProps.contains(devicePath)) {
        emit pairingComplete(false, "Device is not available");
        return;
    }
    if (!pairingDevicePath.isEmpty())
        return;

    pairingDevicePath = devicePath;
    recomputeState();

    setDeviceProperty(devicePath, "Trusted", true);
    auto *watcher = asyncDeviceCall(devicePath, "Connect", CONNECT_TIMEOUT_MS);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(connectReplyReady(QDBusPendingCallWatcher*)));
}

void BluetoothMonitor::removeDeviceInThread(QString devicePath)
{
    if (!bluezAvailable || adapterPath.isEmpty())
        return;

    // RemoveDevice also deletes the stored link key; bluetoothd then emits
    // InterfacesRemoved which drops it from our cache.
    QDBusMessage msg = QDBusMessage::createMethodCall(BLUEZ_SERVICE, adapterPath,
                                                      ADAPTER_IFACE, "RemoveDevice");
    msg << QVariant::fromValue(QDBusObjectPath(devicePath));
    QDBusConnection::systemBus().asyncCall(msg);
}

void BluetoothMonitor::reportPairingResult(bool success, const QString &errorMessage)
{
    pairingDevicePath.clear();
    recomputeState();
    emit pairingComplete(success, errorMessage);
}

// --- D-Bus helpers ---

void BluetoothMonitor::setAdapterProperty(const char *name, const QVariant &value)
{
    if (adapterPath.isEmpty())
        return;

    QDBusMessage msg = QDBusMessage::createMethodCall(BLUEZ_SERVICE, adapterPath,
                                                      PROPERTIES_IFACE, "Set");
    msg << QString(ADAPTER_IFACE) << QString(name) << QVariant::fromValue(QDBusVariant(value));
    QDBusConnection::systemBus().asyncCall(msg);
}

void BluetoothMonitor::setDeviceProperty(const QString &devicePath, const char *name, const QVariant &value)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(BLUEZ_SERVICE, devicePath,
                                                      PROPERTIES_IFACE, "Set");
    msg << QString(DEVICE_IFACE) << QString(name) << QVariant::fromValue(QDBusVariant(value));
    QDBusConnection::systemBus().asyncCall(msg);
}

QDBusPendingCallWatcher *BluetoothMonitor::asyncDeviceCall(const QString &devicePath,
                                                           const char *method, int timeoutMs)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(BLUEZ_SERVICE, devicePath,
                                                      DEVICE_IFACE, method);
    return new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(msg, timeoutMs), this);
}

bool BluetoothMonitor::looksLikeKeyboard(const QVariantMap &props)
{
    if (props.value("Icon").toString() == "input-keyboard")
        return true;

    QStringList uuids = props.value("UUIDs").toStringList();
    if (uuids.contains(HID_UUID, Qt::CaseInsensitive) || uuids.contains(HOG_UUID, Qt::CaseInsensitive))
        return true;

    // Class of Device: major class 5 (peripheral) with the keyboard minor bit
    quint32 cod = props.value("Class").toUInt();
    if (((cod >> 8) & 0x1F) == 0x05 && (cod & 0x40))
        return true;

    return false;
}

int BluetoothMonitor::rssiToStrength(const QVariant &rssi)
{
    bool okay = false;
    int level = rssi.toInt(&okay);
    if (!okay || level == 0)
        return -1;

    // Same dBm mapping the WiFi monitor uses
    level = 2 * (100 + level);
    if (level > 100)
        level = 100;
    if (level < 0)
        level = 0;
    return level;
}

// --- State publication ---

void BluetoothMonitor::publishDevices()
{
    QList<BtDeviceInfo> devices;
    for (auto itr = deviceProps.constBegin(); itr != deviceProps.constEnd(); itr++) {
        const QVariantMap &props = itr.value();

        BtDeviceInfo info;
        info.path = itr.key();
        info.address = props.value("Address").toString();
        info.name = props.value("Alias").toString();
        if (info.name.isEmpty())
            info.name = props.value("Name").toString();
        if (info.name.isEmpty())
            info.name = info.address;
        info.signalStrength = rssiToStrength(props.value("RSSI"));
        info.paired = props.value("Paired").toBool();
        info.connected = props.value("Connected").toBool();
        info.isKeyboard = looksLikeKeyboard(props);
        devices.append(info);
    }

    m_devicesMutex.lock();
    m_devices = devices;
    m_devicesMutex.unlock();

    emit scanResultsChanged();
    emit pairedDevicesChanged();
}

void BluetoothMonitor::recomputeState()
{
    BtState state;
    if (!bluezAvailable || adapterPath.isEmpty() || !adapterPowered) {
        state = BT_OFF;
    }
    else if (!pairingDevicePath.isEmpty()) {
        state = BT_PAIRING;
    }
    else if (adapterDiscovering) {
        state = BT_SCANNING;
    }
    else {
        state = BT_IDLE;
        for (auto itr = deviceProps.constBegin(); itr != deviceProps.constEnd(); itr++) {
            if (itr->value("Paired").toBool() && itr->value("Connected").toBool()) {
                state = BT_CONNECTED;
                break;
            }
        }
    }

    if (state != m_btState) {
        m_btState = state;
        emit btStateChanged(state);
    }
}

QList<BtDeviceInfo> BluetoothMonitor::scanResults()
{
    m_devicesMutex.lock();
    QList<BtDeviceInfo> devices = m_devices;
    m_devicesMutex.unlock();

    QList<BtDeviceInfo> results;
    foreach (const BtDeviceInfo &device, devices) {
        if (device.isKeyboard)
            results.append(device);
    }

    std::sort(results.begin(), results.end(), [](const BtDeviceInfo &v1, const BtDeviceInfo &v2) {
        if (v1.signalStrength == v2.signalStrength)
            return v1.name < v2.name;  // Strict weak ordering fallback
        return v1.signalStrength > v2.signalStrength;
    });
    return results;
}

QList<BtDeviceInfo> BluetoothMonitor::pairedDevices()
{
    m_devicesMutex.lock();
    QList<BtDeviceInfo> devices = m_devices;
    m_devicesMutex.unlock();

    QList<BtDeviceInfo> results;
    foreach (const BtDeviceInfo &device, devices) {
        if (device.paired)
            results.append(device);
    }

    std::sort(results.begin(), results.end(), [](const BtDeviceInfo &v1, const BtDeviceInfo &v2) {
        return v1.name < v2.name;
    });
    return results;
}

} // namespace WingletUI
