#include <stdio.h>
#include "SDL.h"
#include "Math.h"
#include "Rendering.h"
#include "Entity.h"
#include "TiledInterop.h"
#include "WinUtilities.h"
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

/*
TODO(choman):
    -camera zoom
    -sub pixel rendering
    -multithread saving/png compression
    -add layered background(s)
    -windows settings local grid highlight
    -add gun to change blocks aka <spring/wind, tacky(can hop off walls with it), timed explosion to get more height>
    -moving platforms
    -enemy ai
    -power ups
    -explosives!?

    -loading bar at the begining of the game
    -multithread loading
    -scaled total time based on incrimenting the delta time
    -game states
        -main menu
        -pausing
        -settings
            -keybinds
    -Player spawning
        -creating a new player on player death
    -store player ID on levels
    -use more IDs
    -fix bullets ending at the end of where you clicked instead of ending immediatly passing where you clicked.
        reference is the tail of the sprite not the head
    -fix player collision against enemy
    -improve actor input system.
        -Moving slightly when you release a horizontal movement key and the next frame you press a horizontal movement key.
        -rewriting the horizontal speed to zero when it should instead only write it once.
            -This causes problems when the player hits an enemy and the enemy sets the movement to a nonzero value, the next frame the players horizontal speed is set to zero again.

    Chris Questions:
        -quaturnions
        -shaders?
        -class vs structs
        -ideas on input system, player collision

    CS20
    -singly linked list
    -doubly linked list
    -stack
    -queue
    -sort funct
    -priority queue
    -hashmap


	-game design:
		*jump_on_head>melee>ranged>magic
		*Levels:
			1.  Just jumping to get use to the mechanics;
			2.  jump on top of enemies to kill them;
			3.  boss fight, jump on head 3 times and boss flees;
			4.  learn how to melee and use it on enemies, continue platforming;
			5.  boss fight with same boss but looks different since its the next phase, try to kill him with melees and timing, he flees;
			6.  learn ranged and continue platforming;
			7.  boss fight with same boss but looks different since its the next phase, try to kill him with ranged and timing, he flees;
			8.  learn magic and continue platforming;
			9.  with same boss but looks different since its the next phase, try to kill him with magic but he flees;
			10. final fight with the boss where you have to do all the phases at once but different arena and what not. fin;

Core Gameplay
    -get to the /end/ of the level
    -use the tools you have to get there with the least amount of blocks added/changed
    -tools include block place and block modifier?

}
*/



enum class DebugOptions
{
    none,
    playerCollision,
    blockCollision,
    clickLocation,
    paintMethod,
    editBlocks
};


struct Key
{
    bool down;
    bool downPrevFrame;
    bool downThisFrame;
    bool upThisFrame;
};

std::unordered_map<DebugOptions, bool> debugList;

struct MouseButtonState {
    Uint32 type;        /**< ::SDL_MOUSEBUTTONDOWN or ::SDL_MOUSEBUTTONUP */
    double timestamp;   /**< In milliseconds, populated using SDL_GetTicks() */
    //Uint32 windowID;  /**< The window with mouse focus, if any */
    //Uint32 which;     /**< The mouse instance id, or SDL_TOUCH_MOUSEID */
    Uint8 button;       /**< The mouse button index */
    Uint8 state;        /**< ::SDL_PRESSED or ::SDL_RELEASED */
    //Uint8 clicks;     /**< 1 for single-click, 2 for double-click, etc. */
    VectorInt location;
};

