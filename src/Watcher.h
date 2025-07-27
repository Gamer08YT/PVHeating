//
// Created by JanHe on 27.07.2025.
//

#ifndef WATCHER_H
#define WATCHER_H


class Watcher
{
public:
    static void loop();
    static void setup();

private:
    static void readTemperature();
};


#endif //WATCHER_H
