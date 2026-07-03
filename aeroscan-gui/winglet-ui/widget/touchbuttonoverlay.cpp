#include "touchbuttonoverlay.h"
#include <QPainter>
#include <QPolygon>
#include <QMouseEvent>

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
        int zone = qBound(0, event->pos().y() * L_ZONES / height(), L_ZONES - 1);
        showPress(zone);
        emit zonePressed(zone);
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

    // Graphics confined to left 80px; right 80px is blank.
    // sz=9 → icon bounding box 18×18px.  Label sits to the right of icon.
    const int sz     = 9;
    const int iconCX = 22;               // icon centre x (within left 80px)
    const int labelX = iconCX + sz + 6;  // = 37

    for (int i = 0; i < L_ZONES; i++) {
        const int zoneTop = i * L_ZONE_HEIGHT;

        if (i == m_pressedZone)
            p.fillRect(QRect(0, zoneTop, w, L_ZONE_HEIGHT), C_PRESS);

        if (i > 0) {
            p.setPen(QPen(C_DIVIDER, 1));
            p.drawLine(4, zoneTop, 76, zoneTop);  // divider within left 80px
        }

        const int iconCY = zoneTop + L_ZONE_HEIGHT / 2;

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

        // Label confined to left 80px
        const QRect labelRect(labelX, zoneTop, 76 - labelX, L_ZONE_HEIGHT);
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

    // Graphics confined to right 80px; left 80px is blank.
    const int halfX = w / 2;  // = 80

    for (int i = 0; i < R_SEGS; i++) {
        const int segTop = i * R_SEG_HEIGHT;

        // Divider within right 80px only (skip first)
        if (i > 0) {
            p.setPen(QPen(C_DIVIDER, 1));
            p.drawLine(halfX + 4, segTop, w - 4, segTop);
        }

        if (i >= 2) continue;  // segments 2-7 blank

        const QColor &col = segColor[i];
        if (!col.isValid()) continue;  // off state — leave dark

        // Indicator bar inside right 80px
        const int barH   = 20;
        const int barPad = 8;
        const int barY   = segTop + (R_SEG_HEIGHT - barH) / 2 - 8;
        p.setPen(Qt::NoPen);
        p.setBrush(col);
        p.drawRect(halfX + barPad, barY, w - halfX - barPad * 2, barH);

        // Label centred in right 80px below bar
        const QRect labelRect(halfX, barY + barH + 2, w - halfX, 14);
        p.setPen(C_MUTED);
        p.drawText(labelRect, Qt::AlignHCenter | Qt::AlignVCenter, R_LABELS[i]);
    }
}

} // namespace WingletUI
