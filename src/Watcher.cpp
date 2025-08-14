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
bool Watcher::tempLock = false;
bool Watcher::powerLock = false;
float Watcher::temperatureIn = 0.0f;
float Watcher::temperatureOut = 0.0f;
float Watcher::maxConsume = 0.0f;
float Watcher::startConsumed = 0.0F;
float Watcher::currentPower = 0.0f;
float Watcher::maxPower = 6000.0f;
float Watcher::minPower = 500.0f;
float Watcher::housePower = 0.0f;
float Watcher::consumption = 0.0f;
u_int32_t Watcher::duty = 0;
u_int8_t Watcher::standbyCounter = 0;
float Watcher::temperatureMax = 60.0F;
float Watcher::flowRate = 0.0f;

// Sometimes the OneWire Lib is unable to decode buffer and Returns +85°C.
int oneWireOutOfRange = 0;

// Store OneWire Clear Interval.
int oneWireClearInterval = 0;

// Switch between Display Mode.
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
OneButton faultButton(BUTTON_FAULT, false);
OneButton modeButton(BUTTON_MODE, false);

// Store LED Instances.
//LEDFader faultLed(LED_FAULT);
//LEDFader modeLed(LED_MODE);

// Store Timer.
SimpleTimer fastInterval(500);
SimpleTimer slowInterval(SLOW_INTERVAL);

// Publish Timer for HA to save Bandwidth.
SimpleTimer publishInterval(PUBLISH_INTERVAL);

// Store Read State.
bool readTimer = false;

// Store Flow Meter Instance (I know it's easy, but I have Time rush).
FlowSensor meter(YFB5, FLOW_PULSE);

/**
 * @brief Updates the fault and mode LEDs using their respective controllers.
 *
 * This method invokes the update functionality of the LEDFader objects associated with
 * fault and mode LEDs. It ensures the LEDs reflect any ongoing transitions or effects,
 * such as fading, occurring due to changes in their state or value.
 *
 * Designed for integration in the main program loop to provide periodic updates
 * to the LED states and maintain consistency with the system's functionality.
 */
void Watcher::handleButtonLeds()
{
    //faultLed.update();
    //modeLed.update();
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

        // Check for DallaTemp Lib Error.
        if (temperatureIn > 0)
        {
            HomeAssistant::setTemperatureIn(temperatureIn);
        }

        publishInterval.reset();
    }
}

/**
 * @brief Handles the PWM control logic, managing temperatures, power, and system states.
 *
 * This method governs the system’s PWM (Pulse Width Modulation) operations by executing
 * conditional checks and actions based on temperature thresholds, power limits, and
 * operating modes. It ensures safe operation by implementing temperature locks,
 * managing duty cycles, and toggling system components like SCR and the pump.
 *
 * Key functionalities include:
 * - Monitoring over-temperature and taking critical actions to prevent damage,
 *   including disabling components and setting an error state.
 * - Applying a temperature lock to delay operation when the temperature exceeds
 *   defined limits and ensuring sufficient cooling before resuming normal function.
 * - Adjusting PWM duty cycle dynamically based on the operational mode, such as DYNAMIC
 *   or CONSUME, and verifying power limits to optimize system performance.
 * - Controlling SCR and pump activation based on the current state of the system,
 *   including standby, locks, and power constraints.
 *
 * Intended to run regularly as part of the system's main or interval loop to
 * maintain operational safety and efficiency.
 */
