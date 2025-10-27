#include "Debug.h"

void Debug::Message(const char* prefix, uint16_t attribute, const char* format, va_list args) {
    // Map Windows colors to ANSI escape codes
    const char* colorCode = "";
    switch (attribute) {
    case 12: colorCode = "\033[31m"; break; // Red
    case 14: colorCode = "\033[33m"; break; // Yellow
    case 7:  colorCode = "\033[37m"; break; // White
    case 10: colorCode = "\033[32m"; break; // Green
    case 11: colorCode = "\033[36m"; break; // Cyan
    case 13: colorCode = "\033[35m"; break; // Magenta
    case 15: colorCode = "\033[1;37m"; break; // Bright White
    default: colorCode = "\033[0m";  break; // Reset
    }

    printf("%s%s", colorCode, prefix);
    vprintf(format, args);
    printf("\033[0m\n"); // Reset color
}

void Debug::Error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    Message("[ERROR] ", 12, format, args); // Red
    va_end(args);
}

void Debug::Warning(const char* format, ...) {
    va_list args;
    va_start(args, format);
    Message("[WARNING] ", 14, format, args); // Yellow
    va_end(args);
}

void Debug::Info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    Message("[INFO] ", 7, format, args); // White
    va_end(args);
}

void Debug::Info(LevelH levelH, const char* format, ...) {
    if (DEBUG_LEVEL < levelH) return;

    va_list args;
    va_start(args, format);
    Message("[INFO] ", 7, format, args);
    va_end(args);
}

void Debug::Info(LevelH levelH, uint16_t color, const char* format, ...)
{
    if (DEBUG_LEVEL < levelH) return;

    va_list args;
    va_start(args, format);
    Message("[INFO] ", color, format, args);
    va_end(args);
}