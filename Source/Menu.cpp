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
	g_gameState = GameState::mainMenu;
	Audio audio = {};
	audio.nameOfSound = "Halo";
	audio.flags |= AUDIO_FADEOUT | AUDIO_FADEIN;
	audio.fadeOutTime = 3.0f;
	audio.fadeInTime = 3.0f;
	s_menuMusic = PlayAudio(audio);

	Audio temp1 = {};
	temp1.flags |= AUDIO_FADEOUT | AUDIO_REPEAT;
	temp1.nameOfSound = "Grass";
	temp1.fadeOutTime = 2.0f;
	temp1.loopCount = 10;
	//PlayAudio(temp1);

	Audio temp2 = {};
	temp2.flags |= AUDIO_REPEAT;
	temp2.nameOfSound = "Button_Hover";
	temp2.fadeOutTime = 2.0f;
	temp2.loopCount = 20;
	temp2.secondsToPlay = 10.0f;
	PlayAudio(temp2);

    ConsoleLog("Switched To Menu");
}

void DoPlayMenu(float deltaTime, std::unordered_map<int32, Key>& keyStates, VectorInt mouseLocation)
{
	DrawText(g_fonts["2"], White, "J U M P E R", 4.0f, { g_windowInfo.width / 2, g_windowInfo.height / 4 }, UIX::mid, UIY::mid);

	if (DrawButton(g_fonts["1"], "Start Game", { g_windowInfo.width / 2, g_windowInfo.height / 2 },
		UIX::mid, UIY::mid, Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame) ||
		keyStates[SDLK_RETURN].downThisFrame || keyStates[SDLK_e].downThisFrame)
	{

        //PlayAudio("Button_Confirm");
		s_menuMusic = StopAudio(s_menuMusic);
		SwitchToGame();
	}
	if (DrawButton(g_fonts["1"], "Settings", { g_windowInfo.width / 2, g_windowInfo.height / 2 + 64 }, 
					UIX::mid, UIY::mid, Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame))
	{

        //PlayAudio("Button_Confirm");
	}

	if (DrawButton(g_fonts["1"], "Quit", { g_windowInfo.width / 2, g_windowInfo.height / 2 + 128 },
		UIX::mid, UIY::mid, Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame) ||
		keyStates[SDLK_q].downThisFrame)
	{

        //PlayAudio("Button_Confirm");
		QuitApp();
	}

	AddTextureToRender({}, {}, RenderPrio::Background, g_sprites["MainMenuBackground"], White, 0, {}, false, CoordinateSpace::UI);
}
