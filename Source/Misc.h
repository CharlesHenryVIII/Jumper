#pragma once
#include "Math.h"

#include <unordered_map>
#include <chrono>

typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::time_point<Clock, std::chrono::nanoseconds> TimePoint;
//typedef std::chrono::duration<long long, std::nano> duration;
typedef std::chrono::seconds Seconds;
#define FAIL assert(false)
#define ARRAY_COUNT(arr_) (sizeof(arr_) / sizeof(arr_[0]))

enum class DebugOptions
{
	none,
	PlayerCollision,
	BlockCollision,
	ClickLocation,
	PaintMethod,
	EditBlocks,
	ShowAudio,
};

struct Key
{
	bool down;
	bool downPrevFrame;
	bool downThisFrame;
	bool upThisFrame;
};

enum class GameState
{
    none,
    MainMenu,
    Game,
};
extern GameState g_gameState;
extern bool g_running;
extern std::unordered_map<DebugOptions, bool> g_debugList;

float GetTimer();
//String Compare that is not case sensative
bool StringsMatchi(const std::string& a, const std::string& b);
bool StringsMatch(const std::string& a, const std::string& b);
std::string FloatToString(double v, int32 precision = 6);

#define _JMP_CONCAT(a, b) a ## b
#define JMP_CONCAT(a, b) _JMP_CONCAT(a, b)


#define ENABLE_PROFILE 0

#if ENABLE_PROFILE
struct ScopeTimer
{
	const char* name;
	float start;

	ScopeTimer(const char* name_)
		: name(name_)
	{
		start = GetTimer();
	}

	~ScopeTimer();
};

#define PROFILE_FUNCTION() ScopeTimer JMP_CONCAT(__timer_, __COUNTER__)(__FUNCTION__)
#define PROFILE_SCOPE(name) ScopeTimer JMP_CONCAT(__timer_, __COUNTER__)(name)
#else
#define PROFILE_FUNCTION() (void)0
#define PROFILE_SCOPE(...) (void)0
#endif

