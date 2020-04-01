#include <SDL.h>

#include <stdio.h>
#include "SDL.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <iostream>
#include <cstdint>
#include <vector>
#include <unordered_map>


//impliment square collider
//look into casey muratori's path walk implimentation for the collider.
//cave story's 3 to 4 point collider method.
//draw colliders using SD

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;


int32 blockSize = 32;


struct WindowInfo
{
    int32 left;
    int32 top;
    int32 width;
    int32 height;
    SDL_Window* SDLWindow;
    SDL_Renderer* renderer;
} windowInfo = { 200, 200, 1280, 720 };


struct Sprite {
    SDL_Texture* texture;
    int32 width;
    int32 height;
};


struct Vector {
    float x = 0;
    float y = 0;
};


struct VectorInt {
    int x = 0;
    int y = 0;
};


struct Player {
    Sprite sprite;
    Vector position;
    Vector velocity = {};
    Vector acceleration;
    Vector collisionSize;
    int jumpCount = 2;
};


enum class TileType {
    invalid,
    grass
};


struct Block {
    Vector location;
    TileType tileType;

    Vector PixelPosition()
    {
        return { location.x * blockSize, location.y * blockSize };
    }

};


std::unordered_map<uint64, Block> blocks2;


struct Rectangle {
    Vector bottomLeft;
    Vector topRight;
    
    float Width()
    {
        return topRight.x - bottomLeft.x;
    }

    float Height()
    {
        return topRight.y - bottomLeft.y;
    }
};


enum class CollisionDirection {
    right,
    left,
    top,
    bottom
};


Sprite CreateSprite(SDL_Renderer* renderer, const char* name, SDL_BlendMode blendMode)
{
    int32 textureHeight, textureWidth, colorChannels;

    unsigned char* image = stbi_load(name, &textureWidth, &textureHeight, &colorChannels, STBI_rgb_alpha);

    SDL_Texture* testTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, textureWidth, textureHeight);

    SDL_SetTextureBlendMode(testTexture, blendMode);

    uint32 stride = STBI_rgb_alpha * textureWidth;

    void* pixels;
    int32 pitch;
    SDL_LockTexture(testTexture, NULL, &pixels, &pitch);

    for (int32 y = 0; y < textureHeight; y++)
    {
        memcpy((uint8*)pixels + (size_t(y) * pitch), image + (size_t(y) * stride), stride);
    }

    stbi_image_free(image);
    SDL_UnlockTexture(testTexture);

    Sprite result;
    result.texture = testTexture;
    result.height = textureHeight;
    result.width = textureWidth;
    return result;
}


uint64 HashingFunction(int32 x, int32 y)
{
    return (uint64(y) << 32 | uint64(x));
}


bool CheckForBlock(Vector input)
{
    uint32 x = uint32(input.x + 0.5f) / blockSize;
    uint32 y = uint32(input.y + 0.5f) / blockSize;

    return blocks2[HashingFunction(x, y)].tileType != TileType::invalid;
}


Vector GetBlock(Vector input)
{
    float x = float(uint32(input.x + 0.5f)) / blockSize;
    float y = float(uint32(input.y + 0.5f)) / blockSize;

    return { x, y };
}


void SpriteMapRender(Sprite sprite, TileType tile, Vector position)
{
    int32 blocksPerRow = 16;
    int32 x = (uint32(tile) - 1) % blocksPerRow * blockSize;
    int32 y = (uint32(tile) - 1) / blocksPerRow * blockSize;

    SDL_Rect blockRect = { x, y, blockSize, blockSize };
    SDL_Rect DestRect = {int(position.x), int(windowInfo.height - position.y - blockSize), blockSize, blockSize};
    SDL_RenderCopyEx(windowInfo.renderer, sprite.texture, &blockRect, &DestRect, 0, NULL, SDL_FLIP_NONE);
}


