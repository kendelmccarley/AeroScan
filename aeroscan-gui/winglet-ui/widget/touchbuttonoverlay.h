#ifndef WINGLETUI_TOUCHBUTTONOVERLAY_H
#define WINGLETUI_TOUCHBUTTONOVERLAY_H

#include <QPixmap>
#include <QWidget>
#include <QTimer>

class QPainter;
class QMouseEvent;

namespace WingletUI {

// Left overlay: 160x480, four 160x120 touch zones (Up/Down/Select/Back).
// Right overlay: 160x480, eight 160x60 status segments.
//   Segment 0 — GPS:  off = disconnected, amber = no lock, green = locked
//   Segment 1 — ADSB: off = no dongle,    amber = connected, green = aircraft tracked
//   Segments 2–7 — reserved, blank
class TouchButtonOverlay : public QWidget
{
    Q_OBJECT
public:
    enum Side { Left, Right };
    explicit TouchButtonOverlay(Side side, QWidget *parent = nullptr);

signals:
    void zonePressed(int zone);   // Left overlay only; emitted on press

public slots:
    // Left overlay
    void showPress(int zone);
    // Right overlay
    void setGpsState(int state);       // WingletUI::GPSReceiver::GPSState
    void setAdsbConnected(bool on);
    void setAdsbAircraftCount(int count);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void clearPress();

private:
    void paintLeft(QPainter &p);
    void paintRight(QPainter &p);

    Side m_side;

    // Left state
    int m_pressedZone = -1;
    // Monitored stats for the right column (SDR: slots 2-3, Pi: slots 4-5)
    void pollStats();
    bool   m_sdrValid = false;
    double m_sdrMsgPerMin = -1, m_sdrPosPerMin = -1;
    double m_sdrSignal = 0, m_sdrNoise = 0, m_sdrDropPct = 0;
    bool   m_sdrSignalValid = false, m_sdrNoiseValid = false;
    int m_cpuPct = -1, m_ramPct = -1, m_tempC = -1;
    double m_cpuGHz = -1;
    QString m_load1, m_uptime;
    int m_netKBps = -1, m_diskPct = -1;
    bool m_throttled = false;
    quint64 m_prevNetBytes = 0;
    quint64 m_prevCpuBusy = 0, m_prevCpuTotal = 0;
    int m_statTick = 0;
    QPixmap m_logo;   // Parhelia wordmark, rotated for the upper left strip
    QTimer *m_pressTimer = nullptr;

    // Right state
    int  m_gpsState       = 0;     // 0=disconnected 1=no-lock 2=locked
    bool m_adsbConnected  = false;
    bool m_adsbHasAircraft = false;
};

} // namespace WingletUI

#endif // WINGLETUI_TOUCHBUTTONOVERLAY_H
