#include "dronereceiver.h"

namespace WingletUI {

DroneReceiver::DroneReceiver(QThread *ownerThread)
{
    moveToThread(ownerThread);
}

DroneReceiver::~DroneReceiver() {}

} // namespace WingletUI
