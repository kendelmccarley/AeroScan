#include "touchbuttonoverlay.h"
#include "wingletgui.h"
#include <QPainter>
#include <QPolygon>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStorageInfo>
#include <QMouseEvent>
#include <QTimer>
#include <QTransform>

namespace WingletUI {

// ── Left overlay constants ────────────────────────────────────────────────────
static const int L_ZONES       = 4;
static const int L_ZONE_HEIGHT = 120;
static const char* L_LABELS[L_ZONES] = { "UP", "DOWN", "SEL", "BACK" };

// ── Right overlay constants ───────────────────────────────────────────────────
static const int R_SEGS        = 8;
static const int R_SEG_HEIGHT  = 60;
static const char* R_LABELS[2] = { "GPS", "ADSB" };

// ── Shared palette (theme.cpp dark-mode values) ───────────────────────────────
static const QColor C_BG      (0x11, 0x11, 0x11);  // Window
static const QColor C_AMBER   (0xff, 0xac, 0x11);  // Text / hero accent
static const QColor C_GREEN   (0x22, 0xcc, 0x44);  // Status OK / lock
static const QColor C_MUTED   (0x99, 0x99, 0x99);  // WindowText / labels
static const QColor C_DIVIDER (0x36, 0x45, 0x4f);  // Base / charcoal-slate
static const QColor C_PRESS   (0x1e, 0x2d, 0x35);  // press highlight

// ─────────────────────────────────────────────────────────────────────────────

TouchButtonOverlay::TouchButtonOverlay(Side side, QWidget *parent)
    : QWidget{parent}, m_side(side)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    if (m_side == Right) {
        // 1 Hz repaint for the seconds display; stats polled every 2nd tick
        // (2 s cadence — a handful of small /run and /proc reads).
        QTimer *clock = new QTimer(this);
        clock->setInterval(1000);
        connect(clock, &QTimer::timeout, this, [this]() {
            if ((m_statTick++ % 2) == 0)
                pollStats();
            update();
        });
        clock->start();
        pollStats();
    }
    if (m_side == Left) {
        m_pressTimer = new QTimer(this);
        m_pressTimer->setSingleShot(true);
        m_pressTimer->setInterval(120);
        connect(m_pressTimer, &QTimer::timeout, this, &TouchButtonOverlay::clearPress);
    }
}

// ── Mouse handling (left overlay only) ───────────────────────────────────────

void TouchButtonOverlay::mousePressEvent(QMouseEvent *event)
{
    if (m_side == Left && event->button() == Qt::LeftButton) {
        qDebug("TouchOverlay: pos x=%d y=%d  widget w=%d h=%d",
               event->pos().x(), event->pos().y(), width(), height());
        // Touch zones occupy the bottom half; the upper half is the logo.
        const int half = height() / 2;
        if (event->pos().y() >= half) {
            int zone = qBound(0, (event->pos().y() - half) * L_ZONES / half,
                              L_ZONES - 1);
            showPress(zone);
            emit zonePressed(zone);
        }
    }
    event->accept();  // don't propagate; overlay never steals focus
}

// ── Left slots ────────────────────────────────────────────────────────────────

void TouchButtonOverlay::showPress(int zone)
{
    if (m_side != Left) return;
    m_pressedZone = zone;
    update();
    m_pressTimer->start();
}

void TouchButtonOverlay::clearPress()
{
    m_pressedZone = -1;
    update();
}

// ── Right slots ───────────────────────────────────────────────────────────────

void TouchButtonOverlay::setGpsState(int state)
{
    if (m_side != Right) return;
    m_gpsState = state;
    update();
}

void TouchButtonOverlay::setAdsbConnected(bool on)
{
    if (m_side != Right) return;
    m_adsbConnected = on;
    if (!on) m_adsbHasAircraft = false;
    update();
}

void TouchButtonOverlay::setAdsbAircraftCount(int count)
{
    if (m_side != Right) return;
    m_adsbHasAircraft = (count > 0);
    update();
}

