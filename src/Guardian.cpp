//
// Created by JanHe on 27.07.2025.
//

#include <Arduino.h>
#include "Guardian.h"
#include <Adafruit_SSD1306.h>

#include "PinOut.h"
#include "Watcher.h"
#include "WebSerial.h"

// Store OLED Instance.
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Override Header Vars.
const char* Guardian::errorTitle = "";
int Guardian::errorCode = -1;
Guardian::ErrorType Guardian::errorLevel = WARNING;


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
    // First Print to Serial.
    Serial.println(str);

    // Print to WebSerial.
    WebSerial.println(str);

    // Clear last Message.
    display.clearDisplay();

    // Start on 0/0.
    display.setCursor(0, 0);

    // Write a new Line.
    display.println(F(str));

    // Update Display Buffer.
    display.display();
}

/**
 * @brief Sets the title of the error for display or logging purposes.
 *
 * This function assigns the provided string to the internal error title property.
 * It can be used to define a human-readable message describing the nature of an error.
 *
 * @param str The string representing the error title or message.
 */
void Guardian::error_title(const char* str)
{
    errorTitle = str;

    // Update Display and HA States.
    updateFault();
}

/**
 * @brief Sets the error level for the guardian system.
 *
 * This function updates the error severity level, which is used to indicate the type
 * of fault or issue occurring in the system. The error level can typically represent
 * different severity levels such as WARNING or CRITICAL.
 *
 * @param mode The error severity level, represented by the ErrorType enumeration.
 */
void Guardian::error_level(ErrorType mode)
{
    errorLevel = mode;
}

void Guardian::updateFault()
{
}

/**
 * @brief Sets an error state with a specific code, message, and severity level.
 *
 * This method updates the error code, title, and error level to represent the current
 * fault condition. It ensures that the provided information is logged and displayed
 * as necessary.
 *
 * @param i The error code indicating the specific fault.
 * @param str A descriptive string providing information about the error.
 * @param level The severity level of the error, defined by the ErrorType enum.
 */
void Guardian::setError(int i, const char* str, ErrorType level)
{
    Serial.print("Error: ");
    Serial.print(i);
    Serial.print(" - ");
    Serial.println(str);

    WebSerial.print("Error: ");
    WebSerial.print(i);
    WebSerial.print(" - ");
    WebSerial.println(str);

    error_code(i);
    error_title(str);
    error_level(level);
}

/**
 * @brief Sets an error with a specified code and message as a warning.
 *
 * This function invokes the overloaded version of setError to set an error
 * with a specific code and message, defaulting the error type to WARNING.
 *
 * @param i The error code to be set.
 * @param str The error message to be displayed or logged.
 */
void Guardian::setError(int i, const char* str)
{
    setError(i, str, WARNING);
}

/**
 * @brief Scans for I2C devices on the bus and logs their addresses.
 *
 * This function iterates over all possible I2C addresses (1 to 126) and checks
 * for connected devices. For each detected device, it prints the address
 * in hexadecimal format to the Serial monitor. If an address reports an
 * unknown transmission error, it logs the address with an error message.
 *
 * This function uses the Wire library to communicate on the I2C bus and
 * provides useful feedback for locating and debugging connected devices.
 */
void Guardian::testScan()
{
    byte error, address;
    int nDevices = 0;

    for (address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0)
        {
            Serial.print("I2C device found at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.println(address, HEX);
            nDevices++;
        }
        else if (error == 4)
        {
            Serial.print("Unknown error at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.println(address, HEX);
        }
    }


    if (nDevices == 0)
        Serial.println("No I2C devices found\n");
    else
        Serial.println("Scan complete\n");
}


/**
 * @brief Sets the error code for the Guardian system.
 *
 * This method assigns the specified integer value to the `errorCode` property,
 * which indicates a specific error state or code in the Guardian system.
 *
 * @param i The integer code representing an error condition.
 */
void Guardian::error_code(int i)
{
    errorCode = i;

    // Update Display and HA States.
    updateFault();
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
    // Print Debug Message.
    Serial.println("Begin I2C");

    // Set up IC2 Bus.
    Wire.begin(DISPLAY_I2C_SDA, DISPLAY_I2C_SCL);


    // Test Scan for Devices.
    testScan();

    // Print Debug Message.
    Serial.println("I2C ready");

    // Display Setup.
    if (!display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_ADDRESS))
    {
        // Set Warning.
        setError(10, "Display Initialization Failed.");
    }
    else
    {
        Serial.println("Display ready.");

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.ssd1306_command(SSD1306_SETCONTRAST);
        display.ssd1306_command(0xFF); // Max. Kontrast
        display.setCursor(0, 0);
        display.println("Hallo vom ESP32!");
        display.display();
    }
}

/**
 * @brief Checks if an error condition is present.
 *
 * This function determines whether the system is currently in an error state
 * by evaluating if the error code has been set to a value other than -1.
 *
 * @return true if an error exists (errorCode is not -1); false otherwise.
 */
bool Guardian::hasError()
{
    return errorCode != -1;
}

/**
 * @brief Retrieves the current error title of the Guardian class.
 *
 * This method returns the value of the static errorTitle member,
 * which represents a descriptive title of the current error, if any.
 *
 * @return The error title as a constant character pointer.
 */
const char* Guardian::getErrorTitle()
{
    return errorTitle;
}
