//
// Created by JanHe on 27.07.2025.
//

#include "Watcher.h"

#include "PinOut.h"
#include "DallasTemperature.h"
#include "Guardian.h"
#include "HomeAssistant.h"
#include "LEDFader.h"
#include "LocalModbus.h"
#include "OneButton.h"
#include "OneWire.h"
#include "SimpleTimer.h"
#include "MeterRegisters.h"
#include "FlowSensor.h"
#include "LocalNetwork.h"
#include "WebSerial.h"

#define SLOW_INTERVAL 2000
#define PUBLISH_INTERVAL 1000

// Definitions from Header.
Watcher::ModeType Watcher::mode = Watcher::CONSUME;
bool Watcher::standby = true;
float Watcher::temperatureIn = 0.0f;
float Watcher::temperatureOut = 0.0f;
float Watcher::maxConsume = 0.0f;
float Watcher::startConsumed = 0.0F;
float Watcher::currentPower = 0.0f;
float Watcher::maxPower = 6000.0f;
float Watcher::housePower = 0.0f;
float Watcher::consumption = 0.0f;
int Watcher::duty = 0;
int Watcher::temperatureMax = 60;
float Watcher::flowRate = 0.0f;
bool displayFlow = false;

// Store One Wire Instance.
OneWire oneWire(ONE_WIRE);

// Store Temperature Sensor Instance.
DallasTemperature sensors(&oneWire);

// Store Found Addresses.
DeviceAddress address;

// Store count of Devices.
int foundDevices = 0;

// Store Button Instances.
OneButton faultButton(BUTTON_FAULT, true);
OneButton modeButton(BUTTON_MODE, true);

// Store LED Instances.
LEDFader faultLed(LED_FAULT);
LEDFader modeLed(LED_MODE);
LEDFader scrPWM(SCR_PWM);

// Store Timer.
SimpleTimer fastInterval(500);
SimpleTimer slowInterval(SLOW_INTERVAL);

// Publish Timer for HA to save Bandwidth.
SimpleTimer publishInterval(PUBLISH_INTERVAL);

// Store Read State.
bool readTimer = false;

// Store Flow Meter Instance (I know it's easy, but i have Time rush).
FlowSensor meter(YFB5, FLOW_PULSE);

/**
 * @brief Updates the fault and mode LEDs using their respective controllers.
 *
 * This method invokes the update functionality of the LEDFader objects associated with
 * fault and mode LEDs. It ensures the LEDs reflect any ongoing transitions or effects
 * such as fading, occurring due to changes in their state or value.
 *
 * Designed for integration in the main program loop to provide periodic updates
 * to the LED states and maintain consistency with the system's functionality.
 */
void Watcher::handleButtonLeds()
{
    faultLed.update();
    modeLed.update();
}

/**
 * @brief Publishes updated power and PWM values to HomeAssistant at regular intervals.
 *
 * This method checks if the specified timer interval for publishing data is ready using the
 * `publishInterval` timer. If the interval has elapsed, it sends the current system power and
 * PWM duty cycle to HomeAssistant by calling respective methods in the HomeAssistant class.
 * After publishing, the timer is reset to ensure periodic execution.
 *
 * Designed for integration within the main loop to synchronize system data with HomeAssistant.
 */
void Watcher::handleHAPublish()
{
    if (publishInterval.isReady())
    {
        HomeAssistant::setCurrentPower(currentPower);
        HomeAssistant::setPWM(duty);
        HomeAssistant::setFlow(flowRate);

        publishInterval.reset();
    }
}

/**
 * @brief Handles the Pulse-Width Modulation (PWM) functionality based on power conditions.
 *
 * This method evaluates the `currentPower` member variable to determine if the power level
 * is acceptable for further action. It facilitates the PWM control mechanism by incorporating
 * system-specific power logic, updating or managing outputs as needed.
 *
 * This function is intended to be invoked within the periodic system update cycle, ensuring
 * real-time responsiveness to changes in the power state.
 */
