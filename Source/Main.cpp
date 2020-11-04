#include <stdio.h>
#define GB_MATH_IMPLEMENTATION
#include "SDL.h"
#include "GL/glew.h"
#include "Console.h"
#include "Math.h"
#include "Rendering.h"
#include "Entity.h"
#include "TiledInterop.h"
#include "WinUtilities.h"
#include "GamePlay.h"
#include "Menu.h"
#include "Misc.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
//#define STB_TRUETYPE_IMPLEMENTATION
//#include "stb/stb_truetype.h"

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cmath>

CONSOLE_FUNCTION(ExitApp)
{
    running = false;
}

struct AudioFileData {
    SDL_AudioSpec spec = {};
    uint8* buffer = nullptr;
    uint32 length = 0;
};

struct AudioDriverData {
    double totalTime = 0;
    float loudness = 0;
#if 1
    uint16 samplesTaken = 0;
#else 
    float samplesTaken = 0;
#endif
    float frequency = 0;
    AudioFileData fileToPlay;
} audioData;



AudioFileData LoadWavFile(const char* fileLocation)
{
    AudioFileData data = {};
    if (SDL_LoadWAV(fileLocation, &data.spec, &data.buffer, &data.length) == NULL)
        ConsoleLog("Could not open %s: %s\n", fileLocation, SDL_GetError());
    return data;
}

double previousTime = 0;
void CSH_AudioCallback(void* userdata, Uint8* stream, int len)
{
    uint16* writeBuff = reinterpret_cast<uint16*>(stream);
    int32 count = len / sizeof(*writeBuff);
    AudioDriverData* audioDataInfo = reinterpret_cast<AudioDriverData*>(userdata);

#if 1

    //Only write size of the buffer
    //handle incrementing song with audioDataInfo->samples
    //handle overflow at the end of the song
    if (audioDataInfo->fileToPlay.buffer)
        SDL_MixAudioFormat(stream, audioDataInfo->fileToPlay.buffer, audioDataInfo->fileToPlay.spec.format, audioDataInfo->fileToPlay.length, 40);

#else

    //LRLRLR ordering
    //Should I use a buffer method or audio queing?
    ConsoleLog("length: %d, count: %d, timeStamp: %0.5f\n", len, count, audioDataInfo->totalTime - previousTime);
    previousTime = audioDataInfo->totalTime;
    for (int32 i = 0; i < count / 2; i++)
    {
        
        float sampleFreq = 1 / audioDataInfo->frequency;
        uint16 information = (uint16)(sinf((audioDataInfo->samplesTaken++) * sampleFreq * tau * 100) * 20000 + 20000);

        *buf = information;
        buf += 1;
        *buf = information;
        buf++;
    }

    //audioDataInfo->samplesTaken += count / 2;
    assert((uintptr_t)writeBuff == uintptr_t(stream + len));
#endif
}

