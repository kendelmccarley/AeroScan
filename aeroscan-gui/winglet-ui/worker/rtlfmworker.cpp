#include "rtlfmworker.h"
#include <QDir>
#include <QFile>

namespace WingletUI {

RtlFmWorker::RtlFmWorker(QObject *parent)
    : QObject(parent)
{
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(3000);
    connect(m_pollTimer, &QTimer::timeout, this, &RtlFmWorker::pollAvailability);
    m_pollTimer->start();

    // Check immediately so the menu item reflects state at startup
    pollAvailability();
}

RtlFmWorker::~RtlFmWorker()
{
    stopPipeline();
}

void RtlFmWorker::tune(uint32_t freqScaled, Mode mode)
{
    m_freq = freqScaled;
    m_mode = mode;
    if (m_available) {
        stopPipeline();
        startPipeline();
    }
}

void RtlFmWorker::setVolume(int pct)
{
    m_volume = qBound(0, pct, 100);
    // Best-effort: try common ALSA mixer control names
    QString volStr = QString::number(m_volume) + "%";
    QProcess::startDetached("sh", {"-c",
        QString("amixer sset Master %1 quiet 2>/dev/null || "
                "amixer sset PCM %1 quiet 2>/dev/null || "
                "amixer sset Headphone %1 quiet 2>/dev/null").arg(volStr)});
}

void RtlFmWorker::stop()
{
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
        if (!m_available)
            stopPipeline();
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
    } else {
        freqHz     = (uint32_t)m_freq * 1000u;
        modeArg    = "am";
        sampleRate = "200000";
    }

    // Two QProcess instances connected via stdout→stdin pipe.
    // aplay is started first so it is ready to receive data immediately.
    m_aplay = new QProcess(this);
    m_aplay->start("aplay", {"-r", "48000", "-f", "S16_LE", "-t", "raw", "-c", "1", "-"});

    m_rtlFm = new QProcess(this);
    m_rtlFm->setStandardOutputProcess(m_aplay);
    connect(m_rtlFm, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &RtlFmWorker::rtlFmFinished);
    m_rtlFm->start("rtl_fm", {
        "-d", "1",
        "-f", QString::number(freqHz),
        "-M", modeArg,
        "-s", sampleRate,
        "-r", "48000",
        "-g", "20",
        "-"
    });

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
