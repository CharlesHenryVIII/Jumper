#include "WinUtilities.h"
#include "Math.h"

#include <Windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <cassert>

void DebugPrint(const char* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), fmt, list);
    OutputDebugStringA(buffer);
    va_end(list);
}

std::vector<std::string> GetFilesInDir(std::string dir, bool appendFilePath)
{
	assert(dir.length());
	std::vector<std::string> result;

	if (dir.back() != '/')
		dir.push_back('/');

	std::string wildDir = dir;
	wildDir.push_back('*');
	WIN32_FIND_DATAA fileData;

	HANDLE handle = {};
	BOOL hasFile = true;
	for (	HANDLE tempHandle = FindFirstFileA(wildDir.c_str(), &fileData);
			tempHandle != INVALID_HANDLE_VALUE && hasFile;
			hasFile = FindNextFileA(tempHandle, &fileData))
	{
		handle = tempHandle;
		if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{

			if (appendFilePath)
				result.push_back(dir + fileData.cFileName);
			else
				result.push_back(fileData.cFileName);
		}
	}

	if (handle != INVALID_HANDLE_VALUE)
	{
		FindClose(handle);
	}
	return result;
}
