//
// Created by JanHe on 27.07.2025.
//
#include "LocalModbus.h"
#include <HardwareSerial.h>
#include <WebServer.h>
#include "Guardian.h"
#include "PinOut.h"
#include "MeterRegisters.h"
#include "ModbusClientRTU.h"
#include "WebSerial.h"

// #include "ModbusClientTCP.h"


// Begin HW Serial 2 (TX=17, RX=16).
HardwareSerial serial(2);

// Store Instanceo of TCP Instance.
// ModbusClientTCP modbusTCP(client);

// Store Modbus Instance.
ModbusClientRTU* modbusRTU;

// Store local Queue.
int localQueue = 0;


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

    // Begin Modbus RTU Client.
    beginRTU();

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
 * @brief Reads local input registers using Modbus RTU.
 *
 * This function attempts to read input registers from a specified address using the Modbus RTU protocol.
 * If the Modbus RTU server is inactive, a read request is initiated for the given address and callback.
 * The function then processes the Modbus task queue to execute the request.
 *
 * @param address The starting address of the input registers to read.
 * @param callback A callback of type cbTransaction to handle the result or errors from the read operation.
 *
 * @note Uses a fixed register length defined as REGISTER_LENGTH. Ensure this value matches
 *       the expected number of registers for the given address.
 *
 * @attention If the Modbus RTU server is not active, a warning message is logged via the Guardian system.
 *            Ensure that Modbus communication is properly initialized and active before calling this function.
 *
 * @see ModbusRTU::readIreg
 * @see ModbusRTU::task
 */
bool LocalModbus::readLocal(int address)
{
    String local = "Local: ";
    String addressStr = String(address);

    // Append String.
    local.concat(addressStr);

    // Print to Console.
    Guardian::println(local.c_str());

    // Clear Queue if to full.
    if (localQueue > 10)
    {
        modbusRTU->clearQueue();
        localQueue = 0;
    }

    // Increment Queue.
    localQueue++;

    // https://github.com/eModbus/eModbus/blob/648a14b2f49de0c3ffcd9821e6b7a1180fd3f3f4/examples/RTU16example/main.cpp#L64
    // uint32_t token, uint8_t serverID, uint8_t functionCode, uint16_t p1, uint16_t p2
    Error error = modbusRTU->addRequest(localQueue, MODBUS_CORE, READ_INPUT_REGISTER, address, REGISTER_LENGTH);

    handleRequestError(error);

    return (error == SUCCESS);
}

/**
 * @brief Handles errors that occur during Modbus requests.
 *
 * This method processes a given Modbus error by creating a ModbusError instance
 * and logging the corresponding error message through WebSerial. It provides
 * formatted output for debugging, including the error code and description.
 *
 * @param error The error code returned by a Modbus operation, representing the failure reason.
 */
void LocalModbus::handleRequestError(Error error)
{
    if (error != SUCCESS)
    {
        ModbusError e(error);

        WebSerial.printf("Error creating request: %02X - %s\n", error, (const char*)e);
    }
}

/**
 * @brief Handles response errors encountered during Modbus communication.
 *
 * This method processes errors returned in response to Modbus requests.
 * If the error is not a successful status, it converts the error into a
 * ModbusError object and logs a detailed error message using WebSerial.
 *
 * @param error The error code associated with the Modbus response.
 *              This parameter represents a status code indicating the nature
 *              of the error, with `SUCCESS` indicating no error.
 */
void LocalModbus::handleResponseError(Error error, uint32_t token)
{
    if (error != SUCCESS)
    {
        ModbusError e(error);

        WebSerial.printf("Error response: %02X - %s\n", error, (const char*)e);
    }
}

/**
 * @brief Initializes the Modbus RTU communication system.
 *
 * This function sets up the hardware serial interface for Modbus RTU communication,
 * including specifying baud rate, serial parameters, and the RX/TX pins. It creates
 * an instance of the ModbusClientRTU, assigns an error handler for managing Modbus errors,
 * and configures the timeout for communication. Lastly, it starts the Modbus RTU client
 * on the specified serial interface.
 *
 * @note This function assumes that the RX (MODBUS_RX), TX (MODBUS_TX), and RE (MODBUS_RE) pins,
 * as well as other communication parameters like the baud rate (MODBUS_BAUD) and timeout (MODBUS_TIMEOUT),
 * are predefined.
 *
 * @attention Ensure the hardware is properly configured, and the RS485 driver is correctly
 * connected to the respective pins before invoking this function.
 */
void LocalModbus::beginRTU()
{
    // Prepare Hardware Serial.
    RTUutils::prepareHardwareSerial(serial);

    // Begin Second Serial Channel.
    serial.begin(MODBUS_BAUD, SERIAL_8N1, MODBUS_RX, MODBUS_TX);

    // Initialize RTU Instance.
    modbusRTU = new ModbusClientRTU(MODBUS_RE);

    // Add Error Handler.
    modbusRTU->onErrorHandler(handleResponseError);

    // Add Message Handler.
    modbusRTU->onDataHandler(handleData);

    // Set Timeout.
    modbusRTU->setTimeout(MODBUS_TIMEOUT);

    // Begin Modbus RTU Client.
    modbusRTU->begin(serial, MODBUS_CORE);

    // Print Debug Message.
    Guardian::boot(55, "RTU");
}

/**
 * @brief Handles and processes the received Modbus message data.
 *
 * This method extracts and displays information from the provided ModbusMessage, such as the server ID,
 * function code, data length, and the full message content in hexadecimal format. The information is
 * logged using the WebSerial interface for debugging or monitoring purposes.
 *
 * @param msg The ModbusMessage object containing the message data to be processed.
 * @param token A unique identifier associated with the request or transaction being processed.
 */
void LocalModbus::handleData(ModbusMessage msg, uint32_t token)
{
    WebSerial.printf("Response: serverID=%d, FC=%d, Token=%08X, length=%d:\n", msg.getServerID(), msg.getFunctionCode(),
                     token, msg.size());


    // Create new Onion for Converting.
    union
    {
        uint32_t i;
        float f;
    } converter;

    // Add Register (16bit) 0 and Register (16bit) 1.
    converter.i = ((uint32_t)msg[0] << 16) | msg[1];

    // for (auto& byte : msg)
    // {
    //     WebSerial.printf("%02X ", byte);
    // }


    WebSerial.println(converter.f);
}


void LocalModbus::beginTCP()
{
    // // Set Target of TCP Connection.
    // modbusTCP.setTarget(IPAddress(MODBUS_HOUSE), 502);
    //
    // // Begin Modbus TCP Client.
    // modbusTCP.begin(1);

    // Print Debug Message.
    Guardian::boot(60, "TCP");
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
bool LocalModbus::validChecksum(const uint8_t* data, size_t messageLength)
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
uint16_t LocalModbus::calculateCRC(const uint8_t* array, uint8_t len)
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
