#include "radiopresetsmodel.h"
#include "wingletgui.h"
#include <QtMath>

namespace WingletUI {

// Child action rows, keyed by whether the parent preset is a user favorite
static const QList<QPair<QString, int>> favoriteActions = {
    {"Tune",   RadioPresetsModel::TUNE_PRESET},
    {"Rename", RadioPresetsModel::RENAME_FAVORITE},
    {"Delete", RadioPresetsModel::DELETE_FAVORITE},
};
static const QList<QPair<QString, int>> nearbyActions = {
    {"Tune",             RadioPresetsModel::TUNE_PRESET},
    {"Add to Favorites", RadioPresetsModel::ADD_FAVORITE},
};

static const quintptr presetRootRowInternalId = 0xFFFFFFFF;

RadioPresetsModel::RadioPresetsModel(RtlFmWorker::Mode mode, QObject *parent)
    : QAbstractItemModel{parent}, m_mode(mode)
{
    // Favorites for this band come first
    const auto favorites = WingletGUI::inst->presets->presetsForBand(mode);
    for (const auto &p : favorites) {
        Entry e;
        e.name       = p.name;
        e.detail     = QString("%1 MHz").arg(p.freqKhz / 1000.0, 0, 'f', 3);
        e.freqKhz    = p.freqKhz;
        e.isFavorite = true;
        e.presetId   = p.id;
        m_entries.append(e);
    }

    // Nearby NASR airports supplement the airband list
    if (mode == RtlFmWorker::MODE_AIRBAND && WingletGUI::inst->nasr->isLoaded()) {
        float lat = (float) WingletGUI::inst->settings.lastLatitude();
        float lon = (float) WingletGUI::inst->settings.lastLongitude();
        const auto nearby = WingletGUI::inst->nasr->queryNearby(lat, lon, 20.0f, 30);
        for (const auto &a : nearby) {
            Entry e;
            e.name       = QString("%1 %2").arg(a.ident, a.freqType);
            e.detail     = QString("%1 MHz").arg(a.freqMhz, 0, 'f', 3);
            e.freqKhz    = (uint32_t) qRound(a.freqMhz * 1000.0f);
            e.isFavorite = false;
            e.presetId   = -1;
            m_entries.append(e);
        }
    }
}

QVariant RadioPresetsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        if (role == Qt::DisplayRole) {
            switch (m_mode) {
            case RtlFmWorker::MODE_FM:      return "Presets - FM";
            case RtlFmWorker::MODE_AIRBAND: return "Presets - Airband";
            case RtlFmWorker::MODE_HAM2M:   return "Presets - 2m";
            }
        }
        return {};
    }

    if (index.internalId() == presetRootRowInternalId) {
        // Root row: one preset
        if (index.row() < 0 || index.row() >= m_entries.size() || index.column() != 0)
            return {};
        const Entry &e = m_entries.at(index.row());
        if (role == Qt::DisplayRole)
            return e.name;
        else if (role == Qt::EditRole)
            return e.detail;
        return {};
    }

    // Action row: internalId is the parent preset row
    int presetIdx = (int) index.internalId();
    if (presetIdx < 0 || presetIdx >= m_entries.size() || index.column() != 0)
        return {};
    const Entry &e = m_entries.at(presetIdx);
    const auto &actions = e.isFavorite ? favoriteActions : nearbyActions;
    if (index.row() < 0 || index.row() >= actions.size())
        return {};

    switch (role) {
    case Qt::DisplayRole:  return actions.at(index.row()).first;
    case Qt::UserRole:     return actions.at(index.row()).second;
    case FreqKhzRole:      return e.freqKhz;
    case IsFavoriteRole:   return e.isFavorite;
    case PresetIdRole:     return e.presetId;
    case NameRole:         return e.name;
    default:               return {};
    }
}

QModelIndex RadioPresetsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        if (parent.row() < 0 || parent.row() >= m_entries.size() || parent.column() != 0
                || parent.internalId() != presetRootRowInternalId) {
            return {};
        }
        return createIndex(row, column, parent.row());
    }
    return createIndex(row, column, presetRootRowInternalId);
}

QModelIndex RadioPresetsModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};
    if (index.internalId() == presetRootRowInternalId)
        return {};
    return this->index(index.internalId(), 0, {});
}

int RadioPresetsModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return m_entries.size();
    }
    if (parent.internalId() == presetRootRowInternalId && parent.row() >= 0
            && parent.row() < m_entries.size()) {
        return m_entries.at(parent.row()).isFavorite ? favoriteActions.size()
                                                      : nearbyActions.size();
    }
    return 0;
}

int RadioPresetsModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid() || parent.internalId() == presetRootRowInternalId)
        return 1;
    return 0;
}

} // namespace WingletUI
