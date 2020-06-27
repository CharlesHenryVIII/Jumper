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
    -portals = green;
    -levels
    -fix bullets ending at the end of where you clicked instead of ending immediatly passing where you clicked.  
        reference is the tail of the sprite not the head

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


//topLeft is origin
//ActorID CreatePlayer(/*Sprite* playerSprite, */Rectangle_Int rect, float scaledWidth, double totalTime)
//{
//    Player* player = new Player();
//    //player->sprite = playerSprite;
//    player->colOffset.x = 0.2f;
//    player->colOffset.y = 0.3f;
//    player->colRect = rect;
//    player->scaledWidth = scaledWidth;
//    player->damage = 100;
//    player->inUse = true;
//    player->lastAnimationTime = totalTime;
//    AttachAnimation(player);
//    actorList.push_back(player);
//    return player->id;
//}


int main(int argc, char* argv[])
{

    DebugPrint("testing please work");

    //Window and Program Setups:
    std::unordered_map<SDL_Keycode, double> keyBoardEvents;
    MouseButtonState mouseButtonState = {};
    SDL_MouseMotionEvent mouseMotionEvent = {};
    bool running = true;
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
    InstatiateActorAnimations("Dino");
    InstatiateActorAnimations("HeadMinion");
    InstatiateActorAnimations("Bullet");

    Sprite* spriteMap = CreateSprite("SpriteMap.png", SDL_BLENDMODE_BLEND);
    FontSprite* textSheet = CreateFont("Text.png", SDL_BLENDMODE_BLEND, 32, 20, 16);
    Sprite* background = CreateSprite("Background.png", SDL_BLENDMODE_BLEND);
    

    //Player Creation
    float playerAccelerationAmount = 50;
    //ActorID playerID = CreateActor(ActorType::player, ActorType::none, totalTime);
    //Actor* actorPlayer = FindActor(playerID);
    //actorPlayer->jump->fps = 30.0f;

    //Level instantiations
    JsonStruct(totalTime, nullptr);//.filename = "DefaultLevel.PNG";
    currentLevel = levels["Default"];

    //ActorID playerID = CreateActor(ActorType::player, ActorType::none, totalTime);
    Actor* actorPlayer = FindActor(ActorType::player, &currentLevel->actors);
    actorPlayer->jump->fps = 30.0f;

    //add enemies to current level (temporoary,  add to each level's metadata)
    //actorList.push_back(FindActor(CreateActor(ActorType::enemy, ActorType::none, totalTime)));

    //currentLevel.enemyList.push_back(enemy);
    //LoadLevel(&currentLevel, *(Player*)FindActor(playerID));
    
    Level cacheLevel = {};
    cacheLevel.filename = "Level.PNG";

    currentLevel->blocks.UpdateAllBlocks();

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
        if (FindActor(ActorType::player, &currentLevel->actors) == nullptr)
        {
            DebugPrint("player not found");
            //running = false;
            //playerID = CreateActor(ActorType::player, ActorType::none, totalTime);
            //LoadLevel(currentLevel, *(Player*)FindActor(playerID));
        }
            
        Player* player = (Player*)FindActor(ActorType::player, &currentLevel->actors);


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
                    player->jump->index = 0;
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
                    CreateLaser(player, mouseLocationTranslated, paintType);
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
                    screenFlash = CollisionWithEnemy(*player, *currentLevel->actors[i], float(totalTime));
            }
        }

        for (int i = 0; i < currentLevel->actors.size(); i++)
            currentLevel->actors[i]->UpdateHealth(totalTime);



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

        for (auto& block : *currentLevel->blocks.blockList())
        {
            if (block.second.tileType != TileType::invalid)
            {
                SpriteMapRender(spriteMap, block.second);

                if (debugList[DebugOptions::blockCollision] && block.second.tileType != TileType::invalid)
                {
                    Rectangle blockRect;
                    blockRect.bottomLeft = block.second.location;
                    blockRect.topRight = { block.second.location.x + 1 , block.second.location.y + 1 };

                    DebugRectRender(blockRect, lightRed);
                }
            }
        }
        if (debugList[DebugOptions::clickLocation])
        {
            DebugRectRender(clickRect, transGreen);
        }
        RenderActors(totalTime);
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
            if (!p->inUse/* && (p->GetActorType() != ActorType::player)*/)
            {
                delete p;
                return true;
            }
            return false;
        });
    }
    
    return 0;
}