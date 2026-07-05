#include "canardboard.h"
#include "wingletgui.h"
#include "winglet-ui/theme.h"
#include "winglet-ui/windowcore/selectorbox.h"
#include "winglet-ui/windowcore/circularkeyboard.h"
#include "winglet-ui/model/radiopresetsmodel.h"
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
    m_savedHam2mFreq   = WingletGUI::inst->settings.canardLastHam2mFreq();
    m_freq = m_savedFmFreq;
    m_mode = RtlFmWorker::MODE_FM;

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 38, 10, 10);
    mainLayout->setSpacing(6);

    QLabel *title = new QLabel("Radio Tuner", this);
    title->setFont(QFont(activeTheme->titleFont, 20));
    title->setForegroundRole(QPalette::Text);
    title->setAutoFillBackground(true);
    mainLayout->addWidget(title, 0, Qt::AlignHCenter);

    // Band row
    bandLabel = new QLabel(this);
    bandLabel->setFont(QFont(activeTheme->standardFont, 16, QFont::Bold));
    bandLabel->setAlignment(Qt::AlignCenter);
    bandLabel->setFixedWidth(320);
    mainLayout->addWidget(bandLabel, 0, Qt::AlignHCenter);

    // Frequency digit cells: 3 integer digits + decimal + up to 3 fractional
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
        box->setFocusPolicy(Qt::NoFocus);
        spinboxRow->addWidget(box, 0, Qt::AlignCenter);
        freqBoxes.append(box);
    }
    mainLayout->addLayout(spinboxRow);
    mainLayout->setAlignment(spinboxRow, Qt::AlignHCenter);

    unitLabel = new QLabel("MHz", this);
    unitLabel->setFont(QFont(activeTheme->standardFont, 11));
    unitLabel->setForegroundRole(QPalette::HighlightedText);
    unitLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(unitLabel, 0, Qt::AlignHCenter);

    // Preset stepper row
    presetLabel = new QLabel(this);
    presetLabel->setFont(QFont(activeTheme->standardFont, 14, QFont::Bold));
    presetLabel->setAlignment(Qt::AlignCenter);
    presetLabel->setFixedWidth(340);
    mainLayout->addWidget(presetLabel, 0, Qt::AlignHCenter);

    allPresetsLabel = new QLabel("All Presets…", this);
    allPresetsLabel->setFont(QFont(activeTheme->standardFont, 13));
    allPresetsLabel->setAlignment(Qt::AlignCenter);
    allPresetsLabel->setFixedWidth(340);
    mainLayout->addWidget(allPresetsLabel, 0, Qt::AlignHCenter);

    // Volume row
    volumeLabel = new QLabel(this);
    volumeLabel->setFont(QFont(activeTheme->standardFont, 13, QFont::Bold));
    volumeLabel->setAlignment(Qt::AlignCenter);
    volumeLabel->setFixedWidth(320);
    mainLayout->addWidget(volumeLabel, 0, Qt::AlignHCenter);

    volumeSlider = new QSlider(Qt::Horizontal, this);
    volumeSlider->setRange(0, 100);
    volumeSlider->setFixedWidth(220);
    volumeSlider->setStyleSheet(SliderStyle);
    volumeSlider->setFocusPolicy(Qt::NoFocus);
    mainLayout->addWidget(volumeSlider, 0, Qt::AlignHCenter);

    // Squelch row
    squelchLabel = new QLabel(this);
    squelchLabel->setFont(QFont(activeTheme->standardFont, 13, QFont::Bold));
    squelchLabel->setAlignment(Qt::AlignCenter);
    squelchLabel->setFixedWidth(320);
    mainLayout->addWidget(squelchLabel, 0, Qt::AlignHCenter);

    // Save row
    saveLabel = new QLabel("★ Save Preset", this);
    saveLabel->setFont(QFont(activeTheme->standardFont, 13));
    saveLabel->setAlignment(Qt::AlignCenter);
    saveLabel->setFixedWidth(340);
    mainLayout->addWidget(saveLabel, 0, Qt::AlignHCenter);

    // Status line
    statusLabel = new QLabel(this);
    statusLabel->setFont(QFont(activeTheme->standardFont, 11, QFont::Bold));
    statusLabel->setForegroundRole(QPalette::HighlightedText);
    statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(statusLabel, 0, Qt::AlignHCenter);

    helpLabel = new QLabel(
        "<html><body><b>Up/Down:</b> Move &nbsp; <b>A:</b> Select"
        " &nbsp; <b>B:</b> Back</body></html>", this);
    helpLabel->setFont(QFont(activeTheme->standardFont, 9));
    helpLabel->setForegroundRole(QPalette::HighlightedText);
    helpLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(helpLabel, 0, Qt::AlignHCenter);

    mainLayout->addStretch(1);

    statusBar = new StatusBar(this);

    bgLogoLabel = new QLabel(this);
    activeTheme->loadMonochromeIcon(&bgLogo, ":/images/canard_logo.png", QPalette::Shadow);
    bgLogoLabel->resize(bgLogo.size());
    bgLogoLabel->setPixmap(bgLogo);
    bgLogoLabel->lower();
    bgLogoLabel->move(240 - bgLogo.width() / 2, 240 - bgLogo.height() / 2);

    connect(WingletGUI::inst->rtlFm, &RtlFmWorker::availabilityChanged,
            this, &CanardBoard::rtlFmAvailabilityChanged);
    connect(WingletGUI::inst->presets, &RadioPresets::presetsChanged,
            this, &CanardBoard::presetsChanged);

    statusTimer = new QTimer(this);
    statusTimer->setInterval(1000);
    connect(statusTimer, &QTimer::timeout, this, [this]{ updateStatus(); });
    statusTimer->start();

    WingletGUI::inst->rtlFm->setSquelch(currentBandSquelch());
    rebuildBandPresets();
    render();
    applyTune();
}

