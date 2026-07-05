#include "rtlfmworker.h"
#include <QDir>
#include <QFile>

namespace WingletUI {

#define RESTART_DEBOUNCE_MS 300

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
    stopPipeline();
    startPipeline();
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

void RtlFmWorker::stop()
{
    m_restartTimer->stop();  // Cancel any pending debounced restart
    stopPipeline();
}

void RtlFmWorker::restartAudio()
{
    if (m_playing) {
        stopPipeline();
        startPipeline();
    }
}

void RtlFmWorker::pollAvailability()
{
    bool nowAvailable = (countRtlSdrDevices() >= 2);
    if (nowAvailable != m_available) {
        m_available = nowAvailable;
        if (!m_available) {
            m_restartTimer->stop();
            stopPipeline();
        }
        emit availabilityChanged(m_available);
    }
}

void RtlFmWorker::rtlFmFinished(int exitCode, QProcess::ExitStatus status)
{
    (void) exitCode;
    (void) status;
    // rtl_fm exited unexpectedly — clean up aplay too
    stopPipeline();
}

void RtlFmWorker::startPipeline()
{
    if (m_playing || !m_available)
        return;

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

    // Two QProcess instances connected via stdout→stdin pipe.
    // aplay is started first so it is ready to receive data immediately.
    m_aplay = new QProcess(this);
    m_aplay->start("aplay", {"-r", "48000", "-f", "S16_LE", "-t", "raw", "-c", "1", "-"});

    QStringList rtlFmArgs = {
        "-d", "1",
        "-f", QString::number(freqHz),
        "-M", modeArg,
        "-s", sampleRate,
        "-r", "48000",
        "-g", "20",
    };
    if (m_mode != MODE_FM && m_squelch > 0)
        rtlFmArgs << "-l" << QString::number(m_squelch);
    rtlFmArgs << "-";

    m_rtlFm = new QProcess(this);
    m_rtlFm->setStandardOutputProcess(m_aplay);
    connect(m_rtlFm, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &RtlFmWorker::rtlFmFinished);
    m_rtlFm->start("rtl_fm", rtlFmArgs);

    m_playing = true;
    setVolume(m_volume);
}

void RtlFmWorker::stopPipeline()
{
    if (!m_playing)
        return;

    m_playing = false;

    if (m_rtlFm) {
        m_rtlFm->kill();
        m_rtlFm->waitForFinished(500);
        delete m_rtlFm;
        m_rtlFm = nullptr;
    }
    // aplay exits on EOF from rtl_fm's closed stdout; give it a moment
    if (m_aplay) {
        if (!m_aplay->waitForFinished(500))
            m_aplay->kill();
        delete m_aplay;
        m_aplay = nullptr;
    }
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
