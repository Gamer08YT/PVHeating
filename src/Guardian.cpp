//
// Created by JanHe on 27.07.2025.
//

#include <Arduino.h>
#include "Guardian.h"
#include <Adafruit_SSD1306.h>

#include "PinOut.h"

// Store OLED Instance.
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/**
 * @brief Outputs a provided message to Serial and an OLED display.
 *
 * This function performs the following tasks:
 * - Prints the provided string to the Serial monitor for debugging.
 * - Clears the OLED display to remove any previous message.
 * - Displays the provided string on a new line in the OLED display.
 *
 * @param str The string message to be displayed and logged via Serial.
 */
void Guardian::println(const char* str)
{
    Serial.println(str);

    // Clear last Message.
    display.clearDisplay();

    // Start on 0/0.
    display.setCursor(0, 0);

    // Write a new Line.
    display.println(str);

    // Update Display Buffer.
    display.display();
}

/**
 * @brief Initializes the Guardian system by setting up the I2C bus and configuring the display.
 *
 * This function performs the following tasks:
 * - Initializes the I2C communication using defined pins for SDA and SCL.
 * - Configures the OLED display. If the display setup fails, an error message is output via Serial.
 * - Sets the default text size and color for the display.
 * - Sends specific precharge commands to the display for configuration.
 *
 * @details The function utilizes Adafruit_SSD1306 library commands for display configuration
 * and assumes the hardware is connected with proper wiring configurations. It performs minimal error
 * checking for display initialization.
 */
void Guardian::setup()
{
    // Set up IC2 Bus.
    Wire.begin(DISPLAY_I2C_SDA, DISPLAY_I2C_SCL);

    // Display Setup.
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println(F("SSD1306 allocation failed"));
    }
    else
    {
        // Set Default Size and disable Wrap.
        display.setTextSize(1.5);
        display.setTextColor(WHITE);
        display.ssd1306_command(SSD1306_SETPRECHARGE);
        display.ssd1306_command(17);
    }
}
