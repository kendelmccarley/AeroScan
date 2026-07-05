#include "btscanmodel.h"
#include "wingletgui.h"

namespace WingletUI {

BtScanModel::BtScanModel(BluetoothMonitor::DeviceFilter filter, QObject *parent)
    : QAbstractItemModel{parent}, m_refreshTimer(this), m_filter(filter)
{
    connect(WingletGUI::inst->btMon, SIGNAL(scanResultsChanged()), this, SLOT(scanResultsChanged()));
    m_refreshTimer.setInterval(refreshIntervalMs);
    m_refreshTimer.setSingleShot(true);
    connect(&m_refreshTimer, SIGNAL(timeout()), this, SLOT(refreshTimerFired()));

    WingletGUI::inst->btMon->startScan();
    m_scanResults = WingletGUI::inst->btMon->scanResults(m_filter);
}

BtScanModel::~BtScanModel()
{
    WingletGUI::inst->btMon->stopScan();
}

void BtScanModel::scanResultsChanged()
{
    m_updatePending = true;
    if (!m_refreshTimer.isActive())
        refreshTimerFired();
}

void BtScanModel::refreshTimerFired()
{
    if (!m_updatePending)
        return;
    m_updatePending = false;
    applyScanResults();
    m_refreshTimer.start();
}

void BtScanModel::applyScanResults()
{
    // 1. Prepare for scan results refresh
    emit layoutAboutToBeChanged();

    // 2. Save previous state so we can update the persistent indices
    QModelIndexList oldPersistentIndices = persistentIndexList();
    QList<BtDeviceInfo> oldScanResults = m_scanResults;

    // 3. Refresh scan results
    m_scanResults = WingletGUI::inst->btMon->scanResults(m_filter);

    // 4. Update the persistent model indices (matched by BlueZ object path)
    for (auto oldIdx : oldPersistentIndices) {
        if (!oldIdx.isValid())
            continue;

        QModelIndex newIdx;
        if (!oldIdx.parent().isValid() && oldIdx.column() == 0 && oldIdx.row() >= 0
                && oldIdx.row() < oldScanResults.count()) {
            const BtDeviceInfo &oldResult = oldScanResults.at(oldIdx.row());
            for (int i = 0; i < m_scanResults.count(); i++) {
                if (m_scanResults.at(i).path == oldResult.path) {
                    newIdx = createIndex(i, 0);
                    break;
                }
            }
        }
        changePersistentIndex(oldIdx, newIdx);
    }

    // 5. Notify of layout changed
    emit layoutChanged();
}

QVariant BtScanModel::data(const QModelIndex &index, int role) const
{
    // Invalid index is the root node
    if (!index.isValid()) {
        if (role == Qt::DisplayRole)
            return m_filter == BluetoothMonitor::FILTER_AUDIO ? "Pair\nHeadphones" : "Pair\nKeyboard";
        else
            return {};
    }

    // If no scan results right now, report searching
    if (m_scanResults.empty()) {
        if (role == Qt::DisplayRole)
            return "Searching...";
        else
            return {};
    }

    int idx = index.row();
    if (idx >= m_scanResults.count() || idx < 0)
        return {};

    const BtDeviceInfo &result = m_scanResults.at(idx);

    if (role == Qt::DisplayRole) {
        return result.name;
    }
    else if (role == Qt::UserRole) {
        return QVariant::fromValue(result);
    }
    else if (role == Qt::EditRole) {
        if (result.connected)
            return "Connected";
        else if (result.paired)
            return "Paired";
        else
            return {};
    }
    else {
        return {};
    }
}

QModelIndex BtScanModel::index(int row, int column, const QModelIndex &parent) const
{
    // Only column 0 allowed
    if (column != 0)
        return {};

    // Can't access children elements
    if (parent.isValid())
        return {};

    if (m_scanResults.empty()) {
        // No scan results, index for the Searching... placeholder
        if (row == 0)
            return createIndex(0, 0);
        else
            return {};
    }
    else {
        if (row < 0 || row >= m_scanResults.count())
            return {};

        return createIndex(row, 0);
    }
}

QModelIndex BtScanModel::parent(const QModelIndex &index) const
{
    (void) index;
    // Flat list (no parents)
    return {};
}

Qt::ItemFlags BtScanModel::flags(const QModelIndex &index) const
{
    (void) index;
    if (m_scanResults.empty())
        return Qt::NoItemFlags;  // If empty disable selecting the Searching item
    else
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

int BtScanModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;  // Flat list (no children allowed)

    if (m_scanResults.empty())
        return 1;  // 1 item for the Searching... text
    else
        return m_scanResults.count();
}

int BtScanModel::columnCount(const QModelIndex &parent) const
{
    (void) parent;
    return 1;
}

} // namespace WingletUI
