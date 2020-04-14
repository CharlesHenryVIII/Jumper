#include <SDL.h>

#include <stdio.h>
#include "SDL.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <iostream>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <cmath>

/*
draw colliders using SD
go over void Char*** = &argv
what does .end() do?
set flags for each situation that needs to change what block it is
do this at the start when all the blocks are built then do it only when a block is added or removed.
if block to the right is grass then add >>1, if block to the left is grass then add 1>>2, if there is a block above then 1>>3
if there isnt a block the the left then dont bit shift, if there isnt a block to the left then 

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

const SDL_Color lightRed = { 255, 0, 0, 63 };
const SDL_Color lightGreen = { 0, 255, 0, 63 };
const SDL_Color lightBlue = { 0, 0, 255, 63 };

constexpr auto leftGrassBlock   = 1 << 0;   //1 or odd
constexpr auto rightGrassBlock  = 1 << 1;   //2
constexpr auto blockAbove       = 1 << 2;   //4
constexpr auto rightDirtBlock   = 1 << 3;   //8
constexpr auto leftDirtBlock    = 1 << 4;   //16



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


enum class DebugOptions
{
    none,
    playerCollision,
    blockCollision,
    clickLocation,
    paintMethod
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


struct Player {
    Sprite sprite;
    Vector position;
    Vector velocity = {};
    Vector acceleration;
    Vector collisionSize;
    int jumpCount = 2;
};


struct Camera {
    Vector position;
} camera = {};


enum class TileType {
    invalid,
    dirt,
    grass,
    grassCorner,
    grassEdge
};


struct Block {
    Vector location = {};
    TileType tileType = TileType::invalid;

    Vector PixelPosition()
    {
        return { location.x * blockSize, location.y * blockSize };
    }

};


uint64 HashingFunction(int32 x, int32 y)
{
	return (uint64(y) << 32 | uint64(x));
}


class TileMap
{
public:
	std::unordered_map<uint64, Block> blocks;

	//Block Coordinate System
	Block& GetBlock(Vector loc)
	{
		return blocks[HashingFunction(int32(loc.x), int32(loc.y))];
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
		blocks[HashingFunction(int32(loc.x), int32(loc.y))] = { loc, tileType };
	}
	bool CheckForBlock(Vector loc)
	{
		return blocks.find(HashingFunction(int32(loc.x), int32(loc.y))) != blocks.end();
	}
};
static TileMap tileMap;
//static std::unordered_map<uint64, Block> blocks;


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


struct MouseButtonEvent {
    Uint32 type;        /**< ::SDL_MOUSEBUTTONDOWN or ::SDL_MOUSEBUTTONUP */
    //uint32 padding1;
    double timestamp;   /**< In milliseconds, populated using SDL_GetTicks() */
    //Uint32 windowID;    /**< The window with mouse focus, if any */
    //Uint32 which;       /**< The mouse instance id, or SDL_TOUCH_MOUSEID */
    Uint8 button;       /**< The mouse button index */
    Uint8 state;        /**< ::SDL_PRESSED or ::SDL_RELEASED */
    //Uint8 clicks;       /**< 1 for single-click, 2 for double-click, etc. */
    //Uint16 padding2;
    //Uint32 padding3;
    Vector location;
    
    //float x;           /**< X coordinate, relative to window */
    //float y;           /**< Y coordinate, relative to window */
};


SDL_Rect collisionXRect = {};
SDL_Rect collisionYRect = {};

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


Vector BlocktoPixel(Vector loc)
{
	return {loc.x * blockSize, loc.y * blockSize};
}

Vector PixeltoBlock(Vector loc)
{
	return { float(uint32(loc.x + 0.5f) / blockSize), float(uint32(loc.y + 0.5f) / blockSize) };
}