void Watcher::handlePWM()
{
    if (!isOverTemp())
    {
        // If Temp >= 60°C
        if (!isTempToLow())
        {
            duty = 0;

            // Disable SCR and Pump.
            setSCR(false);

            // Add Templock.
            tempLock = true;

            // Print Debug Message.
            Guardian::println("TempLock");

            // // Disable Pump on Error && Standby, but if no of these are active, don't disable on Temp to high.
            // setPump((!Guardian::isCritical() && !standby && !tempToLow));
        }
        // If Temp < 60°C
        else
        {
            // If Temperature-Lock is active.
            if (tempLock)
            {
                // Wait until the Temperature drops like 5°C.
                if (temperatureOut < (temperatureMax - 5.0F))
                {
                    // Remove Temperature Log.
                    tempLock = false;
                }
            }
            // If Temperature-Lock is not active.
            else
            {
                // If Max Power is exceeded. (currentPower > MaxPower).
                if (checkLocalPowerLimit())
                {
                    Guardian::println("MaxP");

                    if (duty > 0)
                        duty--;
                }
                // If currentPower < MaxPower.
                else
                {
                    // Handle Modes separate.
                    // Dynamic Mode (Zero Export Mode)
                    if (mode == ModeType::DYNAMIC)
                        // Handle Dynamic Mode.
                        handlePowerBasedDuty();
                        // Consume Mode (Consume X Energy)
                    else
                        // Handle Consume Mode.
                        handleConsumeBasedDuty();
                }

                if (!standby && !tempLock && !powerLock)
                {
                    // Update PWM Value.
                    setPWM(duty);

                    // Enable Pump and SCR.
                    setSCR(true);
                    setPump(true);
                }
            }
        }
    }
    else
    {
        // Set Error Log and disable Heater.
        Guardian::setError(55, "Overtemp", Guardian::CRITICAL);
    }
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
        Guardian::setValue(1, "PWM", String(duty).c_str());
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
    // Check for DallaTemp Lib Error.
    if (temperatureOut < 85)
    {
        HomeAssistant::setCurrentTemperature(temperatureOut);
    }
}

/**
 * @brief Manages the reset mechanism for OneWire error counters.
 *
 * This method regulates the decrement of the `oneWireOutOfRange` error counter
 * by utilizing a time-based interval defined by `oneWireClearInterval`.
 * The process ensures that errors are gradually cleared over time, minimizing
 * sudden resumptions or changes in states.
 *
 * Specifically, the interval counter (`oneWireClearInterval`) is incremented with
 * each invocation until it reaches the threshold of 2. Once the threshold is met,
 * one error is subtracted from the `oneWireOutOfRange` counter, and the interval
 * counter is reset. This mechanism operates only when `oneWireOutOfRange` has a
 * value greater than zero, ensuring that the process halts when no errors are present.
 */
void Watcher::handleOneWireClearInterval()
{
    if (oneWireOutOfRange > 0)
    {
        // Every 2 Steps * 2 Seconds, decrement 1 from the Fail List.
        // Increment Step.
        if (oneWireClearInterval < 2)
        {
            oneWireClearInterval++;
        }
        // Decrement one Error, if Interval exceed.
        else
        {
            Guardian::println("OFR: c");

            oneWireOutOfRange--;
            oneWireClearInterval = 0;
        }
    }
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

        // Clear OneWire Errors.
        handleOneWireClearInterval();

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
void Watcher::setPWMHA(u_int32_t duty)
{
    // Write SCR PWM Duty via calculated Duty.
    ledcWrite(SCR_PWM, duty);
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
void Watcher::setPWM(u_int32_t int8)
{
    char buffer[15];
    snprintf(buffer, sizeof(buffer), "PWM: %u", int8);

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
void Watcher::setTargetTemperature(float is_int8)
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
    //faultLed.update();
    //modeLed.update();

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
        HomeAssistant::setMode(mode == ModeType::CONSUME ? 1 : 2);
    });

    // Standby on Long Press.
    modeButton.attachLongPressStart([]
    {
        Guardian::println("Mode L");

        setStandby(true);

        HomeAssistant::setMode(0);
    });
}

/**
 * @brief Initializes and sets up the Watcher system components and peripherals.
 *
 * This method configures the Watcher system by performing the following steps:
 * - Sets default system states using `setDefaults`.
 * - Outputs a boot message to the debug console via `Guardian::boot`.
 * - Initializes fade transitions for the fault LED, mode LED, and SCR PWM using `fade`.
 * - Starts the 1-Wire protocol and scans for connected devices with `begin1Wire`.
 * - Configures GPIO pins via `setupPins`.
 * - Sets up the button listeners with `setupButtons`.
 * - Configures the flow meter using `setupFlowMeter`.
 * - Outputs a "ready" message to the debug console indicating the system is fully initialized.
 *
 * This method is intended to be called during the system initialization phase,
 * providing a comprehensive setup routine for all essential components.
 */
void Watcher::setup()
{
    // Set Default States.
    setDefaults();

    // Print Debug Message.
    Guardian::boot(90, "Watcher");

    // Begin 1-Wire and Scan for Devices.
    begin1Wire();

    // Setup Pins. -> Moved to main.cpp to avoid multiple relais switching on start.
    // setupPins();

    // Setup Button Listeners.
    setupButtons();

    // Setup Flow Meter.
    setupFlowMeter();

    // Print Debug Message.
    Guardian::println("Watcher ready");
}

