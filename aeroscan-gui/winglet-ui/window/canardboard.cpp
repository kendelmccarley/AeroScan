#include "canardboard.h"
#include "wingletgui.h"
#include "winglet-ui/theme.h"
#include <QKeyEvent>
#include <QWheelEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QtMath>

namespace WingletUI {

static const QString QSpinBox_Normal =
    "QSpinBox { font-size: 56; padding: 2px; border-radius: 5px; border-width: 2px;"
    " border-style: solid; border-color: #22d21fff;"
    " background-color: rgba(140,140,160,0.77); }";
static const QString QSpinBox_Highlighted =
    "QSpinBox { font-size: 56; padding: 2px; border-radius: 5px; border-width: 2px;"
    " border-style: solid; border-color: #356a59;"
    " background-color: rgba(80,140,160,1); }";
static const QString QSpinBox_GrayedOut =
    "QSpinBox { font-size: 56; border-radius: 5px;"
    " background-color: #3b4b4773; color: #9c545448; }";
static const QString SliderStyle =
    "QSlider::groove:horizontal { border:1px solid #999; height:8px;"
    " background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #B1B1B1,stop:1 #c4c4c4);"
    " margin:2px 0; }"
    "QSlider::handle:horizontal { background:qlineargradient(x1:0,y1:0,x2:1,y2:1,"
    " stop:0 #b4b4b4,stop:1 #8f8f8f); border:1px solid #5c5c5c; width:18px;"
    " margin:-2px 0; border-radius:3px; }";

CanardBoard::CanardBoard(QWidget *parent)
    : QWidget(parent)
{
    m_savedFmFreq      = WingletGUI::inst->settings.canardLastFmFreq();
    m_savedAirbandFreq = WingletGUI::inst->settings.canardLastAirbandFreq();
    m_freq = m_savedFmFreq;
    m_mode = RtlFmWorker::MODE_FM;

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 40, 10, 10);
    mainLayout->setSpacing(8);

    QLabel *title = new QLabel("Radio Tuner", this);
    title->setFont(QFont(activeTheme->titleFont, 20));
    title->setForegroundRole(QPalette::Text);
    title->setAutoFillBackground(true);
    mainLayout->addWidget(title, 0, Qt::AlignHCenter);

    // Frequency spinboxes: 3 integer digits + decimal + up to 3 fractional digits
    QHBoxLayout *spinboxRow = new QHBoxLayout();
    spinboxRow->setSpacing(6);
    for (int i = 0; i < 6; i++) {
        if (i == DIGITS_BEFORE_DECIMAL) {
            decimalPointLabel = new QLabel(".", this);
            decimalPointLabel->setFont(QFont(activeTheme->standardFont, 30));
            decimalPointLabel->setAlignment(Qt::AlignCenter);
            decimalPointLabel->setFixedSize(10, 60);
            spinboxRow->addWidget(decimalPointLabel, 0);
        }
        QSpinBox *box = new QSpinBox(this);
        box->setFixedSize(35, 60);
        box->setRange(0, 9);
        box->setWrapping(true);
        box->setButtonSymbols(QAbstractSpinBox::NoButtons);
        box->setFont(QFont(activeTheme->standardFont, 24));
        box->setStyleSheet(QSpinBox_Normal);
        box->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        spinboxRow->addWidget(box, 0, Qt::AlignCenter);
        freqBoxes.append(box);
    }
    mainLayout->addLayout(spinboxRow);
    mainLayout->setAlignment(spinboxRow, Qt::AlignHCenter);

    volumeSlider = new QSlider(Qt::Horizontal, this);
    volumeSlider->setRange(0, 100);
    volumeSlider->setFixedWidth(220);
    volumeSlider->setStyleSheet(SliderStyle);
    mainLayout->addWidget(volumeSlider, 0, Qt::AlignHCenter);

    // Preset display (only visible in airband preset mode)
    tunePresetLabel = new QLabel(this);
    tunePresetLabel->setFont(QFont(activeTheme->standardFont, 12, QFont::Bold));
    tunePresetLabel->setAlignment(Qt::AlignCenter);
    tunePresetLabel->setFixedWidth(300);
    mainLayout->addWidget(tunePresetLabel, 0, Qt::AlignHCenter);

