//
// Created by JanHe on 27.07.2025.
//

#include "HomeAssistant.h"

#include "Ethernet.h"
#include "HADevice.h"
#include "HAMqtt.h"
#include "Guardian.h"
#include "LocalNetwork.h"
#include "PinOut.h"
#include "Watcher.h"
#include "device-types/HABinarySensor.h"
#include "device-types/HAButton.h"
#include "device-types/HAHVAC.h"
#include "device-types/HANumber.h"
#include "device-types/HASelect.h"
#include "device-types/HASensorNumber.h"
#include "device-types/HASwitch.h"

// Store Instance of Ethernet Client.
EthernetClient client;

// Store HADevice Instance.
HADevice device;

// Store MQTT Instance.
HAMqtt mqtt(client, device, 16);

// Store HAVAC Instance.
HAHVAC heating("heating", HAHVAC::TargetTemperatureFeature | HAHVAC::ModesFeature);

// Store Power Usage Instance.
HASensorNumber power("heating_load", HABaseDeviceType::PrecisionP2);

// Store Power Consumption Instance.
HASensorNumber consumption("heating_consumption", HABaseDeviceType::PrecisionP2);

// Store Error State Instance.
HABinarySensor fault("heating_fault");

// Store Flow Rate Instance.
HASensorNumber flow("heating_flow", HABaseDeviceType::PrecisionP2);

// Store Consume Start Action Instance.
HAButton consumeStart("heating_consume_start");

// Store Consume Input Value Instance.
HANumber consumeMax("heating_consume_max");

// Store Max Power.
HANumber maxPower("heating_max_power");

// Store SCR Switch Instance.
HASwitch scrSwitch("scr_switch");

// Store Pump Switch Instance.
HASwitch pumpSwitch("pump_switch");

// Store Heating Error Log.
HASensor error_log("heating_error");

// Store Mode Select Instance.
// HASelect modeSelect("mode_select");


unsigned long lastTempPublishAt = 0;
float lastTemp = 45;

/**
 * @brief Configures the pump switch instance by defining its name, icon, and behavior.
 *
 * This method initializes the pump switch instance representation with a display name
 * and icon. It also sets up a command handler to respond to state changes triggered by
 * user interaction. The handler updates the pump's operational state in the system and
 * syncs the state back to the instance to reflect the change.
 */
void HomeAssistant::configurePumpInstance()
{
    pumpSwitch.setName("Pumpe");
    pumpSwitch.setIcon("mdi:pump");

    pumpSwitch.onCommand([](bool state, HASwitch* sender)
    {
        Watcher::setPumpViaHA(state);

        sender->setState(state);
    });
}

/**
 * @brief Configures the mode selection instance by setting its properties and
 * defining behavior for user commands.
 *
 * This method initializes the mode selection instance with its name, icon, and
 * available options. It also defines a command handler to handle user input
 * and update the system's operation mode based on the selected option.
 *
 * The mode options allow switching between specific operational modes, such as
 * "Consume" and "Dynamic," by invoking the appropriate system handler methods.
 */
// void HomeAssistant::configureModeInstance()
// {
//     modeSelect.setName("Modus");
//     modeSelect.setIcon("mdi:thermostat");
//     modeSelect.setOptions("Consume;Dynamic");
//
//     modeSelect.onCommand([](int8_t index, HASelect* sender)
//     {
//         switch (index)
//         {
//         case 0:
//             Watcher::setMode(Watcher::CONSUME);
//
//             break;
//         case 1:
//             Watcher::setMode(Watcher::DYNAMIC);
//
//             break;
//         }
//
//         sender->setState(index);
//     });
// }

/**
 * @brief Configures the SCR (Silicon Controlled Rectifier) instance by defining its name, icon,
 * and behavior for received commands.
 *
 * This method sets the display name and icon for the SCR instance within the Home Assistant system.
 * Additionally, it assigns a command handler that updates the state of the instance when commands
 * are received, allowing for remote control and monitoring of the SCR functionality.
 */
void HomeAssistant::configureSCRInstance()
{
    scrSwitch.setName("SCR");
    scrSwitch.setIcon("mdi:heating-coil");

    scrSwitch.onCommand([](bool state, HASwitch* sender)
    {
        Watcher::setSCRViaHA(state);

        sender->setState(state);
    });
}

/**
 * @brief Configures the heating instance with the required parameters
 * and initializes it for operation.
 *
 * This method prepares and sets up the heating instance, ensuring all
 * necessary configurations are applied before the system is ready for use.
 *
 * Proper configuration is essential for optimal performance and to
 * prevent potential issues during the operation of the heating system.
 */
