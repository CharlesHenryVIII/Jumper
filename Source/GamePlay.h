#pragma once
#include <unordered_map>
#include <chrono>
#include "Math.h"
struct Key;
struct Portal;
struct Level;

void DoPlayGame(float deltaTime, std::unordered_map<int32, Key>& keyStates,
	VectorInt mouseLocation);

void SwitchToGame();

struct Gamestate {
    
	std::unordered_map<std::string, Level> levels;
    Level* GetLevel(const std::string& name);
};

Gamestate* GetGamestate();

