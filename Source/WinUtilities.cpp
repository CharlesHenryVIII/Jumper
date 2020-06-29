
#include "WinUtilities.h"
#include <Windows.h>
#include <stdarg.h>
#include <stdio.h>

void DebugPrint(const char* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), fmt, list);
    OutputDebugStringA(buffer);
    va_end(list);
}