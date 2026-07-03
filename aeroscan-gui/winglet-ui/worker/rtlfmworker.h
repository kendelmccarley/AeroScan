#ifndef WINGLETUI_RTLFMWORKER_H
#define WINGLETUI_RTLFMWORKER_H

#include <QObject>
#include <QProcess>
#include <QTimer>

namespace WingletUI {

// Manages an rtl_fm + aplay pipeline using RTL-SDR device index 1 (the
// second dongle — device 0 is dedicated to ADS-B/dump1090).
//
// FM frequencies are stored as integer × 10  (1017 = 101.7 MHz).
// Airband frequencies are stored as integer × 1000 (121500 = 121.500 MHz).
class RtlFmWorker : public QObject
{
    Q_OBJECT
public:
    enum Mode { MODE_FM = 0, MODE_AIRBAND = 1 };

    static const uint32_t FM_MIN_FREQ       = 879;     // 87.9 MHz
    static const uint32_t FM_MAX_FREQ       = 1079;    // 107.9 MHz
    static const uint32_t AIRBAND_MIN_FREQ  = 118000;  // 118.000 MHz
    static const uint32_t AIRBAND_MAX_FREQ  = 136975;  // 136.975 MHz
    static const uint FM_DECIMAL_COUNT      = 1;
    static const uint AIRBAND_DECIMAL_COUNT = 3;

    explicit RtlFmWorker(QObject *parent = nullptr);
    ~RtlFmWorker();

    bool     isAvailable() const { return m_available; }
    int      volume()      const { return m_volume; }
    Mode     mode()        const { return m_mode; }
    uint32_t freq()        const { return m_freq; }

    void tune(uint32_t freqScaled, Mode mode);
    void setVolume(int pct);
    void stop();
    void restartAudio();  // Re-open aplay after the ALSA default device changed

signals:
    void availabilityChanged(bool available);

private slots:
    void pollAvailability();
    void rtlFmFinished(int exitCode, QProcess::ExitStatus status);

private:
    void startPipeline();
    void stopPipeline();
    int  countRtlSdrDevices();

    QProcess *m_rtlFm  = nullptr;
    QProcess *m_aplay  = nullptr;
    QTimer   *m_pollTimer = nullptr;

    uint32_t m_freq      = 1017;       // 101.7 MHz FM default
    Mode     m_mode      = MODE_FM;
    int      m_volume    = 70;
    bool     m_available = false;
    bool     m_playing   = false;
};

} // namespace WingletUI
#endif // WINGLETUI_RTLFMWORKER_H