void HomeAssistant::configureHeatingInstance()
{
    // Set Instance Name.
    heating.setName("Heizung");

    // Set Temperature Unit.
    heating.setTemperatureUnit(HAHVAC::CelsiusUnit);

    // Set Min Temp Step.
    heating.setTempStep(1);

    // Set Default Modes for PVHeating.
    heating.setModes(HAHVAC::HeatMode | HAHVAC::AutoMode | HAHVAC::OffMode);

    // Set Temperature Limits.
    heating.setMinTemp(45);
    heating.setMaxTemp(60);

    // Set Default Values.
    heating.setCurrentTemperature(10.00F);
    heating.setTargetTemperature(50.00F);
    heating.setCurrentTargetTemperature(10.00F);


    // Retain Heating Value.
    heating.setRetain(true);

    // Register Temperature change Listener.
    heating.onTargetTemperatureCommand([](HANumeric temperature, HAHVAC* sender)
    {
        Guardian::println("Temp changed");

        sender->setTargetTemperature(temperature);
    });

    // Register Mode change Listener.
    heating.onModeCommand([](HAHVAC::Mode mode, HAHVAC* sender)
    {
        Guardian::println("Mode changed");

        switch (mode)
        {
        case HAHVAC::HeatMode:
            Watcher::setMode(Watcher::CONSUME);
            break;
        case HAHVAC::AutoMode:
            Watcher::setMode(Watcher::DYNAMIC);
            break;
        case HAHVAC::OffMode:
            Watcher::setStandby(true);
            break;
        }

        sender->setMode(mode);
    });
}

/**
 * @brief Configures the Power instance for Home Assistant integration.
 *
 * This method performs the necessary setup for the power-related instance in the Home Assistant system.
 * It ensures that the specific configuration for monitoring or controlling power functionality is properly initialized.
 * The configuration is designed to work in conjunction with the Home Assistant framework.
 */
void HomeAssistant::configurePowerInstance()
{
    power.setName("Leistung");
    power.setDeviceClass("power");
    power.setUnitOfMeasurement("W");
    power.setIcon("mdi:flash");
}

/**
 * @brief Configures the consumption instance and initializes its properties.
 *
 * Sets up the consumption instance with predefined parameters such as
 * name, device class, unit of measurement, and a representative icon.
 * This configuration ensures accurate representation and integration of
 * the consumption sensor within the Home Assistant ecosystem.
 */
void HomeAssistant::configureConsumptionInstance()
{
    consumption.setName("Verbrauch");
    consumption.setDeviceClass("energy");
    consumption.setUnitOfMeasurement("kWh");
    consumption.setIcon("mdi:lightbulb");

    consumeMax.setName("Verbrauch Limit");
    consumeMax.setDeviceClass("energy");
    consumeMax.setUnitOfMeasurement("kWh");
    consumeMax.setIcon("mdi:lightbulb");
    consumeMax.setMin(1);
    consumeMax.setMax(10);
    consumeMax.onCommand([](HANumeric number, HANumber* sender)
    {
        Guardian::println("Max changed");

        // Check if no Reset CMD by HA.
        if (number.isSet())
        {
            Watcher::setMaxConsume(number.toFloat());
        }

        sender->setState(number);
    });

    consumeStart.setName("Start");
    consumeStart.onCommand([](HAButton* sender)
    {
        Watcher::startConsume();
    });
}

/**
 * @brief Initializes the Home Assistant integration for the system.
 *
 * This method performs the following actions:
 * - Logs a debug message indicating the initialization process has started.
 * - Establishes a connection to the Home Assistant server using MQTT protocol, with a specified hostname,
 *   username, and password.
 * - Configures the device metrics, including device name, manufacturer, and software version.
 * - Enables shared availability and last-will support for the device.
 * - Configures heating and power instances specific to the Home Assistant setup.
 * - Logs a debug message indicating the Home Assistant integration is ready for use.
 */
void HomeAssistant::begin()
{
    // Print Debug Message.
    Guardian::boot(20, "HomeAssistant");

    // Set Device Metrics.
    device.setUniqueId(LocalNetwork::getMac(), sizeof(LocalNetwork::getMac()));
    device.setModel("ESP32");
    device.setName("PVHeating");
    device.setManufacturer("Jan Heil");
    device.setSoftwareVersion(SOFTWARE_VERSION);
    device.enableSharedAvailability();
    device.enableLastWill();
    device.enableExtendedUniqueIds();

    // Configure all Instances.
    configureHeatingInstance();
    configurePowerInstance();
    configureConsumptionInstance();
    configureMaxPowerInstance();
    configureFaultInstances();
    configureFlowInstance();
    configureSCRInstance();
    //configureModeInstance();
    configurePumpInstance();
    configureErrorInstances();

    // Print Debug Message.
    Guardian::println("HomeAssistant is ready");

    // Handle MQTT Events.
    handleMQTT();

    // Print Debug Message.
    Guardian::boot(40, "MQTT");

    // Connect to HomeAssistant.
    mqtt.begin("192.168.1.181", "pvheating", "pvheating");
}

