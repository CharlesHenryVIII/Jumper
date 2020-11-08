#pragma once
#include "Menu.h"
#include "Math.h"
#include "Rendering.h"
#include "GamePlay.h"
#include "Misc.h"
#include "Console.h"

#include <unordered_map>
struct Key;

void QuitApp()
{
	g_running = false;
}


void SwitchToMenu()
{
	g_gameState = GameState::mainMenu;
    ConsoleLog("Switched To Menu");
}

void DoPlayMenu(float deltaTime, std::unordered_map<int32, Key>& keyStates, VectorInt mouseLocation)
{
	DrawText(g_fonts["2"], White, "J U M P E R", 4.0f, { g_windowInfo.width / 2, g_windowInfo.height / 4 }, UIX::mid, UIY::mid);

	if (DrawButton(g_fonts["1"], "Start Game", { g_windowInfo.width / 2, g_windowInfo.height / 2 },
					UIX::mid, UIY::mid, Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame) ||
		keyStates[SDLK_RETURN].downThisFrame || keyStates[SDLK_e].downThisFrame)
		SwitchToGame();
	DrawButton(g_fonts["1"], "Settings", { g_windowInfo.width / 2, g_windowInfo.height / 2 + 64 }, 
					UIX::mid, UIY::mid, Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame);

	if (DrawButton(g_fonts["1"], "Quit", { g_windowInfo.width / 2, g_windowInfo.height / 2 + 128 }, 
					UIX::mid, UIY::mid, Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame) ||
		keyStates[SDLK_q].downThisFrame)
		QuitApp();

	AddTextureToRender({}, {}, RenderPrio::Background, g_sprites["MainMenuBackground"], {}, 0, {}, false, CoordinateSpace::UI);
}