    presetLine1 = new QLabel(this);
    presetLine1->setFont(QFont(activeTheme->standardFont, 14, QFont::Bold));
    presetLine1->setAlignment(Qt::AlignCenter);
    presetLine1->setFixedWidth(300);
    mainLayout->addWidget(presetLine1, 0, Qt::AlignHCenter);

    presetLine2 = new QLabel(this);
    presetLine2->setFont(QFont(activeTheme->standardFont, 12));
    presetLine2->setAlignment(Qt::AlignCenter);
    presetLine2->setFixedWidth(300);
    mainLayout->addWidget(presetLine2, 0, Qt::AlignHCenter);

    radioModeLabel = new QLabel(this);
    radioModeLabel->setFont(QFont(activeTheme->standardFont, 16, QFont::Bold));
    radioModeLabel->setAlignment(Qt::AlignCenter);
    radioModeLabel->setFixedWidth(300);
    mainLayout->addWidget(radioModeLabel, 0, Qt::AlignHCenter);

    QLabel *help = new QLabel(
        "<html><body>"
        "<b>Up/Down:</b> Change digit &nbsp; <b>Left/Right:</b> Move<br>"
        "<b>Wheel:</b> Volume &nbsp; <b>A:</b> Toggle FM/Airband<br>"
        "<b>Return:</b> Cycle focus &nbsp; <b>B:</b> Exit"
        "</body></html>", this);
    help->setFont(QFont(activeTheme->standardFont, 10));
    help->setForegroundRole(QPalette::HighlightedText);
    help->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(help, 0, Qt::AlignHCenter);

    statusBar = new StatusBar(this);

    bgLogoLabel = new QLabel(this);
    activeTheme->loadMonochromeIcon(&bgLogo, ":/images/canard_logo.png", QPalette::Shadow);
    bgLogoLabel->resize(bgLogo.size());
    bgLogoLabel->setPixmap(bgLogo);
    bgLogoLabel->lower();
    bgLogoLabel->move(240 - bgLogo.width() / 2, 240 - bgLogo.height() / 2);

    mainLayout->addStretch(1);

    connect(WingletGUI::inst->rtlFm, &RtlFmWorker::availabilityChanged,
            this, &CanardBoard::rtlFmAvailabilityChanged);

    renderTuningInfo();
    applyTune();
}

CanardBoard::~CanardBoard()
{
    if (m_mode == RtlFmWorker::MODE_FM)
        m_savedFmFreq = m_freq;
    else
        m_savedAirbandFreq = m_freq;
    WingletGUI::inst->settings.setCanardLastFmFreq(m_savedFmFreq);
    WingletGUI::inst->settings.setCanardLastAirbandFreq(m_savedAirbandFreq);

    WingletGUI::inst->rtlFm->stop();
    freqBoxes.clear();
}

void CanardBoard::showEvent(QShowEvent *event)
{
    (void) event;
    if (!WingletGUI::inst->rtlFm->isAvailable()) {
        WingletGUI::inst->removeWidgetOnTop(this);
        return;
    }
    renderTuningInfo();
}

// ── Rendering ────────────────────────────────────────────────────────────────

