#include <Arduino.h>

#include "HomeAssistant.h"
#include "Network.h"

void setup()
{
    // Begin Serial for Debugging.
    Serial.begin(9600);

    // Begin with Ethernet.
    Network::begin();

    // Begin with HA.
    HomeAssistant::begin();
}

void loop()
{
    // Check for Timeout.
    Network::update();

    // Loop HA.
    HomeAssistant::loop();
}