/**
 * @brief Sets the system to standby mode or exits it based on the given condition.
 *
 * This method adjusts the system's operational state by enabling or disabling
 * standby mode. When entering standby mode:
 * - Outputs a standby message.
 * - Toggles the LED state between fade and blink.
 * - Disables the SCR and sets PWM to 0.
 * The new state is stored for system-wide reference.
 *
 * @param cond If true, the system enters standby mode; if false, it exits standby mode.
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
        setPump(false);
        setPWM(0);
    }
    else
    {
        duty = 0;
        setPWM(0);
    }

    standby = cond;

    HomeAssistant::setStandby(cond);
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
        String floatStr = String(consumption + maxConsume);
        String str = "To: ";

        // Append String.
        str.concat(floatStr);

        // Print to Console.
        Guardian::println(str.c_str());

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
    duty = 0;

    setPWM(duty);

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

    // Use 10 Bit Resolution.
    sensors.setResolution(10);

    // https://github.com/milesburton/Arduino-Temperature-Control-Library/issues/113#issuecomment-389638589
    sensors.setWaitForConversion(false);

    // Grab a count of devices on the wire.
    foundDevices = sensors.getDeviceCount();

    // Loop through each device, print out the address
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
        digitalWrite(LED_MODE, LOW);
        // Fade in 1000-second Interval.
        //modeLed.fade(255, 1000);
    }
    else
    {
        digitalWrite(LED_MODE, HIGH);
        // // Stop Fade set LED Brightness of Duty Cycle.
        // faultLed.stop_fade();
        // modeLed.set_value(duty);
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
        digitalWrite(LED_FAULT, HIGH);
        // Fade in 1000-second Interval.
        //faultLed.fade(255, 1000);
    }
    else
    {
        digitalWrite(LED_FAULT, LOW);
        // Stop Fade and Disable LED.
        //faultLed.stop_fade();
        //faultLed.set_value(0);
    }
}

/**
 * @brief Sets the duty value for the Watcher.
 *
 * This method assigns the provided integer value to the static duty member of the Watcher class.
 * It is used to update and modify the internal duty state, which can affect the system's behavior
 * or operational parameters based on the new duty value.
 *
 * @param int8 The new duty value as an 8-bit signed integer.
 */
void Watcher::setDuty(u_int32_t int8)
{
    duty = int8;
}

/**
 * @brief Sets the minimum power level for the watcher system.
 *
 * This method updates the internal minimum power variable to the specified value.
 * The minimum power setting is utilized to impose constraints or thresholds
 * on system operation, ensuring it adheres to defined parameters.
 *
 * @param to_float The new minimum power value, specified as a floating-point number.
 */
void Watcher::setMinPower(float to_float)
{
    minPower = to_float;
}

/**
 * @brief Configures the input and output pins used by the system.
 *
 * This method initializes the microcontroller's GPIO pins to their required
 * modes based on their functionality in the system. It designates certain pins
 * as inputs, such as buttons and sensors, and others as outputs for controlling
 * peripherals like LEDs, pumps, and SCR modules. Additionally, it configures
 * PWM (Pulse Width Modulation) for the SCR control, setting the desired frequency
 * and resolution.
 *
 * The method is a critical step in the system's initialization process to ensure
 * proper hardware configuration before executing system logic.
 */
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

    // Set PWM Frequency.
    ledcAttach(SCR_PWM, SCR_PWM_FREQUENCY, SCR_PWM_RESOLUTION);
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

        // // Set Fail if Dallas Temperature Sensor failed to initialize.
        // if
        // (tempTemperature >= 85.0F)
        // {
        //     Guardian::setError(100, "TempInit", Guardian::CRITICAL);
        // }
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
 * @brief Determines if the current power generation is sufficient.
 *
 * This method evaluates whether the absolute value of the generated power
 * exceeds the configured minimum power requirement. If the house is
 * exporting power (negative house power) beyond the minimum threshold,
 * it returns true.
 *
 * @return True if the current power generation meets or exceeds the
 *         required minimum power threshold, false otherwise.
 */
bool Watcher::isEnoughPowerGeneration()
{
    return (housePower <= -minPower);
}

/**
 * @brief Manages the standby counter and transitions to standby mode if the interval is reached.
 *
 * This method increments the standby counter on each invocation until it reaches the predefined
 * standby interval (STANDBY_INTERVAL). Once the counter reaches this value, the system transitions
 * to standby mode by invoking the setStandby() method with a true condition.
 *
 * Works in tandem with other system components to ensure the transition to standby mode adheres to
 * defined intervals. Useful for managing power and operational states.
 */
