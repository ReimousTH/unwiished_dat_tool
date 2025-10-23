#include "Debug.h"


UINT16 Debug::GetCurrentAttribute() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(hConsole, &info);
    return info.wAttributes;
}

void Debug::Message(const char* prefix, UINT16 attribute, const char* format, va_list args) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    UINT16 originalAttr = GetCurrentAttribute();

    SetConsoleTextAttribute(hConsole, attribute);
    printf("%s", prefix);
    vprintf(format, args);

    SetConsoleTextAttribute(hConsole, originalAttr);
    printf("\n");
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