#include "Rendering.h"
#include "Entity.h"
#include "Misc.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include <cassert>
#include <algorithm>

WindowInfo windowInfo = { 200, 200, 1280, 720 };
std::unordered_map<std::string, Sprite*> sprites;
std::unordered_map<std::string, FontSprite*> fonts;
Camera camera;


WindowInfo& GetWindowInfo()
{
    return windowInfo;
}

void CreateSDLWindow()
{
    //SDL_Init(SDL_INIT_EVERYTHING);
    windowInfo.SDLWindow = SDL_CreateWindow("Jumper", windowInfo.left, windowInfo.top, windowInfo.width, windowInfo.height, 0);
    windowInfo.renderer = SDL_CreateRenderer(windowInfo.SDLWindow, -1, SDL_RENDERER_ACCELERATED/*| SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE*/);
}
void CreateOpenGLWindow()
{
    windowInfo.SDLWindow = SDL_CreateWindow("Jumper", windowInfo.left, windowInfo.top, windowInfo.width, windowInfo.height, 0);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GLContext GLContext = SDL_GL_CreateContext(windowInfo.SDLWindow);
	SDL_GL_MakeCurrent(windowInfo.SDLWindow, GLContext);
}

static std::vector<RenderInformation> drawCalls;

void AddTextureToRender(SDL_Rect sRect, SDL_Rect dRect, RenderPrio priority, SDL_Texture* texture, SDL_Color colorMod, float rotation, const SDL_Point* rotationPoint, SDL_RendererFlip flippage)
{
    RenderInformation info;
    info.renderType = RenderType::Texture;
	info.sRect = sRect;
	info.dRect = dRect;
    info.prio = priority;
    info.color = colorMod;

    info.texture.texture = texture;
    info.texture.rotation = rotation;
    info.texture.flippage = flippage;
    info.texture.rotationPoint = rotationPoint;

    drawCalls.push_back(info);
}

void AddRectToRender(RenderType type, Rectangle rect, SDL_Color color, RenderPrio prio)
{
    RenderInformation renderInformation;
    renderInformation.renderType = type;
	if (prio == RenderPrio::UI || prio == RenderPrio::foreground)
		renderInformation.dRect = RectangleToSDL(rect, false);
	else
		renderInformation.dRect = CameraOffset(rect.bottomLeft, { rect.Width(), rect.Height() });
    renderInformation.color = color;
    renderInformation.prio = prio;
    drawCalls.push_back(renderInformation);
}

void AddRectToRender(Rectangle rect, SDL_Color color, RenderPrio prio)
{
    AddRectToRender(RenderType::DebugOutline, rect, color, prio);
    AddRectToRender(RenderType::DebugFill, rect, color, prio);
}

void RenderDrawCalls()
{
    std::stable_sort(drawCalls.begin(), drawCalls.end(), [](const RenderInformation &a, const RenderInformation &b) {
        return a.prio < b.prio;
    });
    for (auto& item : drawCalls)
    {
        switch (item.renderType)
        {
            case RenderType::Texture:
            {

                const SDL_Rect* sRect = &item.sRect;
                const SDL_Rect* dRect = &item.dRect;
                if (item.sRect.w == 0 && item.sRect.h == 0)
                    sRect = nullptr;
                if (item.dRect.w == 0 && item.dRect.h == 0)
                    dRect = nullptr;
                if (item.color.r != 0 || item.color.g != 0 || item.color.b != 0 || item.color.a != 0)
					SDL_SetTextureColorMod(item.texture.texture, item.color.r, item.color.g, item.color.b);
				SDL_RenderCopyEx(windowInfo.renderer, item.texture.texture, sRect, dRect, item.texture.rotation, item.texture.rotationPoint, item.texture.flippage);
                break;
            }
            case RenderType::DebugOutline:
            {

                const SDL_Rect* sRect = &item.dRect;
                if (item.sRect.w == 0 && item.dRect.h == 0)
                    sRect = nullptr;
                SDL_SetRenderDrawColor(windowInfo.renderer, item.color.r, item.color.g, item.color.b, item.color.a);
                SDL_RenderDrawRect(windowInfo.renderer, sRect);
                break;
            }
            case RenderType::DebugFill:
            {

                const SDL_Rect* dRect = &item.dRect;
                if (item.sRect.w == 0 && item.dRect.h == 0)
                    dRect = nullptr;
                SDL_SetRenderDrawColor(windowInfo.renderer, item.color.r, item.color.g, item.color.b, item.color.a);
                SDL_RenderFillRect(windowInfo.renderer, dRect);
                break;
            }
        }
    }
    drawCalls.clear();
}

