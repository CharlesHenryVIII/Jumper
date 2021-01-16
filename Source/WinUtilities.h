#pragma once
#include <string>
#include <vector>

//const char* g_workingDirectory = "C:\\Projects\\Jumper";
//extern std::string g_workingDir;

void DebugPrint(const char* fmt, ...);
std::vector<std::string> GetFileNames(std::string dir, bool appendFilePath);
std::vector<std::string> GetDirNames(const std::string& dir, bool appendFilePath);
