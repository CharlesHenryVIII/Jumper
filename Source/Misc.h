#pragma once
#include <unordered_map>


enum class DebugOptions
{
	none,
	playerCollision,
	blockCollision,
	clickLocation,
	paintMethod,
	editBlocks
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
    mainMenu,
    game,
};
extern GameState gameState;
extern bool running;
extern std::unordered_map<DebugOptions, bool> debugList;

float GetTimer();

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

