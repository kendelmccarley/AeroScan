#ifndef WINGLETUI_CANARDBOARD_H
#define WINGLETUI_CANARDBOARD_H

#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QSlider>
#include <QList>
#include <QVector>
#include <QTimer>
#include <QModelIndex>
#include "winglet-ui/worker/rtlfmworker.h"
#include "winglet-ui/worker/radiopresets.h"
#include "winglet-ui/widget/statusbar.h"

namespace WingletUI {

// Radio Tuner (Phase 6i v2): a vertical control stack operable entirely from
// the four touch keys. Up/Down move the selection between rows, A activates the
// selected row (cycle band, enter an edit mode, tune a preset, or open a
// screen), and B (handled globally) closes the tuner. Edit modes use Up/Down to
// change the value live and A to confirm/advance.
class CanardBoard : public QWidget
{
    Q_OBJECT
public:
    explicit CanardBoard(QWidget *parent = nullptr);
    ~CanardBoard();

    void keyPressEvent(QKeyEvent *ev) override;
    void wheelEvent(QWheelEvent *event) override;

protected:
    void showEvent(QShowEvent *event) override;

private:
    // ── Control-stack rows (fixed order; visibility depends on band/state) ────
    enum Row {
        ROW_BAND,
        ROW_FREQ,
        ROW_PRESET,        // hidden when the band has no presets
        ROW_ALL_PRESETS,
        ROW_VOLUME,
        ROW_SQUELCH,       // hidden in FM broadcast (squelch-open)
        ROW_SAVE,
        ROW_COUNT,
    };

    enum EditTarget { EDIT_NONE, EDIT_FREQ, EDIT_VOLUME, EDIT_SQUELCH, EDIT_PRESET };

    // Return-state for results coming back from child screens (browser/keyboard)
    enum ActionState {
        AS_NORMAL,
        AS_BROWSER_RETURN,
        AS_SAVE_KBD_RETURN,
        AS_RENAME_KBD_RETURN,
        AS_MSGBOX_RETURN,
    };

    struct PresetEntry {
        QString  name;     // "Guard" or "KTUS TWR"
        QString  detail;   // "121.500"
        uint32_t freqKhz;
        bool     isFavorite;
        int      presetId; // valid only when isFavorite
    };

    // ── Navigation / activation ──────────────────────────────────────────────
    void buildActiveRows();
    Row  selectedRow() const;
    void moveSelection(int dir);
    void activateRow();
    void handleEditKey(QKeyEvent *ev);

    // ── Rendering ─────────────────────────────────────────────────────────────
    void render();
    void renderFreqCells(bool rowSelected, bool editing);
    void renderFrequencyValue();
    void updateStatus();

    // ── Tuning ─────────────────────────────────────────────────────────────
    void increaseFrequency();
    void decreaseFrequency();
    void nextFreqBox();
    void prevFreqBox();
    uint getFrequencyTuningIncrement();
    uint numDecimals() const;
    uint32_t bandMinFreq() const;
    uint32_t bandMaxFreq() const;

    // ── Mode switching ──────────────────────────────────────────────────────
    void setMode(RtlFmWorker::Mode newMode);
    void cycleMode(int direction);
    void applyTune();

    // ── Squelch / volume ─────────────────────────────────────────────────────
    int  currentBandSquelch() const;
    void changeSquelch(int delta);
    void changeVolume(int delta);

    // ── Presets ──────────────────────────────────────────────────────────────
    void rebuildBandPresets();
    void selectPreset(int idx, bool tune);
    void openPresetBrowser();
    void openSavePresetFlow();
    void openRenameFlow(int presetId, const QString &currentName);

    // ── State ───────────────────────────────────────────────────────────────
    RtlFmWorker::Mode m_mode = RtlFmWorker::MODE_FM;
    uint32_t m_freq       = 1017;
    uint32_t m_savedFmFreq;
    uint32_t m_savedAirbandFreq;
    uint32_t m_savedHam2mFreq;

    QList<Row>  m_activeRows;
    int         m_selRow = 0;
    EditTarget  m_edit   = EDIT_NONE;
    int         frequencyIndex = 0;

    QVector<PresetEntry> m_bandPresets;
    int                  m_presetSel = 0;

    // Pending result from a child screen, applied in showEvent
    ActionState m_actionState = AS_NORMAL;
    int         m_pendingAction = 0;
    uint32_t    m_pendingFreqKhz = 0;
    int         m_pendingPresetId = -1;
    QString     m_pendingName;
    QString     m_kbdResult;
    uint32_t    m_saveFreqKhz = 0;

    static const int DIGITS_BEFORE_DECIMAL = 3;

    // ── Widgets ─────────────────────────────────────────────────────────────
    QLabel           *bandLabel;
    QList<QSpinBox*>  freqBoxes;
    QLabel           *decimalPointLabel;
    QLabel           *unitLabel;
    QLabel           *presetLabel;
    QLabel           *allPresetsLabel;
    QLabel           *volumeLabel;
    QSlider          *volumeSlider;
    QLabel           *squelchLabel;
    QLabel           *saveLabel;
    QLabel           *statusLabel;
    QLabel           *helpLabel;
    StatusBar        *statusBar;
    QLabel           *bgLogoLabel;
    QPixmap           bgLogo;
    QTimer           *statusTimer;

private slots:
    void rtlFmAvailabilityChanged(bool available);
    void presetsChanged();
    void selectorIndexSelected(QModelIndex index);
    void kbdTextEntered(QString val);
};

} // namespace WingletUI
#endif // WINGLETUI_CANARDBOARD_H