void HomeAssistant::publishChanges()
{
    if ((millis() - lastTempPublishAt) > 3000)
    {
        heating.setCurrentTemperature(lastTemp);
        lastTempPublishAt = millis();
        lastTemp += 0.5;
    }
}

/**
 * @brief Configures the fault instances with necessary attributes for
 * integration with HomeAssistant.
 *
 * This method sets up the fault instance by assigning a name, specifying
 * its device class, and defining an associated icon. These configurations
 * allow the fault entity to integrate seamlessly with HomeAssistant and
 * convey its purpose accurately.
 *
 * Proper fault instance setup is crucial to ensure effective monitoring
 * and handling of system problems within the HomeAssistant environment.
 */
void HomeAssistant::configureFaultInstances()
{
    fault.setName("Fehler");
    fault.setDeviceClass("problem");
    fault.setIcon("mdi:alert");
}

/**
 * @brief Configures the flow instance with the specified parameters
 * and initializes it with appropriate settings for monitoring flow rates.
 *
 * This method sets up the properties of the flow instance, including
 * its name, device class, unit of measurement, and icon. These configurations
 * ensure proper integration with Home Assistant and accurate representation
 * of flow data.
 *
 * The flow instance represents the flow measurement in the system, and its
 * configuration enables effective monitoring and visualization of flow rate data.
 */
void HomeAssistant::configureFlowInstance()
{
    flow.setName("Fluss");
    flow.setDeviceClass("volume_flow_rate");
    flow.setUnitOfMeasurement("L/min");
    flow.setIcon("mdi:water");
}

/**
 * @brief Configures the error log instance by defining its name and icon.
 *
 * This method initializes the error log instance for the HomeAssistant system.
 * It assigns a display name and an icon for visualization in the user interface.
 * The error log instance is meant to represent the heating error logs.
 */
void HomeAssistant::configureErrorInstances()
{
    error_log.setName("Log");
    error_log.setIcon("mdi:alert");
}

/**
 * @brief Configures the "Max Power" instance with its properties and representation.
 *
 * This method sets up the "Max Power" instance by defining its display name, device class,
 * unit of measurement, and custom icon. These configurations ensure proper integration
 * and representation in HomeAssistant or similar systems.
 */
void HomeAssistant::configureMaxPowerInstance()
{
    maxPower.setName("Max Leistung");
    maxPower.setDeviceClass("power");
    maxPower.setUnitOfMeasurement("kW");
    maxPower.setMin(2);
    maxPower.setMax(6);
    maxPower.setIcon("mdi:flash");
    maxPower.onCommand([](HANumeric number, HANumber* sender)
    {
        Watcher::setMaxPower(number.toFloat());

        sender->setState(number);
    });
}

/**
 * @brief Handles MQTT connection and disconnection events for the HomeAssistant system.
 *
 * This method sets up the callbacks to monitor the MQTT connection status. When the
 * MQTT connection is established or disconnected, respective debug messages will be
 * printed to provide status updates. It allows for dynamic handling of MQTT-based
 * events and ensures that feedback is given in the event of connectivity changes.
 */
void HomeAssistant::handleMQTT()
{
    // On MQTT Disconnect.
    mqtt.onDisconnected([]
    {
        // Print Debug Message.
        Guardian::println("MQTT is disconnected");
    });

    // On MQTT Connect.
    mqtt.onConnected([]
    {
        // Print Debug Message.
        Guardian::println("MQTT is connected");

        // Check for Errors before MQTT was initialized.
        if (Guardian::hasError())
        {
            error_log.setValue(Guardian::getErrorTitle());
        }
    });
}

/**
 * @brief Continuously executes the main execution cycle of the program.
 *
 * This method is the central loop responsible for performing repeated actions
 * within the lifetime of the program. These actions may include:
 * - Updating program state or processing incoming data.
 * - Monitoring device status or sensors.
 * - Scheduling and executing tasks at regular or event-driven intervals.
 * - Managing interactions with external systems or components.
 *
 * This method is designed to run indefinitely and is typically invoked to
 * maintain the ongoing functionality of the system.
 */
void HomeAssistant::loop()
{
    // Loop MQTT.
    mqtt.loop();

    // Publish changes.
    publishChanges();
}

HAHVAC HomeAssistant::getHVAC()
{
    return heating;
}

HASensorNumber HomeAssistant::getCurrentPower()
{
    return power;
}

HASensorNumber HomeAssistant::getConsumption()
{
    return consumption;
}

HASwitch HomeAssistant::getPump()
{
    return pumpSwitch;
}

HASwitch HomeAssistant::getSCR()
{
    return scrSwitch;
}
