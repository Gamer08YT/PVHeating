//
// Created by JanHe on 27.07.2025.
//

#ifndef NETWORK_H
#define NETWORK_H

#include "HADevice.h"


class Network
{
public:
    static void begin();
    static void update();
    static String getMac();
};


#endif //NETWORK_H
