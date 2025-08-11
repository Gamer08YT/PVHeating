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
#include "ModbusClientTCPasync.h"
#include "Watcher.h"
#include "WebSerial.h"

// Begin HW Serial 2 (TX=17, RX=16).
HardwareSerial serial(2);

// Store Instance of TCP Instance.
ModbusClientTCPasync* modbusTCP;

// Store Modbus Instance.
ModbusClientRTU* modbusRTU;


// Store local Queue.
int localQueue = 0;

// Store remote Queue.
int remoteQueue = 0;


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
    handleReadMessage("Remote", address);

    // Clear Queue if to full.
    if (remoteQueue > 10)
    {
        modbusTCP->clearQueue();
        remoteQueue = 0;
    }

    // Increment Queue.
    remoteQueue++;

    // https://github.com/eModbus/eModbus/blob/648a14b2f49de0c3ffcd9821e6b7a1180fd3f3f4/examples/RTU16example/main.cpp#L64
    // uint32_t token, uint8_t serverID, uint8_t functionCode, uint16_t p1, uint16_t p2
    Error error = modbusTCP->addRequest(address, MODBUS_CORE, READ_INPUT_REGISTER, address, REGISTER_LENGTH);

    handleRequestError(error);

    return (error == SUCCESS);
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
    handleReadMessage("Local", address);

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
    Error error = modbusRTU->addRequest(address, MODBUS_CORE, READ_INPUT_REGISTER, address, REGISTER_LENGTH);

    handleRequestError(error);

    return (error == SUCCESS);
}

/**
 * @brief Handles the creation and logging of a Modbus read message.
 *
 * Combines a string prefix with the provided Modbus address to create a detailed log message
 * and outputs the resulting string using the Guardian logging system.
 *
 * @param str The string prefix to be appended to the address.
 * @param address The Modbus address to be included in the message.
 */
void LocalModbus::handleReadMessage(String str, int address)
{
#ifdef DEBUG
    String addressStr = String(address);

    // Append String.
    str.concat(addressStr);

    // Print to Console.
    Guardian::println(str.c_str());
#endif
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
    modbusRTU->onDataHandler(handleLocalData);

    // Set Timeout.
    modbusRTU->setTimeout(MODBUS_TIMEOUT);

    // Begin Modbus RTU Client.
    modbusRTU->begin(serial, MODBUS_CORE);

    // Print Debug Message.
    Guardian::boot(55, "RTU");
}

/**
 * @brief Handles the response for a Modbus query and extracts float values from it.
 *
 * This function processes the incoming Modbus message, extracting data in the form of IEEE754
 * float32 values from the message payload. It logs the server ID, function code, token,
 * message length, extracted float values, and the raw byte data for debugging purposes.
 *
 * The function assumes the response data is formatted in consecutive registers as 32-bit
 * floating-point values (two registers per value). Extracted values are stored in a local buffer.
 *
 * @param msg The Modbus message containing the response data from the queried device.
 *            It includes the server ID, function code, payload, etc.
 * @param token A unique identifier that corresponds to the triggered Modbus operation.
 *
 * @return The first extracted float value from the response message payload.
 *
 * @note The data in the message is accessed starting from a predefined offset. This assumes
 *       the message structure as per the device's implementation and protocol specifics.
 *
 * @warning The function makes use of fixed buffer sizes and assumes all required data is
 *          present in the Modbus response. Ensure the response matches the expected
 *          length and format to avoid memory access issues.
 */
float LocalModbus::handleResponse(ModbusMessage& msg, uint32_t token)
{
#ifdef DEBUG
            WebSerial.printf("Response: serverID=%d, FC=%d, Token=%08X, length=%d:\n", msg.getServerID(), msg.getFunctionCode(),
                             token, msg.size());
#endif

    // Add Float Buffer.
    float values[REGISTER_LENGTH];

    // First value is on pos 3, after server ID, function code and length byte
    uint16_t offset = MODBUS_OFFSET;

    // The device has values all as IEEE754 float32 in two consecutive registers
    // Read the requested in a loop
    for (uint8_t i = 0; i < REGISTER_LENGTH; ++i)
    {
        offset = msg.get(offset, values[i]);
    }

#ifdef DEBUG
        WebSerial.printf("Values: %f, %f\n", values[0], values[1]);

        for (auto& byte : msg)
        {
            WebSerial.printf("%02X ", byte);
        }
#endif


    return values[0];
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
void LocalModbus::handleLocalData(ModbusMessage msg, uint32_t token)
{
    float response = handleResponse(msg, token);

    switch (token)
    {
    case POWER_USAGE:
        Watcher::setPower(response);
        break;
    case POWER_IMPORT:
        Watcher::setConsumption(response);
        break;
    default:
        unknownToken(token);

        break;
    }
}

/**
 * @brief Logs an error message for an unrecognized token received in Modbus communication.
 *
 * This method generates a formatted string indicating an unknown token and logs it
 * using the Guardian logging system for debugging purposes.
 *
 * @param token The unrecognized token value that triggered the error.
 */
void LocalModbus::unknownToken(uint32_t token)
{
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "Unknown Token: %u", token);

    Guardian::println(buffer);
}

/**
 * @brief Handles incoming Modbus messages and processes data based on a token identifier.
 *
 * This method processes a Modbus message received from a remote source by invoking
 * the `handleResponse` method to retrieve a floating-point response value. Based on
 * the token parameter, it performs specific actions, such as setting house power readings.
 * If the token is unrecognized, a diagnostic message is logged via the Guardian system.
 *
 * @param msg The ModbusMessage object representing the data received.
 * @param token A unique identifier specifying the type of data or action to be performed.
 */
void LocalModbus::handleRemoteData(ModbusMessage msg, uint32_t token)
{
    float response = handleResponse(msg, token);

    switch (token)
    {
    case POWER_USAGE:
        Watcher::setHousePower(response);
        break;
    default:
        unknownToken(token);

        break;
    }
}


/**
 * @brief Initializes the Modbus TCP communication system.
 *
 * This method sets up and starts the Modbus TCP client for communication. It configures
 * the target IP address, port, and other parameters required for Modbus TCP operation.
 * Additionally, it initializes the timeout setting for the associated ModbusRTU client.
 * A debug message is logged to indicate the status of the initialization process.
 *
 * @note This function assumes predefined constants for IP address (MODBUS_TCP),
 * port (MODBUS_TCP_PORT), timeout (MODBUS_TIMEOUT), and core assignment (MODBUS_CORE).
 *
 * @attention Ensure network connectivity to the target Modbus TCP device before calling this method.
 */
void LocalModbus::beginTCP()
{
    // Initialize TCP Instance.
    modbusTCP = new ModbusClientTCPasync(MODBUS_TCP, MODBUS_TCP_PORT);

    // Add Error Handler.
    modbusTCP->onErrorHandler(handleResponseError);

    // Add Message Handler.
    modbusTCP->onDataHandler(handleRemoteData);

    // Set Timeout.
    modbusTCP->setTimeout(MODBUS_TIMEOUT);

    // Begin Modbus TCP Client.
    modbusTCP->connect();

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
