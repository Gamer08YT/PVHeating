//
// Created by JanHe on 27.07.2025.
//

#ifndef HOMEASSISTANT_H
#define HOMEASSISTANT_H
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
    static void publishChanges();
    static void configureFaultInstances();
    static void configureFlowInstance();
    static void configureErrorInstances();
    static void handleMQTT();

public:
    static void begin();
    static void loop();
    static HAHVAC getHVAC();
    static HASensorNumber getCurrentPower();
    static HASensorNumber getConsumption();
    static HASwitch getPump();
    static HASwitch getSCR();
};


#endif //HOMEASSISTANT_H
