#include "rtlfmworker.h"
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace WingletUI {

#define RESTART_DEBOUNCE_MS 300

// When rtl_fm dies right after starting, it is almost always because aplay
// couldn't open the ALSA output (e.g. the route is Bluetooth and the headset
// isn't connected yet). Retry on a capped backoff while the tuner is open so a
// later headset connect / output switch recovers on its own.
#define AUDIO_RETRY_BASE_MS   1000
#define AUDIO_RETRY_MAX_MS    5000
#define PIPELINE_STABLE_MS    4000  // ran at least this long => a fresh failure

RtlFmWorker::RtlFmWorker(QObject *parent)
    : QObject(parent)
{
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(3000);
    connect(m_pollTimer, &QTimer::timeout, this, &RtlFmWorker::pollAvailability);
    m_pollTimer->start();

    m_restartTimer = new QTimer(this);
    m_restartTimer->setInterval(RESTART_DEBOUNCE_MS);
    m_restartTimer->setSingleShot(true);
    connect(m_restartTimer, &QTimer::timeout, this, &RtlFmWorker::restartTimerFired);

    m_audioRetryTimer = new QTimer(this);
    m_audioRetryTimer->setSingleShot(true);
    connect(m_audioRetryTimer, &QTimer::timeout, this, &RtlFmWorker::audioRetryFired);

    // Check immediately so the menu item reflects state at startup
    pollAvailability();
}

RtlFmWorker::~RtlFmWorker()
{
    stopPipeline();
}

void RtlFmWorker::tune(uint32_t freqScaled, Mode mode)
{
    if (m_freq == freqScaled && m_mode == mode && m_playing)
        return;
    m_freq = freqScaled;
    m_mode = mode;
    scheduleRestart();
}

void RtlFmWorker::setSquelch(int level)
{
    level = qBound(0, level, SQUELCH_MAX);
    if (m_squelch == level)
        return;
    m_squelch = level;
    // FM broadcast runs squelch-open; no need to bounce the pipeline
    if (m_mode != MODE_FM)
        scheduleRestart();
}

void RtlFmWorker::scheduleRestart()
{
    if (m_available)
        m_restartTimer->start();  // Restarts the window while keys repeat
}

void RtlFmWorker::restartTimerFired()
{
    if (!m_available)
        return;
    // A deliberate (re)start — retune, squelch, hot-plug — resets the audio
    // backoff so a fresh attempt starts from the short delay. startPipeline()
    // keeps the persistent aplay (and any Bluetooth A2DP transport) alive and
    // bounces only rtl_fm, so retuning no longer drops the audio stream.
    m_audioRetryTimer->stop();
    m_retryCount = 0;
    startPipeline();
}

void RtlFmWorker::scheduleAudioRetry()
{
    int delay = qMin(AUDIO_RETRY_BASE_MS * (m_retryCount + 1), AUDIO_RETRY_MAX_MS);
    m_retryCount++;
    m_audioRetryTimer->start(delay);
}

void RtlFmWorker::audioRetryFired()
{
    if (!m_sessionActive || !m_available)
        return;
    startPipeline();  // a repeat failure reschedules via rtlFmFinished()
}

void RtlFmWorker::setVolume(int pct)
{
    m_volume = qBound(0, pct, 100);
    // The ALSA "default" device wraps every output route in a softvol plugin
    // whose control is named "Radio" (see WingletGUI::writeAudioOutputConf), so
    // one command adjusts volume uniformly on the jack, HDMI, and Bluetooth.
    // Fall back to the raw card controls if softvol isn't present.
    QString volStr = QString::number(m_volume) + "%";
    // The softvol "Radio" control is registered on the always-present
    // Headphones card, so target it there explicitly — for the Bluetooth route
    // the default ctl is bluealsa and wouldn't expose it. Fall back to the raw
    // card controls if softvol isn't loaded.
    QProcess::startDetached("sh", {"-c",
        QString("amixer -c Headphones sset Radio %1 quiet 2>/dev/null || "
                "amixer sset Radio %1 quiet 2>/dev/null || "
                "amixer sset Master %1 quiet 2>/dev/null || "
                "amixer sset PCM %1 quiet 2>/dev/null || "
                "amixer sset Headphone %1 quiet 2>/dev/null").arg(volStr)});
}

void RtlFmWorker::startSession()
{
    m_sessionActive = true;
    // With a single dongle this stops dump1090 so device 0 is free before the
    // pipeline starts; with two dongles it is a no-op (tuner uses device 1).
    reconcileDump1090();
}

void RtlFmWorker::stop()
{
    m_restartTimer->stop();    // Cancel any pending debounced restart
    m_audioRetryTimer->stop();   // and any pending audio retry
    m_retryCount = 0;
    stopPipeline();
    m_sessionActive = false;
    reconcileDump1090();       // hand device 0 back to dump1090 if we borrowed it
}

int RtlFmWorker::desiredDeviceIndex() const
{
    if (m_dongleCount >= 2) return 1;  // leave device 0 to dump1090/ADS-B
    if (m_dongleCount == 1) return 0;  // single-dongle handoff
    return -1;                         // no dongle
}

void RtlFmWorker::reconcileDump1090()
{
    // dump1090 must be out of the way only while the tuner is open on device 0
    // (single-dongle handoff). With two dongles it keeps running on device 0.
    bool needHandoff = m_sessionActive && (m_dongleCount == 1);
    if (needHandoff && !m_handoffActive) {
        stopDump1090();
        m_handoffActive = true;
    }
    else if (!needHandoff && m_handoffActive) {
        startDump1090();
        m_handoffActive = false;
    }
}

void RtlFmWorker::stopDump1090()
{
    // Block until the unit is inactive so device 0 is released before rtl_fm
    // opens it. dump1090 handles SIGTERM promptly; cap the wait defensively.
    QProcess proc;
    proc.start("systemctl", {"stop", "dump1090.service"});
    proc.waitForFinished(5000);
}

void RtlFmWorker::startDump1090()
{
    // Fire and forget — ADSBReceiver reconnects to dump1090's SBS port once it
    // is back up.
    QProcess::startDetached("systemctl", {"start", "dump1090.service"});
}

void RtlFmWorker::restartAudio()
{
    // Called when the ALSA default device changed (output switch) or a
    // Bluetooth headset (re)connected. Re-open the stream so it picks up the
    // new route — and recover a session that had backed off retrying while
    // audio was unavailable (m_playing false but the tuner still open).
    if (!m_sessionActive || !m_available)
        return;
    m_audioRetryTimer->stop();
    m_retryCount = 0;
    stopPipeline();
    startPipeline();
}

void RtlFmWorker::pollAvailability()
{
    int prevCount = m_dongleCount;
    m_dongleCount = countRtlSdrDevices();

    bool nowAvailable = (m_dongleCount >= 1);
    if (nowAvailable != m_available) {
        m_available = nowAvailable;
        if (!m_available) {
            m_restartTimer->stop();
            stopPipeline();
        }
        emit availabilityChanged(m_available);
    }

    // A hot-plug during a session can change which device the tuner should use
    // (2->1 hands off to device 0 and stops dump1090; 1->2 releases device 0
    // back to ADS-B). Reconcile dump1090 and re-open the pipeline if the index
    // moved or the pipeline died with the removed dongle.
    if (m_sessionActive && m_dongleCount != prevCount) {
        reconcileDump1090();
        if (m_available && (!m_playing || m_activeDeviceIndex != desiredDeviceIndex()))
            scheduleRestart();
    }
}

void RtlFmWorker::rtlFmFinished(int exitCode, QProcess::ExitStatus status)
{
    (void) exitCode;
    (void) status;

    if (m_suppressRtlFinished)
        return;  // we killed it deliberately (retune / stop) — not a crash

    // If rtl_fm held up for a while before dying, this is a fresh failure
    // (dongle unplugged) rather than a start that never took — restart the
    // backoff from the short delay.
    qint64 ranMs = m_pipelineStartMs
        ? QDateTime::currentMSecsSinceEpoch() - m_pipelineStartMs : 0;
    if (ranMs >= PIPELINE_STABLE_MS)
        m_retryCount = 0;

    // rtl_fm died on its own. Drop it but KEEP the audio sink (and any Bluetooth
    // A2DP transport) up, then retry just rtl_fm on a capped backoff while the
    // tuner is open — so a transient SDR hiccup doesn't tear down the headset
    // stream or stick the UI at STARTING.
    if (m_rtlFm) {
        m_rtlFm->deleteLater();
        m_rtlFm = nullptr;
    }
    m_playing = false;

    if (m_sessionActive && m_available)
        scheduleAudioRetry();
}

void RtlFmWorker::aplayFinished(int exitCode, QProcess::ExitStatus status)
{
    (void) exitCode;
    (void) status;

    if (m_suppressAplayFinished)
        return;  // deliberate teardown (stop / output switch)

    // The audio sink died on its own — typically the Bluetooth transport
    // dropped. Tear the whole pipeline down (rtl_fm has nowhere to play) and
    // retry from scratch; the retry rebuilds the sink once audio is available.
    if (m_aplay) {
        m_aplay->deleteLater();
        m_aplay = nullptr;
    }
    if (m_fifoFd >= 0) {
        ::close(m_fifoFd);
        m_fifoFd = -1;
    }
    stopRtlFm();
    m_playing = false;

    if (m_sessionActive && m_available)
        scheduleAudioRetry();
}

bool RtlFmWorker::ensureAudioSink()
{
    // A long-lived aplay reading from a FIFO. rtl_fm writes into the same FIFO;
    // because retunes bounce only rtl_fm, aplay never closes and the ALSA/A2DP
    // output (Bluetooth transport included) stays acquired across frequency
    // changes. The GUI holds the FIFO open O_RDWR so aplay never sees EOF when
    // rtl_fm exits (deliberately or by crashing); the kernel FIFO buffer keeps
    // rtl_fm paced to the sink exactly like the old direct pipe did.
    if (m_aplay && m_aplay->state() != QProcess::NotRunning)
        return true;

    const QByteArray path = m_fifoPath.toLocal8Bit();
    ::unlink(path.constData());
    if (::mkfifo(path.constData(), 0600) != 0) {
        qWarning("RtlFmWorker: mkfifo(%s) failed", path.constData());
        return false;
    }
    if (m_fifoFd >= 0)
        ::close(m_fifoFd);
    // O_RDWR never blocks on a FIFO and holds both a reader and a writer
    // reference; we never read/write it ourselves. O_CLOEXEC so the aplay/rtl_fm
    // children don't inherit this extra descriptor.
    m_fifoFd = ::open(path.constData(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (m_fifoFd < 0) {
        qWarning("RtlFmWorker: open(%s) failed", path.constData());
        ::unlink(path.constData());
        return false;
    }

    m_aplay = new QProcess(this);
    m_aplay->setStandardInputFile(m_fifoPath);  // aplay reads the FIFO
    connect(m_aplay, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &RtlFmWorker::aplayFinished);
    m_aplay->start("aplay", {"-r", "48000", "-f", "S16_LE", "-t", "raw", "-c", "1", "-"});
    return true;
}

void RtlFmWorker::stopRtlFm()
{
    if (!m_rtlFm)
        return;
    m_suppressRtlFinished = true;
    m_rtlFm->kill();
    m_rtlFm->waitForFinished(500);
    delete m_rtlFm;
    m_rtlFm = nullptr;
    m_suppressRtlFinished = false;
}

void RtlFmWorker::stopAudioSink()
{
    if (m_aplay) {
        m_suppressAplayFinished = true;
        m_aplay->kill();
        m_aplay->waitForFinished(500);
        delete m_aplay;
        m_aplay = nullptr;
        m_suppressAplayFinished = false;
    }
    if (m_fifoFd >= 0) {
        ::close(m_fifoFd);
        m_fifoFd = -1;
    }
    ::unlink(m_fifoPath.toLocal8Bit().constData());
}

void RtlFmWorker::startPipeline()
{
    if (!m_available)
        return;

    int deviceIdx = desiredDeviceIndex();
    if (deviceIdx < 0)
        return;  // no dongle
    // Single-dongle handoff must have stopped dump1090 before we open device 0.
    if (deviceIdx == 0 && !m_handoffActive)
        return;
    m_activeDeviceIndex = deviceIdx;

    // Bring up (or reuse) the persistent audio sink first. If it can't open
    // (e.g. the Bluetooth route with the headset not yet connected), back off
    // and retry rather than spin up rtl_fm with nowhere to play.
    if (!ensureAudioSink()) {
        scheduleAudioRetry();
        return;
    }

    // Restart only rtl_fm — aplay (and its output stream) stays up.
    stopRtlFm();

    uint32_t freqHz;
    QString  modeArg;
    QString  sampleRate;

    if (m_mode == MODE_FM) {
        freqHz     = (uint32_t)m_freq * 100000u;
        modeArg    = "wbfm";
        sampleRate = "170000";
    } else if (m_mode == MODE_AIRBAND) {
        freqHz     = (uint32_t)m_freq * 1000u;
        modeArg    = "am";
        sampleRate = "200000";
    } else {  // MODE_HAM2M — narrowband FM ("fm" = NBFM in rtl_fm terms)
        freqHz     = (uint32_t)m_freq * 1000u;
        modeArg    = "fm";
        sampleRate = "12000";
    }

    QStringList rtlFmArgs = {
        "-d", QString::number(deviceIdx),
        "-f", QString::number(freqHz),
        "-M", modeArg,
        "-s", sampleRate,
        "-r", "48000",
        "-g", "20",
    };
    if (m_mode != MODE_FM && m_squelch > 0)
        rtlFmArgs << "-l" << QString::number(m_squelch);
    rtlFmArgs << "-";

    // rtl_fm writes its raw audio into the FIFO that the persistent aplay reads.
    // A reader reference always exists (aplay plus the GUI's O_RDWR fd), so the
    // O_WRONLY open never blocks; when this rtl_fm later exits, aplay keeps
    // playing because the GUI's fd holds the FIFO open.
    m_rtlFm = new QProcess(this);
    m_rtlFm->setStandardOutputFile(m_fifoPath);
    connect(m_rtlFm, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &RtlFmWorker::rtlFmFinished);
    m_rtlFm->start("rtl_fm", rtlFmArgs);

    m_playing = true;
    m_pipelineStartMs = QDateTime::currentMSecsSinceEpoch();
    setVolume(m_volume);
}

void RtlFmWorker::stopPipeline()
{
    // Full teardown: both rtl_fm and the persistent audio sink. Used for session
    // end, output-route switch, and dongle loss — not for retunes.
    stopRtlFm();
    stopAudioSink();
    m_playing = false;
}

int RtlFmWorker::countRtlSdrDevices()
{
    int count = 0;
    QDir usbDir("/sys/bus/usb/devices");
    const QStringList entries = usbDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &entry : entries) {
        const QString base = "/sys/bus/usb/devices/" + entry + "/";
        QFile vidFile(base + "idVendor");
        if (!vidFile.open(QIODevice::ReadOnly))
            continue;
        if (vidFile.readAll().trimmed() != "0bda")  // Realtek USB VID
            continue;
        QFile pidFile(base + "idProduct");
        if (!pidFile.open(QIODevice::ReadOnly))
            continue;
        const QString pid = pidFile.readAll().trimmed();
        if (pid == "2832" || pid == "2838" || pid == "2838b")
            count++;
    }
    return count;
}

} // namespace WingletUI
