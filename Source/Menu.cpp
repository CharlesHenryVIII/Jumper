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
AudioID s_testSound = 0;

void QuitApp()
{
	g_running = false;
}


void SwitchToMenu()
{
	g_gameState = GameState::mainMenu;
	//Audio audio = {};
	//audio.nameOfSound = "Halo";
	//audio.fadeOutDuration = 3.0f;
	//audio.fadeInDuration = 3.0f;
	//s_menuMusic = PlayAudio(audio);

	//Audio temp1 = {};
	//temp1.nameOfSound = "Grass";
	//temp1.fadeOutDuration = 2.0f;
	//temp1.loopCount = 10;
	////PlayAudio(temp1);

	//Audio temp2 = {};
	//temp2.nameOfSound = "Button_Hover";
	//temp2.fadeOutDuration = 2.0f;
	//temp2.loopCount = 20;
	//temp2.secondsToPlay = 10.0f;
	//PlayAudio(temp2);

    ConsoleLog("Switched To Menu");
}

void DoPlayMenu(float deltaTime, std::unordered_map<int32, Key>& keyStates, VectorInt mouseLocation)
{
	DrawText(g_fonts["2"], White, "J U M P E R", 4.0f, { g_windowInfo.width / 2, g_windowInfo.height / 4 }, UIX::mid, UIY::mid);

	if (DrawButton(g_fonts["1"], "Start Game", { g_windowInfo.width / 2, g_windowInfo.height / 2 },
		UIX::mid, UIY::mid, Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame) ||
		keyStates[SDLK_RETURN].downThisFrame || keyStates[SDLK_e].downThisFrame)
	{

		StopAudio(s_menuMusic);
		SwitchToGame();
	}
	if (DrawButton(g_fonts["1"], "Settings", { g_windowInfo.width / 2, g_windowInfo.height / 2 + 64 }, 
					UIX::mid, UIY::mid, Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame))
	{

	}

	if (DrawButton(g_fonts["1"], "Quit", { g_windowInfo.width / 2, g_windowInfo.height / 2 + 128 },
		UIX::mid, UIY::mid, Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame) ||
		keyStates[SDLK_q].downThisFrame)
	{

		QuitApp();
	}

	if (DrawButton(g_fonts["1"], "Play Music", { 0, 128 }, UIX::left, UIY::mid, 
		Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame, false))
	{
		AudioParams audio = {};
		audio.nameOfSound = "Halo";
		audio.fadeOutDuration = 3.0f;
		audio.fadeInDuration = 3.0f;
		s_menuMusic = PlayAudio(audio);
	}

	if (DrawButton(g_fonts["1"], "Stop Music", { 0, 128 + 64 }, UIX::left, UIY::mid, 
		Red, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame, false))
	{
		StopAudio(s_menuMusic);
	}

	if (DrawButton(g_fonts["1"], "Play Sound", { 0, 256 }, UIX::left, UIY::mid, 
		Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame, false))
	{
		s_testSound = PlayAudio("Grass");
	}

	if (DrawButton(g_fonts["1"], "Play Sound x5", { 0, 256 + 64 }, UIX::left, UIY::mid, 
		Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame, false))
	{
		AudioParams audio = {};
		audio.nameOfSound = "Grass";
		audio.loopCount = 5;
		s_testSound = PlayAudio(audio);
	}

	if (DrawButton(g_fonts["1"], "Play Sound inf", { 0, 256 + 128 }, UIX::left, UIY::mid, 
		Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame, false))
	{
		AudioParams audio = {};
		audio.nameOfSound = "Grass";
		audio.loopCount = AUDIO_MAXLOOPS;
		s_testSound = PlayAudio(audio);
	}

	if (DrawButton(g_fonts["1"], "Play Sound inf fadein and Fadeout", { 0, 256 + 128 + 64}, UIX::left, UIY::mid, 
		Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame, false))
	{
		AudioParams audio = {};
		audio.nameOfSound = "Grass";
		audio.loopCount = AUDIO_MAXLOOPS;
		audio.fadeInDuration  = 1.0f;
		audio.fadeOutDuration = 1.0f;
		s_testSound = PlayAudio(audio);
	}

	if (DrawButton(g_fonts["1"], "Stop Sound", { 0, 512 }, UIX::left, UIY::mid, 
		Red, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame, false))
	{
		StopAudio(s_testSound);
	}

	if (DrawButton(g_fonts["1"], "Play Button Sound", { 0, 512 + 64 }, UIX::left, UIY::mid,
		Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame, false))
	{
		AudioParams audio = {};
		audio.nameOfSound = "Button_Hover";
		audio.fadeOutDuration = 2.0f;
		audio.loopCount = 20;
		audio.secondsToPlay = 4.0f;
		PlayAudio(audio);
	}

	if (DrawButton(g_fonts["1"], "Play Button Sound Broken", { 0, 512 + 128 }, UIX::left, UIY::mid,
		Green, White, mouseLocation, keyStates[SDL_BUTTON_LEFT].downThisFrame, false))
	{
		AudioParams audio = {};
		audio.nameOfSound = "Button_Hover";
		audio.fadeInDuration = 10.0f;
		audio.fadeOutDuration = 12.0f;
		audio.loopCount = 5;
		PlayAudio(audio);
	}
	AddTextureToRender({}, {}, RenderPrio::Background, g_sprites["MainMenuBackground"], White, 0, {}, false, CoordinateSpace::UI);
}
