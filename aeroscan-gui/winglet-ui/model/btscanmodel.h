#ifndef WINGLETUI_BTSCANMODEL_H
#define WINGLETUI_BTSCANMODEL_H

#include <QAbstractItemModel>
#include <QTimer>
#include "winglet-ui/worker/bluetoothmonitor.h"

namespace WingletUI {

class BtScanModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit BtScanModel(BluetoothMonitor::DeviceFilter filter, QObject *parent = nullptr);
    ~BtScanModel();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

private slots:
    void scanResultsChanged();
    void refreshTimerFired();

private:
    void applyScanResults();

    // Discovery pushes RSSI updates several times a second; coalesce them so
    // the menu doesn't constantly re-sort under the user's finger
    QTimer m_refreshTimer;
    const int refreshIntervalMs = 1000;
    bool m_updatePending = false;

    BluetoothMonitor::DeviceFilter m_filter;
    QList<BtDeviceInfo> m_scanResults;
};

} // namespace WingletUI

#endif // WINGLETUI_BTSCANMODEL_H
