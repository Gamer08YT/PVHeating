#include <Arduino.h>

#include "ElegantOTA.h"
#include "HomeAssistant.h"
#include "Modbus.h"
#include "LocalNetwork.h"


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
}

void loop()
{
    // Check for Timeout.
    LocalNetwork::update();

    // Loop HA.
    HomeAssistant::loop();

    // Loop Modbus.
    Modbus::loop();
}
