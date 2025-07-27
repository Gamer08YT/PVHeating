//
// Created by JanHe on 27.07.2025.
//

#include "Watcher.h"

#include "PinOut.h"
#include "DallasTemperature.h"
#include "Guardian.h"
#include "HomeAssistant.h"
#include "LEDFader.h"
#include "OneButton.h"
#include "OneWire.h"
#include "SimpleTimer.h"

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
SimpleTimer sensorInterval(1000);

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

void Watcher::handleSensors()
{
    if (sensorInterval.isReady())
    {
        // Read Temperatures via OneWire.
        readTemperature();

        // Reset Timer (Endless Loop).
        sensorInterval.reset();
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
    readTemperature();
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

void Watcher::readTemperature()
{
    // Request Temperatures from Sensor.
    sensors.requestTemperatures();

    setTemperatureIn(0);
    setTemperatureOut(0);
}
