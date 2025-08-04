//
// Created by JanHe on 27.07.2025.
//
#include "LocalModbus.h"

#include <HardwareSerial.h>
#include <WebServer.h>

#include "DFRobot_RTU.h"
#include "Guardian.h"
#include "PinOut.h"
#include "MeterRegisters.h"
#include "Ethernet.h"
#include "ModbusRTU.h"
// #include "ModbusClientTCP.h"


// Begin HW Serial 2 (TX=17, RX=16).
HardwareSerial serial(2);

// Store Instanceo of TCP Instance.
// ModbusClientTCP modbusTCP(client);

// Store Modbus Instance.
ModbusRTU modbusRTU;


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
    modbusRTU.begin(&serial, MODBUS_RE, true);

    // Set Baud of MAX485.
    modbusRTU.setBaudrate(9600);

    // Set Server Mode (Old. Master)
    modbusRTU.server();

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
    // Check if transaction is active.
    if (!modbusRTU.server())
    {
        modbusRTU.readIreg()
    }
    else
        Guardian::println("Modbus RTU is not active");

    // Add Request to Task and Request.
    modbusRTU.task();
}


void LocalModbus::beginTCP()
{
    // // Set Target of TCP Connection.
    // modbusTCP.setTarget(IPAddress(MODBUS_HOUSE), 502);
    //
    // // Begin Modbus TCP Client.
    // modbusTCP.begin(1);
}


/**
 * @brief Validates the checksum of the received data packet.
 *
 * This function checks the integrity of a data packet by calculating and comparing the checksum.
 * It ensures that the received data is not corrupted during transmission.
 *
 * @return true if the calculated checksum matches the expected checksum, false otherwise.
 *
 * @note The input data format and its checksum must adhere to the expected protocol standards.
 *
 * @warning Invalid checksums indicate a failure in data integrity and may require retransmission.
 *
 * @link https://github.com/reaper7/SDM_Energy_Meter/blob/5d1653a0550fab7de13fbe596a1b09d1fae103ad/SDM.cpp#L647
 **/
bool LocalModbus::validChecksum(const uint8_t *data, size_t messageLength) const
{
    const uint16_t temp = calculateCRC(data, messageLength - 2); // calculate out crc only from first 6 bytes

    return data[messageLength - 2] == lowByte(temp) &&
           data[messageLength - 1] == highByte(temp);
}


/**
 * @brief Computes the CRC-16 checksum for a given data array.
 *
 * This function implements the CRC-16 (Modbus RTU standard) checksum algorithm
 * to calculate the cyclical redundancy check value for error detection.
 * It processes each byte of the input array, performing bit-wise operations
 * to generate the checksum.
 *
 * @param array Pointer to the byte array for which the CRC is to be calculated.
 * @param len The number of bytes in the array.
 * @return The computed 16-bit CRC value.
 *
 * @note The CRC-16 algorithm uses a polynomial value of 0xA001.
 *
 * @attention It is the caller's responsibility to ensure the validity and size
 * of the input array to avoid buffer overflows or memory issues.
 *
 * @link https://github.com/reaper7/SDM_Energy_Meter/blob/5d1653a0550fab7de13fbe596a1b09d1fae103ad/SDM.cpp#L604
 **/
uint16_t LocalModbus::calculateCRC(const uint8_t *array, uint8_t len)
{
    uint16_t _crc, _flag;
    _crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++)
    {
        _crc ^= (uint16_t)array[i];
        for (uint8_t j = 8; j; j--)
        {
            _flag = _crc & 0x0001;
            _crc >>= 1;
            if (_flag)
                _crc ^= 0xA001;
        }
    }
    return _crc;
}