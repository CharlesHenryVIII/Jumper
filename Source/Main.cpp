#include <stdio.h>
#include "SDL.h"
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


//void foo(std::vector<int>* a)
//{
//    a[4]; // == std::vector<int>
//    (*a)[4]; // == int
//}
//
//void bar(std::vector<int>& a)
//{
//    a[4];
//}
//
//
//std::string test(const std::string& a)
//{
//}
//
//void foo2()
//{
//    test("a");
//
//    const std::string& a = test("a");
//}


int main(int argc, char* argv[])
{
    //Window and Program Setups:Key
    std::unordered_map<int32, Key> keyStates;
    VectorInt mouseLocation;
    running = true;

	gameState = GameState::game;
    CreateWindow();
	SDL_SetRenderDrawBlendMode(windowInfo.renderer, SDL_BLENDMODE_BLEND);
    double freq = double(SDL_GetPerformanceFrequency()); //HZ
    double totalTime = SDL_GetPerformanceCounter() / freq; //sec
    double previousTime = totalTime;

    /*
    SDL_Surface iconSurface = {};
    iconSurface.
    */

    //Sprite Creation
    LoadAllAnimationStates("Dino");
    LoadAllAnimationStates("HeadMinion");
    LoadAllAnimationStates("Bullet");
    LoadAllAnimationStates("Portal");
    LoadAllAnimationStates("Striker");
    {
		AnimationList& dino = actorAnimations["Dino"];
		dino.GetAnimation(ActorState::jump)->fps = 30.0f;
		dino.GetAnimation(ActorState::run)->fps = 15.0f;
		int spriteHeight = dino.GetAnyValidAnimation()->anime[0]->height;
		dino.colOffset.x = 0.2f;
		dino.colOffset.y = 0.3f;
		dino.colRect = { { 130, spriteHeight - 421 }, { 331, spriteHeight - 33 } };//680 x 472
		dino.scaledWidth = 32;

		AnimationList& headMinion = actorAnimations["HeadMinion"];
		spriteHeight = headMinion.GetAnyValidAnimation()->anime[0]->height;
		headMinion.colOffset.x = 0.125f;
		headMinion.colOffset.y = 0.25f;
		headMinion.colRect = { { 4, spriteHeight - 32 }, { 27, spriteHeight - 9 } };
		headMinion.scaledWidth = (float)headMinion.colRect.Width();

		AnimationList& portal = actorAnimations["Portal"];
		spriteHeight = portal.GetAnyValidAnimation()->anime[0]->height;
		portal.GetAnimation(ActorState::idle)->fps = 20.0f;
		portal.colOffset.x = 0.125f;
		portal.colOffset.y = 0.25f;
		portal.colRect = { { 85, spriteHeight - 299 }, { 229, spriteHeight - 19 } };
		portal.scaledWidth = 32;

		AnimationList& striker = actorAnimations["Striker"];
		striker.GetAnimation(ActorState::run)->fps = 20.0f;
		spriteHeight = striker.GetAnyValidAnimation()->anime[0]->height;
		striker.colOffset.x = 0.125f;
		striker.colOffset.y = 0.25f;
		striker.colRect = { { 36, spriteHeight - 67 }, { 55, spriteHeight - 35 } };
		striker.scaledWidth = 40;
    }
	sprites["spriteMap"] = CreateSprite("SpriteMap.png", SDL_BLENDMODE_BLEND);
	sprites["background"] = CreateSprite("Background.png", SDL_BLENDMODE_BLEND);
	fonts["1"] = CreateFont("Text.png", SDL_BLENDMODE_BLEND, 32, 20, 16);
	fonts["2"] = CreateFont("Text 2.png", SDL_BLENDMODE_BLEND, 20, 20, 15);

    //Level instantiations
    LoadLevel("Level 1");
    LoadLevel("Default");

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
                break;

            case SDL_KEYUP:
                keyStates[SDLEvent.key.keysym.sym].down = false;
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                keyStates[SDLEvent.button.button].down = SDLEvent.button.state;
                break;

            case SDL_MOUSEMOTION:
                mouseLocation.x = SDLEvent.motion.x;
                mouseLocation.y = SDLEvent.motion.y;
                break;
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
                if (key.second.downPrevFrame)
                    key.second.downThisFrame = false;
                else
                    key.second.downThisFrame = true;
            }

            else
            {
                if (key.second.downPrevFrame)
                    key.second.upThisFrame = true;
                else
                    key.second.upThisFrame = false;
            }
            key.second.downPrevFrame = key.second.down;
        }

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
		SDL_RenderPresent(windowInfo.renderer);
    }

    return 0;
}
