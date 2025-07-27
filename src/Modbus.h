//
// Created by JanHe on 27.07.2025.
//

#ifndef MODBUS_H
#define MODBUS_H


/**
 * @class Modbus
 *
 * @brief Provides functionality for initializing and managing the Modbus communication.
 *
 * The Modbus class is designed to handle Modbus communication protocols.
 * It contains methods to initialize the communication and execute the
 * main loop for processing messages or tasks related to Modbus.
 */
class Modbus {
public:
    static void begin();
    static void loop();
};



#endif //MODBUS_H
