#include "Rendering.h"
#include "Entity.h"
#include "Misc.h"
#include "Math.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "GL/glew.h"
#include "WinUtilities.h"
#include <cassert>
#include <algorithm>

WindowInfo windowInfo = { 200, 200, 1280, 720 };
std::unordered_map<std::string, Sprite*> sprites;
std::unordered_map<std::string, FontSprite*> fonts;
Camera camera;

void CheckError()
{
    while (GLenum error = glGetError())
    {
        DebugPrint("Error: %d\n", error);
    }
}

const char* vertexShaderText = R"term(
#version 330

layout(location = 0) in vec2 i_position;
layout(location = 1) in vec2 i_uv;

uniform mat4 u_ortho;
uniform float u_textureWidth;
uniform float u_textureHeight;

out vec2 o_uv;
void main()
{
    o_uv.x = i_uv.x / u_textureWidth;
    o_uv.y = i_uv.y / u_textureHeight;
    gl_Position = u_ortho * vec4(i_position, 0, 1);
}
)term";

const char* pixelShaderText = R"term(
#version 330

uniform sampler2D sampler;
uniform vec4 u_color;

in vec2 o_uv;
out vec4 color;
void main()
{
    color = u_color * texture(sampler, o_uv);
}
)term";

std::vector<Vertex> vertexBuffer;

WindowInfo& GetWindowInfo()
{
    return windowInfo;
}

