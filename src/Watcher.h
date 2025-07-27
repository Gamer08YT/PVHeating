//
// Created by JanHe on 27.07.2025.
//

#ifndef WATCHER_H
#define WATCHER_H
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
    static void handleButtonLeds();
    static void handleSensors();
    static void loop();
    static void setupButtons();
    static void setup();
    static void setStandby(bool cond);
    static void setError(bool cond);
    static void setMaxConsume(float to_float);

    enum ModeType
    {
        CONSUME, DYNAMIC
    };


    static void setMode(ModeType mode);
    static ModeType mode;
    static bool error;
    static bool standby;
    static float temperatureIn;
    static float temperatureOut;
    static float maxConsume;

private:
    static void readLocalPower();
    static void readHouseMeterPower();
    static void handleStandbyLedFade(bool cond);
    static void handleErrorLedFade(bool cond);
    static void setupPins();
    static void readButtons();
    static void setTemperatureIn(float i);
    static void setTemperatureOut(float i);
    static void readTemperature();
};


#endif //WATCHER_H
