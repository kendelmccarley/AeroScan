#ifndef WINGLETUI_DRONERECEIVER_H
#define WINGLETUI_DRONERECEIVER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QDateTime>

namespace WingletUI {

struct Drone {
    QString mac;
    QString id;
    QString radio;      // "BLE" or "WiFi"
    double lat = 0, lon = 0;
    double altM = 0;
    double speedMs = 0;
    double hdg = 0;
    int rssi = 0;
    QDateTime timestamp;
    double distance = 0;    // nm from own-ship
    double bearing = 0;     // degrees true
};

// Stub worker — binds UDP 9999, returns empty map until Phase 10 services run.
class DroneReceiver : public QObject
{
    Q_OBJECT
public:
    explicit DroneReceiver(QThread *ownerThread);
    ~DroneReceiver();

    QMap<QString, Drone> droneSpace() const { return m_drones; }

private:
    QMap<QString, Drone> m_drones;
};

} // namespace WingletUI

#endif // WINGLETUI_DRONERECEIVER_H