void CreateOpenGLWindow()
{
    SDL_Init(SDL_INIT_VIDEO);
    windowInfo.SDLWindow = SDL_CreateWindow("Jumper", windowInfo.left, windowInfo.top, windowInfo.width, windowInfo.height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GLContext GLContext = SDL_GL_CreateContext(windowInfo.SDLWindow);
	SDL_GL_MakeCurrent(windowInfo.SDLWindow, GLContext);

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
		DebugPrint("Error: %s\n", glewGetErrorString(err));
	}
	DebugPrint("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
	glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glClearColor(0,0.5f,0.5f,0);
    glClearColor(0,0.0f,0.0f,0);
}

static std::vector<RenderInformation> drawCalls;

struct OpenGLInfo
{
    GLuint program;
    GLuint orthoLocation;
    GLuint widthLocation;
    GLuint heightLocation;
    GLuint colorLocation;
    GLuint vao;
    GLuint vertexBuffer;
    GLuint whiteTexture;
}GLInfo = {};

void AddTextureToRender(SDL_Rect sRect, Rectangle dRect, RenderPrio priority,
    Sprite* sprite, SDL_Color colorMod, float rotation,
    const SDL_Point* rotationPoint, SDL_RendererFlip flippage, CoordinateSpace coordSpace)
{
    RenderInformation info;
    info.renderType = RenderType::Texture;
	info.sRect = sRect;
	info.dRect = dRect;
    float height = info.dRect.Height();
	info.dRect.botLeft.y += height;
	info.dRect.topRight.y -= height;

    info.prio = priority;
    info.color = colorMod;
    info.coordSpace = coordSpace;

    info.texture.texture = sprite->texture;
    info.texture.rotation = rotation;
    info.texture.flippage = flippage;
    info.texture.rotationPoint = rotationPoint;
    info.texture.width = sprite->width;
    info.texture.height = sprite->height;

    drawCalls.push_back(info);
}

void AddRectToRender(RenderType type, Rectangle rect, SDL_Color color, RenderPrio prio, CoordinateSpace coordSpace)
{

    RenderInformation renderInformation = {};
	renderInformation.renderType = type;
	renderInformation.dRect = rect;
    renderInformation.color = color;
    renderInformation.prio = prio;
    renderInformation.coordSpace = coordSpace;

    renderInformation.texture.width = 1;
    renderInformation.texture.height= 1;
    renderInformation.texture.texture = GLInfo.whiteTexture;
    drawCalls.push_back(renderInformation);
}

void AddRectToRender(Rectangle rect, SDL_Color color, RenderPrio prio, CoordinateSpace coordSpace)
{
    AddRectToRender(RenderType::DebugOutline, rect, color, prio, coordSpace);
    AddRectToRender(RenderType::DebugFill, rect, color, prio, coordSpace);
}

GLuint JMP_CreateTexture(int32 width, int32 height, uint8* data)
{
    GLuint result;
    glGenTextures(1, &result);
    glBindTexture(GL_TEXTURE_2D, result);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    return result;
}

GLuint JMP_CreateShader(GLenum shaderType, const char* shaderText)
{
    GLuint result = glCreateShader(shaderType);
	glShaderSource(result, 1, &shaderText, NULL);
    glCompileShader(result);

	GLint status;
	glGetShaderiv(result, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{

		GLint log_length;
		glGetShaderiv(result, GL_INFO_LOG_LENGTH, &log_length);
		GLchar info[4096];
		glGetShaderInfoLog(result, log_length, NULL, info);
		DebugPrint("CreateShader() Shader compilation error: %s\n", info);
	}

    return result;
}

GLuint JMP_CreateShaderProgram()
{
    GLuint vertexShaderID = JMP_CreateShader(GL_VERTEX_SHADER, vertexShaderText);
    GLuint pixelShaderID = JMP_CreateShader(GL_FRAGMENT_SHADER, pixelShaderText);
    
    GLuint result = glCreateProgram();
    glAttachShader(result , vertexShaderID);
    glAttachShader(result, pixelShaderID);
    glLinkProgram(result);
    
    GLint status;
	glGetProgramiv(result, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint log_length;
		glGetProgramiv(result, GL_INFO_LOG_LENGTH, &log_length);
		GLchar info[4096];
		glGetProgramInfoLog(result, log_length, NULL, info);
		DebugPrint("Shader linking error: %s\n", info);
	}
    return result;
}

void InitializeOpenGL()
{
    glGenVertexArrays(1, &GLInfo.vao);
    glBindVertexArray(GLInfo.vao);

    GLInfo.program = JMP_CreateShaderProgram();

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

	GLInfo.orthoLocation = glGetUniformLocation(GLInfo.program, "u_ortho");
	GLInfo.widthLocation = glGetUniformLocation(GLInfo.program, "u_textureWidth");
	GLInfo.heightLocation = glGetUniformLocation(GLInfo.program, "u_textureHeight");
    GLInfo.colorLocation = glGetUniformLocation(GLInfo.program, "u_color");

	glGenBuffers(1, &GLInfo.vertexBuffer);

	Uint8 image[] = {255, 255, 255, 255,};
    GLInfo.whiteTexture = JMP_CreateTexture(1, 1, image);
	glLineWidth(1.0f);
}

int32 size = 0;

void RenderDrawCalls()
{
    PROFILE_FUNCTION();

    vertexBuffer.clear();
    {

		PROFILE_SCOPE("Vertex Buffer Update");
		uint32 i = 0;
		for (auto& item : drawCalls)
		{
			item.vertexIndex = (int32)vertexBuffer.size();
			item.vertexLength = 4;
			item.prioIndex = i++;
			if (item.renderType != RenderType::Texture)
			{

				float L = item.dRect.botLeft.x;
				float R = item.dRect.topRight.x;
				float T = item.dRect.botLeft.y;
				float B = item.dRect.topRight.y;
				vertexBuffer.push_back({ L, T, 1.0f, 1.0f });
				vertexBuffer.push_back({ R, T, 1.0f, 1.0f });
				vertexBuffer.push_back({ R, B, 1.0f, 1.0f });
				vertexBuffer.push_back({ L, B, 1.0f, 1.0f });
			}
			else
			{

				float uvxo = (float)item.sRect.x;
				float uvyo = (float)item.sRect.y;
				float uvxd = (float)item.sRect.x + (float)item.sRect.w;
				float uvyd = (float)item.sRect.y + (float)item.sRect.h;
				if (item.sRect.w == 0)
					uvxd = (float)item.texture.width;
				if (item.sRect.h == 0)
					uvyd = (float)item.texture.height;
				std::swap(uvyo, uvyd);

				float posxo = item.dRect.botLeft.x;
				float posyo = item.dRect.botLeft.y;
				float posxd = item.dRect.topRight.x;
				float posyd = item.dRect.topRight.y;
				if (item.dRect.Width() == 0)
					posxd = (float)windowInfo.width;
				if (item.dRect.Height() == 0)
					posyd = (float)windowInfo.height;

				if (item.texture.flippage)
					std::swap(posxo, posxd);

				vertexBuffer.push_back({ posxo, posyd, uvxo, uvyo });
				vertexBuffer.push_back({ posxo, posyo, uvxo, uvyd });
				vertexBuffer.push_back({ posxd, posyd, uvxd, uvyo });
				vertexBuffer.push_back({ posxd, posyo, uvxd, uvyd });
			}
		}

	}
	{

        PROFILE_SCOPE("Buffer Upload");
		int32 newSize = (int32)vertexBuffer.size() * sizeof(vertexBuffer[0]);
		glBindBuffer(GL_ARRAY_BUFFER, GLInfo.vertexBuffer);
		if (newSize > size)
		{
			size = newSize * 2;
			glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_DRAW);
		}
		glBufferSubData(GL_ARRAY_BUFFER, 0, newSize, vertexBuffer.data());
	}
	{

        PROFILE_SCOPE("Sort");
		std::sort(drawCalls.begin(), drawCalls.end(), [](const RenderInformation& a, const RenderInformation& b) {
			if (a.prio == b.prio)
				return (a.prioIndex < b.prioIndex);
			else
				return (a.prio < b.prio);
		});
	}

    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, windowInfo.width, windowInfo.height);

    //world and UI matrix
    float windowWidth = (float)windowInfo.width;
    float windowHeight = (float)windowInfo.height;
    Vector cameraPixel;
    //cameraPixel.x = (float)BlockToPixel(camera.position.x);
    //cameraPixel.y = (float)BlockToPixel(camera.position.y);

    gbMat4 worldMatrix;
	gb_mat4_ortho2d(&worldMatrix, camera.position.x - camera.size.x / 2, camera.position.x + camera.size.x / 2, camera.position.y - camera.size.y / 2, camera.position.y + camera.size.y / 2);
    gbMat4 UIMatrix;
	gb_mat4_ortho2d(&UIMatrix, 0, windowWidth, windowHeight, 0);

	glBindBuffer(GL_ARRAY_BUFFER, GLInfo.vertexBuffer);
	glBindVertexArray(GLInfo.vao);
	glEnableVertexArrayAttrib(GLInfo.vao, 0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
	glEnableVertexArrayAttrib(GLInfo.vao, 1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));
	glUseProgram(GLInfo.program);

    {
        PROFILE_SCOPE("RenderCalls");
        for (auto& item : drawCalls)
        {
            switch (item.renderType)
            {
            case RenderType::DebugFill:
            case RenderType::Texture:
            {

                float width = (float)item.texture.width;
                float height = (float)item.texture.height;
                glUniform1f(GLInfo.widthLocation, width);
                glUniform1f(GLInfo.heightLocation, height);
                if (item.color.r != 0 || item.color.g != 0 || item.color.b != 0 || item.color.a != 0)
                    glUniform4f(GLInfo.colorLocation, item.color.r / 255.0f, item.color.g / 255.0f, item.color.b / 255.0f, item.color.a / 255.0f);
                else
                    glUniform4f(GLInfo.colorLocation, 1.0f, 1.0f, 1.0f, 1.0f);

                glBindTexture(GL_TEXTURE_2D, item.texture.texture);

                gbMat4 orthoMatrix;
                if (item.coordSpace == CoordinateSpace::World)
                    orthoMatrix = worldMatrix;
                else if (item.coordSpace == CoordinateSpace::UI)
                    orthoMatrix = UIMatrix;
                glUniformMatrix4fv(GLInfo.orthoLocation, 1, GL_FALSE, orthoMatrix.e);

                glDrawArrays(GL_TRIANGLE_STRIP, item.vertexIndex, item.vertexLength);

                break;
            }
            case RenderType::DebugOutline:
			{

				glBindTexture(GL_TEXTURE_2D, GLInfo.whiteTexture);

				glUniform1f(GLInfo.widthLocation, 1);
				glUniform1f(GLInfo.heightLocation, 1);

				gbMat4 orthoMatrix;
				if (item.coordSpace == CoordinateSpace::World)
					orthoMatrix = worldMatrix;
				else if (item.coordSpace == CoordinateSpace::UI)
					orthoMatrix = UIMatrix;
				glUniformMatrix4fv(GLInfo.orthoLocation, 1, GL_FALSE, orthoMatrix.e);

				if (item.color.r != 0 || item.color.g != 0 || item.color.b != 0 || item.color.a != 0)
					glUniform4f(GLInfo.colorLocation, item.color.r / 255.0f, item.color.g / 255.0f, item.color.b / 255.0f, item.color.a / 255.0f);
				else
					glUniform4f(GLInfo.colorLocation, 1.0f, 1.0f, 1.0f, 1.0f);

				glDrawArrays(GL_LINE_LOOP, item.vertexIndex, item.vertexLength);
				break;
			}
			}
        }
    }
    drawCalls.clear();
}


