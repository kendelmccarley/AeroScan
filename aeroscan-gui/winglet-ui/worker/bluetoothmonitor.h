#ifndef WINGLETUI_BLUETOOTHMONITOR_H
#define WINGLETUI_BLUETOOTHMONITOR_H

#include <QObject>
#include <QMap>
#include <QMutex>
#include <QStringList>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDBusContext>
#include <QDBusMessage>
#include <QVariantMap>

class QDBusServiceWatcher;
class QDBusPendingCallWatcher;

// D-Bus compound types used by org.freedesktop.DBus.ObjectManager
typedef QMap<QString, QVariantMap> InterfaceMap;                 // a{sa{sv}}
typedef QMap<QDBusObjectPath, InterfaceMap> ManagedObjectMap;    // a{oa{sa{sv}}}
Q_DECLARE_METATYPE(InterfaceMap)
Q_DECLARE_METATYPE(ManagedObjectMap)

namespace WingletUI {

struct BtDeviceInfo {
    QString path;        // BlueZ object path (unique key)
    QString address;
    QString name;
    int signalStrength;  // 0-100, -1 if unknown
    bool paired;
    bool connected;
    bool isKeyboard;
};

class BluetoothMonitor;

// Implements org.bluez.Agent1 so bluetoothd can ask us to display pairing
// codes. Capability is DisplayOnly: keyboards type the passkey we show.
class BtPairingAgent : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.bluez.Agent1")

public:
    explicit BtPairingAgent(BluetoothMonitor *monitor);

public slots:
    void Release();
    QString RequestPinCode(const QDBusObjectPath &device);
    void DisplayPinCode(const QDBusObjectPath &device, const QString &pincode);
    quint32 RequestPasskey(const QDBusObjectPath &device);
    void DisplayPasskey(const QDBusObjectPath &device, quint32 passkey, quint16 entered);
    void RequestConfirmation(const QDBusObjectPath &device, quint32 passkey);
    void RequestAuthorization(const QDBusObjectPath &device);
    void AuthorizeService(const QDBusObjectPath &device, const QString &uuid);
    void Cancel();

private:
    BluetoothMonitor *monitor;
};

class BluetoothMonitor : public QObject
{
    Q_OBJECT

public:
    explicit BluetoothMonitor(QThread *ownerThread);
    ~BluetoothMonitor();

    enum BtState {
        BT_OFF,        // No bluetoothd / no adapter / adapter powered down
        BT_IDLE,
        BT_SCANNING,
        BT_PAIRING,
        BT_CONNECTED   // At least one paired input device connected
    };

    BtState btState() const { return m_btState; }

    // Thread-safe snapshots for models in the GUI thread
    QList<BtDeviceInfo> scanResults();     // Discovered keyboards (incl. paired ones, flagged)
    QList<BtDeviceInfo> pairedDevices();

    // Thread-safe requests (proxied onto the worker thread)
    void startScan();
    void stopScan();
    void pairDevice(const QString &devicePath);
    void cancelPairing();
    void connectDevice(const QString &devicePath);
    void removeDevice(const QString &devicePath);

signals:
    void btStateChanged(int state);
    void scanResultsChanged();
    void pairedDevicesChanged();
    // Pairing feedback for the UI. displayCode is a string of digits the user
    // must type on the keyboard (followed by enter).
    void pairingDisplayCode(QString displayCode);
    void pairingComplete(bool success, QString errorMessage);

    // Private signals
    void doStartScan(QPrivateSignal);
    void doStopScan(QPrivateSignal);
    void doPairDevice(QString devicePath, QPrivateSignal);
    void doCancelPairing(QPrivateSignal);
    void doConnectDevice(QString devicePath, QPrivateSignal);
    void doRemoveDevice(QString devicePath, QPrivateSignal);

private slots:
    void bluezRegistered();
    void bluezUnregistered();
    void managedObjectsReady(QDBusPendingCallWatcher *watcher);
    void interfacesAdded(const QDBusObjectPath &path, InterfaceMap interfaces);
    void interfacesRemoved(const QDBusObjectPath &path, const QStringList &interfaces);
    void propertiesChanged(const QString &interface, const QVariantMap &changed,
                           const QStringList &invalidated, const QDBusMessage &msg);
    void startScanInThread();
    void stopScanInThread();
    void pairDeviceInThread(QString devicePath);
    void cancelPairingInThread();
    void connectDeviceInThread(QString devicePath);
    void removeDeviceInThread(QString devicePath);
    void pairReplyReady(QDBusPendingCallWatcher *watcher);
    void connectReplyReady(QDBusPendingCallWatcher *watcher);

private:
    friend class BtPairingAgent;

    // Setup / teardown of the org.bluez session
    void setupBluez();
    void teardownBluez();
    void registerAgent();

    // Device bookkeeping (worker thread only)
    void ingestObject(const QString &path, const InterfaceMap &interfaces);
    void applyDeviceProperties(const QString &path, const QVariantMap &props);
    void applyAdapterProperties(const QVariantMap &props);
    void publishDevices();
    void recomputeState();
    void reportPairingResult(bool success, const QString &errorMessage);

    // D-Bus helpers (worker thread only)
    void setAdapterProperty(const char *name, const QVariant &value);
    void setDeviceProperty(const QString &devicePath, const char *name, const QVariant &value);
    QDBusPendingCallWatcher *asyncDeviceCall(const QString &devicePath, const char *method,
                                             int timeoutMs);
    static bool looksLikeKeyboard(const QVariantMap &props);
    static int rssiToStrength(const QVariant &rssi);

    QDBusServiceWatcher *bluezWatcher = nullptr;
    BtPairingAgent *agent = nullptr;
    bool agentObjectRegistered = false;
    bool agentRegistered = false;
    bool bluezAvailable = false;

    QString adapterPath;
    bool adapterPowered = false;
    bool adapterDiscovering = false;

    // Full property cache per device path (worker thread only)
    QMap<QString, QVariantMap> deviceProps;

    // Pairing state (worker thread only)
    QString pairingDevicePath;

    // Snapshot shared with the GUI thread
    QMutex m_devicesMutex;
    QList<BtDeviceInfo> m_devices;

    BtState m_btState = BT_OFF;
};

} // namespace WingletUI

Q_DECLARE_METATYPE(WingletUI::BtDeviceInfo)

#endif // WINGLETUI_BLUETOOTHMONITOR_H
