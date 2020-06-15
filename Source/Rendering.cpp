#include "Rendering.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include <cassert>

WindowInfo windowInfo = { 200, 200, 1280, 720 };
Camera camera;


WindowInfo& GetWindowInfo()
{
    return windowInfo;
}

void CreateWindow()
{
    windowInfo.SDLWindow = SDL_CreateWindow("Jumper", windowInfo.left, windowInfo.top, windowInfo.width, windowInfo.height, 0);
    windowInfo.renderer = SDL_CreateRenderer(windowInfo.SDLWindow, -1, SDL_RENDERER_ACCELERATED/*| SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE*/);
}

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
    return { input.x + xOffset, windowInfo.height - input.y + yOffset };
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


void DrawText(FontSprite fontSprite, SDL_Color c, std::string text, float size, VectorInt loc, UIX XLayout, UIY YLayout)
{
    //ABC
    int32 CPR = fontSprite.charPerRow;

    //Upper Left corner and size
    SDL_Rect SRect = {};
    SDL_Rect DRect = {};

    SRect.w = fontSprite.xCharSize;
    SRect.h = fontSprite.charSize;
    DRect.w = int(SRect.w * size);
    DRect.h = int(SRect.h * size);

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
    if (point.x > rect.bottomLeft.x && point.x < rect.topRight.x)
        if (point.y > rect.bottomLeft.y && point.y < rect.topRight.y)
            result = true;
    return result;
}


bool SDLPointRectangleCollision(VectorInt point, Rectangle rect)
{
    bool result = false;
    if (point.x > rect.bottomLeft.x && point.x < rect.topRight.x)
        if (point.y < rect.bottomLeft.y && point.y > rect.topRight.y)
            result = true;
    return result;
}


//top left 0,0
bool DrawButton(FontSprite textSprite, std::string text, VectorInt loc, UIX XLayout, UIY YLayout, SDL_Color BC, SDL_Color TC)
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

    VectorInt mousePosition;
    bool leftButtonPressed = (SDL_GetMouseState(&mousePosition.x, &mousePosition.y) & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;

    if (SDLPointRectangleCollision(mousePosition, cshRect))
    {
        SDL_SetRenderDrawColor(windowInfo.renderer, BC.r, BC.g, BC.b, BC.a / 2);
        SDL_RenderFillRect(windowInfo.renderer, &buttonRect);
    }
    if (leftButtonPressed)
    {
        if (SDLPointRectangleCollision(mousePosition, cshRect))
        {
            SDL_SetRenderDrawColor(windowInfo.renderer, BC.r, BC.g, BC.b, BC.a);
            SDL_RenderFillRect(windowInfo.renderer, &buttonRect);
            result = true;
        }
    }
    DrawText(textSprite, TC, text, 1.0f, { buttonRect.x + buttonRect.w / 2, buttonRect.y + buttonRect.h / 2 }, UIX::mid, UIY::mid);
    return result;
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
        SDL_SetTextureColorMod(laser.idle.anime[0]->texture, PC.r, PC.g, PC.b);
        SDL_Rect rect = CameraOffset(laser.position, { Pythags(laser.destination - laser.position), PixelToBlock(laser.idle.anime[0]->height) });
        SDL_Point rotationPoint = { 0, rect.h / 2 };
        SDL_RenderCopyEx(windowInfo.renderer, laser.idle.anime[0]->texture, NULL, &rect, laser.rotation, &rotationPoint, SDL_FLIP_NONE);
    }
}

void IndexIncrimentor(Animation& anime, bool stayOnLastFrame, Actor* actor, double totalTime)
{
    bool shouldIncriment = ((actor->lastAnimationTime + (1.0 / anime.fps)) <= totalTime);
    
    if (stayOnLastFrame)
        shouldIncriment = (shouldIncriment && (anime.index + 1 < (int32)anime.anime.size()));

    if (shouldIncriment)
    {
        anime.index++;
        actor->lastAnimationTime = totalTime;
        if (anime.index >= anime.anime.size())
            anime.index = 0;
    }
}

Sprite* SpriteChoice(Animation& anime, ActorState& actorState)
{
    if (actorState != ActorState::dead)
    {
        anime.index = 0;
        actorState = ActorState::dead;
    }
    return anime.anime[anime.index];
}

void RenderActor(Actor* actor, float rotation, double totalTime)
{
    SDL_Rect destRect = {};
    SDL_RendererFlip flippage = SDL_FLIP_NONE;
    int32 xPos;
    Sprite* sprite = {};

    std::vector<Sprite*>* list = {};

    if (actor->health <= 0)
    {
        //death animation
        sprite = SpriteChoice(actor->death, actor->actorState);
        IndexIncrimentor(actor->death, true, actor, totalTime);
    }
    else if (actor->velocity.x == 0 && actor->velocity.y == 0)
    {
        //idle
        sprite = SpriteChoice(actor->idle, actor->actorState);
        IndexIncrimentor(actor->idle, false, actor, totalTime);
    }
    else if (actor->velocity.y != 0)
    {
        //jumping/in-air, 
        sprite = SpriteChoice(actor->jump, actor->actorState);
        IndexIncrimentor(actor->jump, true, actor, totalTime);

        //TODO:  Fix issue with when this wont be true at the top of a jump when velcoity approaches zero
    }
    else if (actor->velocity.x != 0)
    {
        //walking
        sprite = SpriteChoice(actor->run, actor->actorState);
        IndexIncrimentor(actor->run, false, actor, totalTime);
    }



    if (actor->lastInputWasLeft)
    {
        flippage = SDL_FLIP_HORIZONTAL;
        xPos = sprite->width - actor->colRect.topRight.x;
    }
    else
    {
        xPos = actor->colRect.bottomLeft.x;
    }
    destRect = CameraOffset({ actor->position.x - PixelToBlock((int(xPos * actor->SpriteRatio()))), 
                              actor->position.y - PixelToBlock((int(actor->colRect.bottomLeft.y * actor->SpriteRatio()))) },
        PixelToBlock({ (int)(actor->SpriteRatio() * sprite->width),
                       (int)(actor->SpriteRatio() * sprite->height) }));
    SDL_RenderCopyEx(windowInfo.renderer, sprite->texture, NULL, &destRect, rotation, NULL, flippage);
}

void GameSpaceRectRender(Rectangle rect, SDL_Color color)
{
    SDL_Rect tempRect = CameraOffset( rect.bottomLeft , PixelToBlock({ int32(BlockToPixel(rect.Width())), int32(rect.Height()) }));
    SDL_SetRenderDrawColor(windowInfo.renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(windowInfo.renderer, &tempRect);
}