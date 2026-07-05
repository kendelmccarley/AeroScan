#ifndef WINGLETUI_RADIOPRESETS_H
#define WINGLETUI_RADIOPRESETS_H

#include <QObject>
#include <QVector>
#include <QString>
#include "winglet-ui/worker/rtlfmworker.h"

namespace WingletUI {

// GUI-thread store of user radio presets (favorites), persisted to
// /var/lib/aeroscan/radio_presets.json. Seed defaults are imported once from
// the image at /usr/share/aeroscan-gui/frequencies.json when the writable
// store does not yet exist.
//
// Frequencies are stored uniformly in kHz (89100 = 89.1 MHz). The tuner uses
// per-band encodings (FM = int ×10, airband/2m = kHz); convert at the store
// boundary with uiFreqToKhz()/khzToUiFreq().
class RadioPresets : public QObject
{
    Q_OBJECT
public:
    struct Preset {
        int id;                  // runtime-assigned, stable while loaded
        RtlFmWorker::Mode band;
        QString name;
        uint32_t freqKhz;
    };

    static const QString STORE_PATH;  // /var/lib/aeroscan/radio_presets.json
    static const QString SEED_PATH;   // /usr/share/aeroscan-gui/frequencies.json

    explicit RadioPresets(QObject *parent = nullptr);

    QVector<Preset> presetsForBand(RtlFmWorker::Mode mode) const;
    int  add(RtlFmWorker::Mode mode, const QString &name, uint32_t freqKhz);
    void remove(int id);
    void rename(int id, const QString &name);

    // Convert between the tuner's per-band UI frequency encoding and stored kHz
    static uint32_t uiFreqToKhz(RtlFmWorker::Mode mode, uint32_t uiFreq);
    static uint32_t khzToUiFreq(RtlFmWorker::Mode mode, uint32_t khz);

signals:
    void presetsChanged();

private:
    void load();
    bool importFrom(const QString &path);
    void save();
    static QString bandToString(RtlFmWorker::Mode mode);
    static bool    bandFromString(const QString &s, RtlFmWorker::Mode *mode);

    QVector<Preset> m_presets;
    int  m_nextId = 1;
    bool m_loaded = false;
};

} // namespace WingletUI
#endif // WINGLETUI_RADIOPRESETS_H
