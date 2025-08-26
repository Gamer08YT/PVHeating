//
// Created by JanHe on 27.07.2025.
//

#ifndef LOCALNETWORK_H
#define LOCALNETWORK_H


/**
 * @class LocalNetwork
 * @brief A class that handles initialization and management of the local network, including
 * retrieving MAC addresses and enabling OTA (Over-the-Air) updates.
 *
 * This class provides functionality to initialize and update the local network using an
 * Ethernet driver. It also supports managing and obtaining the MAC address for the device.
 */
class LocalNetwork
{
private:
    static void handleOTA();
    static void handleSerial();
    static uint8_t mac[6];  // Speicher für die MAC-Adresse
    static char macStr[18]; // Für die String-Repräsentation (XX:XX:XX:XX:XX:XX\0)

public:
    static void begin();
    static bool isUploading();
    static void update();
    static uint8_t* getMac();
    static int reconnect();
};


#endif //LOCALNETWORK_H