void Watcher::handlePWM()
{
    bool tempToLow = isTempToLow();

    if (standby || Guardian::isCritical() || !tempToLow)
    {
        Guardian::println("Shutdown");

        duty = 0;

        // Disable SCR and Pump.
        setSCR(false);

        // Disable Pump on Error && Standby but if no of these are active don't disable on Temp to high.
        setPump((!Guardian::isCritical() && !standby && !tempToLow));
    }
    else
    {
        // Enable Pump and SCR.
        setSCR(true);
        setPump(true);

        // Handle Max Power Limit.
        if (checkLocalPowerLimit())
        {
            Guardian::println("MaxP");

            if (duty > 0)
            {
                duty--;
            }
        }
        else
        {
            // Handle Modes serpate.
            if (mode == ModeType::DYNAMIC)
            {
                // Handle Dynamic Mode.
                handlePowerBasedDuty();
            }
            else
            {
                // Handle Consume Mode.
                handleConsumeBasedDuty();
            }
        }
    }

    // Update PWM Value.
    setPWM(duty);
}

/**
 * @brief Updates the display with current system metrics and information.
 *
 * This method updates the title and key-value pairs displayed on the user interface.
 * It reflects the current state of critical parameters such as duty cycle, power,
 * temperature, and consumption. After setting these values, the display is refreshed
 * to ensure the information presented is up-to-date.
 *
 * Designed for periodic invocation to maintain consistency between the system's
 * internal state and its visual representation on the dashboard.
 */
void Watcher::updateDisplay()
{
    if (!Guardian::hasError() && LocalNetwork::isUploading)
    {
        Guardian::clear();
        Guardian::setTitle("Dashboard");
        Guardian::setValue(1, "PWM", String(duty, 2).c_str());
        Guardian::setValue(2, "Pin", String(currentPower, 2).c_str(), "W");

        if (displayFlow)
            Guardian::setValue(3, "Tout", String(temperatureOut, 2).c_str(), "C");
        else
            Guardian::setValue(3, "Tin", String(temperatureIn, 2).c_str(), "C");

        // Show Flow or Work.
        if (displayFlow)
            Guardian::setValue(4, "Flow", String(flowRate, 2).c_str(), "l/min");
        else
            Guardian::setValue(4, "Wheat", String(consumption, 2).c_str(), "kWh");

        displayFlow = !displayFlow;

        Guardian::update();
    }
}

/**
 * @brief Updates the current flow rate and propagates it to the HomeAssistant system.
 *
 * This method captures the current flow rate from the flow meter or sensor,
 * assigns it to a local variable, and synchronizes the value with the HomeAssistant
 * platform to ensure the system has the updated flow data for further processing or display.
 *
 * @param get_current_flowrate The current flow rate measured by the flow meter.
 */
void Watcher::setFlow(float get_current_flowrate)
{
    flowRate = get_current_flowrate;

    // Removed do to MQTT Timeouts.
    // HomeAssistant::setFlow(get_current_flowrate);
}

/**
 * @brief Updates the current temperature in the HomeAssistant system.
 *
 * This function communicates the system's current temperature (stored in
 * the temperatureOut variable) to HomeAssistant, ensuring synchronization
 * between the Watcher component and HomeAssistant's representation of the
 * environmental conditions.
 *
 * Typically invoked during the periodic sensor update routine.
 */
void Watcher::updateTemperature()
{
    HomeAssistant::setCurrentTemperature(temperatureOut);
}

/**
 * @brief Handles operations that occur at a slower predefined interval.
 *
 * This method performs a sequence of tasks when the slow timer interval is ready:
 * - Reads temperature data using a OneWire interface.
 * - Updates the temperature information in HomeAssistant for monitoring or automation purposes.
 * - Calculates and updates the flow rate based on the readings from the flow meter.
 * - Reads local power consumption data to monitor the current usage.
 * - If the system is in DYNAMIC mode, reads the active power of the house meter for real-time adjustments.
 * - Updates the OLED display with the latest information.
 *
 * Once all these operations are complete, the slow timer interval is reset to ensure proper timing
 * for the next cycle of operations.
 *
 * Designed to be called within the main program loop or as part of a periodic task handler to
 * execute time-sensitive updates and calculations at a slower rate.
 */
