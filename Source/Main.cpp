#include <SDL.h>

#include <stdio.h>
#include "SDL.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <cstdint>
#include <vector>

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
    int32 height = 1280;
    int32 width = 720;
    SDL_Window* SDLWindow;
    SDL_Renderer* renderer;
} windowInfo = { 200, 200, 1280, 720 };

struct Sprite {
    SDL_Texture* texture;
    int32 width;
    int32 height;
    int32 x = 100;
    int32 y = 100;
};

struct Vector {
    float x = 0;
    float y = 0;
};

struct Player {
    Sprite sprite;
    Vector position;
    Vector velocity;
    Vector acceleration;

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
    return { testTexture, textureWidth, textureHeight, 100, 100 };
}

void Movement(Player player)
{

}

void startup()
{

}

int main(int argc, char* argv[])
{

    std::vector<SDL_KeyboardEvent> keyBoardEvents;

    bool running = true;
    SDL_Event SDLEvent;

    //WindowInfo windowInfo = { 200, 200, 1280, 720 };
    SDL_Window* SDLWindow = SDL_CreateWindow("Jumper_beta", windowInfo.left, windowInfo.top, windowInfo.height, windowInfo.width, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(SDLWindow, -1, SDL_RENDERER_ACCELERATED || SDL_RENDERER_TARGETTEXTURE);
    
    Sprite playerSprite = CreateSprite(renderer, "Player.png", SDL_BLENDMODE_BLEND);

    while (running)
    {
        //Event Queing and handling:
        while (SDL_PollEvent(&SDLEvent))
        {
            switch (SDLEvent.type)
            {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    keyBoardEvents.push_back(SDLEvent.key); //keyboardEvent
                    break;
                case SDL_KEYUP:
                    //keyBoardEvents.erase(keyBoardEvents.); //SDLEvent.key
                    break;
            }
        }

        //Keyboard Control:
        for (uint16 i = 0; keyBoardEvents.size() > 0; i++)
        {
            if (keyBoardEvents[i].keysym.sym == SDLK_w || keyBoardEvents[i].keysym.sym == SDLK_SPACE || keyBoardEvents[i].keysym.sym == SDLK_UP)
                break; 
            else if (keyBoardEvents[i].keysym.sym == SDLK_s || keyBoardEvents[i].keysym.sym == SDLK_DOWN)
                break;
            else if (keyBoardEvents[i].keysym.sym == SDLK_a || keyBoardEvents[i].keysym.sym == SDLK_LEFT)
                break;
            else if (keyBoardEvents[i].keysym.sym == SDLK_d || keyBoardEvents[i].keysym.sym == SDLK_RIGHT)
                break;
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        SDL_Rect tempRect = { playerSprite.x, playerSprite.y, playerSprite.width, playerSprite.height };
        SDL_RenderCopyEx(renderer, playerSprite.texture, NULL, &tempRect, 0, NULL, SDL_FLIP_NONE);

        SDL_RenderPresent(renderer);
    }
    
    return 0;
}