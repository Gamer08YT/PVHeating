//
// Created by JanHe on 27.07.2025.
//

#ifndef MODBUS_H
#define MODBUS_H

#include <Arduino.h>

#include "ModbusMessage.h"

/**
 * @class LocalModbus
 *
 * @brief Provides functionality for initializing and managing the Modbus communication.
 *
 * The Modbus class is designed to handle Modbus communication protocols.
 * It contains methods to initialize the communication and execute the
 * main loop for processing messages or tasks related to Modbus.
 */
class LocalModbus
{
public:
    static void begin();
    static void loop();
    static float readRemote(int address);
    static bool readLocal(int address);

private:
    static void handleRequestError(Error error);
    static void handleResponseError(Error error, uint32_t token);
    static void beginRTU();
    static float handleResponse(ModbusMessage& msg, uint32_t token);
    static void handleLocalData(ModbusMessage msg, uint32_t token);
    static void beginTCP();
    static bool validChecksum(const uint8_t* data, size_t messageLength);
    static uint16_t calculateCRC(const uint8_t* array, uint8_t len);
};


#endif //MODBUS_H
