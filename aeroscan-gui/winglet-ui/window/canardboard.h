#ifndef WINGLETUI_CANARDBOARD_H
#define WINGLETUI_CANARDBOARD_H

#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QSlider>
#include <QList>
#include <QVector>
#include "winglet-ui/worker/rtlfmworker.h"
#include "winglet-ui/worker/nasrdatabase.h"
#include "winglet-ui/widget/statusbar.h"

namespace WingletUI {

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
    // ── Rendering ──────────────────────────────────────────────────────────
    void renderTuningInfo();
    void renderFrequencyValue();
    void renderPresetInfo();
    void renderVolumeValue();

    // ── Tuning ─────────────────────────────────────────────────────────────
    void increaseFrequency();
    void decreaseFrequency();
    void nextFreqBox();
    void prevFreqBox();
    uint getFrequencyTuningIncrement();

    // ── Mode switching ──────────────────────────────────────────────────────
    void setFMMode();
    void setAirbandMode();
    void setPresetMode(bool on);
    void applyTune();

    // ── Preset navigation ───────────────────────────────────────────────────
    void loadNearbyPresets();
    void nextPreset();
    void prevPreset();

    // ── Volume ──────────────────────────────────────────────────────────────
    void changeVolume(int delta);

    // ── State ───────────────────────────────────────────────────────────────
    RtlFmWorker::Mode m_mode = RtlFmWorker::MODE_FM;
    uint32_t m_freq       = 1017;
    uint32_t m_savedFmFreq;
    uint32_t m_savedAirbandFreq;
    bool     m_presetMode = false;
    int      m_presetIdx  = 0;
    QVector<NASRDatabase::AirportFreq> m_presets;

    int frequencyIndex = 0;

    static const int DIGITS_BEFORE_DECIMAL = 3;

    enum WidgetIndex {
        WIDGET_IDX_FREQ = 0,
        WIDGET_IDX_TUNE_PRESET_SEL,
        WIDGET_IDX_RADIO_MODE,
        WIDGET_IDX_COUNT,
    };
    int widgetIndex = 0;

    // ── Widgets ─────────────────────────────────────────────────────────────
    QLabel           *presetLine1;
    QLabel           *presetLine2;
    QLabel           *tunePresetLabel;
    QLabel           *radioModeLabel;
    QLabel           *decimalPointLabel;
    QList<QSpinBox*>  freqBoxes;
    QSlider          *volumeSlider;
    StatusBar        *statusBar;
    QLabel           *bgLogoLabel;
    QPixmap           bgLogo;

private slots:
    void rtlFmAvailabilityChanged(bool available);
};

} // namespace WingletUI
#endif // WINGLETUI_CANARDBOARD_H
