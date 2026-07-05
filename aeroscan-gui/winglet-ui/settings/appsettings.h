#ifndef WINGLETUI_APPSETTINGS_H
#define WINGLETUI_APPSETTINGS_H

#include <QObject>
#include <QElapsedTimer>
#include "abstractsettingsentry.h"


namespace WingletUI {

class GPSReading;

#define DEFINE_SETTING(type, name, defaultVal, setName) \
    public: \
        Q_PROPERTY(type name READ name WRITE setName NOTIFY name ## Changed STORED true) \
        type name() const { return m_ ## name; } \
        void setName(type val) { \
            if (val != m_ ## name) { \
                m_ ## name = val; \
                emit name ## Changed(val); \
                saveSettings(); \
            } \
        } \
        Q_SIGNAL void name ## Changed(type newVal); \
    private: \
        type m_ ## name = defaultVal;

class AppSettings : public QObject
{
    Q_OBJECT

public:
    explicit AppSettings(QObject *parent = nullptr);
    void loadSettings();
    void saveSettings();
    const SettingsEntryContainer *settingsEntryRoot() const { return m_settingsEntryRoot; }
    bool rebootNeeded = false;

    DEFINE_SETTING(int, screenBrightness, 750, setScreenBrightness)
    DEFINE_SETTING(bool, clockShowSeconds, true, setClockShowSeconds)
    DEFINE_SETTING(int, adsbDecayTimeSec, 60, setAdsbDecayTimeSec)
    DEFINE_SETTING(bool, invertedScrollDirection, true, setInvertedScrollDirection)
    DEFINE_SETTING(int, ledBrightness, 9, setLedBrightness)
    DEFINE_SETTING(bool, darkMode, true, setDarkMode)
    DEFINE_SETTING(bool, timeFormat12hr, false, setTimeFormat12hr)
    DEFINE_SETTING(bool, sdCardMaps, false, setSdCardMaps)

    // ALSA output routing: 0 = 3.5mm headphone jack, 1 = HDMI0,
    // 2 = Bluetooth headphones (via bluez-alsa, requires a paired audio device)
    DEFINE_SETTING(int, audioOutput, 0, setAudioOutput)

    // Default position: Tucson, AZ — used if GPS has never produced a fix
    DEFINE_SETTING(double, lastLatitude, 32.231917, setLastLatitude)
    DEFINE_SETTING(double, lastLongitude, -110.951333, setLastLongitude)

    // Holds defaults for resuming radio tuner settings
    DEFINE_SETTING(uint, canardLastFmFreq, 1017, setCanardLastFmFreq)
    DEFINE_SETTING(uint, canardLastFmPreset, 1, setCanardLastFmPreset)
    DEFINE_SETTING(uint, canardLastAirbandFreq, 121500, setCanardLastAirbandFreq)
    DEFINE_SETTING(uint, canardLastAirbandPreset, 1, setCanardLastAirbandPreset)
    // 146.520 = 2m national simplex calling frequency
    DEFINE_SETTING(uint, canardLastHam2mFreq, 146520, setCanardLastHam2mFreq)
    DEFINE_SETTING(uint, canardLastHam2mPreset, 1, setCanardLastHam2mPreset)
    // rtl_fm -l squelch (linear amplitude units); FM broadcast is always open
    DEFINE_SETTING(int, canardSquelchAm, 60, setCanardSquelchAm)
    DEFINE_SETTING(int, canardSquelchNfm, 50, setCanardSquelchNfm)

public:
    void reportGPSReading(GPSReading* reading);

    // List of actions that the settings screen can run
    enum AppSettingsAction {
        ACTION_ABOUT,
        ACTION_PRIVATE_DISCORD,
        ACTION_WIFI_SCAN,
        ACTION_WIFI_MANUAL,
        ACTION_MANAGE_WIFI_NETWORKS,
        ACTION_BT_SCAN,
        ACTION_BT_SCAN_AUDIO,
        ACTION_MANAGE_BT_DEVICES,
        ACTION_CLEAR_ROOT_PASSWORD,
        ACTION_RELEASE_NOTES,
        ACTION_COLD_BOOT_GPS,
        ACTION_UPDATE_NASR
    };

private:
    QElapsedTimer gpsSaveTimer;
    SettingsEntryContainer *m_settingsEntryRoot;
};

} // namespace WingletUI

#endif // WINGLETUI_APPSETTINGS_H
