//
// Created by JanHe on 27.07.2025.
//

/**
 * Usable Pins:
 * 4, 13, 14, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33
 *
 * Free PINS:
 * 21, 25, 26, 27, 33, 32
 *
 * Input Only:
 * 34, 35, 36, 39
 *
 * Free PINS:
 * X
 **/

#ifndef PINOUT_H
#define PINOUT_H

#define SOFTWARE_VERSION "1.0.2"
#define MODBUS_HOUSE "192.168.5.24"
#define MODBUS_TIMEOUT 2000

// Ethernet Stuff.
#define ETHERNET_CS 5
#define ETHERNET_SCK 18
#define ETHERNET_MISO 19
#define ETHERNET_MOSI 23

// Modbus Stuff.
#define MODBUS_TX 17
#define MODBUS_RX 16
#define MODBUS_RE 4

#define ONE_WIRE 22

// Hardware Control IO.
#define BUTTON_FAULT 34
#define LED_FAULT 14
#define BUTTON_MODE 35
#define LED_MODE 13

// Pump and Flow Meter.
#define FLOW_PULSE 36
#define PUMP_ENABLE 10

// SCR Stuff.
#define SCR_FAULT 36
#define SCR_ENABLE 4
#define SCR_PWM 15

// Display and I2C Stuff.
#define DISPLAY_I2C_SDA 32
#define DISPLAY_I2C_SCL 33
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define DISPLAY_ADDRESS 0x3C

#endif //PINOUT_H
