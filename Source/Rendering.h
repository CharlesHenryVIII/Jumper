#pragma once
#include "Math.h"
#include "SDL.h"
//#include "Entity.h"
#include <string>

struct SDL_Texture;

struct WindowInfo
{
    int32 left;
    int32 top;
    int32 width;
    int32 height;
    SDL_Window* SDLWindow;
    SDL_Renderer* renderer;
};

WindowInfo& GetWindowInfo();
void CreateWindow();


enum class UIY
{
    top,
    mid,
    bot,
};


enum class UIX
{
    left,
    mid,
    right,
};

struct Sprite
{
    SDL_Texture* texture = nullptr;
    int32 width = 0;
    int32 height = 0;
};


struct FontSprite
{
    Sprite* sprite;
    int32 xCharSize;
    int32 charSize;
    int32 charPerRow;
};


enum class RectRenderType
{
    Outline,
    Fill
};

enum class RenderPrio
{
    Background,
    Sprites,
    foreground,
    UI,
    Debug,
};

struct RenderInformation
{
    RectRenderType renderType;
    SDL_Rect rect;
    SDL_Color color;
    RenderPrio prio;
};

struct Camera {
    Vector position;
};


struct Animation;
struct Actor;
struct Block;


void AddRectToRender(RectRenderType type, Rectangle rect, SDL_Color color, RenderPrio prio);
void AddRectToRender(Rectangle rect, SDL_Color color, RenderPrio prio);
void RenderDrawCalls();
Sprite* CreateSprite(const char* name, SDL_BlendMode blendMode);
FontSprite* CreateFont(const char* name, SDL_BlendMode blendMode, int32 charSize, int32 xCharSize, int32 charPerRow);
SDL_Rect CameraOffset(Vector gameLocation, Vector gameSize);
//difference between the player and the center of the screen
VectorInt CameraToPixelCoord(VectorInt input);
void BackgroundRender(Sprite* sprite, Camera* camera);
void SpriteMapRender(Sprite* sprite, int32 i, int32 itemSize, int32 xCharSize, Vector loc);
void SpriteMapRender(Sprite* sprite, const Block& block);
void DrawText(FontSprite* fontSprite, SDL_Color c, const std::string& text, float size, VectorInt loc, UIX XLayout, UIY YLayout);
SDL_Rect RectangleToSDL(Rectangle rect);
//bottom left and top right
Rectangle SDLToRectangle(SDL_Rect rect);
//camera space
bool pointRectangleCollision(VectorInt point, Rectangle rect);
bool SDLPointRectangleCollision(VectorInt point, Rectangle rect);
//top left 0,0
bool DrawButton(FontSprite* textSprite, const std::string& text, VectorInt loc, UIX XLayout, UIY YLayout, SDL_Color BC, SDL_Color TC);
Sprite* GetSpriteFromAnimation(Actor* actor);
void RenderLaser();
void IndexIncrimentor(Animation& anime, bool stayOnLastFrame, Actor* actor, double totalTime);
void RenderActor(Actor* actor, float rotation);
void GameSpaceRectRender(Rectangle rect, SDL_Color color);

extern Camera camera;
extern WindowInfo windowInfo;