#ifndef WINGLETUI_GPIOCONTROL_H
#define WINGLETUI_GPIOCONTROL_H

#include <QObject>

namespace WingletUI {

// Phase 2: stub. Phase 3 adds RPi button GPIO input handling via libgpiod.
class GPIOControl : public QObject
{
    Q_OBJECT
public:
    explicit GPIOControl(QObject *parent);
    ~GPIOControl();
};

} // namespace WingletUI

#endif // WINGLETUI_GPIOCONTROL_H
