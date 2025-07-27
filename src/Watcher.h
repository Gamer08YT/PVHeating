//
// Created by JanHe on 27.07.2025.
//

#ifndef WATCHER_H
#define WATCHER_H


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

    static enum ModeType
    {
        CONSUME, DYNAMIC
    };

    static void setMode(ModeType mode);
    static ModeType mode;
    static bool error;
    static bool standby;

private:
    static void handleErrorLedFade(bool cond);
    static void setupPins();
    static void readButtons();
    static void readTemperature();
};


#endif //WATCHER_H
