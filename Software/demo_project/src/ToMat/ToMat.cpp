#include "ToMat.h"

#include "OneWire.h"
#include "DallasTemperature.h"

OneWire oneWire(TM::ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);
Melody themeMelody("TEMPO=140 USECUTOFF=1 CUTOFFPERCENT=20 F5#/8 F5#/8 D5/8 B4/8 R/8 B4/8 R/8 E5/8 R/8 E5/8 R/8 E5/8 G5#/8 G5#/8 A5/8 B5/8 A5/8 A5/8 A5/8 E5/8 R/8 D5/8 R/8 F5#/8 R/8 F5#/8 R/8 F5#/8 E5/8 E5/8 F5#/8 E5/8 F5#/8 F5#/8 D5/8 B4/8 R/8 B4/8 R/8 E5/8 R/8 E5/8 R/8 E5/8 G5#/8 G5#/8 A5/8 B5/8 A5/8 A5/8 A5/8 E5/8 R/8 D5/8 R/8 F5#/8 R/8 F5#/8 R/8 F5#/8 E5/8 E5/8 F5#/8 E5/8 F5#/8 F5#/8 D5/8 B4/8 R/8 B4/8 R/8 E5/8 R/8 E5/8 R/8 E5/8 G5#/8 G5#/8 A5/8 B5/8 A5/8 A5/8 A5/8 E5/8 R/8 D5/8 R/8 F5#/8 R/8 F5#/8 R/8 F5#/8 E5/8 E5/8 F5#/8 E5/8");

void TM::refreshTaskQuick(void * parameter) {
    for(;;) {
        if(ToMat.getDisplayRefresh()) {
            ToMat.display.update();
        }
        ToMat.touchBar.update();
        ToMat.illumination.update();
        delay(20);
    }
}

uint32_t prevWeatherUpdate = 0;
bool startUpWeatherUpdate = true;
uint32_t slowRefreshStart = 0;
void TM::refreshTaskSlow(void * parameter) {
    for(;;) {
		slowRefreshStart = millis();

        ToMat.power.update();
        ToMat.updateTemperature();

		if(((millis() - prevWeatherUpdate > TM::WEATHER_UPDATE) || startUpWeatherUpdate) && ToMat.pingInternet()){
			startUpWeatherUpdate = false;
			printf("Updating weather!\n");
			ToMat.weatherApi.updateBothWF();

			prevWeatherUpdate = millis();
		}

        delay(constrain(1000+slowRefreshStart-millis(), 1, 1000));
    }
}

void ToMat_class::begin() {
    beginCalled = true;

    power.update();
    illumination.update();
    display.begin();
    time.begin();
    touchBar.begin();
    piezo.begin(TM::BUZZER_CHANNEL, TM::BUZZER_PIN);

	weatherApi.init();
	if(weatherApi.getKey() == ""){
		weatherApi.setKey(TM::WEATHER_API_KEY);
	}

    for(int i = 0; i < 3; ++i) {
        pinMode(TM::BUTTON_PIN[i], INPUT_PULLUP);
    }

    sensors.begin();
    
    xTaskCreatePinnedToCore(TM::refreshTaskQuick, "refreshTaskQuick", 10000 , NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(TM::refreshTaskSlow, "refreshTaskSlow", 10000 , NULL, 0, NULL, 0);

    displayRefreshActive = true;
}

bool ToMat_class::buttonRead(int buttonID) {
    if(buttonID < 0 || buttonID > 2) {
        printf("Invalid button ID: %d\n", buttonID);
        return 0;
    }
    return !digitalRead(TM::BUTTON_PIN[buttonID]);  // 1 = pressed
}

void ToMat_class::updateTemperature() {
    sensors.requestTemperatures();
    float newTemp = sensors.getTempCByIndex(0);
    if(newTemp > -100.0) {
        // Filter out nonsense measurements
        temperature = newTemp;
    }
}

float ToMat_class::getTemperature() {
    return temperature;
}

void ToMat_class::printDiagnostics() {
    for(int i = 0; i <= 2; ++i) {
        printf("btn%d: %d ", i, buttonRead(i));
    }
    
    printf("touchDigit: ");
    for(int i = 0; i <= 7; ++i) {
        printf("%d", touchBar.getPressed(i));
    }

    printf(" %s", power.getVoltagesText().c_str());
    printf("%s", illumination.getIlluminationText().c_str());

    //printf("priority: %d ", uxTaskPriorityGet(NULL));

    String timeDisp = ToMat.time.getClockText();
    printf("time: %s ", timeDisp.c_str());

    printf("temp: %f \n", temperature);
}


void ToMat_class::startWiFiCaptain(String name, String password) {
    if(!beginCalled) {
        begin();
    }

    setDisplayRefresh(false);
    display.setText("----", red);
    display.update();

    String ssid_final = "ToMat-";
    if(name.isEmpty() || name == "<your_name>") {
        ssid_final += WiFi.macAddress();
    }
    else {
        ssid_final += name;
    }
    setApCredentials(ssid_final, password);
    wifiCaptInit();
    connectionEnabled = true;

    display.setText("    ", red);
    display.update();
    setDisplayRefresh(true);
}

void ToMat_class::checkConnection() {
    if(!connectionEnabled) {
        return;
    }
    if(millis() > prevCommunicationTime + TM::communicationTimeout) {
        connectionActive = false;
    }
    else {
        connectionActive = true;
    }
}

uint32_t prevPingTime = 0;
bool prevPing = false;
bool ToMat_class::pingInternet(){
	if(!WiFi.isConnected()){
		prevPing = false;
		return false;
	}
	if(millis() - prevPingTime < TM::PING_DELAY){
		return prevPing;
	}

	prevPing = Ping.ping(IPAddress(8,8,8,8), 1);

	prevPingTime = millis();
	return prevPing;
}

String ToMat_class::commandGet() {
    String command = String(commandGetCaptain());
    command.toLowerCase();
    return command;
}

String ToMat_class::commandGetIndexed(uint8_t index) {
    char commandBuffer[64];
    sprintf(commandBuffer, commandGetCaptain());
    const char delimiter[2] = " ";
    char *token;
    token = strtok((char *)commandBuffer, delimiter);
    for(uint8_t i = 0; i < index; ++i) {
        token = strtok(NULL, delimiter);
    }
    String command = String(token);
    command.toLowerCase();
    return command;
}

void ToMat_class::commandClear() {
    commandClearCaptain();
}

void ToMat_class::internCommandHandle() {
    static uint8_t counter = 0;
    if(counter < 20) {
        counter++;
        return;
    }
    else {
        counter = 0;
    }
    if(ToMat.commandGetIndexed(0) == "reset" || ToMat.commandGetIndexed(0) == "restart") {
        ESP.restart();
    }
    else if(ToMat.commandGet() == "encoder calibrate") {
        ToMat.commandClear();
    }
}

void ToMat_class::commandSend(String type, String text) {
    commandSendCaptain(type, text);
}

void ToMat_class::commandDisp(String text) {
    commandSend("commandDisp", text);
}

void ToMat_class::setDisplayRefresh(bool state) {
    displayRefreshActive = state;
}

bool ToMat_class::getDisplayRefresh() {
    return displayRefreshActive;
}

ToMat_class ToMat;