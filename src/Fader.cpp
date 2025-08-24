//
// Created by JanHe on 18.08.2025.
//

#include <Arduino.h>
#include "Fader.h"

bool allowFade = false;
uint64_t fadeSpeed = 500;
uint8_t fadeDuty = 255;
uint8_t currentDuty = 0;
unsigned long lastMs = 0;
bool dutyIncrement = true;
int dutyPin;

/**
 * @brief Constructs a Fader object with the specified pin and initializes the PWM functionality.
 *
 * This constructor assigns the provided pin to the internal duty pin and sets up the
 * necessary configuration for PWM signaling on that pin.
 *
 * @param pin The pin number to which the fading functionality is attached.
 */
Fader::Fader(int pin)
{
    dutyPin = pin;

    ledcAttach(pin, 950, 8);
}

/**
 * @brief Enables or disables the fading functionality.
 *
 * This method sets the internal flag to allow or prevent fading based on the
 * provided condition.
 *
 * @param cond A boolean value where `true` enables fading and `false` disables it.
 */
void Fader::setFade(bool cond)
{
    allowFade = cond;
}

void Fader::update()
{
    if (allowFade)
    {
        unsigned long now = millis();

        if (now - lastMs >= fadeSpeed)
        {
            lastMs += fadeSpeed;

            handleFade();
        }
    }
}

/**
 * @brief Configures the fade behavior by setting the maximum duty cycle and speed.
 *
 * This method adjusts the fade behavior by assigning the provided duty cycle
 * and speed values to control the fading effect.
 *
 * @param maxDuty The maximum duty cycle for fading, specified as an integer.
 * @param speed   The speed of fading, specified as an integer in milliseconds.
 */
void Fader::fade(int maxDuty, int speed)
{
    fadeDuty = maxDuty;
    fadeSpeed = speed;
}

/**
 * @brief Sets the PWM duty cycle value for the pin associated with the Fader object.
 *
 * This method updates the brightness level or intensity of the associated pin by
 * writing a new duty cycle value using PWM functionality.
 *
 * @param i The duty cycle value to be set, typically ranging between 0 and 255,
 * where 0 represents off and 255 represents maximum brightness.
 */
void Fader::setValue(int i)
{
    ledcWrite(dutyPin, i);
}

/**
 * @brief Handles the fading operation for LED brightness.
 *
 * This method adjusts the duty cycle incrementally to achieve a fade effect.
 * Depending on the current state, it either increases or decreases the brightness.
 * When the maximum duty cycle (fadeDuty) is reached, the brightness starts
 * decreasing. Similarly, when the brightness reaches zero, it starts increasing
 * again. The updated duty cycle is written to the specified pin using `ledcWrite`.
 */
void Fader::handleFade()
{
    if (dutyIncrement)
    {
        if (currentDuty < fadeDuty)
        {
            currentDuty++;
        }
        else
        {
            dutyIncrement = false;
        }
    }
    if (currentDuty > 0)
    {
        currentDuty--;
    }
    else
    {
        dutyIncrement = true;
    }

    // Write to LEDC.
    ledcWrite(dutyPin, currentDuty);
}


bool everyMs(unsigned long intervalMs, unsigned long& lastTick)
{
    unsigned long now = millis();
    if (now - lastTick >= intervalMs)
    {
        lastTick += intervalMs;
        return true;
    }
    return false;
}