void TouchButtonOverlay::pollStats()
{
    // dump1090 decoder stats (rolling last-1-minute window)
    m_sdrValid = false;
    QFile f(QStringLiteral("/run/dump1090/stats.json"));
    if (f.open(QIODevice::ReadOnly)) {
        const QJsonObject m =
            QJsonDocument::fromJson(f.readAll()).object()
                .value(QStringLiteral("last1min")).toObject();
        if (!m.isEmpty()) {
            const QJsonObject loc = m.value(QStringLiteral("local")).toObject();
            const QJsonObject cpr = m.value(QStringLiteral("cpr")).toObject();
            m_sdrMsgPerMin = m.value(QStringLiteral("messages")).toDouble(-1);
            m_sdrPosPerMin = cpr.value(QStringLiteral("global_ok")).toDouble()
                           + cpr.value(QStringLiteral("local_ok")).toDouble();
            m_sdrSignalValid = loc.contains(QStringLiteral("signal"));
            m_sdrNoiseValid  = loc.contains(QStringLiteral("noise"));
            m_sdrSignal = loc.value(QStringLiteral("signal")).toDouble();
            m_sdrNoise  = loc.value(QStringLiteral("noise")).toDouble();
            const double sp = loc.value(QStringLiteral("samples_processed")).toDouble();
            const double sd = loc.value(QStringLiteral("samples_dropped")).toDouble();
            m_sdrDropPct = sp > 0 ? (sd / sp) * 100.0 : 0.0;
            m_sdrValid = true;
        }
    }

    // CPU load % from /proc/stat deltas
    QFile st(QStringLiteral("/proc/stat"));
    if (st.open(QIODevice::ReadOnly)) {
        const QList<QByteArray> parts = st.readLine().simplified().split(' ');
        quint64 total = 0, idle = 0;
        for (int i = 1; i < parts.size(); i++) {
            const quint64 v = parts[i].toULongLong();
            total += v;
            if (i == 4 || i == 5)  // idle + iowait
                idle += v;
        }
        const quint64 busy = total - idle;
        if (m_prevCpuTotal && total > m_prevCpuTotal)
            m_cpuPct = int(100.0 * (busy - m_prevCpuBusy)
                                 / (total - m_prevCpuTotal) + 0.5);
        m_prevCpuBusy = busy;
        m_prevCpuTotal = total;
    }

    // RAM usage % from /proc/meminfo
    QFile mi(QStringLiteral("/proc/meminfo"));
    if (mi.open(QIODevice::ReadOnly)) {
        quint64 memTotal = 0, memAvail = 0;
        while (memTotal == 0 || memAvail == 0) {
            const QByteArray line = mi.readLine();
            if (line.isEmpty()) break;
            if (line.startsWith("MemTotal:"))          memTotal = line.mid(9).simplified().split(' ')[0].toULongLong();
            else if (line.startsWith("MemAvailable:")) memAvail = line.mid(13).simplified().split(' ')[0].toULongLong();
        }
        if (memTotal > 0)
            m_ramPct = int(100.0 * (memTotal - memAvail) / memTotal + 0.5);
    }

    // SoC temperature
    QFile tz(QStringLiteral("/sys/class/thermal/thermal_zone0/temp"));
    if (tz.open(QIODevice::ReadOnly))
        m_tempC = int(tz.readAll().trimmed().toLongLong() / 1000);

    // Current CPU frequency
    QFile fq(QStringLiteral("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"));
    if (fq.open(QIODevice::ReadOnly))
        m_cpuGHz = fq.readAll().trimmed().toDouble() / 1e6;

    // 1-minute load average
    QFile la(QStringLiteral("/proc/loadavg"));
    if (la.open(QIODevice::ReadOnly))
        m_load1 = QString::fromLatin1(la.readAll().simplified().split(' ').value(0));

    // Network throughput (rx+tx across wlan0/eth0), KB/s over the 2 s cadence
    quint64 netBytes = 0;
    for (const char *ifc : { "wlan0", "eth0" }) {
        for (const char *dir : { "rx_bytes", "tx_bytes" }) {
            QFile nf(QStringLiteral("/sys/class/net/%1/statistics/%2")
                         .arg(QLatin1String(ifc), QLatin1String(dir)));
            if (nf.open(QIODevice::ReadOnly))
                netBytes += nf.readAll().trimmed().toULongLong();
        }
    }
    if (m_prevNetBytes && netBytes >= m_prevNetBytes)
        m_netKBps = int((netBytes - m_prevNetBytes) / 2 / 1024);
    m_prevNetBytes = netBytes;

    // Root filesystem usage
    QStorageInfo rootFs(QStringLiteral("/"));
    if (rootFs.isValid() && rootFs.bytesTotal() > 0)
        m_diskPct = int(100.0 * (rootFs.bytesTotal() - rootFs.bytesAvailable())
                              / rootFs.bytesTotal() + 0.5);

    // Uptime, compact
    QFile up(QStringLiteral("/proc/uptime"));
    if (up.open(QIODevice::ReadOnly)) {
        const qint64 secs = qint64(up.readAll().simplified().split(' ').value(0).toDouble());
        if (secs >= 86400)
            m_uptime = QStringLiteral("%1d%2h").arg(secs / 86400).arg((secs % 86400) / 3600);
        else if (secs >= 3600)
            m_uptime = QStringLiteral("%1h%2m").arg(secs / 3600).arg((secs % 3600) / 60);
        else
            m_uptime = QStringLiteral("%1m").arg(secs / 60);
    }

    // Firmware throttle status (undervoltage / thermal); low nibble = active now
    QFile th(QStringLiteral("/sys/devices/platform/soc/soc:firmware/get_throttled"));
    if (th.open(QIODevice::ReadOnly))
        m_throttled = (th.readAll().trimmed().toUInt(nullptr, 16) & 0xF) != 0;
}

