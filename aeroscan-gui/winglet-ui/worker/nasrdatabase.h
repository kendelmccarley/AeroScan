#ifndef WINGLETUI_NASRDATABASE_H
#define WINGLETUI_NASRDATABASE_H

#include <QObject>
#include <QVector>
#include <QString>

namespace WingletUI {

class NASRDatabase : public QObject
{
    Q_OBJECT
public:
    struct AirportFreq {
        QString ident;      // KJFK
        QString name;       // JFK INTL
        float   lat;
        float   lon;
        float   freqMhz;    // 119.100
        QString freqType;   // TWR, CTAF, GND, ATIS, etc.
    };

    static const QString DATA_PATH;     // /var/lib/aeroscan/apt_freq.csv
    static const QString EDITION_PATH;  // /var/lib/aeroscan/nasr_edition.txt

    explicit NASRDatabase(QObject *parent = nullptr);

    bool isLoaded() const { return m_loaded; }
    QString edition() const { return m_edition; }

    // Returns airports within maxNm nautical miles of (lat, lon), sorted by
    // distance, capped at maxResults. Loads the CSV on first call.
    QVector<AirportFreq> queryNearby(float lat, float lon,
                                     float maxNm = 20.0f,
                                     int   maxResults = 30) const;

    // Force a reload from disk (call after nasr-update.py completes)
    void reload();

private:
    void load();
    static float distNm(float lat1, float lon1, float lat2, float lon2);

    QVector<AirportFreq> m_records;
    QString m_edition;
    bool m_loaded = false;
};

} // namespace WingletUI
#endif // WINGLETUI_NASRDATABASE_H
