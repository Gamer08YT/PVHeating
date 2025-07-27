#include <Arduino.h>

#include "Network.h"

void setup()
{
    // Begin Serial for Debugging.
    Serial.begin(9600);

    // Begin with Ethernet.
    Network::begin();
}

void loop()
{
    // write your code here
    Network::update();
}
