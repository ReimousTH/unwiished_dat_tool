#pragma once
#include <Windows.h>
#include <cstdarg>
#include <cstdio>
#include <cstdint>

#define DEBUG_LEVEL LevelH::LevelH_Low

class Debug {
public:
    enum LevelH { LevelH_Low = 1, LevelH_Medium, LevelH_High };

    static void Error(const char* format, ...);
    static void Warning(const char* format, ...);
    static void Info(const char* format, ...);
    static void Info(LevelH levelH, const char* format, ...);
    static void Info(LevelH levelH, uint16_t color, const char* format, ...);

private:
    static void Message(const char* prefix, UINT16 attribute, const char* format, va_list args);
    static UINT16 GetCurrentAttribute();
};