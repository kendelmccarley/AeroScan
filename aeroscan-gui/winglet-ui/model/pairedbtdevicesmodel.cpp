#include "pairedbtdevicesmodel.h"
#include "wingletgui.h"

namespace WingletUI {

const QList<QPair<QString, int>> deviceOptions = {
    {"Connect", PairedBtDevicesModel::CONNECT_DEVICE},
    {"Forget Device", PairedBtDevicesModel::FORGET_DEVICE}
};

const quintptr deviceRootRowInternalId = 0xFFFFFFFF;

PairedBtDevicesModel::PairedBtDevicesModel(QObject *parent)
    : QAbstractItemModel{parent}
{
    pairedDevices = WingletGUI::inst->btMon->pairedDevices();
}

QVariant PairedBtDevicesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        if (role == Qt::DisplayRole)
            return "Paired Devices";
        else
            return {};
    }

    if (index.internalId() == deviceRootRowInternalId) {
        // This is the root row (list of devices)
        if (index.row() < 0 || index.row() >= pairedDevices.size() || index.column() != 0)
            return {};

        const BtDeviceInfo &device = pairedDevices.at(index.row());

        if (role == Qt::DisplayRole) {
            return device.name;
        }
        else if (role == Qt::EditRole) {
            if (device.connected)
                return "Connected";
            else
                return {};
        }
        else {
            return {};
        }
    }
    else {
        // This is a row for the list of options; internalId is the device row
        int deviceIdx = (int) index.internalId();
        if (index.row() < 0 || index.row() >= deviceOptions.size() || index.column() != 0)
            return {};
        if (deviceIdx < 0 || deviceIdx >= pairedDevices.size())
            return {};

        if (role == Qt::DisplayRole) {
            return deviceOptions.at(index.row()).first;
        }
        else if (role == Qt::UserRole) {
            return deviceOptions.at(index.row()).second;
        }
        else if (role == DevicePathRole) {
            return pairedDevices.at(deviceIdx).path;
        }
        else if (role == DeviceNameRole) {
            return pairedDevices.at(deviceIdx).name;
        }
        else {
            return {};
        }
    }
}

QModelIndex PairedBtDevicesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        // Make sure parent is a valid device row
        if (parent.row() < 0 || parent.row() >= pairedDevices.size() || parent.column() != 0
                || parent.internalId() != deviceRootRowInternalId) {
            return {};
        }

        return createIndex(row, column, parent.row());
    }
    else {
        return createIndex(row, column, deviceRootRowInternalId);
    }
}

QModelIndex PairedBtDevicesModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};

    if (index.internalId() == deviceRootRowInternalId)
        return {};
    else
        return this->index(index.internalId(), 0, {});
}

int PairedBtDevicesModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return pairedDevices.size();
    }
    else if (parent.internalId() == deviceRootRowInternalId && parent.row() >= 0
             && parent.row() < pairedDevices.size()) {
        return deviceOptions.size();
    }
    else {
        return 0;
    }
}

int PairedBtDevicesModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid() || parent.internalId() == deviceRootRowInternalId)
        return 1;
    else
        return 0;
}

} // namespace WingletUI
