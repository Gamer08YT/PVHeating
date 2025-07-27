//
// Created by JanHe on 27.07.2025.
//

#include <Arduino.h>
#include <WiFi.h>
#include <EthernetESP32.h>
#include "Network.h"
#include "PinOut.h"
#include "Guardian.h"

// Store Object of Ethernet Driver.
ENC28J60Driver driver(ETHERNET_CS);

// Definitions for Reconnect Interval.
const unsigned long INITIAL_TIMEOUT = 5000; // 5 Seconds
const unsigned long RECONNECT_INTERVAL = 10000; // 10 Seconds
unsigned long lastReconnectAttempt = 0;

/**
 * @brief Initializes the network connection and attempts to establish a connection with DHCP.
 *
 * This method initializes the Ethernet driver using the defined driver and attempts to
 * establish an initial network connection within the specified timeout period. The connection
 * is established using the default MAC address and DHCP configuration.
 *
 * During the connection attempt, debug messages are printed to the serial monitor to provide
 * feedback on the connection state. If the connection is successfully established, a success
 * message is printed. If the connection attempt fails within the timeout period, an error message
 * is printed, and the variable lastReconnectAttempt is reset to allow immediate reconnection
 * attempts in the future.
 *
 * A small delay is included between reconnection attempts to avoid continuous retries in quick
 * succession during the initial connection phase.
 */
void Network::begin()
{
    // Print Debug Message.
    Guardian::println("Begin Network");

    // Initialize Ethernet Driver.
    Ethernet.init(driver);

    // Try first connection with timeout.
    unsigned long startTime = millis();
    bool connected = false;

    while (!connected && (millis() - startTime < INITIAL_TIMEOUT))
    {
        // Begin with default MAC address and DHCP IP.
        if (Ethernet.begin() == 1)
        {
            connected = true;
            Guardian::println("Network is ready");
        }
        else
        {
            // Add Small Delay.
            delay(100);
        }
    }

    if (!connected)
    {
        Guardian::println("Network failed - reconnecting");

        // Allow direct reconnect try.
        lastReconnectAttempt = 0;
    }
}

/**
 * @brief Monitors the network connection and attempts to reconnect if the connection is lost.
 *
 * This method checks the current connection status using Ethernet.linkStatus().
 * If the connection is lost (status differs from LinkON), it attempts to reconnect.
 * Reconnection attempts are made only after a specified interval to avoid excessive
 * attempts. The interval is defined by the constant RECONNECT_INTERVAL.
 *
 * If a reconnection attempt is successful, a success message is printed to the serial monitor.
 * Otherwise, an error message is printed. The variable lastReconnectAttempt keeps track
 * of the last time a reconnection was attempted.
 */
void Network::update()
{
    // Maintain Ethernet Connection.
    Ethernet.maintain();

    // Überprüfen, ob die Verbindung noch besteht
    if (Ethernet.linkStatus() != LinkON)
    {
        // Nur alle RECONNECT_INTERVAL versuchen neu zu verbinden
        if (millis() - lastReconnectAttempt >= RECONNECT_INTERVAL)
        {
            Guardian::println("Connection lost, reconnecting");

            if (Ethernet.begin() == 1)
            {
                Guardian::println("Reconnect success");
            }
            else
            {
                Guardian::println("Reconnect failed");
            }

            lastReconnectAttempt = millis();
        }
    }
}

/**
 * @brief Retrieves the MAC address of the current network interface.
 *
 * This method uses the WiFi module to obtain the MAC address of the device
 * in use. The MAC address is returned as a string in a standard format
 * (e.g., XX:XX:XX:XX:XX:XX).
 *
 * @return The MAC address of the device as a string.
 */
String Network::getMac()
{
    return WiFi.macAddress();
}
