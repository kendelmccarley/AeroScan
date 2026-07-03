#ifndef WINGLETUI_TOUCHBUTTONOVERLAY_H
#define WINGLETUI_TOUCHBUTTONOVERLAY_H

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
    QTimer *m_pressTimer = nullptr;

    // Right state
    int  m_gpsState       = 0;     // 0=disconnected 1=no-lock 2=locked
    bool m_adsbConnected  = false;
    bool m_adsbHasAircraft = false;
};

} // namespace WingletUI

#endif // WINGLETUI_TOUCHBUTTONOVERLAY_H
