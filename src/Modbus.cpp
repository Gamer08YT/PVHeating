//
// Created by JanHe on 27.07.2025.
//
#include "Modbus.h"

#include <ArduinoRS485.h>
#include <ArduinoModbus.h>

#include "Guardian.h"
#include "PinOut.h"


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
    // Begin RS485 on Second UART Channel.
    RS485.begin(9600, SERIAL_8N1, MODBUS_RX, MODBUS_TX);

    // Init Modbus RTU Master
    if (!ModbusRTUClient.begin(RS485))
    {
        Guardian::println("Failed to start Modbus RTU Client.");
    }

    Guardian::println("Modbus initialized");
}

void Modbus::loop()
{
}
