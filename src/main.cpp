#include <Arduino.h>

#include "ElegantOTA.h"
#include "HomeAssistant.h"
#include "Modbus.h"
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
    Serial.begin(9600);

    // Begin with Ethernet.
    LocalNetwork::begin();

    // Begin with HA.
    HomeAssistant::begin();

    // Begin Modbus.
    Modbus::begin();

    // Setup Watcher.
    Watcher::setup();
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
    Modbus::loop();

    // Loop Watcher.
    Watcher::loop();
}