CanardBoard::~CanardBoard()
{
    switch (m_mode) {
    case RtlFmWorker::MODE_FM:      m_savedFmFreq      = m_freq; break;
    case RtlFmWorker::MODE_AIRBAND: m_savedAirbandFreq = m_freq; break;
    case RtlFmWorker::MODE_HAM2M:   m_savedHam2mFreq   = m_freq; break;
    }
    WingletGUI::inst->settings.setCanardLastFmFreq(m_savedFmFreq);
    WingletGUI::inst->settings.setCanardLastAirbandFreq(m_savedAirbandFreq);
    WingletGUI::inst->settings.setCanardLastHam2mFreq(m_savedHam2mFreq);

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

    switch (m_actionState) {
    case AS_BROWSER_RETURN:
        m_actionState = AS_NORMAL;
        switch (m_pendingAction) {
        case RadioPresetsModel::TUNE_PRESET:
            if (m_pendingFreqKhz) {
                m_freq = RadioPresets::khzToUiFreq(m_mode, m_pendingFreqKhz);
                applyTune();
            }
            break;
        case RadioPresetsModel::ADD_FAVORITE:
            if (m_pendingFreqKhz)
                WingletGUI::inst->presets->add(m_mode, m_pendingName, m_pendingFreqKhz);
            break;
        case RadioPresetsModel::DELETE_FAVORITE:
            if (m_pendingPresetId >= 0)
                WingletGUI::inst->presets->remove(m_pendingPresetId);
            break;
        case RadioPresetsModel::RENAME_FAVORITE:
            if (m_pendingPresetId >= 0) {
                openRenameFlow(m_pendingPresetId, m_pendingName);
                return;  // keyboard now on top; rename applied on its return
            }
            break;
        }
        m_pendingAction = 0;
        break;

    case AS_SAVE_KBD_RETURN:
        m_actionState = AS_NORMAL;
        if (!m_kbdResult.isEmpty()) {
            WingletGUI::inst->presets->add(m_mode, m_kbdResult, m_saveFreqKhz);
            m_actionState = AS_MSGBOX_RETURN;
            WingletGUI::inst->showMessageBox("Preset saved.", "Radio Tuner");
            return;
        }
        break;

    case AS_RENAME_KBD_RETURN:
        m_actionState = AS_NORMAL;
        if (!m_kbdResult.isEmpty() && m_pendingPresetId >= 0)
            WingletGUI::inst->presets->rename(m_pendingPresetId, m_kbdResult);
        break;

    case AS_MSGBOX_RETURN:
        m_actionState = AS_NORMAL;
        break;

    default:
        break;
    }

    rebuildBandPresets();
    render();
}