void Watcher::handleSlowInterval()
{
    if (slowInterval.isReady())
    {
        // Read Temperatures via OneWire.
        readTemperature();

        // Update Temperature in HomeAssistant.
        updateTemperature();

        // Calculate Flow.
        meter.read();

        // Set Flow Rate.
        setFlow(meter.getFlowRate_m());

        // Read Local Consumption.
        readLocalConsumption();

        // Read HA Power to compensate.
        if (mode == ModeType::DYNAMIC)
        {
            // Read House Meter Active Power.
            readHouseMeterPower();
        }

        // Update OLED.
        updateDisplay();

        // Reset Timer (Endless Loop).
        slowInterval.reset();
    }
}

/**
 * @brief Handles periodic tasks executed at a fast interval for the system.
 *
 * This method is responsible for performing operations that require execution
 * within a short time interval. Specific tasks include:
 * - Reading internal power usage from the Smart Meter using `readLocalPower()`.
 * - Managing the PWM duty cycle to adjust performance based on the system state.
 * - Resetting the fast interval timer to allow continuous periodic execution.
 *
 * Designed to be called frequently to maintain real-time system responsiveness
 * for fast-changing parameters.
 */
void Watcher::handleFastInterval()
{
    if (fastInterval.isReady())
    {
        // Read internal Smart Meter Power Usage.
        readLocalPower();

        // Handle PWM Duty.
        handlePWM();

        // Reset Timer (Endless Loop);
        fastInterval.reset();
    }
}

/**
 * @brief Reads the active power imported by the house meter and updates the house power.
 *
 * This method retrieves the current power reading from the house meter using the
 * LocalModbus communication interface, specifically at the POWER_IMPORT register.
 * The retrieved power value is then passed to the `setHousePower` function to update
 * the state of the system with the latest house power measurement.
 *
 * Used during the dynamic operational mode to compensate for external power data
 * in system calculations and ensure accurate power monitoring.
 */
void Watcher::readHouseMeterPower()
{
    LocalModbus::readRemote(POWER_USAGE);
}

/**
 * @brief Updates the system's power consumption value.
 *
 * This method sets the internal consumption value for the Watcher class and then
 * communicates the updated consumption to the Home Assistant system by updating the
 * corresponding sensor's current value.
 *
 * @param con The updated consumption value to be stored and propagated.
 */
void Watcher::setConsumption(float con)
{
    consumption = con;

    HomeAssistant::setConsumption(consumption);
}

/**
 * @brief Sets the PWM duty cycle for the SCR using the specified value.
 *
 * This method writes the provided duty cycle value to the SCR_PWM pin.
 * It is used to control the PWM signal for the SCR based on the
 * calculated duty cycle value.
 *
 * @param duty The PWM duty cycle value to be set. It is an 8-bit signed integer.
 */
void Watcher::setPWMHA(int8_t duty)
{
    // Write SCR PWM Duty via calculated Duty.
    analogWrite(SCR_PWM, duty);
}

/**
 * @brief Sets the PWM duty cycle and synchronizes it with Home Assistant.
 *
 * This method updates the pulse-width modulation (PWM) duty cycle by setting
 * the calculated value using the system's hardware abstraction and
 * propagates this update to Home Assistant for integration with external systems.
 *
 * @param int8 The duty cycle value to be applied, represented as an 8-bit integer.
 */
void Watcher::setPWM(int8_t int8)
{
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "PWM: %i", int8);

    Guardian::println(buffer);

    setPWMHA(int8);

    // Removed do to MQTT Timeouts.
    // HomeAssistant::setPWM(int8);
}

/**
 * @brief Sets the maximum target temperature for the system.
 *
 * This method updates the `temperatureMax` variable, which represents
 * the highest allowable target temperature. It is used to define
 * temperature settings that the system should aim to maintain.
 *
 * @param is_int8 The new target temperature, specified as an integer.
 */
void Watcher::setTargetTemperature(int is_int8)
{
    temperatureMax = is_int8;
}

/**
 * @brief Reads the local power consumption data and updates the system's consumption state.
 *
 * This method retrieves the local power consumption value from the Modbus system using the
 * POWER_IMPORT register. The value obtained is then passed to the setConsumption method, which
 * updates the internal consumption state of the Watcher class.
 *
 * Intended to be invoked periodically to ensure the system's consumption data remains accurate
 * and aligns with real-time readings.
 */
