//
// Created by JanHe on 27.07.2025.
//

#include "Watcher.h"

#include "PinOut.h"
#include "DallasTemperature.h"
#include "Guardian.h"
#include "OneWire.h"

// Store One Wire Instance.
OneWire oneWire(ONE_WIRE);

// Store Temperature Sensor Instance.
DallasTemperature sensors(&oneWire);


void Watcher::loop()
{
    readTemperature();
}

void Watcher::setup()
{
    // Begin One Wire Sensors.
    sensors.begin();

    // Print Debug Message.
    Guardian::println("Watcher ready");
}

void Watcher::readTemperature()
{
}
