//
// Created by JanHe on 27.07.2025.
//

#include "HomeAssistant.h"

#include "Ethernet.h"
#include "HADevice.h"
#include "HAMqtt.h"
#include "Guardian.h"
#include "Network.h"
#include "PinOut.h""
#include "device-types/HABinarySensor.h"
#include "device-types/HAHVAC.h"
#include "device-types/HASensorNumber.h"

// Store Instance of Ethernet Client.
EthernetClient client;

// Store HADevice Instance.
HADevice device(Network::getMac());

// Store MQTT Instance.
HAMqtt mqtt(client, device);

// Store HAVAC Instance.
HAHVAC heating("heating", HAHVAC::TargetTemperatureFeature | HAHVAC::PowerFeature | HAHVAC::ModesFeature);

// Store Power Usage Instance.
HASensorNumber power("heating_load", HABaseDeviceType::PrecisionP2);

// Store Power Consumption Instance.
HASensorNumber consumption("heating_consumption", HABaseDeviceType::PrecisionP2);

// Store Error State Instance.
HABinarySensor fault("heating_fault");

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
    // Set Temperature Unit.
    heating.setTemperatureUnit(HAHVAC::CelsiusUnit);

    // Set Min Temp Step.
    heating.setTempStep(1);

    // Set Default Modes for PVHeating.
    heating.setModes(HAHVAC::HeatMode | HAHVAC::AutoMode | HAHVAC::OffMode);

    // Set Temperature Limits.
    heating.setMinTemp(45);
    heating.setMaxTemp(60);

    // Register Temperature change Listener.
    heating.onTemperatureChange([](float temperature)
    {
        Guardian::println("Temp changed");
    });

    // Register Power Command change Listener.
    heating.onPowerCommand([](bool state, HAHVAC* sender)
    {
        Guardian::println("Power changed");
    });

    // Register Mode change Listener.
    heating.onModeChange([](HAHVAC::Mode mode, HAHVAC* sender)
    {
        Guardian::println("Mode changed");
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
    Guardian::println("Begin HomeAssistant");

    // Connect to HomeAssistant.
    mqtt.begin("homeassistant.local", "pvheating", "pvheating");

    // Set Device Metrics.
    device.setName("PVHeating");
    device.setManufacturer("Jan Heil");
    device.setSoftwareVersion(SOFTWARE_VERSION);
    device.enableSharedAvailability();
    device.enableLastWill();


    configureHeatingInstance();
    configurePowerInstance();
    configureConsumptionInstance();
    configureFaultInstances();


    // Print Debug Message.
    Guardian::println("HomeAssistant is ready");
}

void HomeAssistant::publishChanges()
{
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