void Watcher::readLocalConsumption()
{
    LocalModbus::readLocal(POWER_IMPORT);
}

/**
 * @brief Handles periodic operations on sensors using predefined intervals.
 *
 * This method utilizes two timer intervals, `fastInterval` and `slowInterval`, to manage
 * sensor-related tasks efficiently. It resets the `fastInterval` timer when it's ready,
 * ensuring continuous operation for high-frequency tasks.
 *
 * Additionally, when the `slowInterval` timer is ready, it performs a set of sensor-related
 * actions, such as printing a log via `Guardian::println`, and prepares for the next execution
 * by resetting the timer.
 *
 * Designed to operate within the main program loop for periodic updates and maintaining
 * consistent interaction with sensors.
 */
void Watcher::handleSensors()
{
    handleFastInterval();
    handleSlowInterval();
}

/**
 * @brief Manages periodic tasks and updates within the main application.
 *
 * This method serves as the central execution loop for the application. It handles
 * tasks such as polling, state updates, or invoking methods necessary to ensure
 * the system operates as expected. Designed to run continuously within the
 * program's execution lifecycle, it facilitates the overall workflow of the application.
 */
void Watcher::loop()
{
    scrPWM.update();
    faultLed.update();
    modeLed.update();

    handleSensors();
    readButtons();
    handleButtonLeds();
    handleHAPublish();
}

/**
 * @brief Resets the system settings and components to their default states.
 *
 * This method ensures the system is initialized with default configurations
 * by disabling the pump and SCR components. It calls respective methods for
 * these components to ensure consistent behavior across dependent systems.
 *
 * Typically used during setup or when resetting the system to a known state.
 */
void Watcher::setDefaults()
{
    setPump(false);
    setSCR(false);
}

/**
 * @brief Configures button interactions for fault and mode functionalities.
 *
 * This method initializes button event listeners for handling specific actions:
 * - Resets the fault state upon a long press of the fault button.
 * - Switches the operational mode between `CONSUME` and `DYNAMIC` upon a single click of the mode button.
 * - Activates standby mode upon a double click of the mode button.
 *
 * The method leverages lambda functions assigned to button events for executing the associated logic.
 */
void Watcher::setupButtons()
{
    // Reset FAULT State on Long Press.
    faultButton.attachLongPressStart([]
    {
        Guardian::println("Fault L");

        Guardian::clearError();
    });

    // Switch MODE State on Click.
    modeButton.attachClick([]
    {
        Guardian::println("Mode S");

        setStandby(false);
        setMode((mode == ModeType::CONSUME) ? ModeType::DYNAMIC : ModeType::CONSUME);
    });

    // Standby on Long Press.
    modeButton.attachLongPressStart([]
    {
        Guardian::println("Mode L");

        setStandby(true);
    });
}

void Watcher::setup()
{
    // Set Default States.
    setDefaults();

    // Print Debug Message.
    Guardian::boot(90, "Watcher");

    scrPWM.fade(255, 1000);
    modeLed.fade(255, 1000);
    faultLed.fade(255, 1000);

    // Begin 1Wire and Scan for Devices.
    begin1Wire();

    // Setup Pins.
    setupPins();

    // Setup Button Listeners.
    setupButtons();

    // Setup Flow Meter.
    setupFlowMeter();

    // Print Debug Message.
    Guardian::println("Watcher ready");
}

/**
 * @brief Sets the standby state of the system.
 *
 * This method updates the standby state to the given condition. It can be
 * used to place the system into a standby mode or take it out of standby
 * by toggling the `standby` member variable.
 *
 * @param cond The boolean condition to set the standby state.
 *             A value of `true` enables standby mode, and `false` disables it.
 */
void Watcher::setStandby(bool cond)
{
    if (cond)
        Guardian::println("Standby");

    // Switch LED Fade vs Blink State.
    handleStandbyLedFade(cond);

    // Disable SCR.
    if (cond)
    {
        setSCR(false);
    }

    standby = cond;
}

