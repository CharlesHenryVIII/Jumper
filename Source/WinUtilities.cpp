#include "WinUtilities.h"
#include "Math.h"

#include <Windows.h>
#include <DbgHelp.h>
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

LONG ApplicationCrashCall(_EXCEPTION_POINTERS *ExceptionInfo)
{
	HANDLE handle = GetCurrentProcess();
	HANDLE file = CreateFileA("CrashLog.dmp", GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);
	MINIDUMP_EXCEPTION_INFORMATION mei = {
			.ThreadId = GetCurrentThreadId(),
			.ExceptionPointers = ExceptionInfo,
			.ClientPointers = FALSE,
	};

	MiniDumpWriteDump(handle, GetProcessId(handle), file,
		(_MINIDUMP_TYPE)(MiniDumpNormal | MiniDumpWithDataSegs | MiniDumpWithFullMemory),
		&mei, NULL, NULL);

	CloseHandle(file);
	return EXCEPTION_EXECUTE_HANDLER;
}

struct PreMainInitializer {
	PreMainInitializer()
	{
		SetUnhandledExceptionFilter(ApplicationCrashCall);

	}
}PreMainInitializer;


std::vector<std::string> GetNames(std::string dir, bool appendFilePath, bool wantDir)
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
	for (HANDLE tempHandle = FindFirstFileA(wildDir.c_str(), &fileData);
		tempHandle != INVALID_HANDLE_VALUE && hasFile;
		hasFile = FindNextFileA(tempHandle, &fileData))
	{
		handle = tempHandle;
		bool isdir = (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
		if (((isdir && wantDir) || (!isdir && !wantDir)) && *fileData.cFileName != '.')
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

std::vector<std::string> GetFileNames(std::string dir, bool appendFilePath)
{
	return GetNames(dir, appendFilePath, false);
}

std::vector<std::string> GetDirNames(const std::string& dir, bool appendFilePath)
{
	return GetNames(dir, appendFilePath, true);
}