// ── Paint ─────────────────────────────────────────────────────────────────────

void TouchButtonOverlay::paintEvent(QPaintEvent *event)
{
    (void)event;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), C_BG);

    if (m_side == Left)
        paintLeft(p);
    else
        paintRight(p);
}

void TouchButtonOverlay::paintLeft(QPainter &p)
{
    QFont labelFont("Lato");
    labelFont.setPixelSize(13);
    p.setFont(labelFont);

    const int w = width();
    const int half = height() / 2;
    const int zoneH = half / L_ZONES;

    // Parhelia logo, rotated 90 deg anti-clockwise, centred in the upper half.
    if (m_logo.isNull()) {
        QPixmap raw(":/images/parhelia_logo.png");
        QTransform t;
        t.rotate(-90);
        m_logo = raw.transformed(t, Qt::SmoothTransformation)
                    .scaled(qRound((w - 12) * 0.9), qRound((half - 24) * 0.9),
                            Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    p.drawPixmap((w - m_logo.width()) / 2, (half - m_logo.height()) / 2, m_logo);

    // Touch zones compressed into the bottom half.
    // sz=9 → icon bounding box 18×18px.  Label sits to the right of icon.
    const int sz     = 9;
    const int iconCX = 26;               // icon centre x
    const int labelX = iconCX + sz + 6;  // = 37

    for (int i = 0; i < L_ZONES; i++) {
        const int zoneTop = half + i * zoneH;

        if (i == m_pressedZone)
            p.fillRect(QRect(0, zoneTop, w, zoneH), C_PRESS);

        // Divider above every zone (the first separates logo from zones)
        p.setPen(QPen(C_DIVIDER, 1));
        p.drawLine(4, zoneTop, w - 4, zoneTop);

        const int iconCY = zoneTop + zoneH / 2;

        p.setPen(Qt::NoPen);
        p.setBrush(C_AMBER);

        switch (i) {
            case 0: {  // UP
                QPolygon tri;
                tri << QPoint(iconCX,      iconCY - sz)
                    << QPoint(iconCX + sz, iconCY + sz)
                    << QPoint(iconCX - sz, iconCY + sz);
                p.drawPolygon(tri);
                break;
            }
            case 1: {  // DOWN
                QPolygon tri;
                tri << QPoint(iconCX,      iconCY + sz)
                    << QPoint(iconCX + sz, iconCY - sz)
                    << QPoint(iconCX - sz, iconCY - sz);
                p.drawPolygon(tri);
                break;
            }
            case 2: {  // SELECT
                p.drawEllipse(QPoint(iconCX, iconCY), sz, sz);
                break;
            }
            case 3: {  // BACK
                QPolygon tri;
                tri << QPoint(iconCX - sz, iconCY)
                    << QPoint(iconCX + sz, iconCY - sz)
                    << QPoint(iconCX + sz, iconCY + sz);
                p.drawPolygon(tri);
                break;
            }
        }

        const QRect labelRect(labelX, zoneTop, w - 4 - labelX, zoneH);
        p.setPen(C_MUTED);
        p.drawText(labelRect, Qt::AlignVCenter | Qt::AlignLeft, L_LABELS[i]);
    }
}

void TouchButtonOverlay::paintRight(QPainter &p)
{
    QFont labelFont("Lato");
    labelFont.setPixelSize(11);
    p.setFont(labelFont);

    const int w = width();

    // Indicator colors for the two defined segments
    // GPS: 0=off, 1=amber(no lock), 2=green(locked)
    // ADSB: off / amber(connected) / green(has aircraft)
    const QColor segColor[2] = {
        // GPS
        (m_gpsState == 2) ? C_GREEN :
        (m_gpsState == 1) ? C_AMBER : QColor(),
        // ADSB
        m_adsbHasAircraft  ? C_GREEN :
        m_adsbConnected    ? C_AMBER : QColor()
    };

    // Full-width 100px draw strip.

    for (int i = 0; i < R_SEGS; i++) {
        const int segTop = i * R_SEG_HEIGHT;

        if (i > 0 && i != 3 && i != 5) {  // 2+3 and 4+5 are merged blocks
            p.setPen(QPen(C_DIVIDER, 1));
            p.drawLine(4, segTop, w - 4, segTop);
        }

        if (i == 2 || i == 4) {  // SDR stats / Pi stats, two segments each
            const QRect blockRect(0, segTop + 5, w, R_SEG_HEIGHT * 2 - 8);
            QFont f("Lato");
            f.setPixelSize(11);
            p.setFont(f);
            QStringList lines;
            if (i == 2) {
                lines << QStringLiteral("SDR");
                if (m_sdrValid) {
                    lines << QStringLiteral("MSG %1/m").arg(qRound(m_sdrMsgPerMin))
                          << QStringLiteral("POS %1/m").arg(qRound(m_sdrPosPerMin))
                          << (m_sdrSignalValid
                                  ? QStringLiteral("SIG %1dB").arg(m_sdrSignal, 0, 'f', 0)
                                  : QStringLiteral("SIG --"))
                          << (m_sdrNoiseValid
                                  ? QStringLiteral("NF %1dB").arg(m_sdrNoise, 0, 'f', 0)
                                  : QStringLiteral("NF --"))
                          << QStringLiteral("DROP %1%").arg(m_sdrDropPct, 0, 'f', 1);
                } else {
                    lines << QStringLiteral("(no data)");
                }
            } else {
                lines << QStringLiteral("PI 4");
                lines << (m_cpuPct >= 0
                              ? (m_cpuGHz > 0
                                     ? QStringLiteral("CPU %1% %2G").arg(m_cpuPct)
                                           .arg(m_cpuGHz, 0, 'f', 1)
                                     : QStringLiteral("CPU %1%").arg(m_cpuPct))
                              : QStringLiteral("CPU --"));
                if (!m_load1.isEmpty())
                    lines << QStringLiteral("LOAD %1").arg(m_load1);
                lines << (m_ramPct >= 0 ? QStringLiteral("RAM %1%").arg(m_ramPct)
                                        : QStringLiteral("RAM --"));
                lines << (m_tempC  >= 0
                              ? QStringLiteral("TEMP %1\u00B0C%2").arg(m_tempC)
                                    .arg(m_throttled ? QStringLiteral("!") : QString())
                              : QStringLiteral("TEMP --"));
                if (m_netKBps >= 0)
                    lines << QStringLiteral("NET %1K/s").arg(m_netKBps);
                if (m_diskPct >= 0)
                    lines << QStringLiteral("DISK %1%").arg(m_diskPct);
                if (!m_uptime.isEmpty())
                    lines << QStringLiteral("UP %1").arg(m_uptime);
            }
            p.setPen(C_MUTED);
            p.drawText(blockRect, Qt::AlignHCenter | Qt::AlignTop,
                       lines.join(QStringLiteral("\n")));
            continue;
        }
        if (i == 3 || i == 5)
            continue;  // covered by the block above

        if (i == 6 || i == 7) {  // date / time segments
            const QDateTime now = WingletGUI::inst->gpsLocalTime();
            const bool timeValid = now.date().year() >= 2002;
            const QRect segRect(0, segTop, w, R_SEG_HEIGHT);
            if (i == 6) {
                p.setPen(C_MUTED);
                QFont f("Lato");
                f.setPixelSize(14);
                p.setFont(f);
                p.drawText(segRect, Qt::AlignCenter,
                           timeValid ? now.toString(QStringLiteral("ddd\nMMM d"))
                                     : QStringLiteral("No\nDate"));
            } else {
                p.setPen(C_AMBER);
                QFont f("Lato");
                f.setPixelSize(15);
                p.setFont(f);
                // Always 24-hour with seconds, in the system's local zone
                p.drawText(segRect, Qt::AlignCenter,
                           timeValid ? now.toString(QStringLiteral("HH:mm:ss"))
                                     : QStringLiteral("--:--:--"));
                p.setFont(labelFont);
            }
            continue;
        }

        if (i >= 2) continue;  // remaining segments blank

        const QColor &col = segColor[i];
        if (!col.isValid()) continue;  // off state — leave dark

        const int barH   = 20;
        const int barPad = 10;
        const int barY   = segTop + (R_SEG_HEIGHT - barH) / 2 - 8;
        p.setPen(Qt::NoPen);
        p.setBrush(col);
        p.drawRect(barPad, barY, w - barPad * 2, barH);

        // Label centred below the bar
        const QRect labelRect(0, barY + barH + 2, w, 14);
        p.setPen(C_MUTED);
        p.drawText(labelRect, Qt::AlignHCenter | Qt::AlignVCenter, R_LABELS[i]);
    }
}

} // namespace WingletUI