int main(int argc, char* argv[])
{
    //Window and Program Setups:Key
    std::unordered_map<SDL_Keycode, Key> keyStates;
    MouseButtonState mouseButtonState = {};
    SDL_MouseMotionEvent mouseMotionEvent = {};
    bool running = true;
    Portal* levelChangePortal = nullptr;
    SDL_Event SDLEvent;
    VectorInt previousMouseLocation = {};
    TileType paintType = TileType::invalid;

    CreateWindow();
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
		AnimationList& dino = animations["Dino"];
		dino.GetAnimation(ActorState::jump)->fps = 30.0f;
		dino.GetAnimation(ActorState::run)->fps = 15.0f;
		int spriteHeight = dino.GetAnyValidAnimation()->anime[0]->height;
		dino.colOffset.x = 0.2f;
		dino.colOffset.y = 0.3f;
		dino.colRect = { { 130, spriteHeight - 421 }, { 331, spriteHeight - 33 } };//680 x 472
		dino.scaledWidth = 32;

		AnimationList& headMinion = animations["HeadMinion"];
		spriteHeight = headMinion.GetAnyValidAnimation()->anime[0]->height;
		headMinion.colOffset.x = 0.125f;
		headMinion.colOffset.y = 0.25f;
		headMinion.colRect = { { 4, spriteHeight - 32 }, { 27, spriteHeight - 9 } };
		headMinion.scaledWidth = (float)headMinion.colRect.Width();

		AnimationList& portal = animations["Portal"];
		spriteHeight = portal.GetAnyValidAnimation()->anime[0]->height;
		portal.GetAnimation(ActorState::idle)->fps = 20.0f;
		portal.colOffset.x = 0.125f;
		portal.colOffset.y = 0.25f;
		portal.colRect = { { 85, spriteHeight - 299 }, { 229, spriteHeight - 19 } };
		portal.scaledWidth = 32;

		AnimationList& striker = animations["Striker"];
		striker.GetAnimation(ActorState::run)->fps = 20.0f;
		spriteHeight = striker.GetAnyValidAnimation()->anime[0]->height;
		striker.colOffset.x = 0.125f;
		striker.colOffset.y = 0.25f;
		striker.colRect = { { 36, spriteHeight - 67 }, { 55, spriteHeight - 35 } };
		striker.scaledWidth = 40;
    }

    Sprite* spriteMap = CreateSprite("SpriteMap.png", SDL_BLENDMODE_BLEND);
    FontSprite* textSheet = CreateFont("Text.png", SDL_BLENDMODE_BLEND, 32, 20, 16);
    Sprite* background = CreateSprite("Background.png", SDL_BLENDMODE_BLEND);

    //Level instantiations
    LoadLevel("Level 1");
    LoadLevel("Default");
    currentLevel = GetLevel("Default");
    Level cacheLevel = {};
    cacheLevel.filename = "Level.PNG";
    currentLevel->blocks.UpdateAllBlocks();

    //Player Creation
    float playerAccelerationAmount = 50;
    Actor* actorPlayer = FindPlayer(*currentLevel);


    //Start Of Running Program
    while (running)
    {

        //Setting Timer
        totalTime = SDL_GetPerformanceCounter() / freq;
        float deltaTime = float(totalTime - previousTime);// / 10;
        previousTime = totalTime;

        if (deltaTime > 1 / 30.0f)
        {
            deltaTime = 1 / 30.0f;
        }

        //Player& player = *(Player*)FindActor(playerID);
        WindowInfo& windowInfo = GetWindowInfo();

        //TODO: fix creating a new player makes a new player ID number;
        if (FindPlayer(*currentLevel) == nullptr)
        {
            DebugPrint("player not found");
            //running = false;
            //playerID = CreateActor(ActorType::player, ActorType::none, totalTime);
            //LoadLevel(currentLevel, *(Player*)FindActor(playerID));
        }

        Player* player = FindPlayer(*currentLevel);




        //Event Queing and handling:
        while (SDL_PollEvent(&SDLEvent))
        {
            switch (SDLEvent.type)
            {
            case SDL_QUIT:
                running = false;
                break;

            //case SDL_KEYDOWN:
            //    if (keyBoardEvents[SDLEvent.key.keysym.sym] == 0)
            //        keyBoardEvents[SDLEvent.key.keysym.sym] = totalTime;
            //    DebugPrint("Down: %f\n", totalTime);
            //    break;
            //case SDL_KEYUP:
            //    keyBoardEvents[SDLEvent.key.keysym.sym] = 0;
            //    DebugPrint("Up: %f\n", totalTime);
            //    break;

            case SDL_KEYDOWN:
                {
                    keyStates[SDLEvent.key.keysym.sym].down = true;
                    break;
                }

            case SDL_KEYUP:
                {
                    keyStates[SDLEvent.key.keysym.sym].down = false;
                    break;
                }
    //        case SDL_KEYDOWN:
    //        {
				//if (keyBoardEvents[SDLEvent.key.keysym.sym].down == true)
				//	keyBoardEvents[SDLEvent.key.keysym.sym].downThisFrame = false;
				//else
				//	keyBoardEvents[SDLEvent.key.keysym.sym].downThisFrame = true;
				//keyBoardEvents[SDLEvent.key.keysym.sym].down = true;
    //            DebugPrint("%f\n", totalTime);
				//break;
    //        }
    //        case SDL_KEYUP:
    //        {
				//if (keyBoardEvents[SDLEvent.key.keysym.sym].down == true)
				//	keyBoardEvents[SDLEvent.key.keysym.sym].upThisFrame = true;
				//else
				//	keyBoardEvents[SDLEvent.key.keysym.sym].upThisFrame = false;
				//keyBoardEvents[SDLEvent.key.keysym.sym].down = false;
				//break;
    //        }

            case SDL_MOUSEBUTTONDOWN:
                mouseButtonState.type = SDLEvent.button.type;
                if (mouseButtonState.timestamp == 0)
                    mouseButtonState.timestamp = totalTime;//SDLEvent.button.timestamp;
                mouseButtonState.button = SDLEvent.button.button;
                mouseButtonState.state = SDLEvent.button.state;
                mouseButtonState.location.x = SDLEvent.button.x;
                mouseButtonState.location.y = SDLEvent.button.y;
                break;
            case SDL_MOUSEBUTTONUP:
                mouseButtonState.type = SDLEvent.button.type;
                mouseButtonState.timestamp = 0;//SDLEvent.button.timestamp;
                mouseButtonState.button = SDLEvent.button.button;
                mouseButtonState.state = SDLEvent.button.state;
                mouseButtonState.location.x = SDLEvent.button.x;
                mouseButtonState.location.y = SDLEvent.button.y;
                break;
            case SDL_MOUSEMOTION:
                mouseMotionEvent = SDLEvent.motion;
                break;
            }
        }

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

        //Keyboard Control:
        if (player != nullptr)
        {
            player->acceleration.x = 0;
            if (keyStates[SDLK_w].downThisFrame || keyStates[SDLK_SPACE].downThisFrame || keyStates[SDLK_UP].downThisFrame)
            {
                if (player->jumpCount > 0)
                {
                    player->velocity.y = 20.0f;
                    player->jumpCount -= 1;
                    PlayAnimation(player, ActorState::jump);
                }
            }
            bool left = keyStates[SDLK_a].down || keyStates[SDLK_LEFT].down;
            bool right = keyStates[SDLK_d].down || keyStates[SDLK_RIGHT].down;
            if (left)
                player->acceleration.x -= playerAccelerationAmount;
            if (right)
                player->acceleration.x += playerAccelerationAmount;
            if (left == right)
            {
                player->velocity.x = 0;
                player->acceleration.x = 0;
            }

        }


        if (keyStates[SDLK_1].downThisFrame)
            debugList[DebugOptions::playerCollision] = !debugList[DebugOptions::playerCollision];
        if (keyStates[SDLK_2].downThisFrame)
            debugList[DebugOptions::blockCollision] = !debugList[DebugOptions::blockCollision];
        if (keyStates[SDLK_3].downThisFrame)
            debugList[DebugOptions::clickLocation] = !debugList[DebugOptions::clickLocation];
        if (keyStates[SDLK_4].downThisFrame)
            debugList[DebugOptions::paintMethod] = !debugList[DebugOptions::paintMethod];
        if (keyStates[SDLK_5].downThisFrame)
            debugList[DebugOptions::editBlocks] = !debugList[DebugOptions::editBlocks];
        if (keyStates[SDLK_0].downThisFrame)
            SaveLevel(&cacheLevel, *player);
        if (keyStates[SDLK_9].downThisFrame)
            LoadLevel(&cacheLevel, *player);

		if (levelChangePortal != nullptr && keyStates[SDLK_w].downThisFrame)
		{
			//remove player from new level, load player from old level, delete player from old level.
			Level* oldLevel = currentLevel;
			currentLevel = GetLevel(levelChangePortal->levelPointer);

			std::erase_if(currentLevel->actors, [](Actor* actor)
			{
				if (actor->GetActorType() == ActorType::player)
				{
					delete actor;
					return true;
				}
				else
					return false;
			});

			currentLevel->actors.push_back(player);
			std::erase_if(oldLevel->actors, [](Actor* actor)
			{
				return actor->GetActorType() == ActorType::player;
			});
			player->position = GetPortalsPointer(levelChangePortal)->position;
		}
		levelChangePortal = nullptr;

        //Mouse Control:
        Rectangle clickRect = {};
        laser.inUse = false;
        if (debugList[DebugOptions::editBlocks] && mouseButtonState.timestamp && &player != nullptr)
        {
            VectorInt mouseLocation = CameraToPixelCoord({ mouseMotionEvent.x, mouseMotionEvent.y });
            Vector mouseLocationTranslated = PixelToBlock(mouseLocation);

            clickRect.bottomLeft = { mouseLocationTranslated.x - 0.5f, mouseLocationTranslated.y - 0.5f };
            clickRect.topRight = { mouseLocationTranslated.x + 0.5f, mouseLocationTranslated.y + 0.5f };
            Block* blockPointer = &currentLevel->blocks.GetBlock(mouseLocationTranslated);
            blockPointer->location = mouseLocationTranslated;
            blockPointer->location.x = floorf(blockPointer->location.x);
            blockPointer->location.y = floorf(blockPointer->location.y);

            if (mouseButtonState.timestamp == totalTime)
            {
                if (mouseButtonState.button == SDL_BUTTON_LEFT)
                    paintType = TileType::filled;
                else if (mouseButtonState.button == SDL_BUTTON_RIGHT)
                    paintType = TileType::invalid;
            }
            if (debugList[DebugOptions::paintMethod] && mouseButtonState.type == SDL_MOUSEBUTTONDOWN)
            {
                if ((mouseButtonState.location.x != previousMouseLocation.x || mouseButtonState.location.y != previousMouseLocation.y))
                {
                    CreateLaser(player, mouseLocationTranslated, paintType, deltaTime);
                    blockPointer->tileType = paintType;
                    UpdateAllNeighbors(blockPointer);
                    previousMouseLocation = mouseLocation;
                }
            }
            else if (mouseButtonState.timestamp == totalTime)
            {
                // TODO: remove the cast
                currentLevel->actors.push_back(CreateBullet(player, mouseLocationTranslated, paintType));
            }
        }

        /*********************
         *
         * Updates
         *
         ********/



        for (int32 i = 0; i < currentLevel->actors.size(); i++)
            currentLevel->actors[i]->Update(deltaTime);

        bool screenFlash = false;
        if (player != nullptr)
        {
            for (int i = 0; i < currentLevel->actors.size(); i++)
            {
                if (currentLevel->actors[i]->GetActorType() == ActorType::enemy)
                {
                    screenFlash = CollisionWithActor(*player, *currentLevel->actors[i], *currentLevel, float(totalTime));
                }
                else if (currentLevel->actors[i]->GetActorType() == ActorType::portal)
                {
                    if (CollisionWithActor(*player, *currentLevel->actors[i], *currentLevel, float(totalTime)))
                        levelChangePortal = (Portal*)currentLevel->actors[i];
                }
            }
        }



        /*********************
         *
         * Renderer
         *
         ********/


        SDL_SetRenderDrawColor(windowInfo.renderer, 0, 0, 0, 255);
        SDL_RenderClear(windowInfo.renderer);
        BackgroundRender(background, &camera);
        SDL_SetRenderDrawBlendMode(windowInfo.renderer, SDL_BLENDMODE_BLEND);
        if (player != nullptr)
            camera.position = player->position;

        Vector offset = { 25, 15 };
        for (float y = camera.position.y - offset.y; y < (camera.position.y + offset.y); y++)
        {
            for (float x = camera.position.x - offset.x; x < (camera.position.x + offset.x); x++)
            {
                Block* block = currentLevel->blocks.TryGetBlock({ x, y });
				if (block != nullptr && block->tileType != TileType::invalid)
				{
					SpriteMapRender(spriteMap, *block);

					if (debugList[DebugOptions::blockCollision] && block->tileType != TileType::invalid)
					{
						Rectangle blockRect;
						blockRect.bottomLeft = block->location;
						blockRect.topRight = { block->location.x + 1 , block->location.y + 1 };

                        AddRectToRender(blockRect, lightRed, RenderPrio::Debug);
					}
				}

			}
        }
        RenderActors();
        RenderLaser();

        DrawText(textSheet, Green, std::to_string(1 / deltaTime) + "fps", 1.0f, { 0, 0 }, UIX::left, UIY::top);
        if (player != nullptr)
        {
            DrawText(textSheet, Green, "{ " + std::to_string(player->position.x) + ", " + std::to_string(player->position.y) + " }", 0.75f, { 0, windowInfo.height - 40 }, UIX::left, UIY::bot);
            DrawText(textSheet, Green, "{ " + std::to_string(player->velocity.x) + ", " + std::to_string(player->velocity.y) + " }", 0.75f, { 0, windowInfo.height - 20 }, UIX::left, UIY::bot);
            DrawText(textSheet, Green, "{ " + std::to_string(player->acceleration.x) + ", " + std::to_string(player->acceleration.y) + " }", 0.75f, { 0, windowInfo.height }, UIX::left, UIY::bot);

        }

        if (screenFlash)
        {
            SDL_SetRenderDrawColor(windowInfo.renderer, lightWhite.r, lightWhite.g, lightWhite.b, lightWhite.a);
            SDL_RenderDrawRect(windowInfo.renderer, NULL);
            SDL_RenderFillRect(windowInfo.renderer, NULL);
        }
        RenderDrawCalls();
        //DrawButton(textSheet, "Test", { 0, 0 }, UIX::left, UIY::top, Green, Orange, mouseButtonEvent, mouseMotionEvent);
        //DrawText(textSheet, Red, "Test", { windowInfo.width / 2, windowInfo.height / 2}, UIX::mid, UIY::mid);

        //Present Screen
        SDL_RenderPresent(windowInfo.renderer);
        std::erase_if(currentLevel->actors, [](Actor* p) {
            if (!p->inUse)
            {
                delete p;
                return true;
            }
            return false;
        });
    }

    return 0;
}