// ── Navigation ───────────────────────────────────────────────────────────────

void CanardBoard::buildActiveRows()
{
    m_activeRows.clear();
    m_activeRows << ROW_BAND << ROW_FREQ;
    if (!m_bandPresets.isEmpty())
        m_activeRows << ROW_PRESET;
    m_activeRows << ROW_ALL_PRESETS << ROW_VOLUME;
    if (m_mode != RtlFmWorker::MODE_FM)
        m_activeRows << ROW_SQUELCH;
    m_activeRows << ROW_SAVE;

    if (m_selRow < 0)
        m_selRow = 0;
    if (m_selRow >= m_activeRows.size())
        m_selRow = m_activeRows.size() - 1;
}

CanardBoard::Row CanardBoard::selectedRow() const
{
    if (m_selRow < 0 || m_selRow >= m_activeRows.size())
        return ROW_BAND;
    return m_activeRows[m_selRow];
}

void CanardBoard::moveSelection(int dir)
{
    m_selRow = qBound(0, m_selRow + dir, m_activeRows.size() - 1);
    render();
}

void CanardBoard::activateRow()
{
    switch (selectedRow()) {
    case ROW_BAND:
        cycleMode(+1);
        break;
    case ROW_FREQ:
        m_edit = EDIT_FREQ;
        frequencyIndex = 0;
        render();
        break;
    case ROW_PRESET:
        if (!m_bandPresets.isEmpty()) {
            m_edit = EDIT_PRESET;
            selectPreset(m_presetSel, true);
        }
        break;
    case ROW_ALL_PRESETS:
        openPresetBrowser();
        break;
    case ROW_VOLUME:
        m_edit = EDIT_VOLUME;
        render();
        break;
    case ROW_SQUELCH:
        m_edit = EDIT_SQUELCH;
        render();
        break;
    case ROW_SAVE:
        openSavePresetFlow();
        break;
    default:
        break;
    }
}

void CanardBoard::handleEditKey(QKeyEvent *ev)
{
    switch (m_edit) {
    case EDIT_FREQ:
        switch (ev->key()) {
        case Qt::Key_Up:    increaseFrequency(); break;
        case Qt::Key_Down:  decreaseFrequency(); break;
        case Qt::Key_Left:  prevFreqBox();       break;
        case Qt::Key_Right: nextFreqBox();       break;
        case Qt::Key_A:
        case Qt::Key_Return:
            // Advance the digit cursor; exit edit after the last digit
            if ((uint) frequencyIndex >= DIGITS_BEFORE_DECIMAL + numDecimals() - 1) {
                m_edit = EDIT_NONE;
                frequencyIndex = 0;
            } else {
                frequencyIndex++;
            }
            render();
            break;
        default:
            ev->ignore();
            break;
        }
        break;

    case EDIT_VOLUME:
        switch (ev->key()) {
        case Qt::Key_Up:    changeVolume(+5); break;
        case Qt::Key_Down:  changeVolume(-5); break;
        case Qt::Key_A:
        case Qt::Key_Return: m_edit = EDIT_NONE; render(); break;
        default: ev->ignore(); break;
        }
        break;

    case EDIT_SQUELCH:
        switch (ev->key()) {
        case Qt::Key_Up:    changeSquelch(+5); break;
        case Qt::Key_Down:  changeSquelch(-5); break;
        case Qt::Key_A:
        case Qt::Key_Return: m_edit = EDIT_NONE; render(); break;
        default: ev->ignore(); break;
        }
        break;

    case EDIT_PRESET:
        switch (ev->key()) {
        case Qt::Key_Up:    selectPreset(m_presetSel - 1, true); break;
        case Qt::Key_Down:  selectPreset(m_presetSel + 1, true); break;
        case Qt::Key_A:
        case Qt::Key_Return: m_edit = EDIT_NONE; render(); break;
        default: ev->ignore(); break;
        }
        break;

    default:
        ev->ignore();
        break;
    }
}

void CanardBoard::keyPressEvent(QKeyEvent *ev)
{
    if (m_edit != EDIT_NONE) {
        handleEditKey(ev);
        return;
    }

    switch (ev->key()) {
    case Qt::Key_Up:     moveSelection(-1); break;
    case Qt::Key_Down:   moveSelection(+1); break;
    case Qt::Key_A:
    case Qt::Key_Return: activateRow();     break;
    default:
        ev->ignore();  // B / back is handled globally by WingletGUI
        break;
    }
}

