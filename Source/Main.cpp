#include <SDL.h>

#include <stdio.h>
#include "SDL.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
//#define STB_TRUETYPE_IMPLEMENTATION
//#include "stb/stb_truetype.h"

#include <iostream>
#include <string>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <cmath>


/*
TODO(choman):
    -entity system
    -camera zoom
    -sub pixel rendering
    -multithread saving/png compression
    -add layered background(s)
    -seperate collision detection function and use it for projectile detection as well
    -windows settings local grid highlight
    -add gun to change blocks aka <spring/wind, tacky(can hop off walls with it), timed explosion to get more height>
    -add momentum
    -handle multiple levels
    -moving platforms
    -enemy ai
    -power ups
    -explosives?



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


using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;


int32 blockSize = 32;



const SDL_Color Red     = { 255, 0, 0, 255 };
const SDL_Color Green   = { 0, 255, 0, 255 };
const SDL_Color Blue    = { 0, 0, 255, 255 };

const SDL_Color transRed    = { 255, 0, 0, 127 };
const SDL_Color transGreen  = { 0, 255, 0, 127 };
const SDL_Color transBlue   = { 0, 0, 255, 127 };

const SDL_Color lightRed    = { 255, 0, 0, 63 };
const SDL_Color lightGreen  = { 0, 255, 0, 63 };
const SDL_Color lightBlue   = { 0, 0, 255, 63 }; 

const SDL_Color White = { 255, 255, 255, 255};
const SDL_Color lightWhite = { 100, 100, 100, 100 };
const SDL_Color Black = { 0, 0, 0, 255 };
const SDL_Color Brown = { 130, 100, 70, 255 };  //used for dirt block
const SDL_Color Mint =  { 0, 255, 127, 255 };   //used for corner block
const SDL_Color Orange ={ 255, 127, 0, 255 };   //used for edge block
const SDL_Color Grey =  { 127, 127, 127, 255 }; //used for floating block


const uint32 CollisionNone = 0;
const uint32 CollisionTop = 1;
const uint32 CollisionBot = 2;
const uint32 CollisionRight = 4;
const uint32 CollisionLeft = 8;



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


struct FontSprite
{
    Sprite sprite;
    int32 xCharSize;
    int32 charSize;
    int32 charPerRow;
};


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


enum class enemyType
{
    head,
    melee,
    ranged,
    magic
};


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


struct Vector {
    float x = 0;
    float y = 0;
};


struct VectorInt {
    int x = 0;
    int y = 0;
};


Vector operator-(const Vector& lhs, const Vector& rhs)
{
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}


Vector operator+(const Vector& lhs, const Vector& rhs)
{
    return { lhs.x + rhs.x, lhs.y + rhs.y };
}


Vector operator*(const Vector& a, const float b)
{
    return { a.x * b,  a.y * b};
}


const Vector& operator+=(Vector& lhs, const Vector& rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    return lhs;
}

template <typename T>
T Min(T a, T b)
{
    return a < b ? a : b;
}

template <typename T>
T Max(T a, T b)
{
    return a > b ? a : b;
}

template <typename T>
T Clamp(T v, T min, T max)
{
    return Max(min, Min(max, v));
}

int BlockToPixel(float loc)
{
    return int(loc * blockSize);
}

VectorInt BlockToPixel(Vector loc)
{
    return { BlockToPixel(loc.x), BlockToPixel(loc.y) };
}


float PixelToBlock(int loc)
{
    return { float(loc) / blockSize };
}

Vector PixelToBlock(VectorInt loc)
{
    return { PixelToBlock(loc.x), PixelToBlock(loc.y) };
}





struct Actor
{
    Sprite sprite;
    Vector position;
    Vector velocity = {};
    Vector acceleration;
    Vector colOffset;
    int jumpCount = 2;
    float health = 100;
    float damage;
    bool inUse = true;
} player = {};


struct Enemy : public Actor
{
    enemyType enemyType;
};


struct Camera {
    Vector position;
} camera = {};


enum class TileType {
    invalid,
    filled
};


struct Projectile : public Actor
{
    Vector destination;
    TileType paintType;
    float rotation = 0;
};


Projectile laser = {};
std::vector<Projectile> bulletList = {};


struct Block {
    Vector location = {};
    TileType tileType = TileType::invalid;
    uint32 flags = {};

    Vector PixelPosition() const
    {
        return { location.x * blockSize, location.y * blockSize };
    }

};


struct Level
{
    std::vector<Enemy> enemyList;
    const char* filename;  //DefaultLevel.PNG;
    Block entrance;
    Block exit;
} currentLevel;


class TileMap
{
    std::unordered_map<uint64, Block> blocks;

public:
    uint64 HashingFunction(Vector loc)
    {
        int32 x = int32(floorf(loc.x));
        int32 y = int32(floorf(loc.y));

        uint64 hash = uint64(y) << 32;
        hash |= (uint64(x) & 0xffffffff);
        return hash;
    }
	//Block Coordinate System
	Block& GetBlock(Vector loc)
	{
        loc.x = floorf(loc.x);
        loc.y = floorf(loc.y);
        auto& result = blocks[HashingFunction(loc)];
        result.location = loc;
        return result;
	}
	Block* TryGetBlock(Vector loc)
	{
		if (CheckForBlock(loc))
			return &GetBlock(loc);
		else
			return nullptr;
	}
	void AddBlock(Vector loc, TileType tileType)
	{
		blocks[HashingFunction(loc)] = { loc, tileType };
	}
	bool CheckForBlock(Vector loc)
	{
        auto it = blocks.find(HashingFunction(loc));
		if (it != blocks.end())
            return it->second.tileType != TileType::invalid;
        return false;
	}
    void UpdateBlock(Block* block)
    {//32 pixels per block 64 total 8x8 = 256x256
        uint32 blockFlags = 0;

        constexpr uint32 left = 1 << 0;     //1 or odd
        constexpr uint32 botLeft = 1 << 1;  //2
        constexpr uint32 bot = 1 << 2;      //4
        constexpr uint32 botRight = 1 << 3; //8
        constexpr uint32 right = 1 << 4;    //16
        constexpr uint32 top = 1 << 5;      //32


        if (CheckForBlock({ block->location.x - 1, block->location.y }))
            blockFlags += left;
        if (CheckForBlock({ block->location.x - 1, block->location.y - 1 }))
            blockFlags += botLeft;
        if (CheckForBlock({ block->location.x,     block->location.y - 1 }))
            blockFlags += bot;
        if (CheckForBlock({ block->location.x + 1, block->location.y - 1 }))
            blockFlags += botRight;
        if (CheckForBlock({ block->location.x + 1, block->location.y }))
            blockFlags += right;
        if (CheckForBlock({ block->location.x,     block->location.y + 1 }))
            blockFlags += top;

        block->flags = blockFlags;
    }
    void UpdateAllBlocks()
    {
        for (auto& block : blocks)
        {
            UpdateBlock(&GetBlock(block.second.location));
        }
    }
    //returns &blocks
    const std::unordered_map<uint64, Block>* blockList()
    {
        return &blocks;
    }
    void ClearBlocks()
    {
        blocks.clear();
    }
    void CleanBlocks()
    {
        for (auto it = blocks.begin(); it != blocks.end();)
        {
            if (it->second.tileType == TileType::invalid)
                it = blocks.erase(it);
            else
                it++;
        }
    }
};
static TileMap tileMap;


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

    bool Collision(VectorInt loc)
    {
        bool result = false;
        if (loc.y > bottomLeft.x&& loc.y < topRight.x)
            if (loc.x > bottomLeft.x&& loc.x < topRight.x)
                result = true;
        return result;
    }
};

struct MouseButtonEvent {
    Uint32 type;        /**< ::SDL_MOUSEBUTTONDOWN or ::SDL_MOUSEBUTTONUP */
    double timestamp;   /**< In milliseconds, populated using SDL_GetTicks() */
    //Uint32 windowID;  /**< The window with mouse focus, if any */
    //Uint32 which;     /**< The mouse instance id, or SDL_TOUCH_MOUSEID */
    Uint8 button;       /**< The mouse button index */
    Uint8 state;        /**< ::SDL_PRESSED or ::SDL_RELEASED */
    //Uint8 clicks;     /**< 1 for single-click, 2 for double-click, etc. */
    VectorInt location;
};


