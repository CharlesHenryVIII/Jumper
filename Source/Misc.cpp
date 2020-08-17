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

bool running;

std::unordered_map<DebugOptions, bool> debugList;
GameState gameState;

uint64 f = SDL_GetPerformanceFrequency();
uint64 ct = SDL_GetPerformanceCounter();
float GetTimer()
{
	uint64 c = SDL_GetPerformanceCounter();
	return float(((c - ct) * 1000) / (double)f);
}
