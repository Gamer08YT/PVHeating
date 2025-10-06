//
// Created by JanHe on 27.07.2025.
//

#include <Arduino.h>
#include <WiFi.h>
#include <EthernetESP32.h>
#include "LocalNetwork.h"

#include <WebServer.h>

#include "ElegantOTA.h"
#include "PinOut.h"
#include "Guardian.h"
#include "Watcher.h"

#ifdef DEBUG
#include "WebSerial.h"
#endif

// Store Object of Ethernet Driver.
ENC28J60Driver driver(ETHERNET_CS);

// Store Instance of Webserver.
AsyncWebServer server(80);

// Definitions for Reconnect Interval.
const unsigned long INITIAL_TIMEOUT = 5000; // 5 Seconds
const unsigned long RECONNECT_INTERVAL = 10000; // 10 Seconds
// unsigned long lastReconnectAttempt = 0;

bool isOTAUploading = false;

// Store MAC Address.
uint8_t LocalNetwork::mac[6];
char LocalNetwork::macStr[18];


/**
 * @brief Configures and initializes the OTA update service.
 *
 * This method sets up the OTA (Over-The-Air) update mechanism, enabling the device to receive
 * firmware updates remotely. It integrates with the provided asynchronous web server to handle*/
void LocalNetwork::handleOTA()
{
    // Begin Webserver.
    server.begin();

    // Add OTA Start Listener.
    ElegantOTA.onStart([]()
    {
        // Disable Heater.
        Watcher::setStandby(true);

        isOTAUploading = true;

        // Clear and Update Display.
        Guardian::clear();
        Guardian::update();
    });

    // Add OTA End Listener.
    ElegantOTA.onProgress([](unsigned int progress, unsigned int total)
    {
        // Display Progress on Display.
        Guardian::setProgress(30, (progress * 100) / total);
    });

    // Begin OTA Server.
    ElegantOTA.begin(&server);
}

/**
 * @brief Configures and starts the WebSerial service.
 *
 * This method initializes the WebSerial service with a specified initial buffer size
 * to optimize performance and configures it to use the provided asynchronous web server instance.
 * By invoking this function, WebSerial is enabled and ready to handle incoming serial messages
 * via a web interface.
 *
 * The initial buffer size is set to ensure efficient memory usage when handling log messages
 * or serial communication.
 */
void LocalNetwork::handleSerial()
{
#ifdef DEBUG
    // Allow initial Buffer Size of 40.
    WebSerial.setBuffer(40);

    // Begin WebSerial.
    WebSerial.begin(&server);
#endif
}

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
void LocalNetwork::begin()
{
    // MAC direkt vom WiFi-Modul holen
    Network.macAddress(mac);

    // Print Debug Message.
    Guardian::boot(10, "Network");

    // Initialize Ethernet Driver.
    Ethernet.init(driver);

    // Try the first connection with timeout.
    unsigned long startTime = millis();
    bool connected = false;

    while (!connected && (millis() - startTime < INITIAL_TIMEOUT))
    {
        // Begin with the default MAC address and DHCP IP.
        if (reconnect() == 1)
        {
            connected = true;
            Guardian::println("Network is ready");
            Serial.println(Ethernet.localIP());
        }
        else
        {
            // Add Small Delay.
            delay(100);
        }
    }

    if (!connected)
    {
        Guardian::println("Network failed"); // - reconnecting");

        // Allow direct reconnect try.
        //lastReconnectAttempt = 0;
    }

    // Setup OTA.
    handleOTA();

    // Setup WebSerial.
    handleSerial();

    // Print Debug Message.
    Guardian::println("OTA is ready");
}

/**
 * @brief Checks if an OTA upload process is currently in progress.
 *
 * This method determines whether the device is actively engaged
 * in receiving a firmware update via OTA mechanisms.
 *
 * @return true if an OTA upload is ongoing, otherwise false.
 */
bool LocalNetwork::isUploading()
{
    return isOTAUploading;
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
void LocalNetwork::update()
{
    // Maintain Ethernet Connection.
    Ethernet.maintain();

    // Überprüfen, ob die Verbindung noch besteht
    // if (Ethernet.linkStatus() != LinkON)
    // {
    //     // Nur alle RECONNECT_INTERVAL versuchen neu zu verbinden
    //     if (millis() - lastReconnectAttempt >= RECONNECT_INTERVAL)
    //     {
    //         Guardian::println("Connection lost, reconnecting");
    //         Guardian::setError(101, "Network", Guardian::WARNING);
    //
    //         if (Ethernet.begin() == 1)
    //         {
    //             Guardian::println("Reconnect success");
    //
    //             if (Guardian::getErrorCode() == 101)
    //             {
    //                 Guardian::clearError();
    //             }
    //         }
    //         else
    //         {
    //             Guardian::println("Reconnect failed");
    //         }
    //
    //         lastReconnectAttempt = millis();
    //     }
    // }

    // Handle OTA Loop.
    ElegantOTA.loop();

#ifdef DEBUG
    // Handle WebSerial Loop.
    WebSerial.loop();
#endif
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
uint8_t* LocalNetwork::getMac()
{
    return mac;
}

/**
 * @brief Attempts to reconnect the Ethernet connection using the default MAC address and DHCP.
 *
 * This method initializes the Ethernet connection by calling the internal Ethernet driver. It
 * returns a status indicating whether the Ethernet begin operation was successful or not.
 *
 * @return int 1 if the Ethernet connection was successfully established, 0 otherwise.
 */
int LocalNetwork::reconnect()
{
    // Print Debug Message.
    Serial.println("Eth begin");

    return Ethernet.begin();
}

/**
 * @brief Reconfigures the Ethernet connection.
 *
 * This method terminates the current Ethernet connection and performs a reinitialization
 * to restore connectivity. It ensures the Ethernet driver is properly reset before attempting
 * a reconnection. A short delay is introduced to allow the hardware components to stabilize.
 */
void LocalNetwork::reconf()
{
    // Print Debug Message.
    Serial.println("Eth reconf");

    // End Ethernet.
    Ethernet.end();

    // Short delay.
    delay(500);

    // Recreate Connection.
    reconnect();
}
