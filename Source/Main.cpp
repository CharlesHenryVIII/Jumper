#include <stdio.h>
#define GB_MATH_IMPLEMENTATION
#include "SDL.h"
#include "GL/glew.h"
#include "Console.h"
#include "Math.h"
#include "Rendering.h"
#include "Entity.h"
#include "JSONInterop.h"
#include "WinUtilities.h"
#include "GamePlay.h"
#include "Menu.h"
#include "Misc.h"
#include "Audio.h"
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
    g_running = false;
}

int main(int argc, char* argv[])
{
    //Window and Program Setups:Key
    std::unordered_map<int32, Key> keyStates;
    VectorInt mouseLocation;
    g_running = true;
    //g_workingDir = "C:\\Projects\\Jumper\\";
	g_gameState = GameState::game;

    CreateOpenGLWindow();
    InitializeOpenGL();
	g_camera.size.x = 16;
	g_camera.size.y = g_camera.size.x * ((float)g_windowInfo.height / (float)g_windowInfo.width);

	double freq = double(SDL_GetPerformanceFrequency()); //HZ
	double totalTime = SDL_GetPerformanceCounter() / freq; //sec
	double previousTime = totalTime;

    /*
    SDL_Surface iconSurface = {};
    iconSurface.
    */
    LoadAnimationStates();
    LoadFonts();

	g_sprites["spriteMap"] = CreateSprite("Assets/SpriteMap.png");
	g_sprites["background"] = CreateSprite("Assets/Backgrounds/Background.png");
	g_sprites["MainMenuBackground"] = CreateSprite("Assets/Backgrounds/MainMenuBackground.png");

    InitializeAudio();
    AddAllLevels();
    ConsoleLog("Loaded all Levels");
    //LoadLevel("Level 1");
    //LoadLevel("Default");

    SwitchToMenu();

    ConsoleAddCommand("exit", ExitApp);
    ConsoleAddCommand("play", c_PlayAudio);
    ConsoleAddCommand("stop", c_StopAudio);
    ConsoleAddCommand("pause", c_PauseAudio);

    //Start Of Running Program
    while (g_running)
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
                g_running = false;
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
                        g_camera.size.x *= 1.1f;
                    else if (wheel > 0)
                        g_camera.size.x *= 0.9f;
                    g_camera.size.x = Clamp(g_camera.size.x, 5.0f, 100.0f);
                    g_camera.size.y = g_camera.size.x * ((float)windowInfo.height / (float)windowInfo.width);
                }

                break;
            }

            case SDL_WINDOWEVENT:
                if (SDLEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                {
                    Console_OnWindowSize(SDLEvent.window.data1, SDLEvent.window.data2);
                    windowInfo.width = SDLEvent.window.data1;
                    windowInfo.height = SDLEvent.window.data2;
                    g_camera.size.y = g_camera.size.x * ((float)windowInfo.height / (float)windowInfo.width);
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

		switch (g_gameState)
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
		RenderDrawCalls(deltaTime);
		SDL_GL_SwapWindow(windowInfo.SDLWindow);
    }

    return 0;
}
