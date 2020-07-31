#pragma once
#include "Menu.h"
#include "Math.h"
#include "Rendering.h"
#include "GamePlay.h"
#include "Misc.h"

#include <unordered_map>
struct Key;

void QuitApp()
{
	running = false;
}


void SwitchToMenu()
{
	gameState = GameState::mainMenu;
}

void DoPlayMenu(float deltaTime, std::unordered_map<int32, Key>& keyStates, VectorInt mouseLocation)
{
	DrawText(fonts["2"], White, "J U M P E R", 4.0f, { windowInfo.width / 2, windowInfo.height / 4 }, UIX::mid, UIY::mid);

	if (DrawButton(fonts["1"], "Start Game", { windowInfo.width / 2, windowInfo.height / 2 },
					UIX::mid, UIY::mid, Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame) ||
		keyStates[SDLK_RETURN].downThisFrame || keyStates[SDLK_e].downThisFrame)
		SwitchToGame();
	DrawButton(fonts["1"], "Settings", { windowInfo.width / 2, windowInfo.height / 2 + 64 }, 
					UIX::mid, UIY::mid, Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame);

	if (DrawButton(fonts["1"], "Quit", { windowInfo.width / 2, windowInfo.height / 2 + 128 }, 
					UIX::mid, UIY::mid, Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame) ||
		keyStates[SDLK_q].downThisFrame)
		QuitApp();
		//
	AddTextureToRender({}, {}, RenderPrio::Background, sprites["MainMenuBackground"]->texture, {}, 0, 0, SDL_FLIP_NONE);

	/*********************
	*
	* Renderer
	*
	********/

	SDL_SetRenderDrawColor(windowInfo.renderer, 0, 0, 0, 255);
	SDL_RenderClear(windowInfo.renderer);

}


