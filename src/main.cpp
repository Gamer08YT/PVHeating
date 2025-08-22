#include <Arduino.h>
#include <Wire.h>

#include "Backtrace.h"
#include "Guardian.h"
#include "HomeAssistant.h"
#include "LocalModbus.h"
#include "LocalNetwork.h"
#include "SimpleTimer.h"
#include "Watcher.h"

extern "C" {
    #include "esp_debug_helpers.h"
}


// Store vars for GDB-.
size_t free_internal;
size_t free_spiram;
UBaseType_t stack_highwater;

// Store Heap Task.
SimpleTimer heapTask(1000);

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
 * @brief Handles heap and stack monitoring and periodically logs memory statistics.
 *
 * This function checks if the heap monitoring task is due based on the timer.
 * If so, it:
 * - Retrieves and logs the current size of free internal and external SPIRAM heap memory.
 * - Retrieves and logs the high-water mark of the task stack, indicating the
 *   minimum free stack space observed during runtime.
 * - Resets the timer to begin monitoring again after the specified interval.
 */
void handleHeap()
{
    if (heapTask.isReady())
    {
        // Free Heap request
        free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        free_spiram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

        // Stack High watermark of actual Task
        stack_highwater = uxTaskGetStackHighWaterMark(NULL);

        Serial.printf("Free internal: %u bytes, Free SPIRAM: %u bytes, Stack HighWater: %u words\n",
                      free_internal, free_spiram, stack_highwater);

        Backtrace::report_backtrace_to_ha();

        heapTask.reset();
    }
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

    handleHeap();
}
