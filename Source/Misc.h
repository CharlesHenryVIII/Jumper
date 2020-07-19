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