Sprite* CreateSprite(const char* name, SDL_BlendMode blendMode)
{
	int32 colorChannels;
	Sprite* result = new Sprite;
	unsigned char* image = stbi_load(name, &result->width, &result->height, &colorChannels, STBI_rgb_alpha);

	if (image == nullptr)
		return nullptr;


#if (OPENGLMODE==1)

    glGenTextures(1, &result->texture);
    glBindTexture(GL_TEXTURE_2D, result->texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, result->width, result->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

#else 

	result->texture = SDL_CreateTexture(windowInfo.renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, result->width, result->height);
	SDL_SetTextureBlendMode(result->texture, blendMode);
	uint32 stride = STBI_rgb_alpha * result->width;

	void* pixels;
	int32 pitch;
	SDL_LockTexture(result->texture, NULL, &pixels, &pitch);

	for (int32 y = 0; y < result->height; y++)
	{
		memcpy((uint8*)pixels + (size_t(y) * pitch), image + (size_t(y) * stride), stride);
	}

	SDL_UnlockTexture(result->texture);
#endif

	stbi_image_free(image);
	return result;
}

//void LoadFonts(const char* fileNames[])
//{
//    int32 nameStart = 0;
//    for (auto& name : fileNames)
//    {
//        fonts[name] = CreateFont(name, SDL_BLENDMODE_BLEND, 32, 20, 16);
//        fonts["1"] = CreateFont("Text.png", SDL_BLENDMODE_BLEND, 32, 20, 16);
//    }
//}

FontSprite* CreateFont(const char* name, SDL_BlendMode blendMode, int32 charSize, int32 actualCharWidth, int32 charPerRow)
{
    Sprite* SP = CreateSprite(name, blendMode);;
    if (SP == nullptr)
        return nullptr;

    FontSprite* result = new FontSprite();
    result->sprite = SP;
    result->charSize = charSize;
    result->xCharSize = actualCharWidth;
    result->charPerRow = charPerRow;
    return result;
}


SDL_Rect CameraOffset(Vector gameLocation, Vector gameSize)
{
    VectorInt location = BlockToPixel(gameLocation);
    VectorInt size = BlockToPixel(gameSize);
    int32 xOffset = BlockToPixel(camera.position.x) - windowInfo.width / 2;
    int32 yOffset = BlockToPixel(camera.position.y) - windowInfo.height / 2;

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
    int32 xOffset = BlockToPixel(camera.position.x) - windowInfo.width / 2;
    int32 yOffset = BlockToPixel(camera.position.y) - windowInfo.height / 2;
    return { input.x + xOffset, windowInfo.height - input.y + yOffset };
}


void BackgroundRender(Sprite* sprite, Camera* camera)
{
    //HARDCODING FOR NOW UNTIL WINDOW SIZE CHANGES
    assert(windowInfo.width == 1280 && windowInfo.height == 720);

    SDL_Rect spriteRect = {};

    //topLeft
    spriteRect.w = windowInfo.width / 2;    //360
    spriteRect.h = windowInfo.height / 2;   //640
    spriteRect.x = 0;
    spriteRect.y = sprite->height - spriteRect.h;


    //checking to make sure the sprite doesnt go further left than the actual sprite and go beyond the border on the right;
    if (camera->position.x > 0)
    {
        int32 spriteRectXOffset = int32(camera->position.x * 2);
        if (spriteRectXOffset < sprite->width - spriteRect.w)
            spriteRect.x = spriteRectXOffset;
        else
            spriteRect.x = sprite->width - spriteRect.w;
    }

    if (camera->position.y > 0)
    {
        int32 spriteRectYOffset = spriteRect.h + int32(camera->position.y / 2);
        if (spriteRectYOffset < sprite->height)
            spriteRect.y = sprite->height - spriteRectYOffset;
        else
            spriteRect.y = 0;
    }


    AddTextureToRender(spriteRect, {}, RenderPrio::Sprites, sprite->texture, {}, 0, NULL, SDL_FLIP_NONE);
}


void SpriteMapRender(Sprite* sprite, int32 i, int32 itemSize, int32 xCharSize, Vector loc)
{
    int32 blocksPerRow = sprite->width / itemSize;
    int32 x = uint32(i) % blocksPerRow * itemSize + (itemSize - xCharSize) / 2;
    int32 y = uint32(i) / blocksPerRow * itemSize;

    SDL_Rect blockRect = { x, y, xCharSize, itemSize };
    float itemSizeTranslatedx = PixelToBlock(xCharSize);
    float itemSizeTranslatedy = PixelToBlock(itemSize);
    SDL_Rect destRect = CameraOffset(loc, { itemSizeTranslatedx, itemSizeTranslatedy });

    AddTextureToRender(blockRect, destRect, RenderPrio::Sprites, sprite->texture, {}, 0, NULL, SDL_FLIP_NONE);
}


void SpriteMapRender(Sprite* sprite, const Block& block)
{
    SpriteMapRender(sprite, block.flags, blockSize, blockSize, block.location);
}



void DrawText(FontSprite* fontSprite, SDL_Color c, const std::string& text, float size, VectorInt loc, UIX XLayout, UIY YLayout)
{
    //ABC
    int32 CPR = fontSprite->charPerRow;

    //Upper Left corner and size
    SDL_Rect SRect = {};
    SDL_Rect DRect = {};

    SRect.w = fontSprite->xCharSize;
    SRect.h = fontSprite->charSize;
    DRect.w = int(SRect.w * size);
    DRect.h = int(SRect.h * size);

    for (int32 i = 0; i < text.size(); i++)
    {
        int32 charKey = text[i] - ' ';

        SRect.x = uint32(charKey) % CPR * SRect.h + (SRect.h - SRect.w) / 2;
        SRect.y = uint32(charKey) / CPR * SRect.h;

        DRect.x = loc.x + i * DRect.w - (int32(XLayout) * DRect.w * int32(text.size())) / 2;
        DRect.y = loc.y - (int32(YLayout) * DRect.h) / 2;
        AddTextureToRender(SRect, DRect, RenderPrio::UI, fontSprite->sprite->texture, c, 0, NULL, SDL_FLIP_NONE);
        //SDL_RenderCopyEx(windowInfo.renderer, fontSprite->sprite->texture, &SRect, &DRect, 0, NULL, SDL_FLIP_NONE);
    }
}

SDL_Rect RectangleToSDL(Rectangle rect, bool yFlip)
{
    if (yFlip)
		return { (int32)rect.bottomLeft.x, windowInfo.height - (int32)rect.topRight.y, (int32)rect.Width(), (int32)rect.Height() };
    else
		return { (int32)rect.bottomLeft.x, (int32)rect.bottomLeft.y, (int32)rect.Width(), (int32)rect.Height() };
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
bool DrawButton(FontSprite* textSprite, const std::string& text, VectorInt loc,
                UIX XLayout, UIY YLayout, SDL_Color BC, SDL_Color TC,
                VectorInt mouseLoc, bool mousePressed)
{
    bool result = false;
    //Copied from DrawText()
    //TODO(choman): change this to be more fluid and remove most of the dependancies.

    SDL_Rect sdlButRect = {};
    int32 widthOffset = 5;
    int32 heightOffset = 5;

    sdlButRect.w = textSprite->xCharSize * int32(text.size()) + widthOffset * 2;
    sdlButRect.h = (textSprite->charSize + heightOffset * 2);
    int32 xOffset = (int32(XLayout) * sdlButRect.w) / 2;
    int32 yOffset = (int32(YLayout) * sdlButRect.h) / 2; //top, mid, bot

    sdlButRect.x = loc.x - xOffset;
    sdlButRect.y = loc.y - yOffset;

    Rectangle butRect = SDLToRectangle(sdlButRect);
    AddRectToRender(RenderType::DebugOutline, butRect, BC, RenderPrio::UI);

    //VectorInt mousePosition;
    //bool leftButtonPressed = (SDL_GetMouseState(&mousePosition.x, &mousePosition.y) & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;

    if (SDLPointRectangleCollision(mouseLoc, butRect))
    {
        AddRectToRender(RenderType::DebugFill, butRect, { BC.r, BC.g, BC.b, BC.a / (uint32)2}, RenderPrio::UI);
    }
    if (mousePressed)
    {
        if (SDLPointRectangleCollision(mouseLoc, butRect))
        {
			AddRectToRender(RenderType::DebugFill, butRect, BC, RenderPrio::UI);
            result = true;
        }
    }
    DrawText(textSprite, TC, text, 1.0f, { sdlButRect.x + sdlButRect.w / 2, sdlButRect.y + sdlButRect.h / 2 }, UIX::mid, UIY::mid);
    return result;
}

Sprite* GetSpriteFromAnimation(Actor* actor)
{
    return actor->currentSprite;
}

void RenderBlocks()
{
	Vector offset = { float(windowInfo.width / blockSize / 2) + 1, float(windowInfo.height / blockSize / 2) + 1 };

	for (float y = camera.position.y - offset.y; y < (camera.position.y + offset.y); y++)
	{
		for (float x = camera.position.x - offset.x; x < (camera.position.x + offset.x); x++)
		{
			Block* block = currentLevel->blocks.TryGetBlock({ x, y });
			if (block != nullptr && block->tileType != TileType::invalid)
			{
				SpriteMapRender(sprites["spriteMap"], *block);

				if (debugList[DebugOptions::blockCollision] && block->tileType != TileType::invalid)
				{
					Rectangle blockRect;
					blockRect.bottomLeft = block->location;
					blockRect.topRight = { block->location.x + 1 , block->location.y + 1 };

					AddRectToRender(blockRect, lightRed, RenderPrio::Debug);
				}
			}

		}
	}
}

void RenderMovingPlatforms()
{
    for (int32 i = 0; i < currentLevel->movingPlatforms.size(); i++)
    {
        currentLevel->movingPlatforms[i]->Render();
    }
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
        Sprite* sprite = GetSpriteFromAnimation(&laser);

        SDL_Rect rect = CameraOffset(laser.position, { Pythags(laser.destination - laser.position), PixelToBlock(sprite->height) });
        static SDL_Point rotationPoint = { 0, rect.h / 2 };
        AddTextureToRender({}, rect, RenderPrio::Sprites, sprite->texture, PC, laser.rotation, &rotationPoint, SDL_FLIP_NONE);
    }
}


void RenderActor(Actor* actor, float rotation)
{
    SDL_Rect destRect = {};
    SDL_RendererFlip flippage = SDL_FLIP_NONE;
    int32 xPos;
    Sprite* sprite = GetSpriteFromAnimation(actor);

    if (actor->lastInputWasLeft && actor->GetActorType() != ActorType::projectile)
    {
        flippage = SDL_FLIP_HORIZONTAL;
        xPos = sprite->width - actor->animationList->colRect.topRight.x;
    }
    else
    {
        xPos = actor->animationList->colRect.bottomLeft.x;
    }

    destRect = CameraOffset({ actor->position.x - PixelToBlock((int(xPos * actor->SpriteRatio()))),
                              actor->position.y - PixelToBlock((int(actor->animationList->colRect.bottomLeft.y * actor->SpriteRatio()))) },
        PixelToBlock({ (int)(actor->SpriteRatio() * sprite->width),
                       (int)(actor->SpriteRatio() * sprite->height) }));
    AddTextureToRender({}, destRect, RenderPrio::Sprites, sprite->texture, actor->colorMod, rotation, NULL, flippage);
}

void GameSpaceRectRender(Rectangle rect, SDL_Color color)
{
    Vector offset = PixelToBlock({ int32(BlockToPixel(rect.Width())), int32(rect.Height()) });
    Rectangle tempRect = { {rect.bottomLeft.x, rect.bottomLeft.y}, {rect.bottomLeft.x + offset.x, rect.bottomLeft.y + offset.y} };
    AddRectToRender(RenderType::DebugFill, tempRect, color, RenderPrio::Debug );
}