void CanardBoard::renderTuningInfo()
{
    uint numDecimal = (m_mode == RtlFmWorker::MODE_FM)
                      ? RtlFmWorker::FM_DECIMAL_COUNT
                      : RtlFmWorker::AIRBAND_DECIMAL_COUNT;

    if ((uint) frequencyIndex >= DIGITS_BEFORE_DECIMAL + numDecimal)
        frequencyIndex = DIGITS_BEFORE_DECIMAL + (int)numDecimal - 1;

    bool freqFocused   = (widgetIndex == WIDGET_IDX_FREQ);
    bool presetFocused = (widgetIndex == WIDGET_IDX_TUNE_PRESET_SEL);
    bool modeFocused   = (widgetIndex == WIDGET_IDX_RADIO_MODE);

    for (int i = 0; i < (int)(DIGITS_BEFORE_DECIMAL + numDecimal); i++) {
        bool selected = freqFocused && (i == frequencyIndex);
        freqBoxes[i]->setStyleSheet(selected ? QSpinBox_Highlighted : QSpinBox_Normal);
        freqBoxes[i]->setVisible(true);
    }
    for (int i = DIGITS_BEFORE_DECIMAL + (int)numDecimal; i < freqBoxes.size(); i++) {
        freqBoxes[i]->setStyleSheet(QSpinBox_GrayedOut);
        freqBoxes[i]->setVisible(true);
    }

    // Preset controls only visible in airband
    bool inAirband = (m_mode == RtlFmWorker::MODE_AIRBAND);
    tunePresetLabel->setVisible(inAirband);
    presetLine1->setVisible(inAirband);
    presetLine2->setVisible(inAirband);

    if (inAirband) {
        if (presetFocused)
            tunePresetLabel->setText(QString("< Presets %1/%2 >")
                .arg(m_presets.isEmpty() ? 0 : m_presetIdx + 1)
                .arg(m_presets.size()));
        else
            tunePresetLabel->setText(m_presets.isEmpty()
                ? "Presets" : QString("Presets (%1)").arg(m_presets.size()));
        tunePresetLabel->setForegroundRole(presetFocused ? QPalette::Link : QPalette::HighlightedText);
        renderPresetInfo();
    }

    if (m_mode == RtlFmWorker::MODE_FM)
        radioModeLabel->setText(modeFocused ? "< FM Mode" : "FM Mode");
    else
        radioModeLabel->setText(modeFocused ? "Airband Mode >" : "Airband Mode");
    radioModeLabel->setForegroundRole(modeFocused ? QPalette::Link : QPalette::HighlightedText);

    renderFrequencyValue();
    renderVolumeValue();
}

void CanardBoard::renderFrequencyValue()
{
    uint numDecimal = (m_mode == RtlFmWorker::MODE_FM)
                      ? RtlFmWorker::FM_DECIMAL_COUNT
                      : RtlFmWorker::AIRBAND_DECIMAL_COUNT;

    uint32_t scaleFactor = 1;
    for (uint i = 0; i < numDecimal; i++) scaleFactor *= 10;

    uint32_t intPart  = m_freq / scaleFactor;
    uint32_t fracPart = m_freq % scaleFactor;

    freqBoxes[0]->setValue(intPart / 100);
    freqBoxes[1]->setValue((intPart / 10) % 10);
    freqBoxes[2]->setValue(intPart % 10);

    uint32_t frac = fracPart;
    for (int i = (int)numDecimal - 1; i >= 0; i--) {
        if (i < 3)
            freqBoxes[3 + i]->setValue(frac % 10);
        frac /= 10;
    }
}

void CanardBoard::renderPresetInfo()
{
    if (!WingletGUI::inst->nasr->isLoaded()) {
        presetLine1->setText("No radio data");
        presetLine2->setText("Settings > Update Radio Data");
        presetLine1->setForegroundRole(QPalette::HighlightedText);
        presetLine2->setForegroundRole(QPalette::HighlightedText);
        return;
    }
    if (m_presets.isEmpty()) {
        presetLine1->setText("No airports within 20 nm");
        presetLine2->setText("");
        presetLine1->setForegroundRole(QPalette::HighlightedText);
        return;
    }
    const NASRDatabase::AirportFreq &p = m_presets[m_presetIdx];
    presetLine1->setText(QString("%1  %2").arg(p.ident, p.freqType));
    presetLine2->setText(QString("%1 MHz").arg(p.freqMhz, 0, 'f', 3));
    presetLine1->setForegroundRole(QPalette::Text);
    presetLine2->setForegroundRole(QPalette::Text);
}

void CanardBoard::renderVolumeValue()
{
    volumeSlider->setValue(WingletGUI::inst->rtlFm->volume());
}

// ── Preset management ────────────────────────────────────────────────────────

void CanardBoard::loadNearbyPresets()
{
    float lat = (float) WingletGUI::inst->settings.lastLatitude();
    float lon = (float) WingletGUI::inst->settings.lastLongitude();
    m_presets   = WingletGUI::inst->nasr->queryNearby(lat, lon, 20.0f, 30);
    m_presetIdx = 0;
}

void CanardBoard::nextPreset()
{
    if (m_presets.isEmpty()) return;
    m_presetIdx = (m_presetIdx + 1) % m_presets.size();
    renderPresetInfo();
}

void CanardBoard::prevPreset()
{
    if (m_presets.isEmpty()) return;
    m_presetIdx = (m_presetIdx + m_presets.size() - 1) % m_presets.size();
    renderPresetInfo();
}

