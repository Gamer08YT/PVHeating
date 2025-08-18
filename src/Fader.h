//
// Created by JanHe on 18.08.2025.
//

#ifndef FADER_H
#define FADER_H


class Fader
{
public:
    Fader() = default;
    explicit Fader(int pin);
    void setFade(bool cond);
    void update();
    void fade(int maxDuty, int speed);
    void setValue(int i);

private:
    void handleFade();
    bool dutyIncrement;
};


#endif //FADER_H