Sprite* CreateSprite(const char* name, SDL_BlendMode blendMode)
{
	int32 colorChannels;
	Sprite* result = new Sprite();
	unsigned char* image = stbi_load(name, &result->width, &result->height, &colorChannels, STBI_rgb_alpha);

	if (image == nullptr)
		return nullptr;

    result->texture= JMP_CreateTexture(result->width, result->height, image);
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

//difference between the player and the center of the screen
Vector CameraToPixelCoord(Vector input)
{
    int32 xOffset = BlockToPixel(camera.position.x) - windowInfo.width / 2;
    int32 yOffset = BlockToPixel(camera.position.y) - windowInfo.height / 2;
    return { input.x + xOffset, windowInfo.height - input.y + yOffset };
}


void BackgroundRender(Sprite* sprite, Camera* camera)
{
    //HARDCODING FOR NOW UNTIL WINDOW SIZE CHANGES
    //assert(windowInfo.width == 1280 && windowInfo.height == 720);

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


    AddTextureToRender(spriteRect, {}, RenderPrio::Sprites, sprite, {}, 0, NULL, SDL_FLIP_NONE, CoordinateSpace::UI);
}


void SpriteMapRender(Sprite* sprite, int32 i, int32 itemSize, int32 xCharSize, Vector loc)
{
    int32 blocksPerRow = sprite->width / itemSize;
    int32 x = uint32(i) % blocksPerRow * itemSize + (itemSize - xCharSize) / 2;
    int32 y = uint32(i) / blocksPerRow * itemSize;
    
    SDL_Rect blockRect = { x, y, xCharSize, itemSize };
    float itemSizeTranslatedx = PixelToBlock(xCharSize);
    float itemSizeTranslatedy = PixelToBlock(itemSize);

    Rectangle destRect = { loc, { loc.x + itemSizeTranslatedx, loc.y +itemSizeTranslatedy } };

    AddTextureToRender(blockRect, destRect, RenderPrio::Sprites, sprite, {}, 0, NULL, SDL_FLIP_NONE, CoordinateSpace::World);
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
    //SDL_Rect DRect = {};

    SRect.w = fontSprite->xCharSize;
    SRect.h = fontSprite->charSize;
    float width = (SRect.w * size);
    float height = (SRect.h * size);
    //DRect.w = int(SRect.w * size);
    //DRect.h = int(SRect.h * size);

    for (int32 i = 0; i < text.size(); i++)
    {
        int32 charKey = text[i] - ' ';

        SRect.x = uint32(charKey) % CPR * SRect.h + (SRect.h - SRect.w) / 2;
        SRect.y = uint32(charKey) / CPR * SRect.h;

        Rectangle DRectangle;
        DRectangle.botLeft.x = loc.x + i * width - (int32(XLayout) * width * int32(text.size())) / 2;
        DRectangle.botLeft.y = loc.y - (int32(YLayout) * height) / 2;
        DRectangle.topRight = { DRectangle.botLeft.x + width, DRectangle.botLeft.y + height };

        float height2 = DRectangle.Height();
        DRectangle.botLeft.y += height2;
        DRectangle.topRight.y -= height2;
        AddTextureToRender(SRect, DRectangle, RenderPrio::UI, fontSprite->sprite, c, 0, NULL, SDL_FLIP_NONE, CoordinateSpace::UI);
    }
}

SDL_Rect RectangleToSDL(Rectangle rect, bool yFlip)
{
	return { (int32)rect.botLeft.x, (int32)rect.botLeft.y, (int32)rect.Width(), (int32)rect.Height() };
}

//bottom left and top right
Rectangle SDLToRectangle(SDL_Rect rect)
{
    return { { (float)rect.x, float(rect.y + rect.h) }, { float(rect.x + rect.w), (float)rect.y } };
}

//camera space
bool pointRectangleCollision(VectorInt point, Rectangle rect)
{
    if (point.x > rect.botLeft.x && point.x < rect.topRight.x)
        if (point.y > rect.botLeft.y && point.y < rect.topRight.y)
            return true;
    return false;
}


bool SDLPointRectangleCollision(VectorInt point, Rectangle rect)
{
    if ((float)point.x > rect.botLeft.x && (float)point.x < rect.topRight.x)
        if ((float)point.y > rect.botLeft.y && (float)point.y < rect.topRight.y)
            return true;
    return false;
}


//top left 0,0
bool DrawButton(FontSprite* textSprite, const std::string& text, VectorInt loc,
                UIX XLayout, UIY YLayout, SDL_Color BC, SDL_Color TC,
                VectorInt mouseLoc, bool mousePressed)
{
    bool result = false;
    //Copied from DrawText()
    //TODO(choman): change this to be more fluid and remove most of the dependancies.

    Rectangle rect = {};
    float widthOffset = 5;
    float heightOffset = 5;
    float width = textSprite->xCharSize * float(text.size()) + widthOffset * 2;
    float height = (textSprite->charSize + heightOffset * 2);

    rect.botLeft.x = loc.x - ((float(XLayout) * width) / 2);
    rect.botLeft.y = loc.y - ((float(YLayout) * height) / 2);//top, mid, bot
    rect.topRight.x = rect.botLeft.x + width;
    rect.topRight.y = rect.botLeft.y + height;

    AddRectToRender(RenderType::DebugOutline, rect, BC, RenderPrio::UI, CoordinateSpace::UI);

    //VectorInt mousePosition;
    //bool leftButtonPressed = (SDL_GetMouseState(&mousePosition.x, &mousePosition.y) & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;

    if (SDLPointRectangleCollision(mouseLoc, rect))
    {
        AddRectToRender(RenderType::DebugFill, rect, { BC.r, BC.g, BC.b, BC.a / (uint32)2}, RenderPrio::UI, CoordinateSpace::UI);
    }
    if (mousePressed)
    {
        if (SDLPointRectangleCollision(mouseLoc, rect))
        {
			AddRectToRender(RenderType::DebugFill, rect, BC, RenderPrio::UI, CoordinateSpace::UI);
            result = true;
        }
    }
    DrawText(textSprite, TC, text, 1.0f, { int32(rect.botLeft.x + rect.Width() / 2), int32(rect.botLeft.y + rect.Height() / 2) }, UIX::mid, UIY::mid);
    return result;
}

Sprite* GetSpriteFromAnimation(Actor* actor)
{
    return actor->currentSprite;
}

void RenderBlocks()
{
    PROFILE_FUNCTION();

	Vector offset = { float(camera.size.x / 2) + 1, float(camera.size.y / 2) + 1 };
	for (float y = camera.position.y - offset.y; y < (camera.position.y + offset.y); y++)
	{
		for (float x = camera.position.x - offset.x; x < (camera.position.x + offset.x); x++)
		{
			Block* block = currentLevel->blocks.TryGetBlock({ x, y });
			if (block != nullptr && block->tileType != TileType::invalid)
			{
				SpriteMapRender(sprites["spriteMap"], *block);

				if (debugList[DebugOptions::blockCollision] && block->tileType != TileType::invalid)
                    AddRectToRender({ block->location, { block->location.x + 1 , block->location.y + 1 } }, lightRed, RenderPrio::Debug, CoordinateSpace::World);
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

        Rectangle rectangle = { {laser.position}, {laser.position.x + Pythags(laser.destination - laser.position), laser.position.y + PixelToBlock(sprite->height)} };
        static SDL_Point rotationPoint = { 0, (int32)rectangle.Height() / 2 };
        AddTextureToRender({}, rectangle, RenderPrio::Sprites, sprite, PC, laser.rotation, &rotationPoint, SDL_FLIP_NONE, CoordinateSpace::World);
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

    if (sprite == nullptr)
        sprite = actor->animationList->GetAnyValidAnimation()->anime[0];

    Rectangle DR;
    DR.botLeft.x = actor->position.x - PixelToBlock((int(xPos * actor->SpriteRatio())));
    DR.botLeft.y = actor->position.y - PixelToBlock((int(actor->animationList->colRect.bottomLeft.y * actor->SpriteRatio())));
	DR.topRight = DR.botLeft + PixelToBlock({ (int)(actor->SpriteRatio() * sprite->width),
			   (int)(actor->SpriteRatio() * sprite->height) });
    AddTextureToRender({}, DR, RenderPrio::Sprites, sprite, actor->colorMod, rotation, NULL, flippage, CoordinateSpace::World);
}
