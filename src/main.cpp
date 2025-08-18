#include <Arduino.h>
#include <Wire.h>

#include "Guardian.h"
#include "HomeAssistant.h"
#include "LocalModbus.h"
#include "LocalNetwork.h"
#include "Watcher.h"


/**
 * @brief Initializes the system components and prepares the application to run.
 *
 * This method performs setup operations required to initialize various system
 * components and ensure they are ready for operation. It includes:
 * - Starting the Serial communication for debugging at a baud rate of 9600.
 * - Initializing the LocalNetwork module for Ethernet communication.
 * - Initializing the HomeAssistant integration module.
 * - Starting the Modbus communication protocol.
 * - Setting up the Watcher module responsible for monitoring and controlling
 *   system states.
 */
void setup()
{
    // Begin Serial for Debugging.
    Serial.begin(115200);

    // Setup Pins.
    Watcher::setupPins();

    // Setup Display.
    Guardian::setup();

    // Show Boot Message.
    Guardian::println("Booting...");

    // // Clear Error Message.
    // Guardian::clearError();

    // Begin with Ethernet.
    LocalNetwork::begin();

    // Begin with HA.
    HomeAssistant::begin();

    // Begin Modbus.
    LocalModbus::begin();

    // Setup Watcher.
    Watcher::setup();

    // Clear Display after Boot Screen.
    Guardian::clear();
    Guardian::update();
}

/**
 * @brief Executes a repetitive process until a certain condition is met.
 *
 * This method is responsible for performing a loop operation. The loop runs
 * repeatedly, typically executing a block of code within it, until a specified
 * termination condition is satisfied. This can be used for iterative processing,
 * continuous checks, or tasks requiring repeated execution.
 */
void loop()
{
    // Check for Timeout.
    LocalNetwork::update();

    // Loop HA.
    HomeAssistant::loop();

    // Loop Modbus.
    LocalModbus::loop();

    // Loop Watcher.
    Watcher::loop();
}