void CanardBoard::setPresetMode(bool on)
{
    m_presetMode = on;
    if (on && m_mode == RtlFmWorker::MODE_AIRBAND) {
        loadNearbyPresets();
        renderTuningInfo();
    }
}

// ── Tuning helpers ───────────────────────────────────────────────────────────

uint CanardBoard::getFrequencyTuningIncrement()
{
    uint numDecimal = (m_mode == RtlFmWorker::MODE_FM)
                      ? RtlFmWorker::FM_DECIMAL_COUNT
                      : RtlFmWorker::AIRBAND_DECIMAL_COUNT;
    int level = (DIGITS_BEFORE_DECIMAL + (int)numDecimal - 1) - frequencyIndex;
    if (level < 0) return 0;
    uint inc = 1;
    for (int i = 0; i < level; i++) inc *= 10;
    return inc;
}

void CanardBoard::increaseFrequency()
{
    uint32_t maxFreq = (m_mode == RtlFmWorker::MODE_FM)
                       ? RtlFmWorker::FM_MAX_FREQ : RtlFmWorker::AIRBAND_MAX_FREQ;
    uint32_t minFreq = (m_mode == RtlFmWorker::MODE_FM)
                       ? RtlFmWorker::FM_MIN_FREQ : RtlFmWorker::AIRBAND_MIN_FREQ;
    uint inc = getFrequencyTuningIncrement();

    uint numDecimal = (m_mode == RtlFmWorker::MODE_FM)
                      ? RtlFmWorker::FM_DECIMAL_COUNT : RtlFmWorker::AIRBAND_DECIMAL_COUNT;
    uint32_t newFreq;
    if (m_mode == RtlFmWorker::MODE_AIRBAND
            && frequencyIndex == DIGITS_BEFORE_DECIMAL + (int)numDecimal - 1) {
        newFreq = ((m_freq / 5) + 1) * 5;
    } else {
        newFreq = m_freq + inc;
    }

    if (newFreq >= minFreq && newFreq <= maxFreq) {
        m_freq = newFreq;
        applyTune();
        renderFrequencyValue();
    }
}

void CanardBoard::decreaseFrequency()
{
    uint32_t maxFreq = (m_mode == RtlFmWorker::MODE_FM)
                       ? RtlFmWorker::FM_MAX_FREQ : RtlFmWorker::AIRBAND_MAX_FREQ;
    uint32_t minFreq = (m_mode == RtlFmWorker::MODE_FM)
                       ? RtlFmWorker::FM_MIN_FREQ : RtlFmWorker::AIRBAND_MIN_FREQ;
    uint inc = getFrequencyTuningIncrement();

    uint numDecimal = (m_mode == RtlFmWorker::MODE_FM)
                      ? RtlFmWorker::FM_DECIMAL_COUNT : RtlFmWorker::AIRBAND_DECIMAL_COUNT;
    uint32_t newFreq;
    if (m_mode == RtlFmWorker::MODE_AIRBAND
            && frequencyIndex == DIGITS_BEFORE_DECIMAL + (int)numDecimal - 1
            && m_freq >= 5) {
        newFreq = ((m_freq / 5) - 1) * 5;
    } else {
        newFreq = (m_freq >= inc) ? m_freq - inc : 0;
    }

    if (newFreq >= minFreq && newFreq <= maxFreq) {
        m_freq = newFreq;
        applyTune();
        renderFrequencyValue();
    }
}

void CanardBoard::nextFreqBox()
{
    uint numDecimal = (m_mode == RtlFmWorker::MODE_FM)
                      ? RtlFmWorker::FM_DECIMAL_COUNT : RtlFmWorker::AIRBAND_DECIMAL_COUNT;
    if ((uint)frequencyIndex == DIGITS_BEFORE_DECIMAL + numDecimal - 1)
        frequencyIndex = 0;
    else
        frequencyIndex++;
    renderTuningInfo();
}

void CanardBoard::prevFreqBox()
{
    uint numDecimal = (m_mode == RtlFmWorker::MODE_FM)
                      ? RtlFmWorker::FM_DECIMAL_COUNT : RtlFmWorker::AIRBAND_DECIMAL_COUNT;
    if (frequencyIndex == 0)
        frequencyIndex = DIGITS_BEFORE_DECIMAL + (int)numDecimal - 1;
    else
        frequencyIndex--;
    renderTuningInfo();
}

