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

// Store TemperatureIn Instance.
HASensorNumber temperatureIn("heating_in", HABaseDeviceType::PrecisionP2);

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

// Store Reset Button Instance.
HAButton reset("heating_restart");

// Store Standby Instance.
HABinarySensor standby("heating_standby");

// Store Consume Input Value Instance.
HANumber consumeMax("heating_consume_max");

// Store PWM Value Instance.
HANumber pwm("heating_pwm");

// Store Max Power.
HANumber maxPower("heating_max_power");

// Store Min Power.
HANumber minPower("heating_min_power");

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
        if (Watcher::standby)
        {
            Watcher::setPumpViaHA(state);

            pumpSwitch.setState(state);
        }
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
        if (Watcher::standby)
        {
            Watcher::setSCRViaHA(state);

            scrSwitch.setState(sender);
        }
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

        // Set Target Temperature.
        Watcher::setTargetTemperature(temperature.toFloat());

        sender->setTargetTemperature(temperature);
    });

    // Register Mode change Listener.
    heating.onModeCommand([](HAHVAC::Mode mode, HAHVAC* sender)
    {
        switch (mode)
        {
        case HAHVAC::HeatMode:
            Guardian::println("ConsumeM");

            Watcher::setMode(Watcher::CONSUME);
            break;
        case HAHVAC::AutoMode:
            Guardian::println("DynamicM");

            Watcher::setMode(Watcher::DYNAMIC);
            break;
        case HAHVAC::OffMode:
            Guardian::println("OffM");

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
    consumption.setStateClass("total_increasing");
    consumption.setUnitOfMeasurement("kWh");
    consumption.setIcon("mdi:lightbulb");

    consumeMax.setName("Verbrauch Limit");
    consumeMax.setDeviceClass("energy");
    consumeMax.setUnitOfMeasurement("kWh");
    consumeMax.setIcon("mdi:lightbulb");
    consumeMax.setMin(1);
    consumeMax.setMax(10);
    consumeMax.setRetain(true);
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
    configureTemperatureInputInstance();
    configurePowerInstance();
    configureConsumptionInstance();
    configureMaxPowerInstance();
    configureMinPowerInstance();
    configureFaultInstances();
    configureFlowInstance();
    configureSCRInstance();
    //configureModeInstance();
    configurePumpInstance();
    configureErrorInstances();
    configurePWMInstance();
    configureResetInstance();
    configureStandbyInstance();

    // Print Debug Message.
    Guardian::println("HomeAssistant is ready");

    // Handle MQTT Events.
    handleMQTT();

    // Print Debug Message.
    Guardian::boot(40, "MQTT");

    // Connect to HomeAssistant.
    mqtt.begin("192.168.1.181", "pvheating", "pvheating");
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
    maxPower.setName("Max");
    maxPower.setDeviceClass("power");
    maxPower.setUnitOfMeasurement("kW");
    maxPower.setMin(2);
    maxPower.setMax(6);
    maxPower.setIcon("mdi:flash");
    maxPower.setRetain(true);
    maxPower.onCommand([](HANumeric number, HANumber* sender)
    {
        Watcher::setMaxPower(number.toFloat() * 1000);

        sender->setState(number);
    });
}

/**
 * @brief Configures the minimum power instance for the Home Assistant integration.
 *
 * This method sets up the properties and behavior of the minimum power instance.
 * It defines the name, device class, unit of measurement, minimum and maximum values,
 * and specifies that the instance retains its state. Additionally, it associates an
 * icon for visual representation and registers a command handler to process changes
 * in the instance's state. The handler updates the system's minimum power value
 * and synchronizes the state with the instance.
 */
void HomeAssistant::configureMinPowerInstance()
{
    minPower.setName("Min");
    minPower.setDeviceClass("power");
    minPower.setUnitOfMeasurement("W");
    minPower.setMin(500);
    minPower.setMax(4000);
    minPower.setRetain(true);
    minPower.setIcon("mdi:flash");
    minPower.onCommand([](HANumeric number, HANumber* sender)
    {
        Watcher::setMinPower(number.toFloat());

        sender->setState(number);
    });
}

/**
 * @brief Configures the PWM instance for controlling the duty cycle.
 *
 * This method sets the display name and maximum value for the PWM instance.
 * It also establishes a command listener to handle value changes. When a new
 * value is received, the listener updates the PWM state in the system
 * and synchronizes the state back to the instance. The operation is
 * conditional on the system’s standby state.
 */
void HomeAssistant::configurePWMInstance()
{
    pwm.setName("Duty");
    pwm.setMax(SCR_PWM_RANGE);

    // Handle PWM Change Listener.
    pwm.onCommand([](HANumeric number, HANumber* sender)
    {
        if (Watcher::standby)
        {
            Watcher::setDuty(number.toUInt32());
            Watcher::setPWM(number.toUInt32());

            sender->setState(number);
        }
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
    // Set MQTT Keep Alive Timeout.
    mqtt.setKeepAlive(120);


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
            fault.setState(Guardian::hasError());
            error_log.setValue(Guardian::getErrorTitle());
        }
    });
}

/**
 * @brief Configures the reset button instance by setting its name and defining its behavior.
 *
 * This method initializes the reset button instance with a display name and assigns
 * a command handler. When activated by the user, the handler triggers a system restart
 * using ESP.restart().
 */
void HomeAssistant::configureResetInstance()
{
    reset.setName("Restart");
    reset.onCommand([](HAButton* sender)
    {
        ESP.restart();
    });
}

/**
 * @brief Configures the temperature input instance with its name, device class, and unit of measurement.
 *
 * This method sets up the temperature input instance by assigning it a display name,
 * categorizing it under the temperature device class, and defining its unit of measurement
 * as degrees Celsius (°C). This configuration establishes the instance for monitoring
 * and reporting temperature data in the system.
 */
void HomeAssistant::configureTemperatureInputInstance()
{
    temperatureIn.setName("Tin");
    temperatureIn.setDeviceClass("temperature");
    temperatureIn.setUnitOfMeasurement("°C");
}

/**
 * @brief Configures the standby binary sensor instance by defining its name.
 *
 * This method sets up the binary sensor instance for standby mode by assigning a
 * display name. The standby instance represents the operational state of
 * the heating system in standby mode.
 */
void HomeAssistant::configureStandbyInstance()
{
    standby.setName("Standby");
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
}

/**
 * @brief Updates the flow sensor value with the current flow rate.
 *
 * This method sets the flow sensor's value to match the provided flow rate.
 * The updated value is used to reflect changes in current flow data, and
 * synchronizes the updated state with the Home Assistant system.
 *
 * @param get_current_flowrate The current flow rate to be recorded by the flow sensor.
 */
void HomeAssistant::setFlow(float get_current_flowrate)
{
    flow.setValue(get_current_flowrate);
}

/**
 * @brief Sets the current power value and updates the corresponding sensor instance.
 *
 * This method updates the value of the power sensor to represent the current
 * operational power. The updated value may be used for monitoring purposes
 * within the Home Assistant system.
 *
 * @param current_power The current power value to be set, represented as a floating point number.
 */
void HomeAssistant::setCurrentPower(float current_power)
{
    power.setValue(current_power);
}

/**
 * @brief Sets the current temperature in the HVAC system.
 *
 * Updates the HVAC system with the latest temperature reading to ensure accurate
 * monitoring and operational adjustments.
 *
 * @param x The current temperature value to be set, in degrees.
 */
void HomeAssistant::setCurrentTemperature(float x)
{
    heating.setCurrentTemperature(x);
}

/**
 * @brief Sets the operational state of the pump via Home Assistant.
 *
 * This method updates the pump state within the system only when in standby mode.
 * It utilizes Watcher to handle state changes through Home Assistant and syncs
 * the new state with the associated pump switch instance.
 *
 * @param state The desired operational state of the pump, where true activates the
 *              pump and false deactivates it.
 */
void HomeAssistant::setPump(bool state)
{
    pumpSwitch.setState(state);
}

/**
 * @brief Updates the SCR (silicon controlled rectifier) state if the system is in standby mode.
 *
 * This method modifies the SCR's operational state through Home Assistant integration.
 * If the system is in standby mode, it resets the SCR state in the Watcher system
 * and synchronizes the state with the Home Assistant SCR switch instance.
 *
 * @param sender The new state to set for the SCR. True represents an active state,
 *               and false represents an inactive state.
 */
void HomeAssistant::setSCR(bool sender)
{
    scrSwitch.setState(sender);
}

/**
 * @brief Sets the consumption value for the Home Assistant instance.
 *
 * Updates the current consumption value associated with the sensor and
 * triggers a corresponding MQTT message if necessary. This value is used
 * to represent the energy or resource consumption in the Home Assistant
 * ecosystem.
 *
 * @param value The new consumption value to be set. Must match the expected
 * precision of the sensor instance.
 */
void HomeAssistant::setConsumption(float value)
{
    consumption.setValue(value);
}

/**
 * @brief Sets the PWM (Pulse Width Modulation) state for the heating system in Home Assistant.
 *
 * This method updates the PWM state of the heating component in the Home Assistant system
 * by assigning the provided value to the internal PWM representation.
 *
 * @param int8 The new PWM value, which defines the duty cycle or intensity of the heating system.
 */
void HomeAssistant::setPWM(uint32_t int8)
{
    pwm.setState(int8);
}

/**
 * @brief Sets the error title for the HomeAssistant instance.
 *
 * This method updates the error log with a specified error title,
 * allowing the system to track and reflect the current error state.
 *
 * @param error_title A string representing the error title to be logged.
 */
void HomeAssistant::setErrorTitle(const char* error_title)
{
    error_log.setValue(error_title);
}

/**
 * @brief Retrieves the Ethernet client used for network communications.
 *
 * This method provides access to the Ethernet client instance that is utilized
 * for communicating with external systems or services. It is typically used in
 * scenarios requiring network-based data exchange.
 *
 * @return The Ethernet client instance currently in use.
 */
NetworkClient* HomeAssistant::getClient()
{
    return &client;
}

/**
 * @brief Sets the operating mode of the heating system based on the provided input.
 *
 * This method updates the heating system's mode by mapping the given numeric input
 * to a corresponding operation mode of the HAHVAC component. The modes include heating,
 * automatic control, and turning the system off. Any unsupported inputs are ignored.
 *
 * @param str The numeric indicator for the desired mode:
 *            - 1: Heat mode
 *            - 2: Auto mode
 *            - 0: Off mode
 */
void HomeAssistant::setMode(int str)
{
    switch (str)
    {
    case 1:
        heating.setMode(HAHVAC::HeatMode);
        break;
    case 2:
        heating.setMode(HAHVAC::AutoMode);
        break;
    case 0:
        heating.setMode(HAHVAC::OffMode);
        break;
    default:
        // Should not happen.
        break;
    }
}

/**
 * @brief Updates the input temperature value for the Home Assistant system.
 *
 * This method sets the current input temperature by assigning the provided value
 * to the associated sensor representation. It ensures the temperature state
 * is properly updated within the system.
 *
 * @param temperature_in The new temperature value to set, typically in degrees.
 */
void HomeAssistant::setTemperatureIn(float temperature_in)
{
    temperatureIn.setValue(temperature_in);
}

/**
 * @brief Sets the standby state of the Home Assistant system.
 *
 * This method updates the internal standby state by setting the associated binary sensor's state.
 *
 * @param cond A boolean value representing the standby state to be set.
 *             `true` indicates standby mode is activated, and `false` indicates it is deactivated.
 */
void HomeAssistant::setStandby(bool cond)
{
    standby.setState(cond);
}