enum class CollisionDirection {
    right,
    left,
    top,
    bottom
};


Sprite CreateSprite(const char* name, SDL_BlendMode blendMode)
{
    int32 textureHeight, textureWidth, colorChannels;

    unsigned char* image = stbi_load(name, &textureWidth, &textureHeight, &colorChannels, STBI_rgb_alpha);

    SDL_Texture* testTexture = SDL_CreateTexture(windowInfo.renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, textureWidth, textureHeight);

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


FontSprite CreateFont(const char* name, SDL_BlendMode blendMode, int32 charSize, int32 xCharSize, int32 charPerRow)
{
    FontSprite result = {};
    result.sprite = CreateSprite(name, blendMode);
    result.charSize = charSize;
    result.xCharSize = xCharSize;
    result.charPerRow = charPerRow;
    return result;
}



TileType CheckColor(SDL_Color color)
{
    if (color.r == Brown.r && color.g == Brown.g && color.b == Brown.b && color.a == Brown.a)
        return TileType::filled;
    else
        return TileType::invalid;
}


SDL_Color GetTileMapColor(const Block& block)
{
    if (block.tileType == TileType::filled)
        return Brown;
    else
        return {};

}


void SaveLevel(Level* level)
{
    //Setup
    stbi_flip_vertically_on_write(true);
    tileMap.CleanBlocks();

    //Finding the edges of the "picture"/level
    int32 left  = int32(player.position.x);
    int32 right = int32(player.position.x);
    int32 top   = int32(player.position.y);
    int32 bot   = int32(player.position.y);

    for (auto& block : *tileMap.blockList())
    {
        if (block.second.location.x < left)
            left = int32(block.second.location.x);
        if (block.second.location.x > right)
            right = int32(block.second.location.x);
        
        if (block.second.location.y < bot)
            bot = int32(block.second.location.y);
        if (block.second.location.y > top)
            top = int32(block.second.location.y);
    }

    int32 width = right - left + 1;
    int32 height = top - bot + 1;
    
    //Getting memory to write to
    std::vector<SDL_Color> memBuff;
    memBuff.resize(width * height, {});

    //writing to memory
    SDL_Color tileColor = {};
    for (auto& block : *tileMap.blockList())
    {
        if (block.second.tileType == TileType::invalid)
            continue;

        memBuff[size_t((block.second.location.x - left) + ((block.second.location.y - bot) * size_t(width)))] = GetTileMapColor(block.second);
    }

    //TODO(choman):  When new entity system is done change this to include other Actors
    int32 playerBlockPositionX = int32(player.position.x);
    int32 playerBlockPositionY = int32(player.position.y);
    memBuff[size_t((playerBlockPositionX - left) + ((playerBlockPositionY - bot) * size_t(width)))] = Blue;

    //Saving written memory to file
    int stride_in_bytes = 4 * width;
    int colorChannels = 4;
    stbi_write_png(level->filename, width, height, colorChannels, memBuff.data(), stride_in_bytes);
}


void LoadLevel(Level* level)
{
    tileMap.ClearBlocks();

    stbi_set_flip_vertically_on_load(true);

    int32 textureHeight, textureWidth, colorChannels;
    SDL_Color* image = (SDL_Color*)stbi_load(level->filename, &textureWidth, &textureHeight, &colorChannels, STBI_rgb_alpha);

    for (int32 y = 0; y < textureHeight; y++)
    {
        for (int32 x = 0; x < textureWidth; x++)
        {
            uint32 index = size_t(x) + size_t(y * textureWidth);

            TileType tileColor = CheckColor(image[index]);
            if (image[index].r == Blue.r && image[index].g == Blue.g && image[index].b == Blue.b && image[index].a == Blue.a)
                player.position = { float(x + 0.5f), float(y + 0.5f) };
            else if (tileColor != TileType::invalid)     
                tileMap.AddBlock( { float(x), float(y) }, tileColor );
        }
    }
    tileMap.UpdateAllBlocks();
    stbi_image_free(image);
}


void UpdateAllNeighbors(Block* block)
{
    for (int32 y = -1; y <= 1; y++)
    {
        for (int32 x = -1; x <= 1; x++)
        {
            if (tileMap.TryGetBlock({ float(block->location.x + x), float(block->location.y + y) }) != nullptr)
                tileMap.UpdateBlock(&tileMap.GetBlock({ float(block->location.x + x), float(block->location.y + y) }));
        }
    }
}


void SurroundBlockUpdate(Block* block, bool updateTop)
{
    //update block below
    if (tileMap.CheckForBlock({ block->location.x, block->location.y - 1 }))
        tileMap.UpdateBlock(&tileMap.GetBlock({ block->location.x, block->location.y - 1 }));
    //update block to the left
    if (tileMap.CheckForBlock({ block->location.x - 1, block->location.y }))
        tileMap.UpdateBlock(&tileMap.GetBlock({ block->location.x - 1, block->location.y }));
    //update block to the right
    if (tileMap.CheckForBlock({ block->location.x + 1, block->location.y }))
        tileMap.UpdateBlock(&tileMap.GetBlock({ block->location.x + 1, block->location.y }));
    if (updateTop && tileMap.CheckForBlock({ block->location.x, block->location.y - 1 }))
        tileMap.UpdateBlock(&tileMap.GetBlock({ block->location.x, block->location.y + 1 }));
}


void ClickUpdate(Block* block, bool updateTop)
{
    tileMap.UpdateBlock(block);
    SurroundBlockUpdate(block, updateTop);
}


SDL_Rect CameraOffset(Vector gameLocation, Vector gameSize)
{
    VectorInt location = BlockToPixel(gameLocation);
    VectorInt size = BlockToPixel(gameSize);
    int xOffset = BlockToPixel(camera.position.x) - windowInfo.width / 2;
    int yOffset = BlockToPixel(camera.position.y) - windowInfo.height / 2;

    SDL_Rect result;
    result.x = location.x - xOffset;
    result.y = windowInfo.height - (location.y - yOffset) - size.y;
    result.w = size.x;
    result.h = size.y;

    return result;
}


//difference between the player and the center of the screen
VectorInt CameraToPixelCoord(VectorInt input)
{
    int xOffset = BlockToPixel(camera.position.x) - windowInfo.width / 2;
    int yOffset = BlockToPixel(camera.position.y) - windowInfo.height / 2;
    return { input.x + xOffset, windowInfo.height - input.y + yOffset};
}


void BackgroundRender(Sprite sprite, Actor* player)
{
    //HARDCODING FOR NOW UNTIL WINDOW SIZE CHANGES
    assert(windowInfo.width == 1280 && windowInfo.height == 720);

    SDL_Rect spriteRect = {};

    //topLeft
    spriteRect.w = windowInfo.width / 2;    //360
    spriteRect.h = windowInfo.height / 2;   //640
    spriteRect.x = 0;
    spriteRect.y = sprite.height - spriteRect.h;

    
    //checking to make sure the sprite doesnt go further left than the actual sprite and go beyond the border on the right;
    if (player->position.x > 0)
    {
        int32 spriteRectXOffset = int32(player->position.x * 2);
        if (spriteRectXOffset < sprite.width - spriteRect.w)
            spriteRect.x = spriteRectXOffset;
        else
            spriteRect.x = sprite.width - spriteRect.w;
    }

    if (player->position.y > 0)
    {
        int32 spriteRectYOffset = spriteRect.h + int32(player->position.y / 2);
        if (spriteRectYOffset < sprite.height)
            spriteRect.y = sprite.height - spriteRectYOffset;
        else
            spriteRect.y = 0;
    }
    
       
    SDL_RenderCopyEx(windowInfo.renderer, sprite.texture, &spriteRect, NULL, 0, NULL, SDL_FLIP_NONE);
}


void SpriteMapRender(Sprite sprite, int32 i, int32 itemSize, int32 xCharSize, Vector loc)
{
    int32 blocksPerRow = sprite.width / itemSize;
    int32 x = uint32(i) % blocksPerRow * itemSize + (itemSize - xCharSize) / 2;
    int32 y = uint32(i) / blocksPerRow * itemSize;

    SDL_Rect blockRect = { x, y, xCharSize, itemSize };
    float itemSizeTranslatedx = PixelToBlock(xCharSize);
    float itemSizeTranslatedy = PixelToBlock(itemSize);
    SDL_Rect DestRect = CameraOffset(loc, { itemSizeTranslatedx, itemSizeTranslatedy });

    SDL_RenderCopyEx(windowInfo.renderer, sprite.texture, &blockRect, &DestRect, 0, NULL, SDL_FLIP_NONE);
}


void SpriteMapRender(Sprite sprite, Block block)
{
    SpriteMapRender(sprite, block.flags, blockSize, blockSize, block.location);
}


void DebugRectRender(Rectangle rect, SDL_Color color)
{
    SDL_SetRenderDrawColor(windowInfo.renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect2 = CameraOffset(rect.bottomLeft, { rect.Width(), rect.Height() });

    SDL_RenderDrawRect(windowInfo.renderer, &rect2);
    SDL_RenderFillRect(windowInfo.renderer, &rect2);
}


void DrawText(FontSprite fontSprite, SDL_Color c, std::string text, VectorInt loc, UIX XLayout, UIY YLayout)
{
    //ABC
    int32 CPR = fontSprite.charPerRow;

    //Upper Left corner and size
    SDL_Rect SRect = {};
    SDL_Rect DRect = {};

    SRect.w = fontSprite.xCharSize;
    SRect.h = fontSprite.charSize;
    DRect.w = SRect.w;
    DRect.h = SRect.h;

    for (int32 i = 0; i < text.size(); i++)
    {
        int32 charKey = text[i] - ' ';

        SRect.x = uint32(charKey) % CPR * SRect.h + (SRect.h - SRect.w) / 2;
        SRect.y = uint32(charKey) / CPR * SRect.h;

        DRect.x = loc.x + i * DRect.w - (int32(XLayout) * DRect.w * int32(text.size())) / 2;
        DRect.y = loc.y - (int32(YLayout) * DRect.h) / 2;

        SDL_SetTextureColorMod(fontSprite.sprite.texture, c.r, c.g, c.b);
        SDL_RenderCopyEx(windowInfo.renderer, fontSprite.sprite.texture, &SRect, &DRect, 0, NULL, SDL_FLIP_NONE);
    }
}


SDL_Rect RectangleToSDL(Rectangle rect)
{
    return { (int32)rect.bottomLeft.x, windowInfo.height - (int32)rect.topRight.y, (int32)rect.Width(), (int32)rect.Height() };
}

//bottom left and top right
Rectangle SDLToRectangle(SDL_Rect rect)
{
    return { { (float)rect.x, float(rect.y + rect.h) }, { float(rect.x + rect.w), (float)rect.y } };
}

//camera space
bool pointRectangleCollision(VectorInt point, Rectangle rect)
{
    bool result = false;
    if (point.x > rect.bottomLeft.x&& point.x < rect.topRight.x)
        if (point.y > rect.bottomLeft.y&& point.y < rect.topRight.y)
            result = true;
    return result;
}


bool SDLPointRectangleCollision(VectorInt point, Rectangle rect)
{
    bool result = false;
    if (point.x > rect.bottomLeft.x&& point.x < rect.topRight.x)
        if (point.y < rect.bottomLeft.y && point.y > rect.topRight.y)
            result = true;
    return result;
}


//top left 0,0
bool DrawButton(FontSprite textSprite, std::string text, VectorInt loc, UIX XLayout, UIY YLayout, SDL_Color BC, SDL_Color TC, MouseButtonEvent mouseButton, SDL_MouseMotionEvent mouseMotion)
{
    bool result = false;
    //Copied from DrawText()
    //TODO(choman): change this to be more fluid and remove most of the dependancies.

    SDL_Rect buttonRect = {};
    int32 widthOffset = 5;
    int32 heightOffset = 5;

    buttonRect.w = textSprite.xCharSize * int32(text.size()) + widthOffset * 2;
    buttonRect.h = (textSprite.charSize + heightOffset * 2);
    int32 xOffset = (int32(XLayout) * buttonRect.w) / 2;
    int32 yOffset = (int32(YLayout) * buttonRect.h) / 2; //top, mid, bot

    buttonRect.x = loc.x - xOffset;
    buttonRect.y = loc.y - yOffset;

    SDL_SetRenderDrawColor(windowInfo.renderer, BC.r, BC.g, BC.b, BC.a);
    Rectangle cshRect = SDLToRectangle(buttonRect);
    SDL_RenderDrawRect(windowInfo.renderer, &buttonRect);

    if (SDLPointRectangleCollision({ mouseMotion.x, mouseMotion.y }, cshRect))
    {
        SDL_SetRenderDrawColor(windowInfo.renderer, BC.r, BC.g, BC.b, BC.a / 2);
        SDL_RenderFillRect(windowInfo.renderer, &buttonRect);
    }
    if (mouseButton.state == SDL_PRESSED)
    {
        if (SDLPointRectangleCollision(mouseButton.location, cshRect))
        {
            SDL_SetRenderDrawColor(windowInfo.renderer, BC.r, BC.g, BC.b, BC.a);
            SDL_RenderFillRect(windowInfo.renderer, &buttonRect);
            result = true;
        }
    }
    DrawText(textSprite, TC, text, { buttonRect.x + buttonRect.w / 2, buttonRect.y + buttonRect.h / 2 }, UIX::mid, UIY::mid);
    return result;
}


Rectangle CollisionXOffsetToRectangle(Actor* actor)
{
    Rectangle result = {};
    result.bottomLeft = { actor->position.x + actor->colOffset.x * PixelToBlock(actor->sprite.width), actor->position.y + actor->colOffset.x * PixelToBlock(actor->sprite.height) };
    result.topRight = { actor->position.x + (1 - actor->colOffset.x) * PixelToBlock(actor->sprite.width), actor->position.y + (1 - actor->colOffset.x) * PixelToBlock(actor->sprite.height) };
    return result;
}
Rectangle CollisionYOffsetToRectangle(Actor* actor)
{
    Rectangle result = {};
    result.bottomLeft = { actor->position.x + actor->colOffset.y * PixelToBlock(actor->sprite.width), actor->position.y };
    result.topRight = { actor->position.x + (1 - actor->colOffset.y) * PixelToBlock(actor->sprite.width), actor->position.y + PixelToBlock(actor->sprite.height) };
    return result;
}


uint32 CollisionWithRect(Actor* actor, Rectangle rect)
{
    uint32 result = CollisionNone;

    Rectangle xRect = CollisionXOffsetToRectangle(actor);
    Rectangle yRect = CollisionYOffsetToRectangle(actor);

    if (debugList[DebugOptions::playerCollision])
    {
        SDL_SetRenderDrawBlendMode(windowInfo.renderer, SDL_BLENDMODE_BLEND);
        DebugRectRender(xRect, transGreen);
        DebugRectRender(yRect, transGreen);
    }

    //checking the right side of player against the left side of a block
    if (rect.bottomLeft.x > xRect.bottomLeft.x&& rect.bottomLeft.x < xRect.topRight.x)
        if ((rect.topRight.y) > xRect.bottomLeft.y&& rect.bottomLeft.y < xRect.topRight.y)
            result += CollisionRight;

    //checking the left side of player against the right side of the block
    if ((rect.topRight.x) > xRect.bottomLeft.x && (rect.topRight.x) < xRect.topRight.x)
        if ((rect.topRight.y) > xRect.bottomLeft.y&& rect.bottomLeft.y < xRect.topRight.y)
            result += CollisionLeft;


    //checking the top of player against the bottom of the block
    if ((rect.topRight.x) > yRect.bottomLeft.x&& rect.bottomLeft.x < yRect.topRight.x)
        if (rect.bottomLeft.y > yRect.bottomLeft.y&& rect.bottomLeft.y < yRect.topRight.y)
            result += CollisionTop;

    //checking the bottom of player against the top of the block
    if ((rect.topRight.x) > yRect.bottomLeft.x&& rect.bottomLeft.x < yRect.topRight.x)
        if ((rect.topRight.y) > yRect.bottomLeft.y && (rect.topRight.y) < yRect.topRight.y)
            result += CollisionBot;
    
    return result;
}


void CollisionWithBlocks(Actor* actor, bool isEnemy)
{
    for (auto& block : *tileMap.blockList())
    {
        if (block.second.tileType == TileType::invalid)
            continue;

        uint32 collisionFlags = CollisionWithRect(actor, { block.second.location, { block.second.location + Vector({ 1, 1 }) } });
        
        if (collisionFlags & CollisionRight)
        {
            actor->position.x = block.second.location.x - (1 - actor->colOffset.x) * PixelToBlock(actor->sprite.width);
            if (isEnemy)
                actor->velocity.x = -1 * actor->velocity.x;
            else
                actor->velocity.x = 0;
        }
        if (collisionFlags & CollisionLeft)
        {
            actor->position.x = (block.second.location.x + 1) - actor->colOffset.x * PixelToBlock(actor->sprite.width);
            if (isEnemy)
                actor->velocity.x = -1 * actor->velocity.x;
            else
                actor->velocity.x = 0;
        }
        if (collisionFlags & CollisionTop)
        {
            actor->position.y = block.second.location.y - PixelToBlock(actor->sprite.height);
            if (actor->velocity.y > 0)
                actor->velocity.y = 0;
        }
        if (collisionFlags & CollisionBot)
        {
            actor->position.y = block.second.location.y + 1;
            if (actor->velocity.y < 0)
                actor->velocity.y = 0;
            actor->jumpCount = 2;
        }
    }
}


bool CollisionWithEnemy(Actor* actor)
{
    bool result = false;
    for (int32 i = 0; i < currentLevel.enemyList.size(); i++)
    {
        Enemy* enemy = &currentLevel.enemyList[i];
        if (!enemy->inUse)
            continue;

        Rectangle xRect = CollisionXOffsetToRectangle(enemy);
        Rectangle yRect = CollisionYOffsetToRectangle(enemy);

        uint32 xCollisionFlags = CollisionWithRect(&player, xRect);
        uint32 yCollisionFlags = CollisionWithRect(&player, yRect);
        float knockBackAmount = 10;

        if (xCollisionFlags & CollisionRight)
        {
            //hit by enemy, knockback, take damage, screen flash
            actor->velocity = { -knockBackAmount, knockBackAmount };
            result = true;
        }
		if (xCollisionFlags & CollisionLeft)
        {
            //hit by enemy, knockback, take damage, screen flash
            actor->velocity = { knockBackAmount, knockBackAmount };
            result = true;
        }

        
		if (yCollisionFlags & CollisionTop)
        {
            //hit enemy, apply damage to enemy
            enemy->health -= actor->damage;
        }
		else if (xCollisionFlags || yCollisionFlags)
		{
			actor->health -= enemy->damage;
		}

		//hit by enemy, take damage, screen flash
		if (yCollisionFlags & CollisionBot)
			result = true;
    }
    return result;
}


void UpdateActorHealth(Actor* actor)
{
    if (actor->inUse)
    {
        Clamp(actor->health, 0.0f, 100.0f);
        if (!actor->health)
        {
            actor->inUse = false;
        }
    }
}
void UpdateEnemyHealth()
{
    for (int32 i = 0; i < currentLevel.enemyList.size(); i++)
    {
        Enemy* enemy = &currentLevel.enemyList[i];
        UpdateActorHealth(enemy);
    }
}


//void CollisionSystemCall(Actor* player, bool isEnemy)
//{
//    Rectangle xCollisionBox;
//    xCollisionBox.bottomLeft = { player->position.x + 0.2f * PixelToBlock(player->sprite.width), player->position.y + 0.2f * PixelToBlock(player->sprite.height) };
//    xCollisionBox.topRight = {player->position.x + 0.8f * PixelToBlock(player->sprite.width), player->position.y + 0.8f * PixelToBlock(player->sprite.height) };
//
//    Rectangle yCollisionBox;
//    yCollisionBox.bottomLeft = { player->position.x + 0.3f * PixelToBlock(player->sprite.width), player->position.y };
//    yCollisionBox.topRight = { player->position.x + 0.7f * PixelToBlock(player->sprite.width), player->position.y + PixelToBlock(player->sprite.height) };
//    
//    
//    if (debugList[DebugOptions::playerCollision])
//    {
//        SDL_SetRenderDrawBlendMode(windowInfo.renderer, SDL_BLENDMODE_BLEND);
//        DebugRectRender(xCollisionBox, transGreen);
//        DebugRectRender(yCollisionBox, transGreen);
//    }
//
//
//    for (auto& block : *tileMap.blockList())
//    {
//        if (block.second.tileType != TileType::invalid)
//        {
//            //checking the right side of player against the left side of a block
//            if (block.second.location.x > xCollisionBox.bottomLeft.x && block.second.location.x < xCollisionBox.topRight.x)
//            {
//                if ((block.second.location.y + 1) > xCollisionBox.bottomLeft.y && block.second.location.y < xCollisionBox.topRight.y)
//                {
//                    player->position.x = block.second.location.x - 0.8f * PixelToBlock(player->sprite.width);
//                    if (isEnemy)
//                        player->velocity.x = -1 * player->velocity.x;
//                    else
//                        player->velocity.x = 0;
//                }
//            }
//            //checking the left side of player against the right side of the block
//            if ((block.second.location.x + 1) > xCollisionBox.bottomLeft.x && (block.second.location.x + 1) < xCollisionBox.topRight.x)
//            {
//                if ((block.second.location.y + 1) > xCollisionBox.bottomLeft.y && block.second.location.y < xCollisionBox.topRight.y)
//                {
//                    player->position.x = (block.second.location.x + 1) - 0.2f * PixelToBlock(player->sprite.width);
//                    if (isEnemy)
//                        player->velocity.x = -1 * player->velocity.x;
//                    else
//                        player->velocity.x = 0;
//                }
//            }
//
//            //checking the top of player against the bottom of the block
//            //checking if the block is within the x bounds of the collisionBox
//            if ((block.second.location.x + 1) > yCollisionBox.bottomLeft.x && block.second.location.x < yCollisionBox.topRight.x)
//            {
//                if (block.second.location.y > yCollisionBox.bottomLeft.y && block.second.location.y < yCollisionBox.topRight.y)
//                {
//                    player->position.y = block.second.location.y - PixelToBlock(player->sprite.height);
//                    if (player->velocity.y > 0)
//                        player->velocity.y = 0;
//                }
//            }
//            //checking the bottom of player against the top of the block
//            //checking if the block is within the x bounds of the collisionBox
//            if ((block.second.location.x + 1) > yCollisionBox.bottomLeft.x && block.second.location.x < yCollisionBox.topRight.x)
//            {
//                if ((block.second.location.y + 1) > yCollisionBox.bottomLeft.y && (block.second.location.y + 1) < yCollisionBox.topRight.y)
//                {
//                    player->position.y = block.second.location.y + 1;
//                    if (player->velocity.y < 0)
//                        player->velocity.y = 0;
//                    player->jumpCount = 2;
//                }
//            }
//        }
//    }
//}

float RadToDeg(float angle)
{
    return ((angle) / (2 * 3.14159f)) * 360;
}


float VectorDistance(Vector A, Vector B)
{
    return sqrtf(powf(B.x - A.x, 2) + powf(B.y - A.y, 2));
}


Vector Normalize(Vector v)
{
    float hyp = sqrt(powf(v.x, 2) + powf(v.y, 2));
    return { (v.x / hyp), (v.y / hyp) };
}


/*
Atan2f return value:

3pi/4      pi/2        pi/4


            O
+/-pi      /|\          0
           / \   


-3pi/4    -pi/2        pi/4
*/


float Atan2fToDegreeDiff(float theta)
{
    float result = 0;
    if (theta < 0)
        return RadToDeg(fabsf(theta));
    else
        return 360 + 360 - RadToDeg(theta);
}


void CreateBullet(Actor* player, Sprite bulletSprite, Vector mouseLoc, TileType blockToBeType)
{
    Projectile bullet = {};
    bullet.paintType = blockToBeType;
    bullet.inUse = true;
    bullet.sprite = bulletSprite;
    Vector adjustedPlayerPosition = { player->position.x/* + 0.5f*/, player->position.y + 1 };

    float playerBulletRadius = 0.5f; //half a block
    bullet.destination = mouseLoc;
    bullet.rotation = Atan2fToDegreeDiff(atan2f(bullet.destination.y - adjustedPlayerPosition.y, bullet.destination.x - adjustedPlayerPosition.x));
    
    float speed = 50.0f;

    Vector ToDest = bullet.destination - adjustedPlayerPosition;

    ToDest = Normalize(ToDest);
    bullet.velocity = ToDest * speed;

    bullet.position = adjustedPlayerPosition + (ToDest * playerBulletRadius);
   

    bool notCached = true;
    for (int32 i = 0; i < bulletList.size(); i++)
    {
        if (!bulletList[i].inUse)
        {
            notCached = false;
            bulletList[i] = bullet;
            break;
        }
    }
    if (notCached)
        bulletList.push_back(bullet);
}


void CreateLaser(Actor* player, Sprite sprite, Vector mouseLoc, TileType paintType)
{
    laser.paintType = paintType;
    laser.inUse = true;
    laser.sprite = sprite;
    Vector adjustedPlayerPosition = { player->position.x + 0.5f, player->position.y + 1 };

    float playerBulletRadius = 0.5f; //half a block
    laser.destination = mouseLoc;
    laser.rotation = Atan2fToDegreeDiff(atan2f(laser.destination.y - adjustedPlayerPosition.y, laser.destination.x - adjustedPlayerPosition.x));

    Vector ToDest = Normalize(laser.destination - adjustedPlayerPosition);

    laser.position = adjustedPlayerPosition + (ToDest * playerBulletRadius);
}


float Pythags(Vector a)
{
    return sqrtf(powf(a.x, 2) + powf(a.y, 2));
}


void RenderLaser()
{
    if (laser.inUse)
    {//x, y, w, h
        SDL_Color PC = {};
        if (laser.paintType == TileType::filled)
            PC = Green;
        else
            PC = Red;
        SDL_SetTextureColorMod(laser.sprite.texture, PC.r, PC.g, PC.b);
        
        SDL_Rect rect = CameraOffset(laser.position, { Pythags(laser.destination - laser.position), PixelToBlock(laser.sprite.height) });
        SDL_Point rotationPoint = { 0, rect.h / 2 };
        SDL_RenderCopyEx(windowInfo.renderer, laser.sprite.texture, NULL, &rect, laser.rotation, &rotationPoint, SDL_FLIP_NONE);
    }
}


float DotProduct(Vector a, Vector b)
{
    return a.x * b.x + a.y * b.y;
}

float terminalVelocity = 300;



void UpdateLocation(Actor* actor, float gravity, float deltaTime)
{
    actor->velocity.y += gravity * deltaTime;
    actor->velocity.y = Clamp(actor->velocity.y, -terminalVelocity, terminalVelocity);
    actor->position += actor->velocity * deltaTime;
}


void UpdateBullets(float deltaTime)
{
    for (int32 i = 0; i < bulletList.size(); i++)
    {
        Projectile* bullet = &bulletList[i];
        
        if (bullet->inUse)
        {
            UpdateLocation(bullet, 0, deltaTime);
            if (DotProduct(bullet->velocity, bullet->destination - bullet->position) < 0)
            {
                //paint block and remove bullet
                Block* blockPointer = &tileMap.GetBlock(bullet->destination);
                blockPointer->tileType = bullet->paintType;
                UpdateAllNeighbors(blockPointer);
                bullet->inUse = false;
            }
        }
    }
}


void RenderActor(Actor* actor, float rotation)
{
    SDL_Rect tempRect = CameraOffset(actor->position, PixelToBlock({ actor->sprite.width, actor->sprite.height }));
    SDL_RenderCopyEx(windowInfo.renderer, actor->sprite.texture, NULL, &tempRect, rotation, NULL, SDL_FLIP_NONE);
}


void RenderBullets()
{
    for (int32 i = 0; i < bulletList.size(); i++)
    {
        Projectile* bullet = &bulletList[i];
        if (bullet->inUse)
        {
            SDL_Color PC = {};
            if (bullet->paintType == TileType::filled)
                PC = Green;
            else
                PC = Red;
            SDL_SetTextureColorMod(bullet->sprite.texture, PC.r, PC.g, PC.b);
            RenderActor(bullet, bullet->rotation);
        }
    }
}


void UpdateEnemiesPosition(float gravity, float deltaTime)
{
    for (int32 i = 0; i < currentLevel.enemyList.size(); i++)
    {
        Enemy* enemyPointer = &currentLevel.enemyList[i];

        UpdateLocation(enemyPointer, gravity, deltaTime);
        CollisionWithBlocks(enemyPointer, true);
    }
}


void RenderEnemies()
{
    for (int32 i = 0; i < currentLevel.enemyList.size(); i++)
    {
        Enemy* enemyPointer = &currentLevel.enemyList[i];
        RenderActor(enemyPointer, 0);
    }
}


int main(int argc, char* argv[])
{
    //Window and Program Setups:
    std::unordered_map<SDL_Keycode, double> keyBoardEvents;
    MouseButtonEvent mouseButtonEvent = {};
    SDL_MouseMotionEvent mouseMotionEvent = {};
    bool running = true;
    SDL_Event SDLEvent;
    VectorInt previousMouseLocation = {};
    TileType paintType = TileType::invalid;

    windowInfo.SDLWindow = SDL_CreateWindow("Jumper", windowInfo.left, windowInfo.top, windowInfo.width, windowInfo.height, 0);
    windowInfo.renderer = SDL_CreateRenderer(windowInfo.SDLWindow, -1, SDL_RENDERER_ACCELERATED/*| SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE*/);

    double freq = double(SDL_GetPerformanceFrequency()); //HZ
    double totalTime = SDL_GetPerformanceCounter() / freq; //sec
    double previousTime = totalTime;
    
    /*
    SDL_Surface iconSurface = {};
    iconSurface.
    */

    //Sprite Creation
    Sprite playerSprite = CreateSprite("Player.png", SDL_BLENDMODE_BLEND);
    Sprite minecraftSprite = CreateSprite("TileMap.png", SDL_BLENDMODE_BLEND);
    Sprite spriteMap = CreateSprite("SpriteMap.png", SDL_BLENDMODE_BLEND);
    FontSprite textSheet = CreateFont("Text.png", SDL_BLENDMODE_BLEND, 32, 20, 16);
    Sprite background = CreateSprite("Background.png", SDL_BLENDMODE_BLEND);
    Sprite bulletSprite = CreateSprite("Bullet.png", SDL_BLENDMODE_BLEND);


    //Player Creation
    player.sprite = playerSprite;
    player.colOffset.x = 0.2f;
    player.colOffset.y = 0.3f;
    player.damage = 100;
	player.inUse = true;

    //Level instantiations
    currentLevel.filename = "DefaultLevel.PNG";
    
    //add enemies to current level (temporoary,  add to each level's metadata)
    Enemy enemy = {};
    enemy.sprite = CreateSprite("HeadMinion.png", SDL_BLENDMODE_BLEND);
    enemy.position = { 28, 1 };
    enemy.velocity.x = 4;
    enemy.colOffset.x = 0.125f;
    enemy.colOffset.y = 0.25f;
    enemy.damage = 5;
    currentLevel.enemyList.push_back(enemy);
    LoadLevel(&currentLevel);

    Level cacheLevel = {};
    cacheLevel.filename = "Level.PNG";




    tileMap.UpdateAllBlocks();

    //Start Of Running Program
    while (running)
    {

        //Setting Timer
        totalTime = SDL_GetPerformanceCounter() / freq;
        float deltaTime = float(totalTime - previousTime);
        previousTime = totalTime;

        if (deltaTime > 1 / 30.0f)
        {
            deltaTime = 1 / 30.0f;
        }

        SDL_SetRenderDrawColor(windowInfo.renderer, 0, 0, 0, 255);
        SDL_RenderClear(windowInfo.renderer);
        BackgroundRender(background, &player);

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
                mouseButtonEvent.type = SDLEvent.button.type;
                if (mouseButtonEvent.timestamp == 0)
                    mouseButtonEvent.timestamp = totalTime;//SDLEvent.button.timestamp;
                mouseButtonEvent.button = SDLEvent.button.button;
                mouseButtonEvent.state = SDLEvent.button.state;
                mouseButtonEvent.location.x = SDLEvent.button.x;
                mouseButtonEvent.location.y = SDLEvent.button.y;
                break;
            case SDL_MOUSEBUTTONUP:
                mouseButtonEvent.type = SDLEvent.button.type;
                mouseButtonEvent.timestamp = 0;//SDLEvent.button.timestamp;
                mouseButtonEvent.button = SDLEvent.button.button;
                mouseButtonEvent.state = SDLEvent.button.state;
                mouseButtonEvent.location.x = SDLEvent.button.x;
                mouseButtonEvent.location.y = SDLEvent.button.y;
                break;
            case SDL_MOUSEMOTION:
                mouseMotionEvent = SDLEvent.motion;
                break;
            }
        }


        //Keyboard Control:
        player.velocity.x = 0;
        if (keyBoardEvents[SDLK_w] == totalTime || keyBoardEvents[SDLK_SPACE] == totalTime || keyBoardEvents[SDLK_UP] == totalTime)
        {
            if (player.jumpCount > 0)
            {
                player.velocity.y = 20.0f;
                player.jumpCount -= 1;
            }
        }
        if (keyBoardEvents[SDLK_a] || keyBoardEvents[SDLK_LEFT])
            player.velocity.x -= 10;
        if (keyBoardEvents[SDLK_d] || keyBoardEvents[SDLK_RIGHT])
            player.velocity.x += 10;

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
            SaveLevel(&cacheLevel);
        if (keyBoardEvents[SDLK_9] == totalTime)
            LoadLevel(&cacheLevel);


        //Mouse Control:
        Rectangle clickRect = {};
        laser.inUse = false;
        if (debugList[DebugOptions::editBlocks] && mouseButtonEvent.timestamp)
        {
            VectorInt mouseLocation = CameraToPixelCoord({ mouseMotionEvent.x, mouseMotionEvent.y });
            Vector mouseLocationTranslated = PixelToBlock(mouseLocation);
            
            clickRect.bottomLeft = { mouseLocationTranslated.x - 0.5f, mouseLocationTranslated.y - 0.5f };
            clickRect.topRight = { mouseLocationTranslated.x + 0.5f, mouseLocationTranslated.y + 0.5f };
            Block* blockPointer = &tileMap.GetBlock(mouseLocationTranslated);
            blockPointer->location = mouseLocationTranslated;
            blockPointer->location.x = floorf(blockPointer->location.x);
            blockPointer->location.y = floorf(blockPointer->location.y);

            if (mouseButtonEvent.timestamp == totalTime)
            {
                if (mouseButtonEvent.button == SDL_BUTTON_LEFT)
                    paintType = TileType::filled;
                else if (mouseButtonEvent.button == SDL_BUTTON_RIGHT)
                    paintType = TileType::invalid;
            }
            if (debugList[DebugOptions::paintMethod] && mouseButtonEvent.type == SDL_MOUSEBUTTONDOWN)
            {
                if ((mouseButtonEvent.location.x != previousMouseLocation.x || mouseButtonEvent.location.y != previousMouseLocation.y))
                {
                    //if (blockPointer->tileType != paintType)
                    {
                        CreateLaser(&player, bulletSprite, mouseLocationTranslated, paintType);
                        blockPointer->tileType = paintType;
                        UpdateAllNeighbors(blockPointer);
                    }
                    previousMouseLocation = mouseLocation;
                }
            }
            else if (mouseButtonEvent.timestamp == totalTime)
            {
                CreateBullet(&player, bulletSprite, mouseLocationTranslated, paintType);
            }
        }

        //Update Player Location
        float gravity = -60.0f;
        UpdateLocation(&player, gravity, deltaTime);

        UpdateEnemiesPosition(gravity, deltaTime);

        CollisionWithBlocks(&player, false);

        bool screenFlash = CollisionWithEnemy(&player);

        UpdateActorHealth(&player);
        UpdateEnemyHealth();

        //Create Renderer:
        SDL_SetRenderDrawBlendMode(windowInfo.renderer, SDL_BLENDMODE_BLEND);

        camera.position = player.position;
        UpdateBullets(deltaTime);

        for (auto& block : *tileMap.blockList())
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
        RenderBullets();
        RenderLaser();
        RenderEnemies();
        RenderActor(&player, 0);
        
        DrawText(textSheet, Green, std::to_string(1 / deltaTime), { 0, 0 }, UIX::left, UIY::top);
        DrawText(textSheet, Green, std::to_string(player.health), { windowInfo.width, 0 }, UIX::right, UIY::top);


        if (screenFlash)
        {
            SDL_SetRenderDrawColor(windowInfo.renderer, lightWhite.r, lightWhite.g, lightWhite.b, lightWhite.a);
            SDL_RenderDrawRect(windowInfo.renderer, NULL);
            SDL_RenderFillRect(windowInfo.renderer, NULL);

            DebugRectRender({ {0,0}, {float(windowInfo.width), float(windowInfo.height)} }, White);
        }
        //DrawButton(textSheet, "Test", { 0, 0 }, UIX::left, UIY::top, Green, Orange, mouseButtonEvent, mouseMotionEvent);
        //DrawText(textSheet, Red, "Test", { windowInfo.width / 2, windowInfo.height / 2}, UIX::mid, UIY::mid);

        //Present Screen
        SDL_RenderPresent(windowInfo.renderer);
    }
    
    return 0;
}