void CanardBoard::wheelEvent(QWheelEvent *event)
{
    QPoint delta = event->angleDelta();
    if (!delta.isNull())
        changeVolume(delta.y() / 30);
}

// ── Rendering ────────────────────────────────────────────────────────────────

void CanardBoard::render()
{
    buildActiveRows();

    Row sel = selectedRow();
    auto rowText = [&](Row r, const QString &text) -> QString {
        bool selected = (sel == r);
        return selected ? QString("‹ %1 ›").arg(text) : text;
    };
    auto rowColor = [&](QLabel *lbl, Row r) {
        lbl->setForegroundRole(sel == r ? QPalette::Link : QPalette::HighlightedText);
    };

    // Band
    QString bandName;
    switch (m_mode) {
    case RtlFmWorker::MODE_FM:      bandName = "FM";      break;
    case RtlFmWorker::MODE_AIRBAND: bandName = "Airband"; break;
    case RtlFmWorker::MODE_HAM2M:   bandName = "2m";      break;
    }
    bandLabel->setText(QString("Band   %1").arg(rowText(ROW_BAND, bandName)));
    rowColor(bandLabel, ROW_BAND);

    // Frequency cells
    renderFreqCells(sel == ROW_FREQ, m_edit == EDIT_FREQ);
    unitLabel->setForegroundRole(sel == ROW_FREQ ? QPalette::Link : QPalette::HighlightedText);

    // Preset stepper
    bool presetActive = m_activeRows.contains(ROW_PRESET);
    presetLabel->setVisible(presetActive);
    if (presetActive) {
        const PresetEntry &e = m_bandPresets[qBound(0, m_presetSel, m_bandPresets.size() - 1)];
        QString star = e.isFavorite ? "★ " : "";
        QString body = QString("%1%2  %3").arg(star, e.name, e.detail);
        presetLabel->setText(QString("Preset  %1").arg(rowText(ROW_PRESET, body)));
        rowColor(presetLabel, ROW_PRESET);
    }

    // All presets
    allPresetsLabel->setText(rowText(ROW_ALL_PRESETS, "All Presets…"));
    rowColor(allPresetsLabel, ROW_ALL_PRESETS);

    // Volume
    int vol = WingletGUI::inst->rtlFm->volume();
    volumeLabel->setText(QString("Volume  %1").arg(rowText(ROW_VOLUME, QString("%1%").arg(vol))));
    rowColor(volumeLabel, ROW_VOLUME);
    volumeSlider->setValue(vol);

    // Squelch
    bool squelchActive = m_activeRows.contains(ROW_SQUELCH);
    squelchLabel->setVisible(squelchActive);
    if (squelchActive) {
        squelchLabel->setText(QString("Squelch %1")
            .arg(rowText(ROW_SQUELCH, QString::number(currentBandSquelch()))));
        rowColor(squelchLabel, ROW_SQUELCH);
    }

    // Save
    saveLabel->setText(rowText(ROW_SAVE, "★ Save Preset"));
    rowColor(saveLabel, ROW_SAVE);

    renderFrequencyValue();
    updateStatus();
}

void CanardBoard::renderFreqCells(bool rowSelected, bool editing)
{
    uint numDecimal = numDecimals();
    int lastDigit = DIGITS_BEFORE_DECIMAL + (int) numDecimal - 1;
    if (frequencyIndex > lastDigit)
        frequencyIndex = lastDigit;

    for (int i = 0; i < DIGITS_BEFORE_DECIMAL + (int) numDecimal; i++) {
        bool lit;
        if (editing)
            lit = (i == frequencyIndex);
        else
            lit = rowSelected;
        freqBoxes[i]->setStyleSheet(lit ? QSpinBox_Highlighted : QSpinBox_Normal);
        freqBoxes[i]->setVisible(true);
    }
    for (int i = DIGITS_BEFORE_DECIMAL + (int) numDecimal; i < freqBoxes.size(); i++) {
        freqBoxes[i]->setStyleSheet(QSpinBox_GrayedOut);
        freqBoxes[i]->setVisible(true);
    }
}

