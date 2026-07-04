#include "settingsmenu.h"

#include "wingletgui.h"
#include <QProcess>
#include "winglet-ui/theme.h"
#include "winglet-ui/model/knownnetworksmodel.h"
#include "winglet-ui/model/wifiscanmodel.h"
#include "winglet-ui/model/btscanmodel.h"
#include "winglet-ui/model/pairedbtdevicesmodel.h"
#include "winglet-ui/settings/appsettingspropentry.h"
#include "winglet-ui/settings/rootpasswordsetting.h"
#include "winglet-ui/windowcore/circularkeyboard.h"
#include "winglet-ui/windowcore/infoviewer.h"
#include "winglet-ui/windowcore/selectorbox.h"
#include "winglet-ui/windowcore/messagebox.h"
#include <QFocusEvent>

#define DISCORD_USERNAME_FILE "/tmp/attestation_username"

const static QRegExp discord_regexp("^(?=.{2,32}$)(?!(?:everyone|here)$)\\.?[a-z0-9_]+(?:\\.[a-z0-9_]+)*\\.?$");

namespace WingletUI {

class WifiPSKValidator: public QValidator {
    using QValidator::QValidator;

    State validate(QString & msg, int & posOut) const override {
        posOut = 0;

        size_t msgLen = msg.length();
        if (msgLen == 0 || (msgLen >= 8 && msgLen <= 63))
            return Acceptable;
        else if (msgLen < 8)
            return Intermediate;
        else
            return Invalid;
    }
};

SettingsMenu::SettingsMenu(QWidget *parent, bool isCanardSettings)
    : QWidget{parent}, avLogoLabel(new QLabel(this)),
      statusBar(new StatusBar(this)), menuWidget(new ScrollableMenu(this)),
      isCanardSettings(isCanardSettings)
{
    menuWidget->setMenuWrap(false);
    menuWidget->setMaxVisibleItems(9);
    menuWidget->setFontSizes(22, 14, 1);
    menuWidget->setGeometry(0, 0, 480, 480);

    menuModel = new SettingsMenuModel(WingletGUI::inst->settings.settingsEntryRoot(), this);
    menuWidget->setModel(menuModel);
    connect(menuWidget, SIGNAL(itemSelected(QModelIndex)), this, SLOT(menuItemSelected(QModelIndex)));
    connect(menuWidget, SIGNAL(menuExit()), this, SLOT(menuExitRequested()));

    // Create AV Logo in background
    activeTheme->renderBgAvLogo(avLogoLabel);
    // Subscribe to color palette change events so the bg logo is updated if dark mode is changed
    connect(activeTheme, SIGNAL(colorPaletteChanged()), this, SLOT(colorPaletteChanged()));

    // Status bar should appear on top
    statusBar->raise();

    // Bluetooth pairing progress reporting (signals come from the worker thread)
    connect(WingletGUI::inst->btMon, SIGNAL(pairingDisplayCode(QString)),
            this, SLOT(btPairingCodeReceived(QString)));
    connect(WingletGUI::inst->btMon, SIGNAL(pairingComplete(bool,QString)),
            this, SLOT(btPairingComplete(bool,QString)));
}

void SettingsMenu::colorPaletteChanged()
{
    // Rerender the AV badge logo to update palette
    activeTheme->renderBgAvLogo(avLogoLabel);
}

void SettingsMenu::showEvent(QShowEvent *event)
{
    (void) event;

    menuWidget->setShowShrinkFromOutside(true);

    if (WingletGUI::inst->settings.rebootNeeded && actionState != ACTION_STATE_REBOOT) {
        WingletGUI::inst->showMessageBox("A reboot is required after setting this item.\nPress the knob/A to reboot.", "Reboot Required", "Reboot");
        actionState = ACTION_STATE_REBOOT;
        return;
    }

    // Change menu fly in direction depending on what we're returning from
    switch (actionState) {
    case ACTION_STATE_MENU_OPEN:
        menuWidget->setShowShrinkFromOutside(false);
        break;

    case ACTION_STATE_REBOOT:
        QApplication::exit(EXIT_CODE_REBOOT);
        return;

    case ACTION_STATE_SELECTOR_RETURN:
    case ACTION_STATE_TEXT_RETURN:
        // Need to clear the active settings item being edited
        activeItem = nullptr;
        break;

    case ACTION_STATE_GET_DISCORD_USERNAME:
        // If we got the discord username, show the message box
        if (QFile::exists(DISCORD_USERNAME_FILE)) {
            actionState = ACTION_STATE_SHOW_DISCORD_MSGBOX;
            WingletGUI::inst->showMessageBox("On the next screen you will see a QR Code... "
                                             "The code contains an email template with your username and a "
                                             "verification code required to join. Please send this email out "
                                             "and you will be then added to the private discord channel. "
                                             "Note that your discord username will be tied to your badge.\n\n"
                                             "This code can be scanned with iOS Camera or Google Lens.",
                                             "Private Discord Code", "Continue");
            return;
        }
        break;

    case ACTION_STATE_SHOW_DISCORD_MSGBOX:
        // Just returned from the join discord msgbox, we're now good to quit the application to the Python script
        QApplication::exit(EXIT_CODE_REGISTER_DISCORD);
        return;

    case ACTION_STATE_WIFI_SCAN_RETURN:
    case ACTION_STATE_WIFI_SSID_RETURN: {
        // Only advance to next step if an SSID was selected
        if (enteredWifiSSID.length() > 0) {
            bool manuallyEntered = actionState == SettingsMenu::ACTION_STATE_WIFI_SSID_RETURN;

            QMap<int, QString> knownNetworks = WingletGUI::inst->wifiMon->knownNetworks();
            if (knownNetworks.key(enteredWifiSSID, -1) != -1) {
                // Saved network: let the user retry with the stored PSK
                // (clears wpa_supplicant's auth-failure backoff) or forget
                // it and enter a new PSK — the stored one may be mistyped.
                actionState = ACTION_STATE_WIFI_SAVED_CONFIRM;
                auto msgbox = new MessageBox(WingletGUI::inst);
                msgbox->setTitleText("Saved Network");
                msgbox->setMessageText("This network is already saved. Reconnect with the saved password, or enter a new one?");
                QStringList myBtns;
                myBtns << "Reconnect";
                myBtns << "New Password";
                msgbox->setButtons(myBtns);
                selectedMsgboxBtnIdx = 0;
                msgbox->setSelectedIndex(selectedMsgboxBtnIdx);
                connect(msgbox, SIGNAL(buttonClicked(int)), this, SLOT(msgboxBtnClicked(int)));
                WingletGUI::inst->addWidgetOnTop(msgbox);
                return;
            }
            else if (manuallyEntered || wifiScanAskPsk) {
                promptWifiPsk(manuallyEntered);
                return;
            }
            else {
                // In this case, this was due to a SSID scan selected on an unencrypted SSID
                // Just directly join the network
                WingletGUI::inst->wifiMon->addOpenNetwork(enteredWifiSSID);
            }
        }
        break;
    }

    case ACTION_STATE_WIFI_SAVED_CONFIRM: {
        QMap<int, QString> knownNetworks = WingletGUI::inst->wifiMon->knownNetworks();
        if (selectedMsgboxBtnIdx == 0) { // Reconnect with saved password
            int knownNetworkId = knownNetworks.key(enteredWifiSSID, -1);
            if (knownNetworkId != -1)
                WingletGUI::inst->wifiMon->selectNetwork(knownNetworkId);
            break;
        }
        // New Password: forget every saved entry for this SSID (manual
        // config edits can leave duplicates), then reuse the normal join
        // keyboard; entry completion re-adds the network.
        for (auto it = knownNetworks.constBegin(); it != knownNetworks.constEnd(); ++it) {
            if (it.value() == enteredWifiSSID)
                WingletGUI::inst->wifiMon->removeNetwork(it.key());
        }
        promptWifiPsk(!wifiScanAskPsk);
        return;
    }

    case ACTION_STATE_BT_SCAN_RETURN:
    case ACTION_STATE_MANAGE_BT_RETURN:
        // Only start pairing/connecting if a device was selected
        if (!selectedBtDevicePath.isEmpty()) {
            startBtPairing();
            return;
        }
        break;

    case ACTION_STATE_BT_PAIRING:
        // Pairing message box was dismissed. If pairing is still running, the
        // user pressed Cancel; otherwise this is just the Okay after completion.
        btPairingMsgbox = nullptr;
        if (!btPairingDone)
            WingletGUI::inst->btMon->cancelPairing();
        break;

    case ACTION_STATE_RESET_ROOT_PASSWORD_CONFIRM:
        if (selectedMsgboxBtnIdx == 0) { // Yes button clicked
            if (!RootPasswordSetting::clearRootPassword()) {
                WingletGUI::inst->showMessageBox("Failed to clear root password!", "Password Set Error");
                actionState = ACTION_STATE_MSGBOX_RETURN;
                return;
            }
        }
        break;

    case ACTION_STATE_COLD_BOOT_GPS_CONFIRM:
        if (selectedMsgboxBtnIdx == 0) { // Yes button clicked
            int ret = system("echo -e \"\\$PCAS10,2*1E\r\n\" > /dev/gnss0");
            if (!ret) {
                WingletGUI::inst->showMessageBox("Please place badge by a window until GPS has position lock", "Almanac Cleared");
                actionState = ACTION_STATE_MSGBOX_RETURN;
                return;
            }
        }
        break;

    case ACTION_STATE_NASR_UPDATE_CONFIRM:
        if (selectedMsgboxBtnIdx == 0) { // Yes button clicked
            // Capture stderr so a failure can show the script's actual
            // error instead of a generic message. Blocks the UI like the
            // confirm box warned; downloads can take minutes on slow WiFi.
            QProcess proc;
            proc.start("python3", {"/usr/libexec/aeroscan/nasr-update.py"});
            bool finished = proc.waitForFinished(10 * 60 * 1000);
            if (!finished)
                proc.kill();
            if (finished && proc.exitStatus() == QProcess::NormalExit
                    && proc.exitCode() == 0) {
                WingletGUI::inst->nasr->reload();
                QString ed = WingletGUI::inst->nasr->edition();
                WingletGUI::inst->showMessageBox(
                    QString("Airport frequency data updated successfully.\nEdition: %1").arg(ed.isEmpty() ? "unknown" : ed),
                    "Radio Data Updated");
            } else {
                // Last non-empty stderr line is the exception summary
                QString detail;
                if (!finished) {
                    detail = "Timed out after 10 minutes";
                } else {
                    const QStringList errLines = QString::fromUtf8(proc.readAllStandardError())
                            .split('\n', Qt::SkipEmptyParts);
                    if (!errLines.isEmpty())
                        detail = errLines.last().trimmed().left(200);
                }
                if (detail.isEmpty())
                    detail = QString("Exit code %1").arg(proc.exitCode());
                WingletGUI::inst->showMessageBox(
                    QString("Update failed:\n%1").arg(detail),
                    "Update Failed");
            }
            actionState = ACTION_STATE_MSGBOX_RETURN;
            return;
        }
        break;

    case ACTION_STATE_SHOW_INVALID_CANARD_SETTINGS:
        actionState = ACTION_STATE_MENU_CLOSE;
        WingletGUI::inst->showMessageBox("Failed to load settings from radio tuner! Unplug/plug it back in and try again.",
                                         "Settings Load Failure!", "Return");
        return;

    case ACTION_STATE_SHOW_CANARD_SETTINGS_SAVE_FAIL:
        actionState = ACTION_STATE_MENU_CLOSE;
        WingletGUI::inst->showMessageBox("Failed to save settings to device!\nReverted to previous settings.", "Failed to Save", "Okay", true);
        return;

    case ACTION_STATE_MENU_CLOSE:
        WingletGUI::inst->removeWidgetOnTop(this);
        return;

    default:
        break;
    }

    actionState = ACTION_STATE_MENU_OPEN;  // Reset state in case we get another show event for some reason

    // Bubble show event to the menu widget so we get the cool animations
    menuWidget->show();
}

void SettingsMenu::promptWifiPsk(bool allowEmpty)
{
    actionState = ACTION_STATE_WIFI_PSK_RETURN;
    CircularKeyboard *kbd = new CircularKeyboard(CircularKeyboard::fullKeyboard, WingletGUI::inst);
    kbd->setTitle("Join:\n" + enteredWifiSSID);
    kbd->setPasswordMaskEnable(true);
    if (allowEmpty) {
        kbd->setPrompt("Enter Network PSK:\n(Leave empty for open network)");
        kbd->setAllowEmptyInput(true);
    }
    else {
        kbd->setPrompt("Enter Network PSK:");
        kbd->setAllowEmptyInput(false);
    }
    QValidator *validator = new WifiPSKValidator(kbd);
    kbd->setValidator(validator);
    kbd->setValidatorFailedMsg("PSK must be between 8-63 characters");
    kbd->setMaxLength(63);  // PSK max length of 63 letters
    connect(kbd, SIGNAL(entryComplete(QString)), this, SLOT(cicularKbdTextEntered(QString)));
    WingletGUI::inst->addWidgetOnTop(kbd);
}

void SettingsMenu::cicularKbdTextEntered(QString val)
{
    if (actionState == ACTION_STATE_GET_DISCORD_USERNAME) {
        QFile file(DISCORD_USERNAME_FILE);
        if (file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
            QTextStream stream(&file);
            stream << val << Qt::endl;
        }
    }
    else if (actionState == ACTION_STATE_WIFI_SSID_RETURN) {
        enteredWifiSSID = val;
        wifiScanAskPsk = false;  // Manually entered SSID — encryption unknown, allow an empty PSK
    }
    else if (actionState == ACTION_STATE_WIFI_PSK_RETURN) {
        // When the PSK is entered, we can join the SSID from the previous stage as well as the new value
        if (val.length() > 0) {
            WingletGUI::inst->wifiMon->addProtectedNetwork(enteredWifiSSID, val);
        }
        else {
            WingletGUI::inst->wifiMon->addOpenNetwork(enteredWifiSSID);
        }
    }
    else if (actionState == ACTION_STATE_TEXT_RETURN) {
        dynamic_cast<AbstractTextSetting*>(activeItem)->setValue(val);
    }
}

void SettingsMenu::selectorIndexSelected(QModelIndex index) {
    if (actionState == ACTION_STATE_SELECTOR_RETURN) {
        dynamic_cast<AbstractListSetting*>(activeItem)->setValue(index);
    }
    else if (actionState == ACTION_STATE_MANAGE_NETWORKS_RETURN) {
        QVariant actionVariant = index.data(Qt::UserRole);
        if (actionVariant.type() == QVariant::Int) {
            int actionEnum = actionVariant.toInt();
            if (actionEnum == KnownNetworksModel::FORGET_NETWORK) {
                WingletGUI::inst->wifiMon->removeNetwork(index.internalId());
            }
        }
    }
    else if (actionState == ACTION_STATE_WIFI_SCAN_RETURN) {
        QVariant selectedSsidVariant = index.data(Qt::UserRole);
        if (selectedSsidVariant.type() == QVariant::UserType) {
            WifiScanResult result = selectedSsidVariant.value<WifiScanResult>();
            enteredWifiSSID = result.ssid;
            wifiScanAskPsk = result.encrypted;
        }
    }
    else if (actionState == ACTION_STATE_BT_SCAN_RETURN) {
        QVariant selectedDeviceVariant = index.data(Qt::UserRole);
        if (selectedDeviceVariant.type() == QVariant::UserType) {
            BtDeviceInfo result = selectedDeviceVariant.value<BtDeviceInfo>();
            selectedBtDevicePath = result.path;
            selectedBtDeviceName = result.name;
        }
    }
    else if (actionState == ACTION_STATE_MANAGE_BT_RETURN) {
        QVariant actionVariant = index.data(Qt::UserRole);
        if (actionVariant.type() == QVariant::Int) {
            int actionEnum = actionVariant.toInt();
            if (actionEnum == PairedBtDevicesModel::FORGET_DEVICE) {
                WingletGUI::inst->btMon->removeDevice(index.data(PairedBtDevicesModel::DevicePathRole).toString());
            }
            else if (actionEnum == PairedBtDevicesModel::CONNECT_DEVICE) {
                // Connection is started from showEvent once the selector closes
                selectedBtDevicePath = index.data(PairedBtDevicesModel::DevicePathRole).toString();
                selectedBtDeviceName = index.data(PairedBtDevicesModel::DeviceNameRole).toString();
            }
        }
    }
}

void SettingsMenu::menuExitRequested()
{
    WingletGUI::inst->removeWidgetOnTop(this);
}

void SettingsMenu::menuExitConfirmClicked(int btnIdx) {
    (void) btnIdx;
}

void SettingsMenu::focusInEvent(QFocusEvent* ev)
{
    // Pass focus events to the underlying menu widget so it receives key events
    menuWidget->setFocus(ev->reason());
}

void SettingsMenu::msgboxBtnClicked(int btnIdx) {
    selectedMsgboxBtnIdx = btnIdx;
}

void SettingsMenu::menuItemSelected(QModelIndex index)
{
    auto userRole = menuWidget->model()->data(index, Qt::UserRole);
    if (userRole.isValid()) {
        auto settingsEntry = userRole.value<AbstractSettingsEntry*>();

        if (auto listEntry = dynamic_cast<AbstractListSetting*>(settingsEntry)) {
            actionState = ACTION_STATE_SELECTOR_RETURN;
            activeItem = settingsEntry;
            auto curIdx = listEntry->curIndex();
            int initialRow = 0;
            while (curIdx.isValid() && curIdx.parent().isValid()) {
                curIdx = curIdx.parent();
            }
            initialRow = curIdx.row();  // Only set initial row when parent is invalid (root node in list)

            auto selector = new SelectorBox(WingletGUI::inst, listEntry->selectionModel(), true, initialRow);
            connect(selector, SIGNAL(indexSelected(QModelIndex)), this, SLOT(selectorIndexSelected(QModelIndex)));
            WingletGUI::inst->addWidgetOnTop(selector);
            return;
        }
        else if (auto textEntry = dynamic_cast<AbstractTextSetting*>(settingsEntry))
        {
            actionState = ACTION_STATE_TEXT_RETURN;
            activeItem = settingsEntry;

            CircularKeyboard* kbd = new CircularKeyboard(textEntry->inputKeys(), WingletGUI::inst);
            kbd->setTitle(textEntry->title());
            kbd->setPrompt(textEntry->prompt());
            kbd->setMaxLength(textEntry->maxLength());
            QValidator *validator = textEntry->validator();
            if (validator) {
                kbd->setValidator(validator);
            }
            QString failedMsg = textEntry->validatorFailedMsg();
            if (!failedMsg.isEmpty()) {
                kbd->setValidatorFailedMsg(failedMsg);
            }
            kbd->setPasswordMaskEnable(textEntry->isPasswordField());
            kbd->setAllowEmptyInput(kbd->allowEmptyInput());
            kbd->setValue(textEntry->value());

            connect(kbd, SIGNAL(entryComplete(QString)), this, SLOT(cicularKbdTextEntered(QString)));
            WingletGUI::inst->addWidgetOnTop(kbd);
            return;
        }
        else if (auto actionEntry = dynamic_cast<SettingsActionEntry*>(settingsEntry)) {
            CircularKeyboard* kbd;

            switch (actionEntry->action()) {
            case AppSettings::ACTION_ABOUT:
                actionState = ACTION_STATE_INFO_RETURN;
                WingletGUI::inst->addWidgetOnTop(new InfoViewer(WingletGUI::inst));
                return;
            case AppSettings::ACTION_RELEASE_NOTES:
                if (WingletGUI::inst->tryShowReleaseNotes(true)) {
                    actionState = ACTION_STATE_MSGBOX_RETURN;
                    return;
                }
                break;
            case AppSettings::ACTION_PRIVATE_DISCORD:
                actionState = ACTION_STATE_GET_DISCORD_USERNAME;
                QFile::remove(DISCORD_USERNAME_FILE);
                kbd = new CircularKeyboard({"abcdefghijklmnopqrstuvwxyz0123456789._"}, WingletGUI::inst);
                kbd->setTitle("Private Badge Owners Access Request");
                kbd->setPrompt("Enter Discord Username:");
                kbd->setMaxLength(32);
                kbd->setValidator(new QRegExpValidator(discord_regexp, kbd));
                kbd->setValidatorFailedMsg("Input is not a valid Discord Username");
                connect(kbd, SIGNAL(entryComplete(QString)), this, SLOT(cicularKbdTextEntered(QString)));
                WingletGUI::inst->addWidgetOnTop(kbd);
                return;
            case AppSettings::ACTION_MANAGE_WIFI_NETWORKS: {
                auto model = new KnownNetworksModel();
                if (model->rowCount() == 0) {
                    delete model;
                    actionState = ACTION_STATE_MSGBOX_RETURN;
                    WingletGUI::inst->showMessageBox("There are no saved wifi networks at this time.", "No Networks");
                    return;
                }
                else {
                    actionState = ACTION_STATE_MANAGE_NETWORKS_RETURN;
                    auto selector = new SelectorBox(WingletGUI::inst, model, true);
                    model->setParent(selector);
                    connect(selector, SIGNAL(indexSelected(QModelIndex)), this, SLOT(selectorIndexSelected(QModelIndex)));
                    WingletGUI::inst->addWidgetOnTop(selector);
                    return;
                }
            }
            case AppSettings::ACTION_WIFI_MANUAL:
                {
                    enteredWifiSSID = "";  // Clear variable before entering selector
                    actionState = ACTION_STATE_WIFI_SSID_RETURN;
                    kbd = new CircularKeyboard(CircularKeyboard::fullKeyboard, WingletGUI::inst);
                    kbd->setTitle("Manual WiFi Entry");
                    kbd->setPrompt("Enter Network SSID:");
                    kbd->setMaxLength(32);  // SSIDs have a max length of 32 bytes
                    connect(kbd, SIGNAL(entryComplete(QString)), this, SLOT(cicularKbdTextEntered(QString)));
                    WingletGUI::inst->addWidgetOnTop(kbd);
                    return;
                }
            case AppSettings::ACTION_WIFI_SCAN: {
                if (WingletGUI::inst->wifiMon->wifiState() == WifiMonitor::WIFI_OFF) {
                    WingletGUI::inst->showMessageBox(
                        "WiFi is not available. Check that a network adapter is present and the system has finished booting.",
                        "WiFi Unavailable");
                    return;
                }
                enteredWifiSSID = "";  // Clear variable before entering selector
                auto model = new WifiScanModel();
                actionState = ACTION_STATE_WIFI_SCAN_RETURN;  // TODO: Replace with actual action when complete
                auto selector = new SelectorBox(WingletGUI::inst, model, true);
                selector->menuWidget->setShrinkOnSelect(false);
                model->setParent(selector);
                connect(selector, SIGNAL(indexSelected(QModelIndex)), this, SLOT(selectorIndexSelected(QModelIndex)));
                WingletGUI::inst->addWidgetOnTop(selector);
                return;
            }
            case AppSettings::ACTION_BT_SCAN: {
                if (WingletGUI::inst->btMon->btState() == BluetoothMonitor::BT_OFF) {
                    WingletGUI::inst->showMessageBox(
                        "Bluetooth is not available. Check that the system has finished booting.",
                        "Bluetooth Unavailable");
                    return;
                }
                selectedBtDevicePath = "";  // Clear variables before entering selector
                selectedBtDeviceName = "";
                auto model = new BtScanModel();
                actionState = ACTION_STATE_BT_SCAN_RETURN;
                auto selector = new SelectorBox(WingletGUI::inst, model, true);
                selector->menuWidget->setShrinkOnSelect(false);
                model->setParent(selector);
                connect(selector, SIGNAL(indexSelected(QModelIndex)), this, SLOT(selectorIndexSelected(QModelIndex)));
                WingletGUI::inst->addWidgetOnTop(selector);
                return;
            }
            case AppSettings::ACTION_MANAGE_BT_DEVICES: {
                auto model = new PairedBtDevicesModel();
                if (model->rowCount() == 0) {
                    delete model;
                    actionState = ACTION_STATE_MSGBOX_RETURN;
                    WingletGUI::inst->showMessageBox("There are no paired bluetooth devices at this time.", "No Devices");
                    return;
                }
                else {
                    selectedBtDevicePath = "";  // Clear variables before entering selector
                    selectedBtDeviceName = "";
                    actionState = ACTION_STATE_MANAGE_BT_RETURN;
                    auto selector = new SelectorBox(WingletGUI::inst, model, true);
                    model->setParent(selector);
                    connect(selector, SIGNAL(indexSelected(QModelIndex)), this, SLOT(selectorIndexSelected(QModelIndex)));
                    WingletGUI::inst->addWidgetOnTop(selector);
                    return;
                }
            }
            case AppSettings::ACTION_CLEAR_ROOT_PASSWORD: {
                actionState = ACTION_STATE_RESET_ROOT_PASSWORD_CONFIRM;
                auto msgbox = new MessageBox(WingletGUI::inst);
                msgbox->setMessageText("Are you sure you want to clear the root password? Note SSH will now only work over the USB Ethernet interface.");
                msgbox->setTitleText("Confirm Clear Password");
                QStringList myBtns;
                myBtns << "Yes";
                myBtns << "No";
                msgbox->setButtons(myBtns);
                selectedMsgboxBtnIdx = 1;  // Set default index (shouldn't happen but if we return without selecting button, we'll do default action)
                msgbox->setSelectedIndex(selectedMsgboxBtnIdx);
                connect(msgbox, SIGNAL(buttonClicked(int)), this, SLOT(msgboxBtnClicked(int)));
                WingletGUI::inst->addWidgetOnTop(msgbox);
                return;
            }
            case AppSettings::ACTION_COLD_BOOT_GPS: {
                actionState = ACTION_STATE_COLD_BOOT_GPS_CONFIRM;
                auto msgbox = new MessageBox(WingletGUI::inst);
                msgbox->setMessageText("Cold Booting the GPS will clear the satellite almanac. Reacquiring GPS position will take about 11 minutes. Are you sure?");
                msgbox->setTitleText("Cold Boot GPS?");
                QStringList myBtns;
                myBtns << "Yes";
                myBtns << "No";
                msgbox->setButtons(myBtns);
                selectedMsgboxBtnIdx = 1;  // Set default index (shouldn't happen but if we return without selecting button, we'll do default action)
                msgbox->setSelectedIndex(selectedMsgboxBtnIdx);
                connect(msgbox, SIGNAL(buttonClicked(int)), this, SLOT(msgboxBtnClicked(int)));
                WingletGUI::inst->addWidgetOnTop(msgbox);
                return;
            }
            case AppSettings::ACTION_UPDATE_NASR: {
                actionState = ACTION_STATE_NASR_UPDATE_CONFIRM;
                auto msgbox = new MessageBox(WingletGUI::inst);
                msgbox->setMessageText("This will download FAA airport frequency data (~100 MB). "
                                       "WiFi must be connected. The download may take several minutes. "
                                       "The screen will be unresponsive until complete. Continue?");
                msgbox->setTitleText("Update Radio Data");
                QStringList myBtns;
                myBtns << "Yes";
                myBtns << "No";
                msgbox->setButtons(myBtns);
                selectedMsgboxBtnIdx = 1;
                msgbox->setSelectedIndex(selectedMsgboxBtnIdx);
                connect(msgbox, SIGNAL(buttonClicked(int)), this, SLOT(msgboxBtnClicked(int)));
                WingletGUI::inst->addWidgetOnTop(msgbox);
                return;
            }
            }
        }
    }

    // If we get here, we fell through and need to show the menu again
    menuWidget->setShowShrinkFromOutside(true);
    menuWidget->show();
}

void SettingsMenu::startBtPairing()
{
    actionState = ACTION_STATE_BT_PAIRING;
    btPairingDone = false;

    auto msgbox = new MessageBox(WingletGUI::inst);
    msgbox->setTitleText("Bluetooth Pairing");
    msgbox->setMessageText(QString("Connecting to:\n%1\n\nMake sure the keyboard is in pairing mode.")
                               .arg(selectedBtDeviceName));
    msgbox->setSingleButtonWithText("Cancel");
    btPairingMsgbox = msgbox;
    WingletGUI::inst->addWidgetOnTop(msgbox);

    WingletGUI::inst->btMon->pairDevice(selectedBtDevicePath);
    selectedBtDevicePath = "";
    selectedBtDeviceName = "";
}

void SettingsMenu::btPairingCodeReceived(QString code)
{
    if (actionState != ACTION_STATE_BT_PAIRING || !btPairingMsgbox)
        return;

    btPairingMsgbox->setMessageText(QString("Type this code on the keyboard,\nthen press Enter:\n\n%1").arg(code));
}

void SettingsMenu::btPairingComplete(bool success, QString errorMessage)
{
    if (actionState != ACTION_STATE_BT_PAIRING)
        return;

    btPairingDone = true;
    if (!btPairingMsgbox)
        return;

    if (success) {
        btPairingMsgbox->setTitleText("Pairing Complete");
        btPairingMsgbox->setMessageText("Keyboard connected!\nIt will reconnect automatically from now on.");
    }
    else {
        btPairingMsgbox->setTitleText("Pairing Failed");
        btPairingMsgbox->setMessageText(QString("Pairing failed:\n%1")
                                            .arg(errorMessage.isEmpty() ? "Unknown error" : errorMessage));
    }
    btPairingMsgbox->setSingleButtonWithText("Okay");
}

void SettingsMenu::canardConnectionChanged(bool connected) {
    (void) connected;
}

} // namespace WingletUI
