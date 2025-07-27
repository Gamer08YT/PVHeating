//
// Created by JanHe on 27.07.2025.
//

#include "Watcher.h"

#include "PinOut.h"
#include "DallasTemperature.h"
#include "Guardian.h"
#include "HomeAssistant.h"
#include "LEDFader.h"
#include "Modbus.h"
#include "OneButton.h"
#include "OneWire.h"
#include "SimpleTimer.h"
#include "MeterRegisters.h"

// Definitions from Header.
Watcher::ModeType Watcher::mode = Watcher::CONSUME;
bool Watcher::error = false;
bool Watcher::standby = false;
float Watcher::temperatureIn = 0.0f;
float Watcher::temperatureOut = 0.0f;
float Watcher::maxConsume = 0.0f;
float Watcher::currentPower = 0.0f;
float Watcher::consumption = 0.0f;


// Store One Wire Instance.
OneWire oneWire(ONE_WIRE);

// Store Temperature Sensor Instance.
DallasTemperature sensors(&oneWire);

// Store Button Instances.
OneButton faultButton(BUTTON_FAULT, true);
OneButton modeButton(BUTTON_MODE, true);

// Store LED Instances.
LEDFader faultLed(LED_FAULT);
LEDFader modeLed(LED_MODE);

// Store Timer.
SimpleTimer fastInterval(500);
SimpleTimer slowInterval(2000);

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

void Watcher::readHouseMeterPower()
{
}

/**
 * @brief Sets the consumption value for the system.
 *
 * This method assigns the given consumption value to the class's static
 * consumption member. It is typically used to update the system's
 * consumption data based on external readings or calculations.
 *
 * @param con The new consumption value to be set.
 */
void Watcher::setConsumption(float con)
{
    consumption = con;
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
    float consumption = Modbus::readLocal(POWER_IMPORT);

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
        readLocalPower();

        // Reset Timer (Endless Loop);
        fastInterval.reset();
    }

    if (slowInterval.isReady())
    {
        Guardian::println("S > Sensors");
        // Read Temperatures via OneWire.
        //readTemperature();

        // Read HA Power to compensate.
        if (mode == ModeType::DYNAMIC)
        {
            readLocalConsumption();
            readHouseMeterPower();
        }

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
    handleSensors();
    readButtons();
    handleButtonLeds();
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
    // Begin One Wire Sensors.
    sensors.begin();

    // Setup Pins.
    setupPins();

    // Setup Button Listeners.
    setupButtons();

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
    float currentPower = Modbus::readLocal(POWER_USAGE);

    setPower(currentPower);
}

/**
 * @brief Manages the fade effect of the standby LED based on a condition.
 *
 * This method adjusts the state of the `modeLed` to reflect whether the system
 * is in standby mode. If the provided condition differs from the current standby
 * state, it either initiates a fade-in effect over a specified duration or stops
 * any active fade and disables the LED.
 *
 * @param cond A boolean indicating the desired standby state. True to activate
 *        fade-in and light the LED, false to stop any fade and turn off the LED.
 */
void Watcher::handleStandbyLedFade(bool cond)
{
    if (standby != cond)
    {
        if (cond)
        {
            // Fade in 1000 Seconds Interval.
            modeLed.fade(255, 1000);
        }
        else
        {
            // Stop Fade and Disable LED.
            faultLed.stop_fade();
            modeLed.set_value(0);
        }
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
    if (error != cond)
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
 * @brief Reads and processes temperature data from connected sensors.
 *
 * This method communicates with the DallasTemperature library to fetch temperature data
 * from sensors connected via the OneWire interface. If two sensors are detected, it identifies the
 * colder and warmer temperature readings and assigns them as "temperatureIn" and "temperatureOut"
 * respectively. The internal states are updated using the setTemperatureIn and setTemperatureOut methods.
 *
 * In scenarios where the number of connected sensors is not equal to two, it sets the temperature values
 * to -1 and triggers an error state using the setError method. The error is also logged via the Guardian system.
 *
 * This function is integral to maintaining accurate environmental data within the system.
 */
void Watcher::readTemperature()
{
    // Request Temperatures from Sensor.
    sensors.requestTemperatures();

    if (sensors.getDeviceCount() == 2)
    {
        float tempOne = sensors.getTempCByIndex(0);
        float tempTwo = sensors.getTempCByIndex(1);

        if (tempOne > tempTwo)
        {
            setTemperatureIn(tempTwo);
            setTemperatureOut(tempOne);
        }
        else
        {
            setTemperatureIn(tempOne);
            setTemperatureOut(tempTwo);
        }
    }
    else
    {
        setTemperatureIn(-1);
        setTemperatureOut(-1);
        setError(true);

        Guardian::println("OneWire Error");
    }
}
