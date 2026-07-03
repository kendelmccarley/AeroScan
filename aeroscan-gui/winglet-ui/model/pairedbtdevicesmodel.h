#ifndef WINGLETUI_PAIREDBTDEVICESMODEL_H
#define WINGLETUI_PAIREDBTDEVICESMODEL_H

#include <QAbstractItemModel>
#include "winglet-ui/worker/bluetoothmonitor.h"

namespace WingletUI {

class PairedBtDevicesModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum PairedDeviceAction {
        INVALID_ACTION,
        CONNECT_DEVICE,
        FORGET_DEVICE
    };

    // Extra roles on the action rows so the settings menu can identify the device
    static const int DevicePathRole = Qt::UserRole + 1;
    static const int DeviceNameRole = Qt::UserRole + 2;

    explicit PairedBtDevicesModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

private:
    QList<BtDeviceInfo> pairedDevices;
};

} // namespace WingletUI

#endif // WINGLETUI_PAIREDBTDEVICESMODEL_H
