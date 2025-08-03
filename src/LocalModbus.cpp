//
// Created by JanHe on 27.07.2025.
//
#include "LocalModbus.h"

#include <HardwareSerial.h>

#include "DFRobot_RTU.h"
#include "Guardian.h"
#include "PinOut.h"
#include "MeterRegisters.h"
#include "Ethernet.h"
// #include "ModbusClientTCP.h"


// Begin HW Serial 2 (TX=17, RX=16).
HardwareSerial serial(2);

// Store Instanceo of TCP Instance.
// ModbusClientTCP modbusTCP(client);

// Store Modbus Instance.
DFRobot_RTU modbusRTU(&serial, MODBUS_RE);


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
void LocalModbus::begin()
{
    // Print Debug Message.
    Guardian::boot(50, "Modbus");

    // Begin Second Serial Channel.
    serial.begin(9600, SERIAL_8N1, MODBUS_RX, MODBUS_TX);

    // Begin Modbus RTU Client.

    // Begin Modbus TCP Client.
    beginTCP();

    // Print Debug Message.
    Guardian::println("Modbus ready");
}

void LocalModbus::loop()
{
}

/**
 * @brief Reads a 32-bit float value from a remote Modbus input register.
 *
 * This method interacts with the Modbus TCP client to read data from a specified
 * remote input register address. The data is read as two 16-bit Modbus registers
 * and converted to a 32-bit float value using a Big-Endian format. If the read
 * operation is unsuccessful, the method will return a default error value.
 *
 * @param address The address of the remote input register to be read.
 *
 * @return The 32-bit float value retrieved from the specified address on success,
 * or -1 if the read operation fails.
 */
float LocalModbus::readRemote(int address)
{
    // Simple Data Buffer Array.
    uint16_t data[REGISTER_LENGTH];

    // Read into Buffer.
    // modbusTCP.addRequest(1, address, data, REGISTER_LENGTH);

    return -1;
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
float LocalModbus::readLocal(int address)
{
    // Simple Data Buffer Array.
    uint16_t data[REGISTER_LENGTH];

    // Read into Buffer.
    uint8_t result = modbusRTU.readInputRegister(1, address, data, REGISTER_LENGTH);

    if (result == 0)
    {
        // Erfolgreiche Lesung
        // Convert the two 16-bit Register to 32-bit Float
        union
        {
            uint32_t i;
            float f;
        } converter;

        // Big-Endian converting.
        converter.i = ((uint32_t)data[0] << 16) | data[1];

        return converter.f;
    }

    return -1;
}

void LocalModbus::beginTCP()
{
    // // Set Target of TCP Connection.
    // modbusTCP.setTarget(IPAddress(MODBUS_HOUSE), 502);
    //
    // // Begin Modbus TCP Client.
    // modbusTCP.begin(1);
}
