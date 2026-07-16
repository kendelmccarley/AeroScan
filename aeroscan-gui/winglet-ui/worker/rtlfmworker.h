#ifndef WINGLETUI_RTLFMWORKER_H
#define WINGLETUI_RTLFMWORKER_H

#include <QObject>
#include <QProcess>
#include <QTimer>

namespace WingletUI {

// Manages an rtl_fm + aplay pipeline on an RTL-SDR dongle.
//
// Dongle sharing: with two dongles the tuner uses device 1 and dump1090 keeps
// device 0 (ADS-B stays live). With a single dongle it hands off — the tuner
// session stops dump1090 and takes device 0, and dump1090 is restarted when the
// session ends (the user leaves the tuner). See startSession()/stop().
//
// FM frequencies are stored as integer × 10  (1017 = 101.7 MHz).
// Airband and 2m frequencies are stored in kHz (121500 = 121.500 MHz).
class RtlFmWorker : public QObject
{
    Q_OBJECT
public:
    enum Mode { MODE_FM = 0, MODE_AIRBAND = 1, MODE_HAM2M = 2 };

    static const uint32_t FM_MIN_FREQ       = 879;     // 87.9 MHz
    static const uint32_t FM_MAX_FREQ       = 1079;    // 107.9 MHz
    static const uint32_t AIRBAND_MIN_FREQ  = 118000;  // 118.000 MHz
    static const uint32_t AIRBAND_MAX_FREQ  = 136975;  // 136.975 MHz
    static const uint32_t HAM2M_MIN_FREQ    = 144000;  // 144.000 MHz
    static const uint32_t HAM2M_MAX_FREQ    = 148000;  // 148.000 MHz
    static const uint FM_DECIMAL_COUNT      = 1;
    static const uint AIRBAND_DECIMAL_COUNT = 3;
    static const uint HAM2M_DECIMAL_COUNT   = 3;

    static const int SQUELCH_MAX = 200;  // rtl_fm -l is linear amplitude, not dB

    explicit RtlFmWorker(QObject *parent = nullptr);
    ~RtlFmWorker();

    bool     isAvailable() const { return m_available; }  // at least one dongle present
    bool     isPlaying()   const { return m_playing; }
    int      volume()      const { return m_volume; }
    Mode     mode()        const { return m_mode; }
    uint32_t freq()        const { return m_freq; }
    int      squelch()     const { return m_squelch; }
    int      deviceIndex() const { return desiredDeviceIndex(); }  // RTL-SDR index in use

    // Begin/end a tuner session. startSession() performs the single-dongle
    // handoff (stops dump1090 so device 0 is free); stop() ends the session and
    // restarts dump1090 if it was handed off. Between them, tune()/setSquelch()
    // just bounce the rtl_fm pipeline and never touch dump1090.
    void startSession();

    void tune(uint32_t freqScaled, Mode mode);
    void setSquelch(int level);  // rtl_fm -l; ignored in MODE_FM (always open)
    void setVolume(int pct);
    void stop();
    void restartAudio();  // Re-open aplay after the ALSA default device changed

signals:
    void availabilityChanged(bool available);

private slots:
    void pollAvailability();
    void rtlFmFinished(int exitCode, QProcess::ExitStatus status);
    void aplayFinished(int exitCode, QProcess::ExitStatus status);
    void restartTimerFired();
    void audioRetryFired();

private:
    void scheduleRestart();
    void scheduleAudioRetry();  // capped-backoff retry after an audio-open failure
    bool ensureAudioSink();     // start the persistent aplay + FIFO if not running
    void startPipeline();       // ensure the sink, then (re)start only rtl_fm
    void stopRtlFm();           // stop rtl_fm, leave the audio sink playing
    void stopAudioSink();       // stop aplay and tear down the FIFO
    void stopPipeline();        // full teardown: rtl_fm + audio sink
    int  countRtlSdrDevices();

    // Dongle sharing / dump1090 handoff
    int  desiredDeviceIndex() const;  // 1 if two dongles, 0 if one, -1 if none
    void reconcileDump1090();          // stop/start dump1090 to match session + count
    void stopDump1090();               // blocking — waits for device 0 to free
    void startDump1090();

    QProcess *m_rtlFm  = nullptr;
    QProcess *m_aplay  = nullptr;      // persistent — outlives retunes
    QTimer   *m_pollTimer = nullptr;

    // rtl_fm and aplay are decoupled through this FIFO so a retune can bounce
    // rtl_fm without closing aplay (and dropping the Bluetooth A2DP stream).
    // The GUI holds m_fifoFd open O_RDWR so aplay never sees EOF between rtl_fm
    // restarts. The suppress flags mark a deliberate kill so the finished slots
    // don't mistake it for a crash and trigger a retry.
    QString  m_fifoPath = QStringLiteral("/tmp/aeroscan-radio.pcm");
    int      m_fifoFd = -1;
    bool     m_suppressRtlFinished = false;
    bool     m_suppressAplayFinished = false;

    // rtl_fm has no runtime control channel — every retune/squelch change
    // restarts the pipeline. Coalesce restarts so held-down tuning keys
    // don't thrash the dongle.
    QTimer   *m_restartTimer = nullptr;

    // Recovers from rtl_fm dying because aplay couldn't open the ALSA output
    // (typically a disconnected Bluetooth route). Retries on a capped backoff
    // while the tuner is open instead of leaving the UI stuck at STARTING.
    QTimer   *m_audioRetryTimer = nullptr;
    int      m_retryCount    = 0;
    qint64   m_pipelineStartMs = 0;    // when the current pipeline started (ms)

    uint32_t m_freq      = 1017;       // 101.7 MHz FM default
    Mode     m_mode      = MODE_FM;
    int      m_volume    = 70;
    int      m_squelch   = 0;
    bool     m_available = false;      // at least one dongle present
    bool     m_playing   = false;
    int      m_dongleCount   = 0;
    int      m_activeDeviceIndex = -1; // device the running pipeline opened
    bool     m_sessionActive = false;  // tuner screen is open
    bool     m_handoffActive = false;  // dump1090 stopped for single-dongle use
};

} // namespace WingletUI
#endif // WINGLETUI_RTLFMWORKER_H
