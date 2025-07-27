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
    readButtons();
}

void Watcher::setup()
{
    // Begin One Wire Sensors.
    sensors.begin();

    // Setup Pins.
    setupPins();

    // Print Debug Message.
    Guardian::println("Watcher ready");
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

void Watcher::readButtons()
{
}

void Watcher::readTemperature()
{
}
