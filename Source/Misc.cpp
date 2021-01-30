#pragma once
#include <unordered_map>
#include "Misc.h"
#include "SDL.h"
#include "Math.h"
#include "WinUtilities.h"

#if ENABLE_PROFILE
ScopeTimer::~ScopeTimer()
{
    float duration = GetTimer() - start;
    DebugPrint("%s took %f ms\n", name, duration);
}
#endif

bool g_running;

std::unordered_map<DebugOptions, bool> g_debugList;
GameState g_gameState;

uint64 f = SDL_GetPerformanceFrequency();
uint64 ct = SDL_GetPerformanceCounter();
float GetTimer()
{
	uint64 c = SDL_GetPerformanceCounter();
	return float(((c - ct) * 1000) / (double)f);
}

//String Compare that is not case sensative
bool StringCompare(const std::string& a, const std::string& b)
{
	if (a.size() != b.size())
		return false;

	for (int32 i = 0; i < a.size(); i++)
	{
		if (a[i] != b[i])
		{
			int32 aVal = a[i];
			int32 bVal = b[i];

			if (a[i] >= 'a' && a[i] <= 'z')
				aVal = a[i] - ('a' - 'A');
			else if (b[i] >= 'a' && b[i] <= 'z')
				bVal = b[i] - ('a' - 'A');

			if (aVal != bVal)
				return false;
		}
	}

	return true;
}