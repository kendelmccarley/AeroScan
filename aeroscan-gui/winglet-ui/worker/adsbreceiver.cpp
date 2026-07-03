#include "adsbreceiver.h"
#include "wingletgui.h"
#include <cmath>
#include <QTimer>

#define earthRadiusKm 6371.0

namespace WingletUI {

ADSBReceiver::ADSBReceiver(QThread *ownerThread):
    AbstractSocketWorker(ownerThread)
{
    currentGPS = GPSReading(WingletGUI::inst->settings.lastLatitude(),
                            WingletGUI::inst->settings.lastLongitude(), false);
    start("localhost", 30003);

    pruneTimer = new QTimer(this);
    pruneTimer->setInterval(30000);
    connect(pruneTimer, SIGNAL(timeout()), this, SLOT(pruneStaleAircraftCallback()));
    pruneTimer->start();
}

void ADSBReceiver::handleConnectionEvent(bool connected)
{
    if (!connected) {
        // GPS Disconnected

        state_mutex.lock();
        m_airspace.clear();
        state_mutex.unlock();
        emit aircraftCountChanged(0);
    }

    m_connected = connected;
    emit connectionStateChanged(connected);
}

QMap<quint32, Aircraft> ADSBReceiver::airspace()
{
    state_mutex.lock();
    QMap<quint32, Aircraft> airspaceCopy = m_airspace;
    state_mutex.unlock();

    return airspaceCopy;
}

void ADSBReceiver::handleLine(const QString &line) {
    QStringList query = line.trimmed().split(',');

    if (query.count() != 22)
        return;
    if (query[0] != "MSG")
        return;

    bool okay;
    quint32 icao24 = query[4].toUInt(&okay, 16);
    if (!okay)
        return;

    state_mutex.lock();

    auto entry = m_airspace.find(icao24);
    bool isNew = (entry == m_airspace.end());
    if (isNew)
        entry = m_airspace.insert(icao24, {});
    entry->icao24 = icao24;

    uint messageType = query[1].toUInt(&okay);
    if (!okay)
        goto unlock;

    entry->messageType = messageType;

    entry->timestamp = QDateTime::currentDateTimeUtc();

    if (messageType == 1) {
        entry->callSign = query[10];
        entry->callSignValid = true;
    }
    if (messageType == 2 || messageType == 3 || messageType == 5 || messageType == 6 || messageType == 7) {
        int alt = query[11].toInt(&okay);
        if (okay) {
            entry->alt = alt;
            entry->altValid = true;
        }
    }
    if (messageType == 2 || messageType == 4) {
        int gndSpeed = query[12].toInt(&okay);
        bool speedOk = okay;
        float planeTrack = query[13].toFloat(&okay);
        if (speedOk && okay) {
            entry->gndSpeed = gndSpeed;
            entry->planeTrack = planeTrack;
            entry->gndSpeedValid = true;
            entry->planeTrackValid = true;
        }
    }
    if (messageType == 2 || messageType == 3) {
        float lat = query[14].toFloat(&okay);
        bool latOk = okay;
        float lon = query[15].toFloat(&okay);
        if (latOk && okay) {
            entry->lat = lat;
            entry->lon = lon;
            entry->lonValid = true;
            entry->latValid = true;
            entry->distance = distanceEarth(lat, lon);
            entry->bearing = get_bearing(lat, lon);
        }
    }
    if (messageType == 6) {
        int squawk = query[17].toInt(&okay);
        if (okay) {
            entry->squawk = squawk;
            entry->squawkValid = true;
        }
    }
    if (messageType == 2 || messageType == 3 || messageType == 5 || messageType == 6 || messageType == 7 || messageType == 8) {
        if (query[21] == "1") {
            entry->isOnGround = true;
        } else if (query[21] == "0") {
            entry->isOnGround = false;
        }
        entry->onGroundValid = true;
    }


unlock:
    if (isNew)
        pruneOldestIfOverLimitUnderLock();
    state_mutex.unlock();

    if (isNew) {
        emit newAircraftAdded();
        emit aircraftCountChanged(m_airspace.size());
    }
}

void ADSBReceiver::gpsUpdated(WingletUI::GPSReading reading)
{
    if (reading.valid)
        currentGPS = reading;
}

void ADSBReceiver::pruneStaleAircraftCallback()
{
    QDateTime now = QDateTime::currentDateTimeUtc();
    state_mutex.lock();
    auto it = m_airspace.begin();
    while (it != m_airspace.end()) {
        if (it.value().timestamp.secsTo(now) > STALE_TIMEOUT_SEC)
            it = m_airspace.erase(it);
        else
            ++it;
    }
    int count = m_airspace.size();
    state_mutex.unlock();
    emit aircraftCountChanged(count);
}

void ADSBReceiver::pruneOldestIfOverLimitUnderLock()
{
    if (m_airspace.size() <= MAX_AIRSPACE_SIZE)
        return;

    auto oldest = m_airspace.begin();
    for (auto it = m_airspace.begin(); it != m_airspace.end(); ++it) {
        if (it.value().timestamp < oldest.value().timestamp)
            oldest = it;
    }
    m_airspace.erase(oldest);
}

float ADSBReceiver::distanceEarth(double lat2d, double lon2d) {
  double lat1r, lon1r, lat2r, lon2r, u, v;
  lat1r = currentGPS.latitude * M_PI / 180;
  lon1r = currentGPS.longitude * M_PI / 180;
  lat2r = lat2d * M_PI / 180;
  lon2r = lon2d * M_PI / 180;
  u = sin((lat2r - lat1r)/2);
  v = sin((lon2r - lon1r)/2);
  return 2.0 * earthRadiusKm * asin(sqrt(u * u + cos(lat1r) * cos(lat2r) * v * v)) * 0.539957;
}

float ADSBReceiver::get_bearing(float lat, float lon){
    /// center location scope
    float startLat = currentGPS.latitude * (M_PI/180);
    float startLon = currentGPS.longitude * (M_PI/180);

    /// location of plane
    float endLat = lat * (M_PI/180);
    float endLon = lon * (M_PI/180);

    float dLong = endLon - startLon;

    float dPhi = log(tan(endLat/2.0 + M_PI/4.0)/tan(startLat/2.0 + M_PI/4.0));
    if(abs(dLong) > M_PI){
        if (dLong > 0.0){
            dLong = -(2.0 * M_PI - dLong);
        } else {
            dLong = (2.0 * M_PI + dLong);
        }
    }
    float bearing = fmod(((atan2(dLong, dPhi)*(180/M_PI)) + 360.0), 360.0);

    return bearing;
}

} // namespace WingletUI