void Watcher::handleStandbyCounterDisable()
{
    // Equals 15 Seconds.
    if (standbyCounter < STANDBY_INTERVAL)
    {
        standbyCounter++;
    }
    else
    {
        powerLock = true;
    }
}

/**
 * @brief Manages the state of the standby counter and transitions out of standby mode if necessary.
 *
 * This method decrements the `standbyCounter` if it is greater than zero. If the `standbyCounter`
 * reaches zero, the system transitions out of standby mode by calling `setStandby(false)`.
 *
 * Typically used as part of the system's operational logic to control standby state
 * based on a countdown mechanism.
 */
void Watcher::handleStandbyCounterEnable()
{
    if (standbyCounter > 0)
    {
        standbyCounter--;
    }
    else
    {
        powerLock = false;
    }
}

/**
 * @brief Adjusts the duty cycle based on the power being imported or exported by the house meter.
 *
 * This method monitors the power flow indicated by `housePower` to determine
 * whether the system is consuming (positive power) or producing/exporting (negative power).
 * It incrementally increases the duty cycle if the system is exporting power and
 * decreases it if the system is consuming power, ensuring the duty remains within
 * the range of 0 to X.
 *
 * Designed for dynamic control in power balancing systems by adapting the duty
 * cycle to reflect the current power state.
 */
void Watcher::handlePowerBasedDuty()
{
    // If Generation is a negative Value.
    // Eq. Exporting
    if (isEnoughPowerGeneration())
    {
        handleStandbyCounterEnable();

        // If Power is producing/exporting like -1000 W
        if (duty < SCR_PWM_RANGE)
            duty = duty + SCR_PWM_STEP;
    }
    // If Generation is a positive Value.
    // eq. Importing
    else
    {
        // If Power is not enough to generate.
        if (duty > SCR_PWM_STEP)
            duty = duty - SCR_PWM_STEP;

        handleStandbyCounterDisable();
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
    {
        Guardian::println("M");

        if (duty > SCR_PWM_STEP)
            duty = duty - SCR_PWM_STEP;
    }
    else
    {
        if (duty < SCR_PWM_RANGE)
            duty = duty + SCR_PWM_STEP;
    }
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
 * @brief Checks if the system temperature is too low based on operational limits.
 *
 * This method evaluates the value of the temperature output and compares it to
 * a predefined maximum threshold. If the temperature is not a finite number (e.g., NaN),
 * the method considers the temperature undefined and returns false. Otherwise, it returns
 * true if the temperature is below the defined maximum.
 *
 * @return True if the temperature is below the maximum threshold and defined,
 *         otherwise false.
 */
bool Watcher::isTempToLow()
{
    // If TemperatureOut is undefined.
    // If TempOut != NaN = true
    if (!std::isfinite(temperatureOut))
    {
        Guardian::println("Tisfinite");

        return false;
    }

    // If piping hot Spot < Max => true
    // If piping hot Spot > Max => false
    return (temperatureOut < temperatureMax);
}

/**
 * @brief Checks if the system is operating beyond allowable temperature limits.
 *
 * This method monitors the internal and external temperatures reported by sensors
 * to determine if the system is overheating. It also detects if the sensor is reporting
 * an out-of-range value (such as 85.0). If the sensor repeatedly reports out-of-range
 * values, the system triggers a critical error using the Guardian system.
 *
 * @return True if either the internal or external temperature exceeds the defined
 * maximum threshold; otherwise, false.
 */
bool Watcher::isOverTemp()
{
    float maxTemp = 62.0F;

    // Check if OneWire Sensor is Out of Range.
    if (temperatureIn == 85.0F || temperatureOut == 85.0F)
    {
        // Increment Value if it's under 6 Errors.
        if (oneWireOutOfRange < 6)
        {
            oneWireOutOfRange++;

            char buffer[10];
            snprintf(buffer, sizeof(buffer), "OFR+: %u", oneWireOutOfRange);

            Guardian::println(buffer);
        }
        // If Error exceeds 6 Fails, Shutdown!
        else
        {
            Guardian::setError(100, "TempInit", Guardian::CRITICAL);
        }

        return false;
    }

    return temperatureOut >= maxTemp || temperatureIn >= maxTemp;
}