void CanardBoard::renderFrequencyValue()
{
    uint numDecimal = numDecimals();

    uint32_t scaleFactor = 1;
    for (uint i = 0; i < numDecimal; i++) scaleFactor *= 10;

    uint32_t intPart  = m_freq / scaleFactor;
    uint32_t fracPart = m_freq % scaleFactor;

    freqBoxes[0]->setValue(intPart / 100);
    freqBoxes[1]->setValue((intPart / 10) % 10);
    freqBoxes[2]->setValue(intPart % 10);

    uint32_t frac = fracPart;
    for (int i = (int) numDecimal - 1; i >= 0; i--) {
        if (i < 3)
            freqBoxes[3 + i]->setValue(frac % 10);
        frac /= 10;
    }
}

void CanardBoard::updateStatus()
{
    QString s;
    if (!WingletGUI::inst->rtlFm->isAvailable())
        s = "✕ NO DONGLE";
    else if (WingletGUI::inst->rtlFm->isPlaying())
        s = "● LIVE      RTL #1";
    else
        s = "○ STARTING   RTL #1";
    statusLabel->setText(s);
}

// ── Presets ──────────────────────────────────────────────────────────────────

void CanardBoard::rebuildBandPresets()
{
    m_bandPresets.clear();

    const auto favorites = WingletGUI::inst->presets->presetsForBand(m_mode);
    for (const auto &p : favorites) {
        PresetEntry e;
        e.name       = p.name;
        e.detail     = QString::number(p.freqKhz / 1000.0, 'f', 3);
        e.freqKhz    = p.freqKhz;
        e.isFavorite = true;
        e.presetId   = p.id;
        m_bandPresets.append(e);
    }

    if (m_mode == RtlFmWorker::MODE_AIRBAND && WingletGUI::inst->nasr->isLoaded()) {
        float lat = (float) WingletGUI::inst->settings.lastLatitude();
        float lon = (float) WingletGUI::inst->settings.lastLongitude();
        const auto nearby = WingletGUI::inst->nasr->queryNearby(lat, lon, 20.0f, 30);
        for (const auto &a : nearby) {
            PresetEntry e;
            e.name       = QString("%1 %2").arg(a.ident, a.freqType);
            e.detail     = QString::number(a.freqMhz, 'f', 3);
            e.freqKhz    = (uint32_t) qRound(a.freqMhz * 1000.0f);
            e.isFavorite = false;
            e.presetId   = -1;
            m_bandPresets.append(e);
        }
    }

    m_presetSel = qBound(0, m_presetSel, qMax(0, m_bandPresets.size() - 1));
}

void CanardBoard::selectPreset(int idx, bool tune)
{
    if (m_bandPresets.isEmpty())
        return;
    m_presetSel = qBound(0, idx, m_bandPresets.size() - 1);
    if (tune) {
        m_freq = RadioPresets::khzToUiFreq(m_mode, m_bandPresets[m_presetSel].freqKhz);
        applyTune();
    }
    render();
}

void CanardBoard::openPresetBrowser()
{
    m_pendingAction = 0;  // no-op if the user backs out without selecting
    auto model = new RadioPresetsModel(m_mode);
    if (model->rowCount() == 0) {
        delete model;
        m_actionState = AS_MSGBOX_RETURN;
        WingletGUI::inst->showMessageBox(
            "No presets for this band yet.\nUse Save Preset to add one.", "No Presets");
        return;
    }
    m_actionState = AS_BROWSER_RETURN;
    auto selector = new SelectorBox(WingletGUI::inst, model, true);
    model->setParent(selector);
    connect(selector, SIGNAL(indexSelected(QModelIndex)), this, SLOT(selectorIndexSelected(QModelIndex)));
    WingletGUI::inst->addWidgetOnTop(selector);
}

void CanardBoard::openSavePresetFlow()
{
    m_kbdResult.clear();
    m_saveFreqKhz = RadioPresets::uiFreqToKhz(m_mode, m_freq);
    QString freqStr = QString::number(m_saveFreqKhz / 1000.0, 'f', numDecimals());

    m_actionState = AS_SAVE_KBD_RETURN;
    CircularKeyboard *kbd = new CircularKeyboard(CircularKeyboard::fullKeyboard, WingletGUI::inst);
    kbd->setTitle("Save Preset");
    kbd->setPrompt(QString("Name for %1 MHz:").arg(freqStr));
    kbd->setValue(freqStr);
    kbd->setMaxLength(16);
    connect(kbd, SIGNAL(entryComplete(QString)), this, SLOT(kbdTextEntered(QString)));
    WingletGUI::inst->addWidgetOnTop(kbd);
}