void CollisionSystemCall(Player* player, CollisionDirection collisionDir)
{
    Vector referenceBlock = { };
    //float referenceBlockCoordinate = 0;

    Vector playerCollisionPoint1 = {};
    Vector playerCollisionPoint2 = {};
    Vector playerCollisionPoint3 = {};
    Vector playerCollisionPoint4 = {};

    float playerCoordinate = {};

    Rectangle xCollisionBox;
    xCollisionBox.bottomLeft = { player->position.x, player->position.y + (0.1f * player->sprite.height) };
    xCollisionBox.topRight = {player->position.x + player->sprite.width, player->position.y + 0.9f * player->sprite.height };

    Rectangle yCollisionBox;
    yCollisionBox.bottomLeft = { player->position.x + (0.1f * player->sprite.width), player->position.y };
    yCollisionBox.topRight = { player->position.x + 0.9f * player->sprite.width, player->position.y + player->sprite.height };

    if (collisionDir == CollisionDirection::right)
    {
        //playerCollisionPoint1 = player->rightCollisionBox[2];//right side of square
        //playerCollisionPoint2 = player->rightCollisionBox[3];
    }
    else if (collisionDir == CollisionDirection::left)
    {
        //playerCollisionPoint1 = player->leftCollisionBox[0];//left side of square
        //playerCollisionPoint2 = player->leftCollisionBox[1];
    }
    else if (collisionDir == CollisionDirection::top)
    {
        //playerCollisionPoint1 = player->topCollisionBox[1];//top of square
        //playerCollisionPoint2 = player->topCollisionBox[2];
    }
    else if (collisionDir == CollisionDirection::bottom)
    {
        //playerCollisionPoint1 = player->bottomCollisionBox[0];//bottom of square
        //playerCollisionPoint2 = player->bottomCollisionBox[3];
    }

    //go over void Char*** = &argv
    for (auto& block : blocks2)
    {
        if (block.second.location.x * blockSize > xCollisionBox.bottomLeft.x && block.second.location.x * blockSize < xCollisionBox.topRight.x)
        {
            //float bottom = xCollisionBox.bottomLeft.y - blockSize * 0.5f; 
            //float top = xCollisionBox.topRight.y + blockSize * 0.5f;
            //float blockCenter = block.second.location.y * blockSize + blockSize * 0.5f;
            //if (blockCenter > bottom && blockCenter < top)
            //{
            //    player->position.x = block.second.location.x - player->sprite.width;
            //}

            if ((block.second.location.y + 1) * blockSize > xCollisionBox.bottomLeft.y && block.second.location.y * blockSize < xCollisionBox.topRight.y)
            {
                player->position.x = block.second.location.x * blockSize - player->sprite.width;
            }
        }
    }
}