/*
bool CheckForBlockf(Vector input)//Pixel Coordinate Space
{
    uint32 x = uint32(input.x + 0.5f) / blockSize;
    uint32 y = uint32(input.y + 0.5f) / blockSize;

    if (blocks.find(HashingFunction(x, y)) != blocks.end())
        return blocks[HashingFunction(x, y)].tileType != TileType::invalid;
    else
        return false;
}


bool checkMapForBlock(Vector input)
{
    uint32 x = uint32(input.x + 0.5f) / blockSize;
    uint32 y = uint32(input.y + 0.5f) / blockSize;

    return blocks.find(HashingFunction(x, y)) != blocks.end();
}


bool CheckForBlocki(Vector input)//Block Coordinate Space
{
    uint32 x = uint32(input.x + 0.5f);
    uint32 y = uint32(input.y + 0.5f);

    if (blocks.find(HashingFunction(x, y)) != blocks.end())
        return blocks[HashingFunction(x, y)].tileType != TileType::invalid;
    else
        return false; //blocks2.find(HashingFunction(x, y)) != blocks2.end();//.tileType != TileType::invalid;
}


Block GetBlock(Vector input) //Block Coordinate Space
{
    uint32 x = uint32(input.x + 0.5f);
    uint32 y = uint32(input.y + 0.5f);
    if (CheckForBlocki(input))
        return blocks[HashingFunction(x, y)];
    else
        return {};
}


Block* ForceGetBlock(Vector input) //Block Coordinate Space
{
    uint32 x = uint32(input.x + 0.5f);
    uint32 y = uint32(input.y + 0.5f);
    return &blocks[HashingFunction(x, y)];
}
*/

/*constexpr auto leftGrassBlock   = 1 << 0;   //1 or odd
constexpr auto rightGrassBlock  = 1 << 1;   //2
constexpr auto blockAbove       = 1 << 2;   //4
constexpr auto rightDirtBlock   = 1 << 3;   //8
constexpr auto leftDirtBlock    = 1 << 4;   //16*/
void UpdateBlock(Block* block)
{
    if (block->tileType == TileType::invalid)
        return;

    Block left = tileMap.GetBlock({ block->location.x - 1, block->location.y });
    Block right = tileMap.GetBlock({ block->location.x + 1, block->location.y });
    Block top = tileMap.GetBlock({ block->location.x, block->location.y + 1});

    //Check if dirt block
    if (top.tileType != TileType::invalid)
        block->tileType = TileType::dirt;
    //checking for grass block
    else if ((left.tileType != TileType::invalid && left.tileType != TileType::dirt))// && (right.tileType != TileType::invalid && right.tileType != TileType::dirt))
        block->tileType = TileType::grass;
    
    //check for left corner block
    else if (left.tileType == TileType::invalid)
        block->tileType = TileType::grassCorner;
    //check for right corner block
    else if (right.tileType == TileType::invalid)
        block->tileType = TileType::grass;

    //check for right edge block
    else if (left.tileType == TileType::dirt)
        block->tileType = TileType::grass;
    //check for right edge block
    else if (right.tileType == TileType::dirt)
        block->tileType = TileType::grassEdge;
}


void SurroundBlockUpdate(Block* block, bool updateTop)
{
    //update block below
    if (tileMap.CheckForBlock({ block->location.x, block->location.y - 1 }))
        UpdateBlock(&tileMap.GetBlock({ block->location.x, block->location.y - 1 }));
    //update block to the left
    if (tileMap.CheckForBlock({ block->location.x - 1, block->location.y }))
        UpdateBlock(&tileMap.GetBlock({ block->location.x - 1, block->location.y }));
    //update block to the right
    if (tileMap.CheckForBlock({ block->location.x + 1, block->location.y }))
        UpdateBlock(&tileMap.GetBlock({ block->location.x + 1, block->location.y }));
    if (updateTop && tileMap.CheckForBlock({ block->location.x, block->location.y - 1 }))
        UpdateBlock(&tileMap.GetBlock({ block->location.x, block->location.y + 1 }));
}


void ClickUpdate(Block* block, bool updateTop)
{
    UpdateBlock(block);
    SurroundBlockUpdate(block, updateTop);
}


SDL_Rect CameraOffset(Vector location, VectorInt size)
{
    float xOffset = camera.position.x - windowInfo.width / 2;
    float yOffset = camera.position.y - windowInfo.height / 2;

    SDL_Rect result;
    result.x = int(location.x - xOffset);
    result.y = windowInfo.height - int(location.y - yOffset) - size.y;
    result.w = size.x;
    result.h = size.y;

    return result;
}


