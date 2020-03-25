#include <SDL.h>

#include <stdio.h>
#include "SDL.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <cstdint>
#include <vector>
#include <unordered_map>

//impliment square collider
//look into casey muratori's path walk implimentation for the collider.
//cave story's 3 to 4 point collider method.

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

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

struct Player {
    Sprite sprite;
    Vector position = {100, 100};
    Vector velocity = {};
    Vector acceleration;
    int jumpCount = 2;
};

enum class TileType {
    grass,
    stone,
    dirt,
    dirtGrass,
    wood
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

void Movement(Player player)
{

}

void startup()
{

}

void SpriteMapRender(Sprite sprite, TileType tile, Vector position)
{
    int32 blockSize = 16;
    int32 blocksPerRow = 16;
    int32 x = uint32(tile) % blocksPerRow * blockSize;
    int32 y = uint32(tile) / blocksPerRow * blockSize;

    SDL_Rect blockRect = { x, y, blockSize, blockSize };
    SDL_Rect DestRect = {int(position.x), int(position.y), blockSize, blockSize};
    SDL_RenderCopyEx(windowInfo.renderer, sprite.texture, &blockRect, &DestRect, 0, NULL, SDL_FLIP_NONE);
}

int main(int argc, char* argv[])
{
    std::unordered_map<SDL_Keycode, double> keyBoardEvents;

    bool running = true;
    SDL_Event SDLEvent;

    windowInfo.SDLWindow = SDL_CreateWindow("Jumper", windowInfo.left, windowInfo.top, windowInfo.width, windowInfo.height, 0);
    windowInfo.renderer = SDL_CreateRenderer(windowInfo.SDLWindow, -1, SDL_RENDERER_ACCELERATED || SDL_RENDERER_TARGETTEXTURE);

    double freq = double(SDL_GetPerformanceFrequency()); //HZ

    double totalTime = SDL_GetPerformanceCounter() / freq; //sec
    double previousTime = totalTime;

    Sprite playerSprite = CreateSprite(windowInfo.renderer, "Player.png", SDL_BLENDMODE_BLEND);
    Sprite minecraftSprite = CreateSprite(windowInfo.renderer, "MinecraftTileMap.png", SDL_BLENDMODE_BLEND);

    float pixelsPerUnit = playerSprite.height / 2.0f; //meter

    Player player;
    player.sprite = playerSprite;
    player.position = { 100, 100 }; //bottom left

    while (running)
    {
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
        player.velocity.x = 0;
        //Keyboard Control:
        for (uint16 i = 0; i < keyBoardEvents.size(); i++)
        {
            if (keyBoardEvents[SDLK_w] == totalTime || keyBoardEvents[SDLK_SPACE] == totalTime || keyBoardEvents[SDLK_UP] == totalTime)
            {
                if (player.jumpCount > 0)
                {
                    player.velocity.y -= 20 * pixelsPerUnit;
                    player.jumpCount -= 1;
                    break;
                }
            }
            //else if (keyBoardEvents[SDLK_s] == totalTime || keyBoardEvents[SDLK_DOWN] == totalTime)
            //    player.velocity.x=0;
            else if (keyBoardEvents[SDLK_a] || keyBoardEvents[SDLK_LEFT])
                player.velocity.x -= 8 * pixelsPerUnit;
            else if (keyBoardEvents[SDLK_d] || keyBoardEvents[SDLK_RIGHT])
                player.velocity.x += 8 * pixelsPerUnit;
        }

        //update y coordinate based on gravity and bounding box:
        float gravity = 50*pixelsPerUnit;
        float futureVelocity = player.velocity.y + gravity * deltaTime;
        float futurePositionY = player.velocity.y + player.velocity.y * deltaTime + 0.5f * gravity * deltaTime * deltaTime;

        if (player.position.y < 500 || (futurePositionY < 500 && futureVelocity < 0))
        {
            player.velocity.y += gravity * deltaTime; //v = v0 + at
            player.position.y += player.velocity.y * deltaTime + 0.5f * gravity * deltaTime * deltaTime; //y = y0 + vt + .5at^2
        }
        else
        {
            player.velocity.y = 0;
            player.position.y = 500;
            player.jumpCount = 2;
        }

        //update x coordinate:
        player.position.x += player.velocity.x * deltaTime;

        if (player.position.x < 0)
            player.position.x = 0;
        else if (player.position.x > windowInfo.width)
            player.position.x = float(windowInfo.width);

        //renderer:
        SDL_RenderClear(windowInfo.renderer);
        SDL_SetRenderDrawBlendMode(windowInfo.renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(windowInfo.renderer, 0, 0, 0, 255);

        SpriteMapRender(minecraftSprite, TileType::grass, { 32, 500 });
        SpriteMapRender(minecraftSprite, TileType::stone, { 48, 500 });
        SpriteMapRender(minecraftSprite, TileType::dirt, { 64, 500 });
        SpriteMapRender(minecraftSprite, TileType::dirtGrass, { 80, 500 });
        SpriteMapRender(minecraftSprite, TileType::wood, { 96, 500 });

        SDL_Rect tempRect = { int(player.position.x - player.sprite.width / 2), int(player.position.y - player.sprite.height), playerSprite.width, playerSprite.height };
        SDL_RenderCopyEx(windowInfo.renderer, playerSprite.texture, NULL, &tempRect, 0, NULL, SDL_FLIP_NONE);

        SDL_RenderPresent(windowInfo.renderer);
    }
    
    return 0;
}