#pragma once
#include <unordered_map>
#include "Math.h"

struct Key;

void QuitApp();
void SwitchToMenu();
void DoPlayMenu(float deltaTime, std::unordered_map<int32, Key> keyStates, VectorInt mouseLocation);
