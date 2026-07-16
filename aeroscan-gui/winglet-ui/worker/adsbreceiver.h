#ifndef WINGLETUI_ADSBRECEIVER_H
#define WINGLETUI_ADSBRECEIVER_H

#include "winglet-ui/worker/gpsreceiver.h"
#include "abstractsocketworker.h"
#include <QDateTime>
#include <QMutex>
#include <QSet>
#include <cmath>

namespace WingletUI {

struct Aircraft {
    int messageType = 0;        /* Message recieved type */
    quint32 icao24 = 0;        /* ADSHex ** Key ID for building array ** */
    QString callSign = "-----"; /* Flight number */
    int alt = 0;                /* Altitude */
    float lat = 0 ;             /* Latitude */
    float lon = 0;              /* Longitude */
    int track = 0;              /* Angle of flight. */
    QDateTime timestamp = QDateTime::currentDateTimeUtc(); /* The date/time the message was last received */
    int squawk = 0;             /* squawk */
    int gndSpeed = 0;           /* Ground Speed */
    float planeTrack = 0;
    bool isOnGround = false;

    float distance = nanf("");  // Initializes distnace as a quiet NaN       // Have distance calculate on receipt of message (vice every paint)
    float bearing = nanf("");   // Initialize bearing as a quiet NaN

    bool callSignValid = false;
    bool altValid = false;
    bool latValid = false;
    bool lonValid = false;
    bool trackValid = false;
    bool squawkValid = false;
    bool gndSpeedValid = false;
    bool planeTrackValid = false;
    bool onGroundValid = false;
};

class ADSBReceiver : public WingletUI::AbstractSocketWorker
{
    Q_OBJECT
public:
    ADSBReceiver(QThread *ownerThread);

    bool connected() {return m_connected;}
    QMap<quint32, Aircraft> airspace();
    int totalAircraftSeen();

signals:
    void connectionStateChanged(bool connected);
    void newAircraftAdded();
    void aircraftCountChanged(int count);

public slots:
    void gpsUpdated(WingletUI::GPSReading reading);
    void pruneStaleAircraftCallback();

protected:
    void handleConnectionEvent(bool connected) override;
    void handleLine(const QString &line) override;
    float distanceEarth(double lat2d, double lon2d);
    float get_bearing(float lat, float lon);

private:
    void pruneOldestIfOverLimitUnderLock();
    bool m_connected = false;
    QMutex state_mutex;
    QMap<quint32, Aircraft> m_airspace;
    // Every distinct ICAO24 heard since app start — never pruned, so the
    // flight list can report a cumulative "seen since startup" total.
    QSet<quint32> m_seenIcaos;
    GPSReading currentGPS;

    static const int MAX_AIRSPACE_SIZE = 20;
    static const int STALE_TIMEOUT_SEC = 90;
    QTimer* pruneTimer = nullptr;
};

} // namespace WingletUI

#endif // WINGLETUI_ADSBRECEIVER_H
