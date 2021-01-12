#pragma once
#include <string>
#include <vector>

//const char* g_workingDirectory = "C:\\Projects\\Jumper";
//extern std::string g_workingDir;

void DebugPrint(const char* fmt, ...);
std::vector<std::string> GetFilesInDir(std::string dir, bool appendFilePath = false);
