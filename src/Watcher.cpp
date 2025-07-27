//
// Created by JanHe on 27.07.2025.
//

#include "Watcher.h"

#include "PinOut.h"
#include "DallasTemperature.h"
#include "Guardian.h"
#include "OneButton.h"
#include "OneWire.h"

// Store One Wire Instance.
OneWire oneWire(ONE_WIRE);

// Store Temperature Sensor Instance.
DallasTemperature sensors(&oneWire);

// Store Button Instances.
OneButton faultButton(BUTTON_FAULT, true);
OneButton modeButton(BUTTON_MODE, true);


void Watcher::loop()
{
    readTemperature();
    readButtons();
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

void Watcher::readTemperature()
{
}