void CanardBoard::openRenameFlow(int presetId, const QString &currentName)
{
    m_kbdResult.clear();
    m_pendingPresetId = presetId;
    m_actionState = AS_RENAME_KBD_RETURN;
    CircularKeyboard *kbd = new CircularKeyboard(CircularKeyboard::fullKeyboard, WingletGUI::inst);
    kbd->setTitle("Rename Preset");
    kbd->setPrompt("New name:");
    kbd->setValue(currentName);
    kbd->setMaxLength(16);
    connect(kbd, SIGNAL(entryComplete(QString)), this, SLOT(kbdTextEntered(QString)));
    WingletGUI::inst->addWidgetOnTop(kbd);
}

// ── Tuning helpers ───────────────────────────────────────────────────────────

uint CanardBoard::numDecimals() const
{
    switch (m_mode) {
    case RtlFmWorker::MODE_FM:      return RtlFmWorker::FM_DECIMAL_COUNT;
    case RtlFmWorker::MODE_AIRBAND: return RtlFmWorker::AIRBAND_DECIMAL_COUNT;
    case RtlFmWorker::MODE_HAM2M:   return RtlFmWorker::HAM2M_DECIMAL_COUNT;
    }
    return RtlFmWorker::FM_DECIMAL_COUNT;
}

uint32_t CanardBoard::bandMinFreq() const
{
    switch (m_mode) {
    case RtlFmWorker::MODE_FM:      return RtlFmWorker::FM_MIN_FREQ;
    case RtlFmWorker::MODE_AIRBAND: return RtlFmWorker::AIRBAND_MIN_FREQ;
    case RtlFmWorker::MODE_HAM2M:   return RtlFmWorker::HAM2M_MIN_FREQ;
    }
    return RtlFmWorker::FM_MIN_FREQ;
}

uint32_t CanardBoard::bandMaxFreq() const
{
    switch (m_mode) {
    case RtlFmWorker::MODE_FM:      return RtlFmWorker::FM_MAX_FREQ;
    case RtlFmWorker::MODE_AIRBAND: return RtlFmWorker::AIRBAND_MAX_FREQ;
    case RtlFmWorker::MODE_HAM2M:   return RtlFmWorker::HAM2M_MAX_FREQ;
    }
    return RtlFmWorker::FM_MAX_FREQ;
}

uint CanardBoard::getFrequencyTuningIncrement()
{
    int level = (DIGITS_BEFORE_DECIMAL + (int) numDecimals() - 1) - frequencyIndex;
    if (level < 0) return 0;
    uint inc = 1;
    for (int i = 0; i < level; i++) inc *= 10;
    return inc;
}

void CanardBoard::increaseFrequency()
{
    uint inc = getFrequencyTuningIncrement();

    uint32_t newFreq;
    if (m_mode != RtlFmWorker::MODE_FM
            && frequencyIndex == DIGITS_BEFORE_DECIMAL + (int) numDecimals() - 1) {
        newFreq = ((m_freq / 5) + 1) * 5;  // kHz bands step 5 kHz on the last digit
    } else {
        newFreq = m_freq + inc;
    }

    if (newFreq >= bandMinFreq() && newFreq <= bandMaxFreq()) {
        m_freq = newFreq;
        applyTune();
        render();
    }
}

void CanardBoard::decreaseFrequency()
{
    uint inc = getFrequencyTuningIncrement();

    uint32_t newFreq;
    if (m_mode != RtlFmWorker::MODE_FM
            && frequencyIndex == DIGITS_BEFORE_DECIMAL + (int) numDecimals() - 1
            && m_freq >= 5) {
        newFreq = ((m_freq / 5) - 1) * 5;
    } else {
        newFreq = (m_freq >= inc) ? m_freq - inc : 0;
    }

    if (newFreq >= bandMinFreq() && newFreq <= bandMaxFreq()) {
        m_freq = newFreq;
        applyTune();
        render();
    }
}