/**
 * @brief Sets the maximum consumption limit for the watcher.
 *
 * This method updates the maximum consumption value used by the Watcher.
 * It allows dynamic adjustment of the consumption limit during runtime, ensuring
 * the system adheres to newly specified constraints.
 *
 * @param to_float The new maximum consumption value to set for the watcher.
 */
void Watcher::setMaxConsume(float to_float)
{
    maxConsume = to_float;
}

/**
 * @brief Sets the pump state and updates it via HomeAssistant.
 *
 * This method updates the state of the pump by invoking the associated HomeAssistant
 * control interface and propagating the state change through the home automation
 * system using helper methods.
 *
 * @param sender The desired state of the pump. True for enabling, false for disabling the pump.
 */
void Watcher::setPump(bool sender)
{
    HomeAssistant::setPump(sender);

    setPumpViaHA(sender);
}

/**
 * @brief Sets the State Control Relay (SCR) using the provided HASwitch object.
 *
 * This method is intended to configure or control an instance of the SCR by
 * utilizing the functionality of the HASwitch passed as the parameter.
 * It is typically invoked within the context of setting up or adjusting
 * the state of the associated system component.
 *
 * @param sender A pointer to the HASwitch object used to configure the SCR.
 */
void Watcher::setSCR(bool sender)
{
    HomeAssistant::setSCR(sender);

    setSCRViaHA(sender);
}

/**
 * @brief Controls the SCR state via an external Home Automation command.
 *
 * This method sets the state of the SCR (Silicon Controlled Rectifier)
 * by writing the specified value to the associated controller pin. The
 * state of the SCR is determined by the input parameter.
 *
 * It is designed to interface with external automation systems that
 * manage SCR control functionality.
 *
 * @param state The desired state of the SCR. A value of `true` enables
 * the SCR, while `false` disables it.
 */
void Watcher::setSCRViaHA(bool state)
{
    digitalWrite(SCR_ENABLE, !state);
}

/**
 * @brief Controls the pump state through Home Automation (HA) commands.
 *
 * This method sets the pump state based on instructions received from the
 * Home Automation system. It ensures that the pump operates in accordance
 * with the desired state communicated by HA, maintaining proper synchronization
 * between the system and the external control.
 *
 * @param state A boolean value where `true` turns the pump on and `false` turns it off.
 */
void Watcher::setPumpViaHA(bool state)
{
    digitalWrite(PUMP_ENABLE, !state);
}

/**
 * @brief Initiates the consume mode and updates initial consumption tracking.
 *
 * This method disables the standby state by calling setStandby(false) and then checks
 * if the current operational mode is set to ModeType::CONSUME. If the condition is met,
 * it sets the startConsumed variable to the current consumption value. This ensures
 * the system begins tracking consumption from the moment consume mode is activated.
 *
 * This function plays a critical part in initializing specific behaviors linked with
 * the consume mode of the Watcher system.
 */
void Watcher::startConsume()
{
    setStandby(false);

    if (mode == ModeType::CONSUME)
    {
        startConsumed = consumption;
    }
    //digitalWrite(LED_MODE, HIGH);
}

/**
 * @brief Sets the maximum power level for the system.
 *
 * This method assigns a new value to the `maxPower` variable, which represents
 * the maximum allowable power level for the system. It is used to adjust the
 * system's operational limits as needed, ensuring control over power usage.
 *
 * @param to_float The new maximum power level to be set.
 */
void Watcher::setMaxPower(float to_float)
{
    maxPower = to_float;
}

/**
 * @brief Sets the operational mode of the system.
 *
 * This method updates the system's mode to the specified value. It is used
 * to transition the system into a new mode of operation.
 *
 * @param cond The mode type to set for the system. Acceptable values
 *             are defined in the `ModeType` enumeration.
 */
void Watcher::setMode(ModeType cond)
{
    mode = cond;
}

/**
 * @brief Incrementally updates the internal pulse count of the flow meter.
 *
 * This method is invoked as an interrupt service routine (ISR) to capture
 * and count the pulses generated by the connected flow sensor. The pulse count
 * is essential for calculating flow rate and other metrics during measurements.
 *
 * Associated with the FlowMeter instance, this ISR ensures real-time pulse
 * capturing for accurate and consistent measurement data. It should be registered
 * as the callback function for the specific flow sensor interrupt.
 */