void CanardBoard::setFMMode()
{
    if (m_mode == RtlFmWorker::MODE_FM) return;
    m_savedAirbandFreq = m_freq;
    m_mode = RtlFmWorker::MODE_FM;
    m_freq = m_savedFmFreq;
    m_presetMode = false;
    m_presets.clear();
    // Preset widget skipped in FM — fall back to FREQ widget
    if (widgetIndex == WIDGET_IDX_TUNE_PRESET_SEL)
        widgetIndex = WIDGET_IDX_FREQ;
    applyTune();
    renderTuningInfo();
}

void CanardBoard::setAirbandMode()
{
    if (m_mode == RtlFmWorker::MODE_AIRBAND) return;
    m_savedFmFreq = m_freq;
    m_mode = RtlFmWorker::MODE_AIRBAND;
    m_freq = m_savedAirbandFreq;
    loadNearbyPresets();
    applyTune();
    renderTuningInfo();
}

void CanardBoard::applyTune()
{
    WingletGUI::inst->rtlFm->tune(m_freq, m_mode);
}

void CanardBoard::changeVolume(int delta)
{
    int newVol = qBound(0, volumeSlider->value() + delta, 100);
    volumeSlider->setValue(newVol);
    WingletGUI::inst->rtlFm->setVolume(newVol);
}

// ── Input handling ───────────────────────────────────────────────────────────

void CanardBoard::keyPressEvent(QKeyEvent *ev)
{
    bool inAirband = (m_mode == RtlFmWorker::MODE_AIRBAND);

    switch (ev->key()) {
    case Qt::Key_Up:
        if      (widgetIndex == WIDGET_IDX_FREQ)             increaseFrequency();
        else if (widgetIndex == WIDGET_IDX_TUNE_PRESET_SEL)  prevPreset();
        break;
    case Qt::Key_Down:
        if      (widgetIndex == WIDGET_IDX_FREQ)             decreaseFrequency();
        else if (widgetIndex == WIDGET_IDX_TUNE_PRESET_SEL)  nextPreset();
        break;
    case Qt::Key_Left:
        if      (widgetIndex == WIDGET_IDX_FREQ)             prevFreqBox();
        else if (widgetIndex == WIDGET_IDX_RADIO_MODE)       setAirbandMode();
        break;
    case Qt::Key_Right:
        if      (widgetIndex == WIDGET_IDX_FREQ)             nextFreqBox();
        else if (widgetIndex == WIDGET_IDX_RADIO_MODE)       setFMMode();
        break;
    case Qt::Key_Return:
        // Preset: select highlighted entry and tune to it
        if (widgetIndex == WIDGET_IDX_TUNE_PRESET_SEL && !m_presets.isEmpty()) {
            const NASRDatabase::AirportFreq &p = m_presets[m_presetIdx];
            m_freq = (uint32_t) qRound(p.freqMhz * 1000.0f);
            applyTune();
            renderFrequencyValue();
            // Switch focus to freq tuner so user can see/adjust
            widgetIndex = WIDGET_IDX_FREQ;
            renderTuningInfo();
        } else {
            // Cycle widget focus: skip TUNE_PRESET_SEL in FM mode
            do {
                widgetIndex = (widgetIndex + 1) % WIDGET_IDX_COUNT;
            } while (!inAirband && widgetIndex == WIDGET_IDX_TUNE_PRESET_SEL);
            renderTuningInfo();
        }
        break;
    case Qt::Key_A:
        if (m_mode == RtlFmWorker::MODE_FM) setAirbandMode();
        else                                setFMMode();
        break;
    default:
        ev->ignore();
        break;
    }
}

void CanardBoard::wheelEvent(QWheelEvent *event)
{
    QPoint delta = event->angleDelta();
    if (!delta.isNull())
        changeVolume(delta.y() / 30);
}

// ── Slots ────────────────────────────────────────────────────────────────────

void CanardBoard::rtlFmAvailabilityChanged(bool available)
{
    if (!available && isVisible())
        WingletGUI::inst->removeWidgetOnTop(this);
}

} // namespace WingletUI
