#ifndef WINGLETUI_RADIOPRESETSMODEL_H
#define WINGLETUI_RADIOPRESETSMODEL_H

#include <QAbstractItemModel>
#include <QVector>
#include "winglet-ui/worker/rtlfmworker.h"

namespace WingletUI {

// Two-level model for the preset browser (SelectorBox). Root rows are presets
// for the tuner's current band — user favorites first, then nearby NASR
// airports (airband only) — and each preset's child rows are the actions that
// apply to it. Mirrors PairedBtDevicesModel's flat two-level layout.
class RadioPresetsModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum PresetAction {
        INVALID_ACTION,
        TUNE_PRESET,
        ADD_FAVORITE,
        RENAME_FAVORITE,
        DELETE_FAVORITE
    };

    // Roles on the action rows so the tuner can act on the parent preset
    static const int FreqKhzRole    = Qt::UserRole + 1;  // uint
    static const int IsFavoriteRole = Qt::UserRole + 2;  // bool
    static const int PresetIdRole   = Qt::UserRole + 3;  // int (favorites only)
    static const int NameRole       = Qt::UserRole + 4;  // QString

    explicit RadioPresetsModel(RtlFmWorker::Mode mode, QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

private:
    struct Entry {
        QString  name;      // "Guard" or "KTUS TWR"
        QString  detail;    // "121.500 MHz" or "2.1 nm"
        uint32_t freqKhz;
        bool     isFavorite;
        int      presetId;  // valid only when isFavorite
    };

    RtlFmWorker::Mode m_mode;
    QVector<Entry> m_entries;
};

} // namespace WingletUI
#endif // WINGLETUI_RADIOPRESETSMODEL_H
