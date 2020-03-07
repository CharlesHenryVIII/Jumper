#include <SDL.h>

#include <stdio.h>
#include "SDL.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <cstdint>

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

struct Window
{
    int left;
    int top;
    int height = 1280;
    int width = 720;
};

int main(int argc, char* argv[])
{
    bool running = true;
    SDL_Event SDLEvent;

    Window windowInfo = { 200, 200, 1280, 720 };
    SDL_Window* SDLWindow = SDL_CreateWindow("Jumper_beta", windowInfo.left, windowInfo.top, windowInfo.height, windowInfo.width, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(SDLWindow, -1, SDL_RENDERER_ACCELERATED || SDL_RENDERER_TARGETTEXTURE);

    SDL_Texture* testTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB4444, SDL_TEXTUREACCESS_TARGET, windowInfo.width, windowInfo.height);
    SDL_Rect testRect = { windowInfo.left, windowInfo.top, windowInfo.width, windowInfo.height };

    unsigned char* image = stbi_load("TestingImage.png", &windowInfo.width, &windowInfo.height, , 4);


    while (running)
    {
        while (SDL_PollEvent(&SDLEvent))
        {
            switch (SDLEvent.type)
            {
            case SDL_QUIT:
                running = false;
                break;
            }
        }

        SDL_RenderCopyEx(renderer, testTexture, &testRect, NULL, 0, NULL, SDL_FLIP_NONE);



        SDL_RenderPresent(renderer);
    }
    
    return 0;
}