void CanardBoard::nextFreqBox()
{
    if ((uint) frequencyIndex == DIGITS_BEFORE_DECIMAL + numDecimals() - 1)
        frequencyIndex = 0;
    else
        frequencyIndex++;
    render();
}

void CanardBoard::prevFreqBox()
{
    if (frequencyIndex == 0)
        frequencyIndex = DIGITS_BEFORE_DECIMAL + (int) numDecimals() - 1;
    else
        frequencyIndex--;
    render();
}

void CanardBoard::setMode(RtlFmWorker::Mode newMode)
{
    if (m_mode == newMode) return;

    switch (m_mode) {
    case RtlFmWorker::MODE_FM:      m_savedFmFreq      = m_freq; break;
    case RtlFmWorker::MODE_AIRBAND: m_savedAirbandFreq = m_freq; break;
    case RtlFmWorker::MODE_HAM2M:   m_savedHam2mFreq   = m_freq; break;
    }
    m_mode = newMode;
    switch (newMode) {
    case RtlFmWorker::MODE_FM:      m_freq = m_savedFmFreq;      break;
    case RtlFmWorker::MODE_AIRBAND: m_freq = m_savedAirbandFreq; break;
    case RtlFmWorker::MODE_HAM2M:   m_freq = m_savedHam2mFreq;   break;
    }

    frequencyIndex = 0;
    m_presetSel    = 0;
    rebuildBandPresets();

    WingletGUI::inst->rtlFm->setSquelch(currentBandSquelch());
    applyTune();
    render();
}

void CanardBoard::cycleMode(int direction)
{
    static const RtlFmWorker::Mode order[] = {
        RtlFmWorker::MODE_FM, RtlFmWorker::MODE_AIRBAND, RtlFmWorker::MODE_HAM2M
    };
    int idx = 0;
    for (int i = 0; i < 3; i++) {
        if (order[i] == m_mode) { idx = i; break; }
    }
    setMode(order[(idx + direction + 3) % 3]);
}

void CanardBoard::applyTune()
{
    WingletGUI::inst->rtlFm->tune(m_freq, m_mode);
}

// ── Squelch / volume ──────────────────────────────────────────────────────────

int CanardBoard::currentBandSquelch() const
{
    switch (m_mode) {
    case RtlFmWorker::MODE_AIRBAND: return WingletGUI::inst->settings.canardSquelchAm();
    case RtlFmWorker::MODE_HAM2M:   return WingletGUI::inst->settings.canardSquelchNfm();
    default:                        return 0;  // FM broadcast is always open
    }
}

void CanardBoard::changeSquelch(int delta)
{
    int level = qBound(0, currentBandSquelch() + delta, RtlFmWorker::SQUELCH_MAX);
    if (m_mode == RtlFmWorker::MODE_AIRBAND)
        WingletGUI::inst->settings.setCanardSquelchAm(level);
    else if (m_mode == RtlFmWorker::MODE_HAM2M)
        WingletGUI::inst->settings.setCanardSquelchNfm(level);
    WingletGUI::inst->rtlFm->setSquelch(level);
    render();
}

void CanardBoard::changeVolume(int delta)
{
    int newVol = qBound(0, WingletGUI::inst->rtlFm->volume() + delta, 100);
    WingletGUI::inst->rtlFm->setVolume(newVol);
    render();
}

// ── Slots ────────────────────────────────────────────────────────────────────

void CanardBoard::rtlFmAvailabilityChanged(bool available)
{
    if (!available && isVisible())
        WingletGUI::inst->removeWidgetOnTop(this);
}

void CanardBoard::presetsChanged()
{
    rebuildBandPresets();
    render();
}

void CanardBoard::selectorIndexSelected(QModelIndex index)
{
    QVariant actionVariant = index.data(Qt::UserRole);
    if (actionVariant.type() != QVariant::Int)
        return;
    m_pendingAction   = actionVariant.toInt();
    m_pendingFreqKhz  = index.data(RadioPresetsModel::FreqKhzRole).toUInt();
    m_pendingPresetId = index.data(RadioPresetsModel::PresetIdRole).toInt();
    m_pendingName     = index.data(RadioPresetsModel::NameRole).toString();
}

void CanardBoard::kbdTextEntered(QString val)
{
    m_kbdResult = val;  // applied on the tuner's next showEvent
}

} // namespace WingletUI
