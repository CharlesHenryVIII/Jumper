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
