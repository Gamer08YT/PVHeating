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

// Definitions from Header.
Watcher::ModeType Watcher::mode = Watcher::CONSUME;
bool Watcher::error = false;
bool Watcher::standby = true;
float Watcher::temperatureIn = 0.0f;
float Watcher::temperatureOut = 0.0f;
float Watcher::maxConsume = 0.0f;
float Watcher::currentPower = 0.0f;
float Watcher::maxPower = 6000.0f;
float Watcher::housePower = 0.0f;
float Watcher::consumption = 0.0f;
int Watcher::duty = 0;
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
    if (standby || error)
    {
        duty = 0;

        // Disable SCR and Pump.
        setSCR(false);
        setPump(false);
    }
    else
    {
        // Handle Max Power Limit.
        if (checkLocalPowerLimit())
        {
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

    // Write SCR PWM Duty via calculated Duty.
    analogWrite(SCR_PWM, duty);
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
    if (!error && LocalNetwork::isUploading)
    {
        Guardian::clear();
        Guardian::setTitle("Dashboard");
        Guardian::setValue(1, "PWM", String(duty, 2).c_str());
        Guardian::setValue(2, "Pin", String(currentPower, 2).c_str(), "W");
        Guardian::setValue(3, "Tout", String(temperatureOut, 2).c_str(), "C");

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

    HomeAssistant::setFlow(get_current_flowrate);
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
    LocalModbus::readRemote(POWER_IMPORT);
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

    HomeAssistant::getConsumption().setCurrentValue(consumption);
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
    float consumption = LocalModbus::readLocal(POWER_IMPORT);

    setConsumption(consumption);
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
    if (fastInterval.isReady())
    {
        // Read internal Smart Meter Power Usage.
        readLocalPower();

        // Handle PWM Duty.
        handlePWM();

        // Reset Timer (Endless Loop);
        fastInterval.reset();
    }

    if (slowInterval.isReady())
    {
        // Read Temperatures via OneWire.
        //readTemperature();

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

        // For Debug.
        readHouseMeterPower();

        // Update OLED.
        updateDisplay();

        // Reset Timer (Endless Loop).
        slowInterval.reset();
    }
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
        setError(false);
    });

    // Switch MODE State on Click.
    modeButton.attachClick([]
    {
        setStandby(false);
        setMode((mode == ModeType::CONSUME) ? ModeType::DYNAMIC : ModeType::CONSUME);
    });

    modeButton.attachDoubleClick([]
    {
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
    Guardian::println("S > Standby");

    // Switch LED Fade vs Blink State.
    handleStandbyLedFade(cond);

    standby = cond;
}

/**
 * @brief Sets an error state and logs the action.
 *
 * This method is used to update the error state of the system.
 * It logs the error update using the `Guardian::println` method
 * and modifies the internal `error` state based on the provided condition.
 *
 * @param cond A boolean value representing the error condition.
 * If `true`, the error state is activated. If `false`, the error state is cleared.
 */
void Watcher::setError(bool cond)
{
    Guardian::println("S > Error");

    handleErrorLedFade(cond);

    error = cond;
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
    HomeAssistant::getPump().setState(sender);

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
    HomeAssistant::getSCR().setState(sender);

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
 * @brief Initiates the consumption process by exiting standby mode.
 *
 * This method alters the system's operational state by disabling standby mode
 * through the invocation of the setStandby method with an input of `false`.
 * It is designed to signal the system to prepare for or transition into
 * an active consumption state.
 */
void Watcher::startConsume()
{
    setStandby(false);
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
    Guardian::println("S > Mode");

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

void Watcher::begin1Wire()
{
    // Begin One Wire Sensors.
    sensors.begin();

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

    HomeAssistant::getCurrentPower().setCurrentValue(currentPower);
}

/**
 * @brief Reads the local power usage and updates the current power state.
 *
 * This method accesses the local power usage data through the Modbus interface by
 * reading the value associated with the POWER_USAGE register. The retrieved power
 * value is then passed to the setPower() method to update the internal state and
 * ensure that the current power is synchronized with the latest reading.
 *
 * Designed to be invoked periodically as part of sensor handling routines, this method
 * ensures that the local power data is accurately reflected in the system's state.
 */
void Watcher::readLocalPower()
{
    float currentPower = LocalModbus::readLocal(POWER_USAGE);

    setPower(currentPower);
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
 * @brief Sets the internal temperature reading.
 *
 * This method assigns the specified temperature value to the internal
 * temperature tracking variable, ensuring the system has the latest
 * updated temperature data for internal operations.
 *
 * @param i The new temperature value to set for the internal temperature.
 */
void Watcher::setTemperatureIn(float i)
{
    temperatureIn = i;
}

/**
 * @brief Sets the output temperature value.
 *
 * This method updates the temperatureOut variable with the specified value.
 *
 * @param i The new temperature value to set for the output.
 */
void Watcher::setTemperatureOut(float i)
{
    temperatureOut = i;

    // Set Current Temperature.
    HomeAssistant::getHVAC().setCurrentTemperature(i, false);
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
    // Request Temperatures from Sensor.
    sensors.requestTemperatures();
    float tempTemperature = -1;

    // Loop through each device, print out temperature data
    for (int i = 0; i < foundDevices; i++)
    {
        // Search the wire for address
        if (sensors.getAddress(address, i))
        {
            // Output the device ID
            Serial.print("Temperature for device: ");
            Serial.println(i,DEC);

            // Print the data
            float tempC = sensors.getTempC(address);

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
    }
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
 * @brief Increments the duty cycle to regulate power consumption in consume mode.
 *
 * When the system operates in consume mode, this function is invoked to
 * increase the duty cycle, which adjusts the power regulation or device
 * behavior accordingly. This gradual adjustment ensures a controlled
 * increase in power consumption levels based on the system's operation.
 *
 * Designed to function as part of the system's power management strategy
 * specifically in consume mode behavior.
 */
void Watcher::handleConsumeBasedDuty()
{
    if (consumption < maxConsume)
    {
        // Increment Duty.
        if (duty < 254)
            duty++;
    }
    else
    {
        // Reset Duty.
        duty = 0;
    }
}
