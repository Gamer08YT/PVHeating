//
// Created by JanHe on 22.08.2025.
//

#include <Arduino.h>
#include "Backtrace.h"

#include "HomeAssistant.h"

extern "C" {
#include "esp_debug_helpers.h"
}

/**
 * Formats an array of addresses into a single space-separated string.
 *
 * This method takes an array of addresses and converts each to a hexadecimal string
 * representation. The formatted addresses are concatenated into a single string,
 * separated by spaces. It is used for debugging or logging purposes, where a readable
 * backtrace or memory address list is needed.
 *
 * @param addrs A pointer to the array of addresses to be formatted.
 * @param count The number of addresses in the array.
 * @return A string containing the space-separated hexadecimal representations of the addresses.
 */
String Backtrace::format_addrs_to_string(const void* const* addrs, size_t count)
{
    String s;
    s.reserve(count * 12);
    for (size_t i = 0; i < count; ++i)
    {
        char tmp[14];
        snprintf(tmp, sizeof(tmp), "0x%08" PRIX32, (uint32_t)reinterpret_cast<uintptr_t>(addrs[i]));
        s += tmp;
        if (i + 1 < count) s += ' ';
    }
    return s;
}

/**
 * Captures and formats the backtrace of the current task up to a specified depth.
 *
 * This method uses the ESP-IDF backtrace API to retrieve the addresses of function calls
 * in the current task's backtrace, up to the given maximum depth. The addresses are then
 * formatted into a string representation suitable for debugging or error reporting.
 *
 * The method ensures compatibility with different versions of the ESP-IDF backtrace API.
 * If obtaining the backtrace fails, a placeholder string indicating unavailability is returned.
 *
 * @param max_depth The maximum number of backtrace frames to capture. Default is 16.
 *                  Values higher than 32 are not supported.
 * @return A string containing the formatted backtrace addresses, or
 *         "backtrace: <unavailable>" if the backtrace could not be obtained.
 */
// Variante B: für IDF mit bool esp_backtrace_get(depth, addrs, &out_count)
String Backtrace::get_current_task_backtrace(size_t max_depth = 16)
{
    void* addrs[32] = {0}; // max_depth <= 32
    size_t captured = 0;

    // Prüfe in deiner esp_debug_helpers.h, welche Signatur vorhanden ist,
    // und kommentiere die jeweils passende Variante ein:

    // --- Variante A (ältere IDF) ---
    // int n = esp_backtrace_get((int)max_depth, addrs);
    // if (n <= 0) return String(F("backtrace: <unavailable>"));
    // captured = (size_t)n;

    // --- Variante B (neuere IDF) ---
    size_t out_count = 0;
    if (!esp_backtrace_get((int)max_depth, addrs, &out_count))
      return String(F("backtrace: <unavailable>"));
    captured = out_count;

    return format_addrs_to_string(addrs, captured);
}

/**
 * Reports the current backtrace of the executing task to Home Assistant.
 *
 * This method captures the backtrace of the current task with a maximum
 * depth of 16 frames. It then converts the backtrace into a string format
 * and passes it to Home Assistant by setting it as the error title.
 *
 * The backtrace information helps in identifying the sequence of function
 * calls leading up to the current point, typically used for debugging or
 * error reporting purposes.
 */
void Backtrace::report_backtrace_to_ha()
{
    const String bt = get_current_task_backtrace(16);

    HomeAssistant::setErrorTitle(bt.c_str());
}
