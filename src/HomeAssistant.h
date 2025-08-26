//
// Created by JanHe on 27.07.2025.
//

#ifndef HOMEASSISTANT_H
#define HOMEASSISTANT_H
#include "Ethernet.h"
#include "ModbusClientRTU.h"
#include "device-types/HAHVAC.h"
#include "device-types/HASensorNumber.h"
#include "device-types/HASwitch.h"


/**
 * @class HomeAssistant
 * @brief The HomeAssistant class is designed to manage and control various smart home devices.
 *
 * This class provides functionality to integrate with, monitor, and manage devices within a smart home
 * ecosystem. It allows the user to control devices, view their statuses, and automate operations.
 *
 * Responsibilities of this class include:
 * - Device management: Adding, removing, and updating smart devices connected to the system.
 * - Automation setup: Configuring automated tasks or routines for smart devices.
 * - Status monitoring: Obtaining and displaying the state or status of devices.
 * - Integration: Providing integration with third-party APIs or services to extend functionality.
 *
 * Thread safety or concurrency considerations must be taken into account if the implementation involves
 * concurrent access or modification of shared resources.
 */
class HomeAssistant
{
private:
    static void configurePumpInstance();
    static void configureModeInstance();
    static void configureSCRInstance();
    static void configureHeatingInstance();
    static void configurePowerInstance();
    static void configureConsumptionInstance();
    static void configureFaultInstances();
    static void configureFlowInstance();
    static void configureErrorInstances();
    static void configureMaxPowerInstance();
    static void configureMinPowerInstance();
    static void configurePWMInstance();
    static void handleMQTT();


public:
    static void configureResetInstance();
    static void configureTemperatureInputInstance();
    static void configureStandbyInstance();
    static void configureRestartInstance();
    static void configureConsumptionRemainInstance();
    static void begin();
    static void loop();
    static void setFlow(float get_current_flowrate);
    static void setCurrentPower(float current_power);
    static void setCurrentTemperature(float x);
    static void setPump(bool state);
    static void setSCR(bool sender);
    static void setConsumption(float value);
    static void setPWM(uint32_t int8);
    static void setErrorTitle(const char* error_title);
    static NetworkClient* getClient();
    static void setMode(int str);
    static void setTemperatureIn(float temperature_in);
    static void setStandby(bool cond);
    static void setErrorState(bool cond);
    static void setConsumptionRemain(float value);
    static void reconnectMQTT();
};


#endif //HOMEASSISTANT_H
