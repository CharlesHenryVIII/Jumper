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
    -handle multiple levels
    -moving platforms
    -enemy ai
    -power ups
    -explosives!?
    
    -loading bar at the begining of the game
    -multithread loading
    -scaled total time based on incrimenting the delta time
    -game states
    -Player spawning
    -store player ID on levels
    -use more IDs 
    -virtual function for actor types
    -edit rendering/list selection of physics and collision to only be a slmall amount of tiles
    -fix bullets ending at the end of where you clicked instead of ending immediatly passing where you clicked.  
        reference is the tail of the sprite not the head

    CURRENT:
    -need to have animations store the collision rect and offsets.
    -creating a new player on player death
    

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



    Casey questions:

    original:
    int val;
    if (somthing is true)
    {
        val = 5;
    }

    solution:
    int val;
    if (somthing is true)
    {
        val = 5;
    }
    else
    {
        val = 0;
    }

    my solution:
    int val = 0;
    if (somthing is true)
    {
        val = 5;
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

    DebugPrint("testing please work");

    //Window and Program Setups:
    std::unordered_map<SDL_Keycode, double> keyBoardEvents;
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
		portal.GetAnimation(ActorState::idle)->fps = 30.0f;
		portal.colOffset.x = 0.125f;
		portal.colOffset.y = 0.25f;
		portal.colRect = { { 85, spriteHeight - 299 }, { 229, spriteHeight - 19 } };
		portal.scaledWidth = 32;
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

            case SDL_KEYDOWN:
                if (keyBoardEvents[SDLEvent.key.keysym.sym] == 0)
                    keyBoardEvents[SDLEvent.key.keysym.sym] = totalTime;
                break;
            case SDL_KEYUP:
                keyBoardEvents[SDLEvent.key.keysym.sym] = 0;
                break;

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


        //Keyboard Control:
        if (player != nullptr)
        {
            player->acceleration.x = 0;
            if (keyBoardEvents[SDLK_w] == totalTime || keyBoardEvents[SDLK_SPACE] == totalTime || keyBoardEvents[SDLK_UP] == totalTime)
            {
                if (player->jumpCount > 0)
                {
                    player->velocity.y = 20.0f;
                    player->jumpCount -= 1;
                    PlayAnimation(player, ActorState::jump);
                }
            }
            bool left = keyBoardEvents[SDLK_a] || keyBoardEvents[SDLK_LEFT];
            bool right = keyBoardEvents[SDLK_d] || keyBoardEvents[SDLK_RIGHT];
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
        
        if (keyBoardEvents[SDLK_1] == totalTime)
            debugList[DebugOptions::playerCollision] = !debugList[DebugOptions::playerCollision];
        if (keyBoardEvents[SDLK_2] == totalTime)
            debugList[DebugOptions::blockCollision] = !debugList[DebugOptions::blockCollision];
        if (keyBoardEvents[SDLK_3] == totalTime)
            debugList[DebugOptions::clickLocation] = !debugList[DebugOptions::clickLocation];
        if (keyBoardEvents[SDLK_4] == totalTime)
            debugList[DebugOptions::paintMethod] = !debugList[DebugOptions::paintMethod];
        if (keyBoardEvents[SDLK_5] == totalTime)
            debugList[DebugOptions::editBlocks] = !debugList[DebugOptions::editBlocks];
        if (keyBoardEvents[SDLK_0] == totalTime)
            SaveLevel(&cacheLevel, *player);
        if (keyBoardEvents[SDLK_9] == totalTime)
            LoadLevel(&cacheLevel, *player);

		if (levelChangePortal != nullptr && keyBoardEvents[SDLK_w] == totalTime)
		{
			//remove player from new level, load player from old level, delete player from old level.
			Level* oldLevel = currentLevel;
			currentLevel = GetLevel(levelChangePortal->levelPointer);

			std::erase_if(currentLevel->actors, [](Actor* actor)
			{
				if (actor->actorType == ActorType::player)
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
				return actor->actorType == ActorType::player;
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
                if (currentLevel->actors[i]->actorType == ActorType::enemy)
                {
                    screenFlash = CollisionWithActor(*player, *currentLevel->actors[i], *currentLevel, float(totalTime));
                }
                else if (currentLevel->actors[i]->actorType == ActorType::portal)
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

						DebugRectRender(blockRect, lightRed);
					}
				}

			}
        }
        if (debugList[DebugOptions::clickLocation])
        {
            DebugRectRender(clickRect, transGreen);
        }
        RenderActors();
        RenderLaser();
        
        DrawText(textSheet, Green, std::to_string(1 / deltaTime), 1.0f, { 0, 0 }, UIX::left, UIY::top);
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

            //DebugRectRender({ {0,0}, {float(windowInfo.width), float(windowInfo.height)} }, White);
        }
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