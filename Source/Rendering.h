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
    Console,
    Debug,
    PostProcess,
};

enum class CoordinateSpace
{
    UI,
    World,
};

enum class ShaderProgram
{
    Sprite,
    ChromaticAberration,
    Count,
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
    bool flippage;
    Vector rotationPoint;
    int32 width;
    int32 height;
};

struct RenderInformation
{
    RenderType renderType;
    Rectangle sRect = {};
    Rectangle dRect = {};
    Rectangle scissor = {};
    RenderPrio prio;
    ShaderProgram shader = ShaderProgram::Sprite;
    uint32 prioIndex;
    Color color = { 1.0f, 1.0f, 1.0f, 1.0f };
    CoordinateSpace coordSpace;
    int32 vertexIndex;
    int32 vertexLength;
    TextureRenderUnion texture; // Union?
};
struct Level;

struct Camera {
    Vector position = {};
    Vector size = {};
    Level* level = nullptr;
};

extern std::unordered_map<std::string, Sprite*> g_sprites;
extern std::unordered_map<std::string, FontSprite*> g_fonts;
extern Camera g_camera;
extern WindowInfo g_windowInfo;

struct Animation;
struct Actor;
struct Block;
class TileMap;

void PushScissor(Rectangle scissor);
void PopScissor();

void AddTextureToRender(Rectangle sRect, Rectangle dRect, RenderPrio priority,
    Sprite* sprite, Color colorMod, float rotation,
    Vector rotationPoint, bool flippage, CoordinateSpace coordSpace);
void InitializeOpenGL();
void AddRectToRender(RenderType type, Rectangle rect, Color color, RenderPrio prio, CoordinateSpace coordSpace);
void AddRectToRender(Rectangle rect, Color color, RenderPrio prio, CoordinateSpace coordSpace);
void RenderDrawCalls(float dt);
Sprite* CreateSprite(const char* name);
FontSprite* CreateFont(const std::string& name, int32 charSize, int32 actualCharWidth, int32 charPerRow);
//difference between the player and the center of the screen
void BackgroundRender(Sprite* sprite, Camera* camera);
void SpriteMapRender(Sprite* sprite, int32 i, int32 itemSize, int32 xCharSize, Vector loc);
void SpriteMapRender(Sprite* sprite, const Block& block);
void DrawText(FontSprite* fontSprite, Color c, const std::string& text, 
              float size, VectorInt loc, UIX XLayout, UIY YLayout, RenderPrio prio = RenderPrio::UI);
//camera space
bool pointRectangleCollision(VectorInt point, Rectangle rect);
bool SDLPointRectangleCollision(VectorInt point, Rectangle rect);
//top left 0,0
bool DrawButton(FontSprite* textSprite, const std::string& text, VectorInt loc, 
                UIX XLayout, UIY YLayout, Color BC, Color TC, 
                VectorInt mouseLoc, bool mousePressed, bool buttonSound = true);
Sprite* GetSpriteFromAnimation(Actor* actor);
void RenderBlocks(TileMap* blocks);
void RenderMovingPlatforms(Level* level);
void RenderLaser();
void RenderActor(Actor* actor, float rotation);
