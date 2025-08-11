//
// Created by JanHe on 27.07.2025.
//

#ifndef WATCHER_H
#define WATCHER_H
#include "DallasTemperature.h"
#include "utils/HANumeric.h"


/**
 * @class Watcher
 * @brief A class to handle sensor readings, button inputs, and system states.
 *
 * The Watcher class is responsible for managing hardware-specific tasks such as
 * handling sensor data, monitoring system states (standby, error), controlling
 * button LEDs, and maintaining operational settings like power consumption limits
 * and operation modes.
 */
class Watcher
{
public:
    static bool powerLock;
    static bool tempLock;
    static void handleButtonLeds();
    static void loop();
    static void setDefaults();
    static void setup();
    static void setStandby(bool cond);
    static void setMaxConsume(float to_float);
    static void setPump(bool sender);
    static void setSCR(bool sender);
    static void setSCRViaHA(bool state);
    static void setPumpViaHA(bool state);
    static void startConsume();
    static void setMaxPower(float to_float);
    static void setPower(float current_power);
    static void setConsumption(float consumption);
    static void setPWMHA(u_int32_t duty);
    static void setPWM(u_int32_t int8);
    static void setTargetTemperature(int is_int8);
    static void setHousePower(float house_power);
    static void handleErrorLedFade(bool cond);
    static void setDuty(u_int32_t int8);
    static void setMinPower(float to_float);


    /**
     * @enum ModeType
     * @brief Represents the operational modes of the system.
     *
     * The ModeType enumeration is used to define the different modes
     * that the system can operate in. These include:
     * - CONSUME: A mode where the system prioritizes consumption behavior.
     * - DYNAMIC: A mode where the system operates dynamically, using external
     *   Housemeter.
     */
    enum ModeType
    {
        CONSUME, DYNAMIC
    };


    static void setMode(ModeType mode);
    static ModeType mode;
    static bool standby;
    static float maxPower;
    static float minPower;
    static float temperatureIn;
    static float temperatureOut;
    static float maxConsume;
    static float currentPower;
    static float housePower;
    static float consumption;
    static float flowRate;
    static float temperatureMax;

private:
    static float startConsumed;
    static u_int8_t standbyCounter;
    static u_int32_t duty;
    static void begin1Wire();
    static void setupFlowMeter();
    static void readLocalPower();
    static void readHouseMeterPower();
    static void readLocalConsumption();
    static void handleStandbyLedFade(bool cond);
    static void setupPins();
    static void readButtons();
    static void readTemperature();
    static bool isEnoughPowerGeneration();
    static void handleStandbyCounterDisable();
    static void handleStandbyCounterEnable();
    static void handlePowerBasedDuty();
    static bool checkLocalPowerLimit();
    static void handleMaxPower(float max_power);
    static void handleConsumeBasedDuty();
    static bool isTempToLow();
    static bool isOverTemp();
    static void handlePWM();
    static void updateDisplay();
    static void setFlow(float get_current_flowrate);
    static void updateTemperature();
    static void handleOneWireClearInterval();
    static void handleSlowInterval();
    static void handleFastInterval();
    static void handleHAPublish();
    static void handleSensors();
    static void setupButtons();
    static void printAddress(DeviceAddress deviceAddress);
};


#endif //WATCHER_H
