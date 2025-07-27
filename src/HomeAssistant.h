//
// Created by JanHe on 27.07.2025.
//

#ifndef HOMEASSISTANT_H
#define HOMEASSISTANT_H


class HomeAssistant
{
private:
    static void configureHeatingInstance();
    static void configurePowerInstance();
    static void configureConsumptionInstance();
    static void publishChanges();

public:
    static void begin();
    static void loop();
};


#endif //HOMEASSISTANT_H