int main(int argc, char* argv[])
{
    //Window and Program Setups:Key
    std::unordered_map<int32, Key> keyStates;
    VectorInt mouseLocation;
    running = true;

	gameState = GameState::game;

    CreateOpenGLWindow();
    InitializeOpenGL();
	camera.size.x = 16;
	camera.size.y = camera.size.x * ((float)windowInfo.height / (float)windowInfo.width);

    {
		//for (int i = 0; i < SDL_GetNumAudioDrivers(); ++i) {
		//	const char* driverName = SDL_GetAudioDriver(i);
		//	if (driverName == "directsound")
		//	{
		//		if (SDL_AudioInit(driverName))
		//		{
		//			DebugPrint("Audio driver failed to initialize: %s\n", driverName);
		//			ConsoleLog("Audio driver failed to initialize: %s\n", driverName);
		//		}
		//		DebugPrint("Audio driver initilized: %s\n", driverName);
		//		ConsoleLog("Audio driver initilized: %s\n", driverName);
		//	}
		//	DebugPrint("Audio driver %s not initilized or used\n", driverName);
		//	continue;
		//}

		SDL_AudioSpec want, have;
		SDL_AudioDeviceID audioDevice;



		SDL_memset(&want, 0, sizeof(want));
		want.freq = 48000;
		want.format = AUDIO_U16;
		want.channels = 2;
		want.samples = 4096;
        want.userdata = &audioData;
		want.callback = CSH_AudioCallback;
		//should I allow any device driver or should I prefer one like directsound?
		audioDevice = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
        audioData.frequency = (float)have.freq;

		if (audioDevice == 0) 
        {
			ConsoleLog("Failed to open audio: %s", SDL_GetError());
		}
		else 
        {
			if (have.format != want.format) // we let this one thing change.
				ConsoleLog("We didn't get Float32 audio format.");

            assert(have.format == AUDIO_U16);
			SDL_PauseAudioDevice(audioDevice, 0); /* start audio playing. */
			//SDL_Delay(1000); /* let the audio callback play some sound for 1 seconds. */

			//SDL_CloseAudioDevice(audioDevice);
		}
		//SDL_AudioQuit(); //used only for "shutting down audio" that was initilized with SDL_AudioInit()
	}
    audioData.fileToPlay = LoadWavFile("C:\\Projects\\Jumper\\Assets\\10. This is the Hour.wav");

	double freq = double(SDL_GetPerformanceFrequency()); //HZ
	double totalTime = SDL_GetPerformanceCounter() / freq; //sec
	double previousTime = totalTime;

    /*
    SDL_Surface iconSurface = {};
    iconSurface.
    */

    LoadAllAnimationStates();
    ConsoleLog("Loaded all Animation States");

	fonts["1"] = CreateFont("Text.png", SDL_BLENDMODE_BLEND, 32, 20, 16);
	fonts["2"] = CreateFont("Text 2.png", SDL_BLENDMODE_BLEND, 20, 20, 15);

	sprites["spriteMap"] = CreateSprite("SpriteMap.png", SDL_BLENDMODE_BLEND);
	sprites["background"] = CreateSprite("Background.png", SDL_BLENDMODE_BLEND);
	sprites["MainMenuBackground"] = CreateSprite("MainMenuBackground.png", SDL_BLENDMODE_BLEND);
//    const char* fontFileNames[] = {
//        "Text.png",
//        "Text 2.png",
//    }
//    LoadFonts(fontFileNames);
//    Cant do this because I would need to pass the font information as well
//    could make a struct but will do when/if there are more fonts

    //Level instantiations
    AddAllLevels();
    ConsoleLog("Loaded all Levels");
    //LoadLevel("Level 1");
    //LoadLevel("Default");

    SwitchToMenu();

    ConsoleAddCommand("exit", ExitApp);

    //Start Of Running Program
    while (running)
    {
        /*********************
         *
         * Setting Timer
         *
         ********/

        totalTime = SDL_GetPerformanceCounter() / freq;
        float deltaTime = float(totalTime - previousTime);// / 10;
        previousTime = totalTime;

        if (deltaTime > 1 / 30.0f)
        {
            deltaTime = 1 / 30.0f;
        }
        audioData.totalTime = totalTime;

        /*********************
         *
         * Getting Window
         *
         ********/
        WindowInfo& windowInfo = GetWindowInfo();


        /*********************
         *
         * Event Queing and handling
         *
         ********/

        SDL_Event SDLEvent;
        while (SDL_PollEvent(&SDLEvent))
        {
            switch (SDLEvent.type)
            {
            case SDL_QUIT:
                running = false;
                break;

            case SDL_KEYDOWN:
            {
                int mods = 0;
                // Ensure that both possible shift flags are set for this message, we don't care to
                // differentiate between left and right ctrl and shift.
                if (SDLEvent.key.keysym.mod & KMOD_SHIFT) mods |= KMOD_SHIFT;
                if (SDLEvent.key.keysym.mod & KMOD_CTRL) mods |= KMOD_CTRL;
                if (SDLEvent.key.keysym.mod & KMOD_ALT) mods |= KMOD_ALT;

                if (!Console_OnKeyboard(SDLEvent.key.keysym.sym, mods, SDLEvent.key.state == SDL_PRESSED, SDLEvent.key.repeat != 0))
                    keyStates[SDLEvent.key.keysym.sym].down = true;
                break;
            }

            case SDL_KEYUP:
                keyStates[SDLEvent.key.keysym.sym].down = false;
                //DebugPrint("Key up:   %f\n", totalTime);
                break;

            case SDL_TEXTINPUT:
            {
                size_t length = strlen(SDLEvent.text.text);
                for (size_t i = 0; i < length; ++i)
                    Console_OnCharacter(SDLEvent.text.text[i]);
                break;
            }

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                if (!Console_OnMouseButton(SDLEvent.button.button, SDLEvent.button.state == SDL_PRESSED))
                    keyStates[SDLEvent.button.button].down = SDLEvent.button.state;
                break;

            case SDL_MOUSEMOTION:
                mouseLocation.x = SDLEvent.motion.x;
                mouseLocation.y = SDLEvent.motion.y;
                break;

            case SDL_MOUSEWHEEL:
            {
                int32 wheel = SDLEvent.wheel.y;
                if (!Console_OnMouseWheel(float(wheel)))
                {
                    if (wheel < 0)
                        camera.size.x *= 1.1f;
                    else if (wheel > 0)
                        camera.size.x *= 0.9f;
                    camera.size.x = Clamp(camera.size.x, 5.0f, 100.0f);
                    camera.size.y = camera.size.x * ((float)windowInfo.height / (float)windowInfo.width);
                }

                break;
            }

            case SDL_WINDOWEVENT:
                if (SDLEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                {
                    Console_OnWindowSize(SDLEvent.window.data1, SDLEvent.window.data2);
                    windowInfo.width = SDLEvent.window.data1;
                    windowInfo.height = SDLEvent.window.data2;
                    camera.size.y = camera.size.x * ((float)windowInfo.height / (float)windowInfo.width);
                    ConsoleLog("Window size changed to %d %d", windowInfo.width, windowInfo.height);
                }
                else if (SDLEvent.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
                {
                    SDL_CaptureMouse(SDL_TRUE);
                }

                break;
            }

        }
        ConsoleRun();

        /*********************
         *
         * Setting Key States
         *
         ********/

        for (auto& key : keyStates)
        {

            if (key.second.down)
            {
                key.second.upThisFrame = false;
                if (key.second.downPrevFrame)
                {
                    //DebugPrint("KeyDown && DownPreviousFrame: %f\n", totalTime);
                    key.second.downThisFrame = false;
                }
                else
                {
                    //DebugPrint("Down this frame: %f\n", totalTime);
                    key.second.downThisFrame = true;
                }
            }

            else
            {
                key.second.downThisFrame = false;
                if (key.second.downPrevFrame)
                {
                    //DebugPrint("Up This frame: %f\n", totalTime);
                    key.second.upThisFrame = true;
                }
                else
                {
                    //DebugPrint("KeyNOTDown && NotDownPreviousFrame: %f\n", totalTime);
                    key.second.upThisFrame = false;
                }
            }
            key.second.downPrevFrame = key.second.down;
        }

        /*********************
         *
         * Game States
         *
         ********/

		switch (gameState)
		{
			case GameState::mainMenu:
			{
				DoPlayMenu(deltaTime, keyStates, mouseLocation);
				break;
			}
			case GameState::game:
			{
				DoPlayGame(deltaTime, keyStates, mouseLocation);
				break;
			}
		}
		RenderDrawCalls();
		SDL_GL_SwapWindow(windowInfo.SDLWindow);
    }

    return 0;
}
