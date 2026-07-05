#include "radiopresets.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace WingletUI {

const QString RadioPresets::STORE_PATH = "/var/lib/aeroscan/radio_presets.json";
const QString RadioPresets::SEED_PATH  = "/usr/share/aeroscan-gui/frequencies.json";

RadioPresets::RadioPresets(QObject *parent)
    : QObject(parent)
{
    load();
}

QString RadioPresets::bandToString(RtlFmWorker::Mode mode)
{
    switch (mode) {
    case RtlFmWorker::MODE_FM:      return "fm";
    case RtlFmWorker::MODE_AIRBAND: return "airband";
    case RtlFmWorker::MODE_HAM2M:   return "ham2m";
    }
    return "fm";
}

bool RadioPresets::bandFromString(const QString &s, RtlFmWorker::Mode *mode)
{
    if (s == "fm")           { *mode = RtlFmWorker::MODE_FM;      return true; }
    else if (s == "airband") { *mode = RtlFmWorker::MODE_AIRBAND; return true; }
    else if (s == "ham2m")   { *mode = RtlFmWorker::MODE_HAM2M;   return true; }
    return false;
}

uint32_t RadioPresets::uiFreqToKhz(RtlFmWorker::Mode mode, uint32_t uiFreq)
{
    // FM UI encoding is int ×10 (1017 = 101.7 MHz = 101700 kHz); airband/2m
    // are already kHz.
    return (mode == RtlFmWorker::MODE_FM) ? uiFreq * 100u : uiFreq;
}

uint32_t RadioPresets::khzToUiFreq(RtlFmWorker::Mode mode, uint32_t khz)
{
    return (mode == RtlFmWorker::MODE_FM) ? khz / 100u : khz;
}

void RadioPresets::load()
{
    if (m_loaded)
        return;
    m_loaded = true;

    if (QFile::exists(STORE_PATH)) {
        importFrom(STORE_PATH);
    } else if (importFrom(SEED_PATH)) {
        // First run: persist the seed set so future edits have a writable copy
        save();
    }
}

bool RadioPresets::importFrom(const QString &path)
{
    QFile inFile(path);
    if (!inFile.open(QFile::ReadOnly))
        return false;

    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(inFile.readAll(), &error);
    inFile.close();
    if (doc.isNull() || !doc.isObject()) {
        qWarning("Failed to decode radio presets '%s': %s",
                 qUtf8Printable(path), qUtf8Printable(error.errorString()));
        return false;
    }

    const QJsonArray presets = doc.object().value("presets").toArray();
    for (const QJsonValue &val : presets) {
        const QJsonObject obj = val.toObject();
        RtlFmWorker::Mode mode;
        if (!bandFromString(obj.value("band").toString(), &mode))
            continue;
        uint32_t khz = (uint32_t) obj.value("freq_khz").toDouble();
        if (khz == 0)
            continue;
        Preset p;
        p.id      = m_nextId++;
        p.band    = mode;
        p.name    = obj.value("name").toString();
        p.freqKhz = khz;
        m_presets.append(p);
    }
    return true;
}

void RadioPresets::save()
{
    QJsonArray presets;
    for (const Preset &p : m_presets) {
        QJsonObject obj;
        obj.insert("band", bandToString(p.band));
        obj.insert("name", p.name);
        obj.insert("freq_khz", (qint64) p.freqKhz);
        presets.append(obj);
    }

    QJsonObject root;
    root.insert("version", 1);
    root.insert("presets", presets);

    // Store lives on the writable /var; create the directory on first save
    QDir().mkpath(QFileInfo(STORE_PATH).absolutePath());

    QFile outFile(STORE_PATH);
    if (outFile.open(QFile::WriteOnly | QFile::Truncate)) {
        outFile.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
        outFile.close();
    } else {
        qWarning("Failed to save radio presets to '%s'", qUtf8Printable(STORE_PATH));
    }
}

QVector<RadioPresets::Preset> RadioPresets::presetsForBand(RtlFmWorker::Mode mode) const
{
    QVector<Preset> out;
    for (const Preset &p : m_presets) {
        if (p.band == mode)
            out.append(p);
    }
    return out;
}

int RadioPresets::add(RtlFmWorker::Mode mode, const QString &name, uint32_t freqKhz)
{
    Preset p;
    p.id      = m_nextId++;
    p.band    = mode;
    p.name    = name;
    p.freqKhz = freqKhz;
    m_presets.append(p);
    save();
    emit presetsChanged();
    return p.id;
}

void RadioPresets::remove(int id)
{
    for (int i = 0; i < m_presets.size(); i++) {
        if (m_presets[i].id == id) {
            m_presets.removeAt(i);
            save();
            emit presetsChanged();
            return;
        }
    }
}

void RadioPresets::rename(int id, const QString &name)
{
    for (Preset &p : m_presets) {
        if (p.id == id) {
            p.name = name;
            save();
            emit presetsChanged();
            return;
        }
    }
}

} // namespace WingletUI
