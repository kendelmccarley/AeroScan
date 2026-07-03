#include "nasrdatabase.h"
#include <QFile>
#include <QTextStream>
#include <QtMath>
#include <algorithm>

namespace WingletUI {

const QString NASRDatabase::DATA_PATH    = "/var/lib/aeroscan/apt_freq.csv";
const QString NASRDatabase::EDITION_PATH = "/var/lib/aeroscan/nasr_edition.txt";

NASRDatabase::NASRDatabase(QObject *parent)
    : QObject(parent)
{
    load();
}

void NASRDatabase::reload()
{
    m_records.clear();
    m_edition.clear();
    m_loaded = false;
    load();
}

void NASRDatabase::load()
{
    QFile edFile(EDITION_PATH);
    if (edFile.open(QIODevice::ReadOnly)) {
        m_edition = QString(edFile.readAll()).trimmed();
        edFile.close();
    }

    QFile f(DATA_PATH);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&f);

    // Skip header line
    if (!in.atEnd())
        in.readLine();

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;

        // CSV columns: ident,lat,lon,freq_mhz,freq_type,name
        QStringList parts = line.split(',');
        if (parts.size() < 6)
            continue;

        AirportFreq rec;
        rec.ident    = parts[0].trimmed();
        rec.lat      = parts[1].toFloat();
        rec.lon      = parts[2].toFloat();
        rec.freqMhz  = parts[3].toFloat();
        rec.freqType = parts[4].trimmed();
        // Name may contain commas — rejoin remaining parts
        rec.name     = QStringList(parts.mid(5)).join(',').trimmed();

        if (rec.ident.isEmpty() || rec.freqMhz < 100.0f)
            continue;

        m_records.append(rec);
    }

    m_loaded = true;
}

QVector<NASRDatabase::AirportFreq> NASRDatabase::queryNearby(
        float lat, float lon, float maxNm, int maxResults) const
{
    struct Ranked { float dist; AirportFreq rec; };
    QVector<Ranked> ranked;
    ranked.reserve(256);

    for (const AirportFreq &r : m_records) {
        float d = distNm(lat, lon, r.lat, r.lon);
        if (d <= maxNm)
            ranked.append({d, r});
    }

    std::sort(ranked.begin(), ranked.end(),
              [](const Ranked &a, const Ranked &b){ return a.dist < b.dist; });

    QVector<AirportFreq> out;
    out.reserve(qMin(maxResults, ranked.size()));
    for (int i = 0; i < qMin(maxResults, ranked.size()); i++)
        out.append(ranked[i].rec);
    return out;
}

float NASRDatabase::distNm(float lat1, float lon1, float lat2, float lon2)
{
    constexpr float R_NM = 3440.065f;  // Earth radius in nautical miles
    float dLat = qDegreesToRadians(lat2 - lat1);
    float dLon = qDegreesToRadians(lon2 - lon1);
    float a = sinf(dLat / 2) * sinf(dLat / 2)
            + cosf(qDegreesToRadians(lat1)) * cosf(qDegreesToRadians(lat2))
            * sinf(dLon / 2) * sinf(dLon / 2);
    return R_NM * 2.0f * atan2f(sqrtf(a), sqrtf(1.0f - a));
}

} // namespace WingletUI
