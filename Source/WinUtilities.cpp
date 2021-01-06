#include "WinUtilities.h"
#include "Math.h"

#include <Windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <cassert>

std::string g_workingDir;

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

	//HANDLE handle = FindFirstFileA(searchPath.c_str(), &find_data);
	//BOOL has_file = handle != INVALID_HANDLE_VALUE;
	//while (has_file)
	//{
	//	if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	//	{
	//		std::string filename = find_data.cFileName;
	//		decimals(filename, deletableText);
	//		filename = directory + filename;
	//		string origin = directory + find_data.cFileName;
	//		BOOL success = MoveFile(origin.c_str(), filename.c_str());
	//		if (success)
	//		{
	//			cout << "Successfully renamed " << find_data.cFileName << " to " << filename << endl;
	//		}
	//		if (!success)
	//		{
	//			cout << "Could not rename " << find_data.cFileName << endl;
	//		}
	//	}
	//	has_file = FindNextFileA(handle, &find_data);
	//}

	//if (handle != INVALID_HANDLE_VALUE)
	//{
	//	FindClose(handle);
	//}
