#pragma once
#include "Math.h"
#include "SDL.h"
#include "Glew/include/GL/glew.h"
#include <string>
#include <unordered_map>

struct SDL_Texture;

struct WindowInfo
{
    int32 left;
    int32 top;
    int32 width;
    int32 height;
    SDL_Window* SDLWindow;
};

WindowInfo& GetWindowInfo();
void CreateSDLWindow();
void CreateOpenGLWindow();


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
    GLuint texture;
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


enum class RenderType
{
    Texture,
    DebugOutline,
    DebugFill
};

enum class RenderPrio
{
    Background,
    Sprites,
    foreground,
    UI,
    Debug,
};

enum class CoordinateSpace
{
    UI,
    World,
};

struct Vertex
{
	float x, y;
	float u, v;
};

struct TextureRenderUnion
{

    GLuint texture;
    float rotation;
    SDL_RendererFlip flippage;
    const SDL_Point* rotationPoint;
    int32 width;
    int32 height;
};

struct RenderInformation
{
    RenderType renderType;
    SDL_Rect sRect = {};
    Rectangle dRect = {};
    RenderPrio prio;
    SDL_Color color = {};
    CoordinateSpace coordSpace;
    union
    {
        TextureRenderUnion texture;
    };
};

struct Camera {
    Vector position;
    Vector size;
};

extern std::unordered_map<std::string, Sprite*> sprites;
extern std::unordered_map<std::string, FontSprite*> fonts;
extern Camera camera;
extern WindowInfo windowInfo;

struct Animation;
struct Actor;
struct Block;


void AddTextureToRender(SDL_Rect sRect, Rectangle dRect, RenderPrio priority,
    Sprite* sprite, SDL_Color colorMod, float rotation,
    const SDL_Point* rotationPoint, SDL_RendererFlip flippage, CoordinateSpace coordSpace);
void InitializeOpenGL();
void AddRectToRender(RenderType type, Rectangle rect, SDL_Color color, RenderPrio prio, CoordinateSpace coordSpace);
void AddRectToRender(Rectangle rect, SDL_Color color, RenderPrio prio, CoordinateSpace coordSpace);
void RenderDrawCalls();
Sprite* CreateSprite(const char* name, SDL_BlendMode blendMode);
FontSprite* CreateFont(const char* name, SDL_BlendMode blendMode, int32 charSize, int32 actualCharWidth, int32 charPerRow);
SDL_Rect CameraOffset(Vector gameLocation, Vector gameSize);
//difference between the player and the center of the screen
VectorInt CameraToPixelCoord(VectorInt input);
void BackgroundRender(Sprite* sprite, Camera* camera);
void SpriteMapRender(Sprite* sprite, int32 i, int32 itemSize, int32 xCharSize, Vector loc);
void SpriteMapRender(Sprite* sprite, const Block& block);
void DrawText(FontSprite* fontSprite, SDL_Color c, const std::string& text, 
              float size, VectorInt loc, UIX XLayout, UIY YLayout);
SDL_Rect RectangleToSDL(Rectangle rect, bool yFlip);
//bottom left and top right
Rectangle SDLToRectangle(SDL_Rect rect);
//camera space
bool pointRectangleCollision(VectorInt point, Rectangle rect);
bool SDLPointRectangleCollision(VectorInt point, Rectangle rect);
//top left 0,0
bool DrawButton(FontSprite* textSprite, const std::string& text, VectorInt loc, 
                UIX XLayout, UIY YLayout, SDL_Color BC, SDL_Color TC, 
                VectorInt mouseLoc, bool mousePressed);
Sprite* GetSpriteFromAnimation(Actor* actor);
void RenderBlocks();
void RenderMovingPlatforms();
void RenderLaser();
void RenderActor(Actor* actor, float rotation);