void MeterISR()
{
    meter.count();
}

/**
 * @brief Initializes and scans devices on the 1-Wire bus.
 *
 * This method configures the DallasTemperature library for use with the connected 1-Wire sensors. It disables the
 * default wait-for-conversion behavior to allow asynchronous operation, ensuring efficient temperature readings.
 *
 * The method then retrieves and stores the number of devices detected on the bus. It iterates through all devices,
 * attempting to retrieve and print their addresses using the provided interface. If a device does not respond with
 * an address, the method outputs a warning message highlighting potential issues with power or cabling.
 *
 * Designed for execution during system setup to prepare the application for ongoing temperature monitoring.
 */
void Watcher::begin1Wire()
{
    // Begin One Wire Sensors.
    sensors.begin();

    // https://github.com/milesburton/Arduino-Temperature-Control-Library/issues/113#issuecomment-389638589
    sensors.setWaitForConversion(false);

    // Grab a count of devices on the wire.
    foundDevices = sensors.getDeviceCount();

    // Loop through each device, print out address
    for (int i = 0; i < foundDevices; i++)
    {
        // Search the wire for address
        if (sensors.getAddress(address, i))
        {
            Serial.print("Found device ");
            Serial.print(i, DEC);
            Serial.print(" with address: ");
            printAddress(address);
            Serial.println();
        }
        else
        {
            Serial.print("Found ghost device at ");
            Serial.print(i, DEC);
            Serial.print(" but could not detect address. Check power and cabling");
        }
    }
}


/**
 * @brief Prints the hexadecimal address of a given device.
 *
 * This method iterates through the bytes of the provided DeviceAddress and
 * prints each byte in hexadecimal format to the serial monitor. A leading zero
 * is added for single-digit hexadecimal values to ensure a consistent two-character
 * format per byte.
 *
 * @param deviceAddress A DeviceAddress object representing the address of the device
 * to be printed. The address is an array of 8 bytes.
 */
void Watcher::printAddress(DeviceAddress deviceAddress)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        if (deviceAddress[i] < 16)
            Serial.print("0");
        Serial.print(deviceAddress[i], HEX);
    }
}

/**
 * @brief Configures and initializes the flow meter object.
 *
 * This method sets up the flow meter by instantiating a FlowMeter object.
 * The object is configured with the specified interrupt pin, sensor properties,
 * an interrupt service routine, and an interrupt trigger mode.
 *
 * The flow meter is used to measure liquid flow rates and volumes, and
 * initialization through this method ensures it is ready for use within the system.
 */
void Watcher::setupFlowMeter()
{
    meter.begin(MeterISR);
}

/**
 * @brief Sets the current power consumption value for the system.
 *
 * This method updates the `currentPower` variable with the provided power value.
 * It is used to store the power consumption reading, which can be leveraged
 * in other parts of the system for monitoring or decision-making processes.
 *
 * @param current_power The power consumption value to be set, represented as a float.
 */
void Watcher::setPower(float current_power)
{
    currentPower = current_power;

    // Removed do to MQTT Timeouts.
    // HomeAssistant::setCurrentPower(current_power);
}

/**
 * @brief Reads the local power usage from the connected device.
 *
 * This method utilizes the LocalModbus interface to retrieve the current
 * power consumption value, specified by the POWER_USAGE register. The
 * information collected is essential for monitoring and analyzing the
 * system's internal power usage, enabling energy management and
 * optimization functionalities.
 *
 * Intended to be called periodically for consistent updates on power
 * consumption in scenarios that involve ongoing monitoring or regulation.
 */
void Watcher::readLocalPower()
{
    LocalModbus::readLocal(POWER_USAGE);
}

/**
 * @brief Sets the house power value in the Watcher class.
 *
 * This method updates the static housePower variable with the given value. It is used
 * to modify the power data associated with the house, enabling the system to reflect
 * the latest power reading or state.
 *
 * @param house_power The power value to set for the house, provided as a float.
 */