//difference between the player and the center of the screen
Vector CameraToPixelCoord(Vector input)
{
    float xOffset = camera.position.x - windowInfo.width / 2;
    float yOffset = camera.position.y - windowInfo.height / 2;
    return { input.x + xOffset, windowInfo.height - input.y + yOffset};
}


void SpriteMapRender(Sprite sprite, TileType tile, Vector position, int32 blocksPerRow)
{
    int32 x = (uint32(tile) - 1) % blocksPerRow * blockSize;
    int32 y = (uint32(tile) - 1) / blocksPerRow * blockSize;

    SDL_Rect blockRect = { x, y, blockSize, blockSize };
    SDL_Rect DestRect = CameraOffset( position, { blockSize, blockSize });  
    SDL_RenderCopyEx(windowInfo.renderer, sprite.texture, &blockRect, &DestRect, 0, NULL, SDL_FLIP_NONE);
}


float ChooseSmallest(float A, float B)
{
    if (A < B)
        return A;
    else
        return B;
}


void DebugRectRender(SDL_Rect rect, SDL_Color color)
{
    SDL_SetRenderDrawColor(windowInfo.renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect2 = CameraOffset({ float(rect.x), float(rect.y) }, { rect.w, rect.h });
    SDL_RenderDrawRect(windowInfo.renderer, &rect2);
    SDL_RenderFillRect(windowInfo.renderer, &rect2);
}


void CollisionSystemCall(Player* player)
{
    collisionXRect = {};
    collisionYRect = {};

    Vector referenceBlock = { };
    
    Vector playerCollisionPoint1 = {};
    Vector playerCollisionPoint2 = {};
    Vector playerCollisionPoint3 = {};
    Vector playerCollisionPoint4 = {};

    float playerCoordinate = {};

    Rectangle xCollisionBox;
    xCollisionBox.bottomLeft = { player->position.x + 0.2f * player->sprite.width, player->position.y + (0.2f * player->sprite.height) };
    xCollisionBox.topRight = {player->position.x + 0.8f * player->sprite.width, player->position.y + 0.8f * player->sprite.height };
    float xCollisionBoxWidth = xCollisionBox.Width();

    Rectangle yCollisionBox;
    yCollisionBox.bottomLeft = { player->position.x + (0.3f * player->sprite.width), player->position.y };
    yCollisionBox.topRight = { player->position.x + 0.7f * player->sprite.width, player->position.y + player->sprite.height };
    
    
    if (debugList[DebugOptions::playerCollision]) //debug draw
    {
        SDL_SetRenderDrawBlendMode(windowInfo.renderer, SDL_BLENDMODE_BLEND);
        
        SDL_Rect xRect = { int(xCollisionBox.bottomLeft.x),
                            int(xCollisionBox.bottomLeft.y),
                            int(xCollisionBox.topRight.x - xCollisionBox.bottomLeft.x),
                            int(xCollisionBox.topRight.y - xCollisionBox.bottomLeft.y) };
        DebugRectRender(xRect, transGreen);

        SDL_Rect yRect = { int(yCollisionBox.bottomLeft.x),
                            int(yCollisionBox.bottomLeft.y),
                            int(yCollisionBox.topRight.x - yCollisionBox.bottomLeft.x),
                            int(yCollisionBox.topRight.y - yCollisionBox.bottomLeft.y) };
        DebugRectRender(yRect, transGreen);

    }


    for (auto& block : tileMap.blocks)
    {
        if (block.second.tileType != TileType::invalid)
        {
            //checking the right side of player against the left side of a block
            if (block.second.location.x * blockSize > xCollisionBox.bottomLeft.x&& block.second.location.x* blockSize < xCollisionBox.topRight.x)
            {
                //float bottom = xCollisionBox.bottomLeft.y - blockSize * 0.5f; 
                //float top = xCollisionBox.topRight.y + blockSize * 0.5f;
                //float blockCenter = block.second.location.y * blockSize + blockSize * 0.5f;
                //if (blockCenter > bottom && blockCenter < top)
                //{
                //    player->position.x = block.second.location.x - player->sprite.width;
                //}

                if ((block.second.location.y + 1) * blockSize > xCollisionBox.bottomLeft.y&& block.second.location.y* blockSize < xCollisionBox.topRight.y)
                {
                    player->position.x = block.second.location.x * blockSize - 0.8f * player->sprite.width;
                    player->velocity.x = 0;

                    //collisionXRect.x = int(ChooseSmallest(block.second.location.x * blockSize, player->position.x));
                    //collisionXRect.y = int(ChooseSmallest(block.second.location.y * blockSize, player->position.y));
                    //collisionXRect.w = int(fabs(block.second.location.x * blockSize - player->position.x));
                    //collisionXRect.h = int(fabs(block.second.location.y * blockSize - player->position.y));

                }
            }
            //checking the left side of player against the right side of the block
            if ((block.second.location.x + 1) * blockSize > xCollisionBox.bottomLeft.x && (block.second.location.x + 1)* blockSize < xCollisionBox.topRight.x)
            {
                if ((block.second.location.y + 1) * blockSize > xCollisionBox.bottomLeft.y&& block.second.location.y* blockSize < xCollisionBox.topRight.y)
                {
                    player->position.x = (block.second.location.x + 1) * blockSize - 0.2f * player->sprite.width;
                    player->velocity.x = 0;
                }
            }

            //checking the top of player against the bottom of the block
            //checking if the block is within the x bounds of the collisionBox
            if ((block.second.location.x + 1) * blockSize > yCollisionBox.bottomLeft.x&& block.second.location.x* blockSize < yCollisionBox.topRight.x)
            {
                if (block.second.location.y * blockSize > yCollisionBox.bottomLeft.y&& block.second.location.y* blockSize < yCollisionBox.topRight.y)
                {
                    player->position.y = block.second.location.y * blockSize - player->sprite.height;
                    if (player->velocity.y > 0)
                        player->velocity.y = 0;
                }
            }
            //checking the bottom of player against the top of the block
            //checking if the block is within the x bounds of the collisionBox
            if ((block.second.location.x + 1) * blockSize > yCollisionBox.bottomLeft.x&& block.second.location.x* blockSize < yCollisionBox.topRight.x)
            {
                if ((block.second.location.y + 1) * blockSize > yCollisionBox.bottomLeft.y && (block.second.location.y + 1)* blockSize < yCollisionBox.topRight.y)
                {
                    player->position.y = (block.second.location.y + 1) * blockSize;
                    if (player->velocity.y < 0)
                        player->velocity.y = 0;
                    player->jumpCount = 2;
                }
            }
        }
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
    Vector previousMouseLocation = {};
    TileType paintType = TileType::invalid;

    windowInfo.SDLWindow = SDL_CreateWindow("Jumper", windowInfo.left, windowInfo.top, windowInfo.width, windowInfo.height, 0);
    windowInfo.renderer = SDL_CreateRenderer(windowInfo.SDLWindow, -1, SDL_RENDERER_ACCELERATED/*| SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE*/);

    double freq = double(SDL_GetPerformanceFrequency()); //HZ
    double totalTime = SDL_GetPerformanceCounter() / freq; //sec
    double previousTime = totalTime;


    //Sprite Creation
    Sprite playerSprite = CreateSprite(windowInfo.renderer, "Player.png", SDL_BLENDMODE_BLEND);
    Sprite minecraftSprite = CreateSprite(windowInfo.renderer, "TileMap.png", SDL_BLENDMODE_BLEND);
    Sprite spriteMap = CreateSprite(windowInfo.renderer, "SpriteMap.png", SDL_BLENDMODE_BLEND);

    //Player Creation
    Player player;
    player.sprite = playerSprite;
    player.position = { 3.0f*blockSize, 10.0f * blockSize }; //bottom left
    player.collisionSize = { 32, 64 }; 

    
    //block creation
    std::vector<Block> initilizedBlocks = {
        { {10, 2}, TileType::grass },
        { {10, 1}, TileType::grass },
        { {12, 2}, TileType::grass },
        { {12, 3}, TileType::grass },
        { {15, 3}, TileType::grass },
        { {17, 4}, TileType::grass },
        { {18, 4}, TileType::grass },
        { {12, 10}, TileType::grass },
        { {22, 16}, TileType::grass }
    };

    for (uint64 i = 0; i < 50; i++)
        initilizedBlocks.push_back({ {0, float(i)}, TileType::grass });

    for (uint64 i = 0; i < 50; i++)
        initilizedBlocks.push_back({ {39, float(i)}, TileType::grass });

    for (auto& block : initilizedBlocks)
        tileMap.AddBlock(block.location, block.tileType);

    for (int32 x = 0; x * blockSize < windowInfo.width; x++)
		tileMap.AddBlock({ float(x), 0 }, TileType::grass);

    //first pass to set most things
    for (auto& block : tileMap.blocks)
    {
        UpdateBlock(&block.second);
    }
    //2nd pass to clean up what was missed aka grass blocks that should be edge blocks
    for (auto& block : tileMap.blocks)
    {
        UpdateBlock(&block.second);
    }

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
                mouseButtonEvent.location.x = float(SDLEvent.button.x);
                mouseButtonEvent.location.y = float(SDLEvent.button.y);
                break;
            case SDL_MOUSEBUTTONUP:
                mouseButtonEvent.type = SDLEvent.button.type;
                mouseButtonEvent.timestamp = 0;//SDLEvent.button.timestamp;
                mouseButtonEvent.button = SDLEvent.button.button;
                mouseButtonEvent.state = SDLEvent.button.state;
                mouseButtonEvent.location.x = float(SDLEvent.button.x);
                mouseButtonEvent.location.y = float(SDLEvent.button.y);
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
                player.velocity.y = float(20 * blockSize);
                player.jumpCount -= 1;
            }
        }
        //else if (keyBoardEvents[SDLK_s] == totalTime || keyBoardEvents[SDLK_DOWN] == totalTime)
        //    player.velocity.x=0;
        if (keyBoardEvents[SDLK_a] || keyBoardEvents[SDLK_LEFT])
            player.velocity.x -= 10 * blockSize;
        if (keyBoardEvents[SDLK_d] || keyBoardEvents[SDLK_RIGHT])
            player.velocity.x += 10 * blockSize;

        if (keyBoardEvents[SDLK_1] == totalTime)
            debugList[DebugOptions::playerCollision] = !debugList[DebugOptions::playerCollision];
        if (keyBoardEvents[SDLK_2] == totalTime)
            debugList[DebugOptions::blockCollision] = !debugList[DebugOptions::blockCollision];
        if (keyBoardEvents[SDLK_3] == totalTime)
            debugList[DebugOptions::clickLocation] = !debugList[DebugOptions::clickLocation];
        if (keyBoardEvents[SDLK_4] == totalTime)
            debugList[DebugOptions::paintMethod] = !debugList[DebugOptions::paintMethod];


        //Mouse Control:
        SDL_Rect clickRect = {};
        SDL_Rect curserRect = {};

        if (mouseButtonEvent.timestamp == totalTime)
        {
            Vector clickLocation = CameraToPixelCoord(mouseButtonEvent.location);
            Vector clickLocationTranslated = { float(int32(clickLocation.x + 0.5f) / blockSize), float(int32(clickLocation.y + 0.5f) / blockSize) };
            clickRect = { int(clickLocation.x - 5), int(clickLocation.y - 5), 10, 10 };

			Block* blockPointer = &tileMap.GetBlock(clickLocationTranslated);
            // TODO(choman): Remove this when we have a type
            blockPointer->location = clickLocationTranslated;
            if (blockPointer->tileType == TileType::invalid)
            {
                paintType = TileType::grass;
                blockPointer->tileType = TileType::grass;
                ClickUpdate(blockPointer, false);
            }
            else
            {
                paintType = TileType::invalid;
                blockPointer->tileType = TileType::invalid;
                SurroundBlockUpdate(blockPointer, false);
            }
        }

        if (debugList[DebugOptions::paintMethod] && mouseButtonEvent.type == SDL_MOUSEBUTTONDOWN)
        {
            Vector mouseLocation = CameraToPixelCoord({ float(mouseMotionEvent.x), float(mouseMotionEvent.y) });
            Vector mouseLocationTranslated = { float(int32(mouseLocation.x + 0.5f) / blockSize), float(int32(mouseLocation.y + 0.5f) / blockSize) };
            curserRect = { int(mouseLocation.x - 5), int(mouseLocation.y - 5), 10, 10 };

            if ((mouseButtonEvent.location.x != previousMouseLocation.x || mouseButtonEvent.location.y != previousMouseLocation.y))
            {
				Block* blockPointer = &tileMap.GetBlock(mouseLocationTranslated);
                // TODO(choman): Remove this when we have a type
                blockPointer->location = mouseLocationTranslated;
                if (blockPointer->tileType != paintType)
                {
                    blockPointer->tileType = paintType;
                    //ClickUpdate(blockPointer, true);
                    //UpdateBlock(blockPointer);
                    ClickUpdate(blockPointer, false);

                }
                else
                {
                    //blocks2[HashingFunction(int32(mouseLocation.x + 0.5f) / blockSize, int32(mouseLocation.y + 0.5f) / blockSize)] = { mouseLocationTranslated, paintType };
                    //ClickUpdate(&blocks2[HashingFunction(int32(mouseLocation.x + 0.5f) / blockSize, int32(mouseLocation.y + 0.5f) / blockSize)], true);
                }
                previousMouseLocation = mouseLocation;
            }
        }



        //update x coordinate:
        player.position.x += player.velocity.x * deltaTime;

        //update y coordinate based on gravity and bounding box:
        float gravity = float(-60 * blockSize);

        player.velocity.y += gravity * deltaTime; //v = v0 + at
        player.position.y += player.velocity.y * deltaTime + 0.5f * gravity * deltaTime * deltaTime; //y = y0 + vt + .5at^2


        CollisionSystemCall(&player);


        //Create Renderer:
        SDL_SetRenderDrawBlendMode(windowInfo.renderer, SDL_BLENDMODE_BLEND);

        camera.position = player.position;

       //debug/testing blocks
        for (auto& block : tileMap.blocks)
        {
            if (block.second.tileType != TileType::invalid)
            {
                //SpriteMapRender(minecraftSprite, block.second.tileType, block.second.PixelPosition(), 16);
                //if (!CheckForBlocki({ block.second.location.x, block.second.location.y + 1 }) && CheckForBlocki({ block.second.location.x + 1, block.second.location.y }) && CheckForBlocki({ block.second.location.x - 1, block.second.location.y }))
                //{
                //    if (GetBlock({ block.second.location.x + 1, block.second.location.y }).tileType != TileType::dirt && GetBlock({ block.second.location.x - 1, block.second.location.y }).tileType != TileType::dirt)
                //        block.second.tileType = TileType::grass;
                //}
                //else if (CheckForBlocki({ block.second.location.x, block.second.location.y + 1 }))
                //    block.second.tileType = TileType::dirt;
                //else if (GetBlock({ block.second.location.x + 1, block.second.location.y }).tileType == TileType::dirt)
                //    block.second.tileType = TileType::grassEdge;
                //else if (GetBlock({ block.second.location.x + 1, block.second.location.y }).tileType == TileType::grass && !CheckForBlocki({ block.second.location.x - 1, block.second.location.y }))
                //    block.second.tileType = TileType::grassCorner;

                SpriteMapRender(spriteMap, block.second.tileType, block.second.PixelPosition(), 2);

                if (debugList[DebugOptions::blockCollision] && block.second.tileType != TileType::invalid)
                {
                    SDL_Rect blockRect = { int(block.second.location.x * blockSize),
                                            int((block.second.location.y) * blockSize),
                                            blockSize,
                                            blockSize };
                    DebugRectRender(blockRect, lightRed);
                }
            }
        }
        
        if (debugList[DebugOptions::clickLocation])
        {
            DebugRectRender(clickRect, transGreen);
            DebugRectRender(curserRect, Red);
        }

        SDL_Rect tempRect = CameraOffset(player.position, { player.sprite.width, player.sprite.height });
        SDL_RenderCopyEx(windowInfo.renderer, playerSprite.texture, NULL, &tempRect, 0, NULL, SDL_FLIP_NONE);



        //Present Screen
        SDL_RenderPresent(windowInfo.renderer);
    }
    
    return 0;
}