int main(int argc, char* argv[])
{
    //Window and Program Setups:
    std::unordered_map<SDL_Keycode, double> keyBoardEvents;
    bool running = true;
    SDL_Event SDLEvent;

    windowInfo.SDLWindow = SDL_CreateWindow("Jumper", windowInfo.left, windowInfo.top, windowInfo.width, windowInfo.height, 0);
    windowInfo.renderer = SDL_CreateRenderer(windowInfo.SDLWindow, -1, SDL_RENDERER_ACCELERATED || SDL_RENDERER_TARGETTEXTURE);

    double freq = double(SDL_GetPerformanceFrequency()); //HZ
    double totalTime = SDL_GetPerformanceCounter() / freq; //sec
    double previousTime = totalTime;


    //Sprite Creation
    Sprite playerSprite = CreateSprite(windowInfo.renderer, "Player.png", SDL_BLENDMODE_BLEND);
    Sprite minecraftSprite = CreateSprite(windowInfo.renderer, "TileMap.png", SDL_BLENDMODE_BLEND);


    //Player Creation
    Player player;
    player.sprite = playerSprite;
    player.position = { 3.0f*blockSize, 10.0f * blockSize }; //bottom left
    player.collisionSize = { 32, 64 }; 

    
    //block creation
    std::vector<Block> blocks = {
        { {10, 2}, TileType::grass },
        { {12, 2}, TileType::grass },
        { {12, 3}, TileType::grass },
        { {15, 3}, TileType::grass },
        { {18, 4}, TileType::grass }
    };
    for (auto& block : blocks)
        blocks2[HashingFunction(int32(block.location.x), int32(block.location.y))] = block;

    for (int32 x = 0; x * blockSize < windowInfo.width; x++)
        blocks2[HashingFunction(x, 0)] = { { float(x), 0 }, TileType::grass };


    //Start Of Running Program
    while (running)
    {

        //Setting Timer
        totalTime = SDL_GetPerformanceCounter() / freq;
        float deltaTime = float(totalTime - previousTime);
        previousTime = totalTime;


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
            }
        }


        //Keyboard Control:
        player.velocity.x = 0;
        for (uint16 i = 0; i < keyBoardEvents.size(); i++)
        {
            if (keyBoardEvents[SDLK_w] == totalTime || keyBoardEvents[SDLK_SPACE] == totalTime || keyBoardEvents[SDLK_UP] == totalTime)
            {
                if (player.jumpCount > 0)
                {
                    player.velocity.y += 20 * blockSize;
                    player.jumpCount -= 1;
                    break;
                }
            }
            //else if (keyBoardEvents[SDLK_s] == totalTime || keyBoardEvents[SDLK_DOWN] == totalTime)
            //    player.velocity.x=0;
            else if (keyBoardEvents[SDLK_a] || keyBoardEvents[SDLK_LEFT])
                player.velocity.x -= 2 * blockSize;
            else if (keyBoardEvents[SDLK_d] || keyBoardEvents[SDLK_RIGHT])
                player.velocity.x += 2 * blockSize;
        }


        //update x coordinate:
        player.position.x += player.velocity.x * deltaTime;

        if (player.position.x < 0)
            player.position.x = 0;
        else if (player.position.x > windowInfo.width)
            player.position.x = float(windowInfo.width);


        //update y coordinate based on gravity and bounding box:
        float gravity = float(-50 * blockSize);

        player.velocity.y += gravity * deltaTime; //v = v0 + at
        player.position.y += player.velocity.y * deltaTime + 0.5f * gravity * deltaTime * deltaTime; //y = y0 + vt + .5at^2

        if (player.position.y > windowInfo.height)
            player.position.y = float(windowInfo.height);
        else if (player.position.y < blockSize)
        {
            player.velocity.y = 0;
            player.position.y = float(blockSize);
            player.jumpCount = 2;
        }



        //NOTES:
        //blocks coordinates are bottom left
        
        //COLLISION

        //Check player against all blocks
        CollisionSystemCall(&player);
        

        //Create Renderer:
        SDL_RenderClear(windowInfo.renderer);
        SDL_SetRenderDrawBlendMode(windowInfo.renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(windowInfo.renderer, 0, 0, 0, 255);



        //debug/testing blocks
        for (auto& block : blocks2)
            SpriteMapRender(minecraftSprite, block.second.tileType, block.second.PixelPosition());
       

        //for (int32 i = 0; i < blockPlacement.size(); i++)
        //{
        //    SpriteMapRender(minecraftSprite, TileType::grass, { float(blockSize * blockPlacement[i].x), float(windowInfo.height - blockSize * blockPlacement[i].y) });
        //}
        
        SDL_Rect tempRect = { int(player.position.x - player.sprite.width / 2), windowInfo.height - int(player.position.y) - player.sprite.height, playerSprite.width, playerSprite.height };
        SDL_RenderCopyEx(windowInfo.renderer, playerSprite.texture, NULL, &tempRect, 0, NULL, SDL_FLIP_NONE);
        

        //Present Screen
        SDL_RenderPresent(windowInfo.renderer);
    }
    
    return 0;
}