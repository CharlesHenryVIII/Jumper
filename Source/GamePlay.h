#pragma once
#include <unordered_map>
#include "Math.h"
struct Key;
struct Portal;
struct Level;

void DoPlayGame(float deltaTime, std::unordered_map<int32, Key>& keyStates,
	VectorInt mouseLocation);

void SwitchToGame();
