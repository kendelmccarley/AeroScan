#ifndef WINGLETUI_DRONEBOARD_H
#define WINGLETUI_DRONEBOARD_H

#include <QWidget>

namespace WingletUI {

// Stub — shows "No drones detected" until Phase 10 services are active.
class DroneBoard : public QWidget
{
    Q_OBJECT
public:
    explicit DroneBoard(QWidget *parent = nullptr);
};

} // namespace WingletUI

#endif // WINGLETUI_DRONEBOARD_H
