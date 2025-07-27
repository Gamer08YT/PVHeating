//
// Created by JanHe on 27.07.2025.
//

#ifndef LOCALNETWORK_H
#define LOCALNETWORK_H


class LocalNetwork
{
private:
    static void handleOTA();
    static uint8_t mac[6];  // Speicher für die MAC-Adresse
    static char macStr[18]; // Für die String-Repräsentation (XX:XX:XX:XX:XX:XX\0)

public:
    static void begin();
    static void update();
    static uint8_t* getMac();
};


#endif //LOCALNETWORK_H
