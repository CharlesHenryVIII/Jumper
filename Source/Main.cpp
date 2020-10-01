#include <stdio.h>
#define GB_MATH_IMPLEMENTATION
#include "SDL.h"
#include "GL/glew.h"
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

    double freq = double(SDL_GetPerformanceFrequency()); //HZ
    double totalTime = SDL_GetPerformanceCounter() / freq; //sec
    double previousTime = totalTime;

    /*
    SDL_Surface iconSurface = {};
    iconSurface.
    */

        {
	        std::vector<AnimationData> animationData;

            AnimationData dino = {};
            dino.name = "Dino";
            dino.animationFPS[int(ActorState::jump)] = 30.0f;
            dino.animationFPS[int(ActorState::run)] = 30.0f;
            dino.collisionOffset = { 0.2f, 0.3f };
            dino.collisionRectangle = { { 130, 421 }, { 331, 33 } };
            animationData.push_back(dino);

            AnimationData headMinion = {};
            headMinion.name = "HeadMinion";
			headMinion.collisionRectangle = { { 4, 32 }, { 27, 9 } };
            headMinion.scaledWidth = inf;
            animationData.push_back(headMinion);

            AnimationData bullet;
            bullet.name = "Bullet";
            bullet.collisionRectangle = { { 0, inf }, { inf, 0 } };
            bullet.scaledWidth = inf;
            animationData.push_back(bullet);

            AnimationData portal;
            portal.name = "Portal";
            portal.animationFPS[int(ActorState::idle)] = 20.0f;
            portal.collisionRectangle = { { 85, 299 }, { 229, 19 } }; 
            animationData.push_back(portal);

            AnimationData striker;
            striker.name = "Striker";
            striker.animationFPS[(int)ActorState::run] = 20.0f;
			striker.collisionRectangle = { { 36, 67 }, { 55, 35 } };
			striker.scaledWidth = 40;
            animationData.push_back(striker);
            
            AnimationData spring;
            spring.name = "Spring";
			spring.collisionRectangle = { { 0, 31 }, { 31, 0 } };
            animationData.push_back(spring);

			AnimationData MP;
            MP.name = "MovingPlatform";
			MP.collisionOffset = {};
			MP.collisionRectangle = { { 0, inf }, { inf, 0 } };
            animationData.push_back(MP);

            AnimationData grapple;
            grapple.name = "Bullet";
			grapple.collisionRectangle = { { 0, inf }, { inf, 0 } };
			grapple.scaledWidth = inf;
            animationData.push_back(grapple);

            AnimationData knight;
            knight.name = "Knight";
			knight.collisionRectangle = { { 419, (44 + 814) }, { 419 + 360, 44} };
            animationData.push_back(knight);
	        LoadAnimationStates(&animationData);
		}


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
	fonts["1"] = CreateFont("Text.png", SDL_BLENDMODE_BLEND, 32, 20, 16);
	fonts["2"] = CreateFont("Text 2.png", SDL_BLENDMODE_BLEND, 20, 20, 15);

    //Level instantiations
    AddAllLevels();
    //LoadLevel("Level 1");
    //LoadLevel("Default");

    SwitchToMenu();

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
                keyStates[SDLEvent.key.keysym.sym].down = true;
                DebugPrint("Key down: %f\n", totalTime);
                break;

            case SDL_KEYUP:
                keyStates[SDLEvent.key.keysym.sym].down = false;
                DebugPrint("Key up:   %f\n", totalTime);
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                keyStates[SDLEvent.button.button].down = SDLEvent.button.state;
                break;

            case SDL_MOUSEMOTION:
                mouseLocation.x = SDLEvent.motion.x;
                mouseLocation.y = SDLEvent.motion.y;
                break;

            case SDL_MOUSEWHEEL:
            {
                int32 wheel = SDLEvent.wheel.y;
                if (wheel < 0)
                    camera.size.x *= 1.1f;
                else if (wheel > 0)
                    camera.size.x *= 0.9f;
                camera.size.x = Clamp(camera.size.x, 5.0f, 100.0f);
				camera.size.y = camera.size.x * ((float)windowInfo.height / (float)windowInfo.width);
                break;
            }

            case SDL_WINDOWEVENT:
                if (SDLEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                {
                    windowInfo.width = SDLEvent.window.data1;
                    windowInfo.height = SDLEvent.window.data2;
                    camera.size.y = camera.size.x * ((float)windowInfo.height / (float)windowInfo.width);
                }

            }

        }

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
                    DebugPrint("KeyDown && DownPreviousFrame: %f\n", totalTime);
                    key.second.downThisFrame = false;
                }
                else
                {
                    DebugPrint("Down this frame: %f\n", totalTime);
                    key.second.downThisFrame = true;
                }
            }

            else
            {
                key.second.downThisFrame = false;
                if (key.second.downPrevFrame)
                {
                    DebugPrint("Up This frame: %f\n", totalTime);
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
