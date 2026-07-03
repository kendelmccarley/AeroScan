#include "droneboard.h"
#include <QLabel>
#include <QVBoxLayout>

namespace WingletUI {

DroneBoard::DroneBoard(QWidget *parent) : QWidget{parent}
{
    setGeometry(0, 0, 480, 480);
    auto label = new QLabel("No drones detected", this);
    label->setAlignment(Qt::AlignCenter);
    auto layout = new QVBoxLayout(this);
    layout->addWidget(label);
}

} // namespace WingletUI