void Watcher::setHousePower(float house_power)
{
    housePower = house_power;
}

/**
 * @brief Manages the fading behavior of the mode and fault LEDs based on a given condition.
 *
 * If the condition is true, this method initiates a fade-in effect on the mode LED over
 * a 1000-millisecond interval. If the condition is false, it halts any ongoing fade
 * effect on the fault LED, prevents further transitions, and sets the mode LED to
 * its minimum brightness level stored in the duty variable.
 *
 * @param cond A boolean value indicating whether to enable or disable the fade effect.
 *             - true: Initiates the fade-in effect for the mode LED.
 *             - false: Stops the fade effect for the fault LED and adjusts the mode LED brightness.
 */
void Watcher::handleStandbyLedFade(bool cond)
{
    if (cond)
    {
        // Fade in 1000 Seconds Interval.
        modeLed.fade(255, 1000);
    }
    else
    {
        // Stop Fade set LED Brightness of Duty Cycle.
        faultLed.stop_fade();
        modeLed.set_value(duty);
    }
}

/**
 * @brief Handles the fading behavior of the error LED based on the given condition.
 *
 * This method checks if the current state of the error LED differs from the specified
 * condition. If the condition indicates activation, the LED begins fading in over
 * a 1000 millisecond interval. Otherwise, the fade process is stopped, and the LED
 * is turned off by setting its value to 0.
 *
 * @param cond Boolean value indicating the desired state of the error LED. If `true`,
 *             the LED will fade in. If `false`, the fade will stop, and the LED will
 *             be disabled.
 */
void Watcher::handleErrorLedFade(bool cond)
{
    if (cond)
    {
        // Fade in 1000-second Interval.
        faultLed.fade(255, 1000);
    }
    else
    {
        // Stop Fade and Disable LED.
        faultLed.stop_fade();
        faultLed.set_value(0);
    }
}

void Watcher::setupPins()
{
    // Set Input Pins.
    pinMode(BUTTON_FAULT, INPUT);
    pinMode(BUTTON_MODE, INPUT);
    pinMode(FLOW_PULSE, INPUT);
    pinMode(SCR_FAULT, INPUT);

    // Set Output Pins.
    pinMode(LED_MODE, OUTPUT);
    pinMode(LED_FAULT, OUTPUT);
    pinMode(PUMP_ENABLE, OUTPUT);
    pinMode(SCR_ENABLE, OUTPUT);
    pinMode(SCR_PWM, OUTPUT);
}

/**
 * @brief Reads the state of input buttons and updates their internal state.
 *
 * This method handles the input from two buttons (`faultButton` and `modeButton`)
 * by invoking their `tick` methods. The button state is updated using the
 * OneButton library to ensure proper debounce and detection of button events.
 *
 * This function is typically called periodically within the main program loop to
 * ensure real-time response to button presses.
 */
void Watcher::readButtons()
{
    faultButton.tick();
    modeButton.tick();
}


/**
 * @brief Reads temperature data from connected sensors and updates internal temperature variables.
 *
 * This method communicates with temperature sensors to retrieve temperature values
 * for all detected devices. For each sensor, it checks its address, retrieves the
 * temperature in Celsius, and determines the relationship between the current readings.
 * The colder value is set to `temperatureIn`, while the warmer value is set to `temperatureOut`.
 *
 * The function also handles initialization of an intermediate temperature
 * variable to ensure proper comparison of sensor outputs.
 *
 * Designed to be part of the periodic sensor handling process for managing temperature data.
 */
void Watcher::readTemperature()
{
    if (readTimer)
    {
        // Fix: https://github.com/milesburton/Arduino-Temperature-Control-Library/issues/113#issuecomment-389638589
        pinMode(ONE_WIRE, OUTPUT);
        digitalWrite(ONE_WIRE, HIGH);

        float tempTemperature = -1.0F;


        // Loop through each device, print out temperature data
        for (int i = 0; i < foundDevices; i++)
        {
            float tempC = sensors.getTempCByIndex(i);

            // Check if temp Temperature is Set.
            if (tempTemperature == -1)
            {
                tempTemperature = tempC;
            }
            else
            {
                if (tempC > tempTemperature)
                {
                    temperatureIn = tempTemperature;
                    temperatureOut = tempC;
                }
                else
                {
                    temperatureIn = tempC;
                    temperatureOut = tempTemperature;
                }
            }
        }

        // Set Fail if Dallas Temperature Sensor failed to initialize.
        if
        (tempTemperature >= 85.0F)
        {
            Guardian::setError(100, "Temp init fail", Guardian::CRITICAL);
        }
    }
    else
    {
        // Request Temperatures from Sensor.
        sensors.requestTemperatures();
    }

    // Switch Read Timer.
    readTimer = !readTimer;
}

