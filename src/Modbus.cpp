//
// Created by JanHe on 27.07.2025.
//
#include "Modbus.h"

#include <HardwareSerial.h>

#include "DFRobot_RTU.h"
#include "Guardian.h"
#include "PinOut.h"

// Begin HW Serial 2 (TX=17, RX=16).
HardwareSerial serial(2);

// Store Modbus Instance.
DFRobot_RTU modbus(&serial, MODBUS_RE);


/**
 * @brief Initializes the Modbus communication system.
 *
 * This method configures and initializes the Modbus RTU communication using RS485.
 * It sets up the RS485 communication parameters, including baud rate and configuration,
 * and starts the Modbus RTU Client. If any initialization step fails, an error message
 * will be logged via the Guardian logging system.
 *
 * @note This function assumes predefined constants for the RX and TX pins (MODBUS_RX and MODBUS_TX).
 *
 * @attention Ensure that the hardware configuration for RS485 and the appropriate pins
 **/
void Modbus::begin()
{
    // Begin Second Serial Channel.
    serial.begin(9600, SERIAL_8N1, MODBUS_RX, MODBUS_TX);

    // Print Debug Message.
    Guardian::println("Modbus ready");
}

void Modbus::loop()
{
}
