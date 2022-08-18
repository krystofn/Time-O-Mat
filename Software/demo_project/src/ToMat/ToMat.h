#ifndef _TOMAT_
#define _TOMAT_

#include <Arduino.h>
#include "Preferences.h"

#include "Display_TM.h"
#include "Time_module.h"
#include "Touchbar_TM.h"
#include "USB_C_power_module.h"
#include "Illumination_module.h"
#include "WiFiCaptain.h"
#include <piezo/piezo.h>

namespace TM {

const int BUTTON_PIN[3] = {18, 19, 21};
const uint8_t BUZZER_PIN = 17;
const uint8_t ONE_WIRE_PIN = 22;

const uint8_t BUZZER_CHANNEL = 3;

const char STORAGE_NAMESPACE[] = "Time_o_mat";
const uint16_t communicationTimeout = 1000;

void refreshTaskQuick(void * param);
void refreshTaskSlow(void * param);
}

class ToMat_class {
    bool beginCalled = false;
    bool connectionEnabled = false;
    bool connectionActive = false;
    uint32_t prevCommunicationTime = 0;

    float temperature = 0.0;
    bool displayRefreshActive = false;

public:
    Display_TM display;
    Time_module time;
    TouchBar_TM touchBar;
    USB_C_power_module power;
    Illumination_module illumination;
    Piezo piezo;

    void begin();
    bool buttonRead(int buttonID);
    void updateTemperature();
    float getTemperature();

    void printDiagnostics();

    void startWiFiCaptain(String name="", String password="");
    void checkConnection();
    String commandGet();
    String commandGetIndexed(uint8_t index);
    void commandClear();
    void internCommandHandle();
    void commandSend(String type, String text);
    void commandDisp(String text);

    void setDisplayRefresh(bool state);
    bool getDisplayRefresh();
};

extern ToMat_class ToMat;
extern Melody themeMelody;

#endif // _TOMAT_