/**
 * @brief Adjusts the duty cycle based on the power being imported or exported by the house meter.
 *
 * This method monitors the power flow indicated by `housePower` to determine
 * whether the system is consuming (positive power) or producing/exporting (negative power).
 * It incrementally increases the duty cycle if the system is exporting power and
 * decreases it if the system is consuming power, ensuring the duty remains within
 * the range of 0 to 254.
 *
 * Designed for dynamic control in power balancing systems by adapting the duty
 * cycle to reflect the current power state.
 */
void Watcher::handlePowerBasedDuty()
{
    // Check if House Meter is Importing or Exporting.
    if (housePower < 0)
    {
        // If Power is producing/exporting like -1000W
        if (duty < 254)
            duty++;
    }
    else
    {
        // If Power is consuming/importing like 800W.
        if (duty > 0)
            duty--;
    }
}

/**
 * @brief Checks if the current power exceeds the maximum allowable power limit.
 *
 * This function compares the current power consumption against the defined
 * maximum power threshold. It helps enforce power constraints and ensures
 * operations adhere to the predefined limits.
 *
 * @return true if the current power exceeds the maximum power limit, false otherwise.
 */
bool Watcher::checkLocalPowerLimit()
{
    return (currentPower > maxPower);
}

/**
 * @brief Adjusts the duty cycle based on the difference between the current power
 * and the specified maximum power.
 *
 * This method compares the current power consumption with the provided maximum power
 * value. If the current power exceeds the maximum power, it decreases or increases
 * the duty cycle (within the range of 0 to 255) to regulate power usage and maintain
 * optimal system performance.
 *
 * This logic helps to manage resource usage dynamically and ensures that the system
 * responds appropriately to varying power demands.
 *
 * @param max_power The maximum allowable power limit to regulate against.
 */
void Watcher::handleMaxPower(float max_power)
{
    if (currentPower > max_power)
        if (duty > 0)
            duty--;
        else if (duty < 255)
            duty++;
}

/**
 * @brief Adjusts the system's duty cycle based on consumption thresholds.
 *
 * This method ensures that the system remains within acceptable consumption limits.
 * If the total consumption is within the defined maximum threshold, it invokes the
 * `handleMaxPower` function to regulate the power consumption. Otherwise, it transitions
 * the system to standby mode and resets the duty cycle.
 *
 * Designed for the "Consume" operating mode to dynamically regulate the duty cycle
 * in response to consumption metrics.
 *
 * Preconditions:
 * - `startConsumed` is a finite value.
 * - `consumption` represents the current total consumed power.
 * - `maxConsume` specifies the maximum allowed additional consumption.
 */
void Watcher::handleConsumeBasedDuty()
{
    if (std::isfinite(startConsumed) && consumption < maxConsume + startConsumed)
    {
        handleMaxPower(maxPower);
    }
    else
    {
        setStandby(true);

        // Reset Duty.
        duty = 0;
    }
}

/**
 * @brief Checks if the external temperature is too low or undefined.
 *
 * This method evaluates whether the `temperatureOut` value indicates a low temperature
 * condition or is in an invalid (undefined) state. It returns true if the temperature
 * is below the defined `temperatureMax` threshold or if the value of `temperatureOut`
 * is not a finite number.
 *
 * @return true if the external temperature is too low or undefined, false otherwise.
 */
bool Watcher::isTempToLow()
{
    // If TemperatureOut is undefined.
    if (!std::isfinite(temperatureOut))
        return true;

    return temperatureOut < temperatureMax;
}
