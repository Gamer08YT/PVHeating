//
// Created by JanHe on 27.07.2025.
//
#include "Modbus.h"

#include <HardwareSerial.h>

#include "DFRobot_RTU.h"
#include "Guardian.h"
#include "PinOut.h"
#include "MeterRegisters.h"

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

/**
 * @brief Reads a 32-bit floating-point value from a local Modbus input register.
 *
 * This method reads data from a specific Modbus input register address. It retrieves
 * two 16-bit registers, combining them into a 32-bit value, and converts it to a
 * floating-point representation. The Big-Endian format is used for byte ordering.
 *
 * @param address The Modbus register address to read from.
 *
 * @return The 32-bit floating-point value read from the specified Modbus register.
 *         If the read operation fails, the return value is undefined.
 */
float Modbus::readLocal(int address)
{
    // Simple Data Buffer Array.
    uint16_t data[REGISTER_LENGTH];

    // Read into Buffer.
    uint8_t result = modbus.readInputRegister(1, address, data, REGISTER_LENGTH);

    if (result == 0) {  // Erfolgreiche Lesung
        // Convert the two 16-bit Register to 32-bit Float
        union {
            uint32_t i;
            float f;
        } converter;

        // Big-Endian converting.
        converter.i = ((uint32_t)data[0] << 16) | data[1];

        return converter.f;
    }

}
