#pragma once

#include <vector>
#include <string>

#define CONSOLE_FUNCTION(name) void name ()
typedef CONSOLE_FUNCTION((*CommandFunc));

#define CONSOLE_FUNCTIONA(name) void name (const std::vector<std::string>& args)
typedef CONSOLE_FUNCTIONA((*CommandFuncArgs));

enum LogLevel
{
    LogLevel_Info,
    LogLevel_Warning,
    LogLevel_Error,
    LogLevel_Internal,
    LogLevel_Count,
};

void ConsoleRun();
void ConsoleLog(LogLevel level, const char* fmt, ...);
void ConsoleLog(const char* fmt, ...);
void ConsoleSetLogLevel(LogLevel level);
void ConsoleAddCommand(const char* name, CommandFunc func);
void ConsoleAddCommand(const char* name, CommandFuncArgs func);

bool Console_OnCharacter(int c);
bool Console_OnKeyboard(int c, int mods, bool pressed, bool repeat);
bool Console_OnMouseButton(int button, bool pressed);
bool Console_OnMouseWheel(float scroll);
void Console_OnWindowSize(int width, int height);

