#pragma once
#include "Menu.h"
#include "Math.h"
#include "Rendering.h"
#include "GamePlay.h"
#include "Misc.h"
#include "Console.h"
#include "Audio.h"

#include <unordered_map>
struct Key;
AudioID s_menuMusic = 0;

void QuitApp()
{
	g_running = false;
}

void SwitchToMenu()
{
	g_applicationState = ApplicationState::MainMenu;

	AudioParams audio = {
	.nameOfSound = "Menu_Synthwave", //Halo
	.fadeInDuration = 10.0f,
	.fadeOutDuration = 3.0f,
	.loopCount = AUDIO_MAXLOOPS,
	};
	s_menuMusic = PlayAudio(audio);

    ConsoleLog("Switched To Menu");
}


void DoPlayMenu(float deltaTime, std::unordered_map<int32, Key>& keyStates, VectorInt mouseLocation)
{
	DrawText(g_fonts["Title"], White, 4.0f, { g_windowInfo.width / 2, g_windowInfo.height / 4 }, UIX::mid, UIY::mid, RenderPrio::UI, "J U M P E R");

	if (DrawButton(g_fonts["Main"], "Start Game", { g_windowInfo.width / 2, g_windowInfo.height / 2 },
		UIX::mid, UIY::mid, Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame) ||
		keyStates[SDLK_RETURN].downThisFrame || keyStates[SDLK_e].downThisFrame)
	{

		StopAudio(s_menuMusic);
		SwitchToGame();
	}
	if (DrawButton(g_fonts["Main"], "Settings", { g_windowInfo.width / 2, g_windowInfo.height / 2 + 64 }, 
					UIX::mid, UIY::mid, Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame))
	{

	}

	if (DrawButton(g_fonts["Main"], "Quit", { g_windowInfo.width / 2, g_windowInfo.height / 2 + 128 },
		UIX::mid, UIY::mid, Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame) ||
		keyStates[SDLK_q].downThisFrame)
	{

		QuitApp();
	}

	AddTextureToRender({}, {}, RenderPrio::Background, g_sprites["MainMenuBackground"], White, 0, {}, false, CoordinateSpace::UI);
}
