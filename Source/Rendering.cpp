#include "Rendering.h"
#include "Entity.h"
#include "Misc.h"
#include "Math.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "GL/glew.h"
#include "WinUtilities.h"
#include "Console.h"
#include "Audio.h"

#include <cassert>
#include <algorithm>

WindowInfo g_windowInfo = { 200, 200, 1280, 720 };
std::unordered_map<std::string, Sprite*> g_sprites;
std::unordered_map<std::string, FontSprite*> g_fonts;
Camera g_camera;
std::vector<Vertex> vertexBuffer;

extern "C" {
#ifdef _MSC_VER
    _declspec(dllexport) uint32_t NvOptimusEnablement = 0x00000001;
    _declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 0x00000001;
#else
    __attribute__((dllexport)) uint32_t NvOptimusEnablement = 0x00000001;
    __attribute__((dllexport)) int AmdPowerXpressRequestHighPerformance = 0x00000001;
#endif
}

static std::vector<RenderInformation> drawCalls;

struct OpenGLInfo
{
    GLuint vao;
    GLuint vertexBuffer;
    GLuint whiteTexture;
    GLuint mainBuffer;
    GLuint mainTexture;
    VectorInt previousWindowSize;
    float time;
}GLInfo = {};

struct ShaderText {
    const char* vertex = nullptr;
    const char* pixel = nullptr;
};

enum ShaderUniform
{
    ShaderUniform_OrthoMatrix,
    ShaderUniform_TextureWidth,
    ShaderUniform_TextureHeight,
    ShaderUniform_Color,
    ShaderUniform_RotationMatrix,
    ShaderUniform_Time,

    ShaderUniform_Count
};

struct ShaderUniformInfo
{
    const char* name;
    int32 variableCount;
};

ShaderUniformInfo s_ShaderUniformInfo[] = {
    { "u_ortho",          16 }, // ShaderUniform_OrthoMatrix,
    { "u_textureWidth",   1  }, // ShaderUniform_TextureWidth,
    { "u_textureHeight",  1  }, // ShaderUniform_TextureHeight,
    { "u_color",          4  }, // ShaderUniform_Color,
    { "u_rotationMatrix", 16 }, // ShaderUniform_RotationMatrix,
    { "u_time",           1  }, // ShaderUniform_Time,
};
static_assert(ARRAY_COUNT(s_ShaderUniformInfo) == ShaderUniform_Count);

class Shader
{
private:

	GLuint CreateShader(GLenum shaderType, const char* shaderText)
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
			ConsoleLog("CreateShader() Shader compilation error: %s\n", info);
            assert(false);
		}
        return result;
    }

public:
    GLuint ID;
    ShaderProgram shader;
    GLuint uniformLocations[ShaderUniform_Count];

    void InitializeUniformLocations()
    {
        for (int32 i = 0; i < ShaderUniform_Count; ++i)
        {
            uniformLocations[i] = glGetUniformLocation(ID, s_ShaderUniformInfo[i].name);
        }
    }

    void SetUniform(ShaderUniform uniform, const float* value)
    {
        GLint location = uniformLocations[uniform];
        if (location != -1)
        {
            switch(s_ShaderUniformInfo[uniform].variableCount)
            {
            case 1:
                glUniform1fv(location, 1, value);
                break;
            case 4:
                glUniform4fv(location, 1, value);
                break;
            case 16:
                glUniformMatrix4fv(location, 1, GL_FALSE, value);
                break;

            default:
                assert(false);
                return;
            }
        }
    }

    void SetUniform(ShaderUniform uniform, int value)
	{
        float v = (float)value;
        SetUniform(uniform, &v);
	}

    Shader(ShaderText text)
	{
		GLuint vertexShaderID = CreateShader(GL_VERTEX_SHADER, text.vertex);
		GLuint pixelShaderID = CreateShader(GL_FRAGMENT_SHADER, text.pixel);

		ID = glCreateProgram();
		glAttachShader(ID, vertexShaderID);
		glAttachShader(ID, pixelShaderID);
		glLinkProgram(ID);

		GLint status;
		glGetProgramiv(ID, GL_LINK_STATUS, &status);
		if (status == GL_FALSE)
		{
			GLint log_length;
			glGetProgramiv(ID, GL_INFO_LOG_LENGTH, &log_length);
			GLchar info[4096];
			glGetProgramInfoLog(ID, log_length, NULL, info);
			ConsoleLog("Shader linking error: %s\n", info);
		}

        InitializeUniformLocations();
	}

	void BindShader(const RenderInformation& info, const gbMat4& orthoMatrix)
	{
        glUseProgram(ID);

		SetUniform(ShaderUniform_OrthoMatrix,   orthoMatrix.e);
		SetUniform(ShaderUniform_TextureWidth,  info.texture.width);
		SetUniform(ShaderUniform_TextureHeight, info.texture.height);
        SetUniform(ShaderUniform_Color,         &info.color.r);
        SetUniform(ShaderUniform_Time,          &GLInfo.time);

		gbMat4 rotationMatrix;
        if (info.texture.rotation)
        {
			gb_mat4_rotate(&rotationMatrix, { 0, 0, 1 }, DegToRad(info.texture.rotation));
			gbMat4 toZeroMatrix;
			gbMat4 fromZeroMatrix;
			gbVec3 vec = { info.dRect.botLeft.x + info.texture.rotationPoint.x, info.dRect.botLeft.y - info.texture.rotationPoint.y, 0 };
			gb_mat4_translate(&toZeroMatrix, -vec);
			gb_mat4_translate(&fromZeroMatrix, vec);
			rotationMatrix = fromZeroMatrix * rotationMatrix * toZeroMatrix;
        }
        else
        {
            gb_mat4_identity(&rotationMatrix);
        }
		SetUniform(ShaderUniform_RotationMatrix, rotationMatrix.e);
	}
};


static std::vector<Rectangle> scissorStack;
void PushScissor(Rectangle scissor)
{
	scissorStack.push_back(scissor);
}

void PopScissor()
{
    assert(!scissorStack.empty());
    if (!scissorStack.empty())
        scissorStack.pop_back();
}

void CheckError()
{
    while (GLenum error = glGetError())
    {
        ConsoleLog("Error: %d\n", error);
    }
}


Shader* shaderPrograms[(int32)ShaderProgram::Count];
const char* vertexShaderText = R"term(
#version 330

layout(location = 0) in vec2 i_position;
layout(location = 1) in vec2 i_uv;

uniform mat4 u_ortho;
uniform float u_textureWidth;
uniform float u_textureHeight;
uniform mat4 u_rotationMatrix;

out vec2 o_uv;
void main()
{
    o_uv.x = i_uv.x / u_textureWidth;
    o_uv.y = i_uv.y / u_textureHeight;
    vec2 position;

    gl_Position = u_ortho * u_rotationMatrix * vec4(i_position, 0, 1);
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

const char* chromaticAberrationText = R"term(
#version 330

uniform sampler2D sampler;
uniform vec4 u_color;
uniform float u_time;

in vec2 o_uv;
out vec4 color;
void main()
{
    vec2 red_shift   = vec2(sin(u_time) * 0.01, cos(u_time) * 0.02);
    vec2 green_shift = vec2(sin(-u_time) * 0.05, cos(-u_time) * -0.01);
    vec2 blue_shift  = vec2(cos(u_time) * 0.01, sin(u_time) * 0.02);

    color.r = (u_color * texture(sampler, o_uv + red_shift)).r;
    color.g = (u_color * texture(sampler, o_uv + green_shift)).g;
    color.b = (u_color * texture(sampler, o_uv + blue_shift)).b;
    color.a = 1.0;
}

)term";

ShaderText shaderTexts[(int32)ShaderProgram::Count] = {
    { vertexShaderText, pixelShaderText },
    { vertexShaderText, chromaticAberrationText },
};

WindowInfo& GetWindowInfo()
{
    return g_windowInfo;
}

void CreateOpenGLWindow()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Init(SDL_INIT_AUDIO);
    g_windowInfo.SDLWindow = SDL_CreateWindow("Jumper", g_windowInfo.left, g_windowInfo.top, g_windowInfo.width, g_windowInfo.height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GLContext GLContext = SDL_GL_CreateContext(g_windowInfo.SDLWindow);
	SDL_GL_MakeCurrent(g_windowInfo.SDLWindow, GLContext);
    //VSYNC HERE:
    SDL_GL_SetSwapInterval(1);

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
        ConsoleLog("Error: %s\n", glewGetErrorString(err));
	}
    ConsoleLog("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
	glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glClearColor(0,0.5f,0.5f,0);
    glClearColor(1.0f,0.0f,1.0f,0.0f);
}


RenderInformation& AllocDrawCall()
{
    RenderInformation info = {};
    if (!scissorStack.empty())
    {
        info.scissor = scissorStack.back();
    }
    drawCalls.push_back(info);
    return drawCalls.back();
}

void AddTextureToRender(Rectangle sRect, Rectangle dRect, RenderPrio priority,
    Sprite* sprite, Color colorMod, float rotation,
    Vector rotationPoint, bool flippage, CoordinateSpace coordSpace)
{
    RenderInformation& info = AllocDrawCall();
    info.renderType = RenderType::Texture;
	info.sRect = sRect;
	info.dRect = dRect;
    float height = info.dRect.Height();
    //flipping height so top is 0 and bottom is max height//
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
}

void AddRectToRender(RenderType type, Rectangle rect, Color color, RenderPrio prio, CoordinateSpace coordSpace)
{

    RenderInformation& renderInformation = AllocDrawCall();
	renderInformation.renderType = type;
	renderInformation.dRect = rect;
    renderInformation.color = color;
    renderInformation.prio = prio;
    renderInformation.coordSpace = coordSpace;

    renderInformation.texture.width = 1;
    renderInformation.texture.height= 1;
    renderInformation.texture.texture = GLInfo.whiteTexture;
}

void AddRectToRender(Rectangle rect, Color color, RenderPrio prio, CoordinateSpace coordSpace)
{
    AddRectToRender(RenderType::DebugOutline, rect, color, prio, coordSpace);
    AddRectToRender(RenderType::DebugFill, rect, color, prio, coordSpace);
}

GLuint JMP_CreateTexture(int32 width, int32 height, uint8* data = nullptr)
{
    GLuint result;
    glGenTextures(1, &result);
    glBindTexture(GL_TEXTURE_2D, result);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //TODO: impliment a way to switch between GL_CLAMP_TO_EDGE and GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    return result;
}

void InitializeOpenGL()
{
    glGenVertexArrays(1, &GLInfo.vao);
    glBindVertexArray(GLInfo.vao);

    for (int32 i = 0; i < (int32)ShaderProgram::Count; i++)
		shaderPrograms[i] = new Shader(shaderTexts[i]);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glGenFramebuffers(1, &GLInfo.mainBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, GLInfo.mainBuffer);
    GLInfo.mainTexture = JMP_CreateTexture(g_windowInfo.width, g_windowInfo.height, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GLInfo.mainTexture, 0);

	glGenBuffers(1, &GLInfo.vertexBuffer);

	Uint8 image[] = {255, 255, 255, 255,};
    GLInfo.whiteTexture = JMP_CreateTexture(1, 1, image);
	glLineWidth(1.0f);
    GLInfo.previousWindowSize = { g_windowInfo.width, g_windowInfo.height };
}


int32 size = 0;
void RenderDrawCalls(float dt)
{
    GLInfo.time += dt;
    PROFILE_FUNCTION();
    assert(scissorStack.empty()); // Unbalanced Push/Pop of scissor rectangles!
    scissorStack.clear();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, g_windowInfo.width, g_windowInfo.height);

	glBindFramebuffer(GL_FRAMEBUFFER, GLInfo.mainBuffer);
	glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, g_windowInfo.width, g_windowInfo.height);

    Sprite mainTextureSprite = { GLInfo.mainTexture, g_windowInfo.width, g_windowInfo.height };
    Rectangle sRect;
    sRect.botLeft = { 0, 0 };
    sRect.topRight = { (float)g_windowInfo.width, (float)g_windowInfo.height };

    Rectangle dRect;
    dRect.botLeft = { 0, 0 };
    dRect.topRight = { (float)g_windowInfo.width, (float)g_windowInfo.height };

    AddTextureToRender(sRect, dRect, RenderPrio::PostProcess, &mainTextureSprite, White, 0, {}, 0, CoordinateSpace::UI);
    //drawCalls.back().shader = ShaderProgram::ChromaticAberration;

    if (GLInfo.previousWindowSize.x != g_windowInfo.width || GLInfo.previousWindowSize.x != g_windowInfo.height )
    {
		glBindTexture(GL_TEXTURE_2D, GLInfo.mainTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_windowInfo.width, g_windowInfo.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        GLInfo.previousWindowSize = { g_windowInfo.width, g_windowInfo.height };
    }

    vertexBuffer.clear();
    {

		PROFILE_SCOPE("Vertex Buffer Update");
		uint32 i = 0;
		for (auto& item : drawCalls)
		{
			item.vertexIndex = (int32)vertexBuffer.size();
			item.vertexLength = 4;
			item.prioIndex = i++;
			if (item.renderType == RenderType::Texture)
			{

				float uvxo = item.sRect.botLeft.x;
				float uvyo = item.sRect.botLeft.y;
				float uvxd = item.sRect.topRight.x;
				float uvyd = item.sRect.topRight.y;
				if (item.sRect.Width() == 0)
					uvxd = (float)item.texture.width;
				if (item.sRect.Height() == 0)
					uvyd = (float)item.texture.height;
				std::swap(uvyo, uvyd);

				float posxo = item.dRect.botLeft.x;
				float posyo = item.dRect.botLeft.y;
				float posxd = item.dRect.topRight.x;
				float posyd = item.dRect.topRight.y;
				if (item.dRect.Width() == 0)
					posxd = (float)g_windowInfo.width;
				if (item.dRect.Height() == 0)
					posyd = (float)g_windowInfo.height;

				if (item.texture.flippage)
					std::swap(posxo, posxd);
				vertexBuffer.push_back({ posxo, posyd, uvxo, uvyo });
				vertexBuffer.push_back({ posxo, posyo, uvxo, uvyd });
				vertexBuffer.push_back({ posxd, posyd, uvxd, uvyo });
				vertexBuffer.push_back({ posxd, posyo, uvxd, uvyd });

			}
            else if (item.renderType == RenderType::DebugFill)
            {

				float L = item.dRect.botLeft.x;
				float R = item.dRect.topRight.x;
				float T = item.dRect.botLeft.y;
				float B = item.dRect.topRight.y;
				vertexBuffer.push_back({ R, B, 1.0f, 1.0f });
				vertexBuffer.push_back({ L, B, 1.0f, 1.0f });
				vertexBuffer.push_back({ R, T, 1.0f, 1.0f });
				vertexBuffer.push_back({ L, T, 1.0f, 1.0f });
            }
			else if (item.renderType == RenderType::DebugOutline)
			{

				float L = item.dRect.botLeft.x;
				float R = item.dRect.topRight.x;
				float T = item.dRect.botLeft.y;
				float B = item.dRect.topRight.y;
				vertexBuffer.push_back({ R, B, 1.0f, 1.0f });
				vertexBuffer.push_back({ L, B, 1.0f, 1.0f });
				vertexBuffer.push_back({ L, T, 1.0f, 1.0f });
				vertexBuffer.push_back({ R, T, 1.0f, 1.0f });
			}
            else
            {
                assert(false);
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


    //world and UI matrix
    float windowWidth = (float)g_windowInfo.width;
    float windowHeight = (float)g_windowInfo.height;
    Vector cameraPixel;

    gbMat4 worldMatrix;
	gb_mat4_ortho2d(&worldMatrix, g_camera.position.x - g_camera.size.x / 2, g_camera.position.x + g_camera.size.x / 2, g_camera.position.y - g_camera.size.y / 2, g_camera.position.y + g_camera.size.y / 2);
    gbMat4 UIMatrix;
	gb_mat4_ortho2d(&UIMatrix, 0, windowWidth, windowHeight, 0);


	glBindBuffer(GL_ARRAY_BUFFER, GLInfo.vertexBuffer);
	glBindVertexArray(GLInfo.vao);
	glEnableVertexArrayAttrib(GLInfo.vao, 0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
	glEnableVertexArrayAttrib(GLInfo.vao, 1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));

    Rectangle currentScissor = {};
    bool scissorEnabled = false;

    {
        PROFILE_SCOPE("RenderCalls");
        for (auto& item : drawCalls)
        {
            if (item.scissor.Height() && item.scissor.Width())
            {
                if (!scissorEnabled)
                    glEnable(GL_SCISSOR_TEST);
                scissorEnabled = true;
                if (currentScissor != item.scissor)
                {
                    currentScissor = item.scissor;
                    glScissor(static_cast<GLsizei>(item.scissor.botLeft.x),
                              static_cast<GLsizei>(item.scissor.botLeft.y),
                              static_cast<GLsizei>(item.scissor.Width()),
                              static_cast<GLsizei>(item.scissor.Height()));
                }
            }
            else
            {
                if (scissorEnabled)
                    glDisable(GL_SCISSOR_TEST);
                scissorEnabled = false;
                currentScissor = {};
            }

			Shader* shader = shaderPrograms[(int32)item.shader];

			gbMat4 orthoMatrix;
			if (item.coordSpace == CoordinateSpace::World)
				orthoMatrix = worldMatrix;
			else if (item.coordSpace == CoordinateSpace::UI)
				orthoMatrix = UIMatrix;
			else
				assert(false);

            if (item.prio == RenderPrio::PostProcess)
            {
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glDisable(GL_BLEND);
            }
            else
            {
                glBindFramebuffer(GL_FRAMEBUFFER, GLInfo.mainBuffer);
                glEnable(GL_BLEND);
            }

            switch (item.renderType)
            {
            case RenderType::DebugFill:
            case RenderType::Texture:
            {

                glBindTexture(GL_TEXTURE_2D, item.texture.texture);
                shader->BindShader(item, orthoMatrix);
                glDrawArrays(GL_TRIANGLE_STRIP, item.vertexIndex, item.vertexLength);

                break;
            }
            case RenderType::DebugOutline:
			{

				glBindTexture(GL_TEXTURE_2D, GLInfo.whiteTexture);
                shader->BindShader(item, orthoMatrix);
				glDrawArrays(GL_LINE_LOOP, item.vertexIndex, item.vertexLength);

				break;
			}
			}
        }
    }
    drawCalls.clear();
}


Sprite* CreateSprite(const char* name)
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


FontSprite* CreateFont(const std::string& name, int32 charSize, int32 actualCharWidth, int32 charPerRow)
{
    std::string nameAppended = "Assets/Fonts/" + name + ".png";
    Sprite* SP = CreateSprite(nameAppended.c_str());
    if (SP == nullptr)
        return nullptr;

    FontSprite* result = new FontSprite();
    result->sprite = SP;
    result->charSize = charSize;
    result->xCharSize = actualCharWidth;
    result->charPerRow = charPerRow;
    return result;
}

void BackgroundRender(Sprite* sprite, Camera* camera)
{
    Rectangle mainRect = {};
    Rectangle lastRect = {};
    Vector backgroundSize = { 720.0f , 360.0f };
    Vector scaleFactor = { 1.0f, 2.0f };
    Vector distanceFromCenter = { sprite->width / 2.0f + camera->position.x / scaleFactor.x, (sprite->height / 2.0f) + camera->position.y / scaleFactor.y};

    mainRect.botLeft.y = Clamp<float>(distanceFromCenter.y - backgroundSize.y / 2.0f, 0, sprite->height - backgroundSize.y);
    mainRect.topRight.y = mainRect.botLeft.y + backgroundSize.y;

#if 0
    //Jumpy looking background with integers:
    mainRect.botLeft.x = float(int(distanceFromCenter.x - backgroundSize.x / 2.0f + 0.5f) % sprite->width);

#else
    //Smooth looking background with floats:
    mainRect.botLeft.x = distanceFromCenter.x - backgroundSize.x / 2.0f;

#endif
    mainRect.topRight.x = mainRect.botLeft.x + backgroundSize.x;

    AddTextureToRender(mainRect, {}, RenderPrio::Sprites, sprite, White, 0, {}, false, CoordinateSpace::UI);
}


void SpriteMapRender(Sprite* sprite, int32 i, int32 itemSize, int32 xCharSize, Vector loc)
{
    int32 blocksPerRow = sprite->width / itemSize;
    uint32 x = uint32(i) % blocksPerRow * itemSize + (itemSize - xCharSize) / 2;
    uint32 y = uint32(i) / blocksPerRow * itemSize;

    Rectangle blockRect = { (float)x, (float)y, (float)(x + xCharSize), (float)(y + itemSize) };
    float itemSizeTranslatedx = PixelToGame(xCharSize);
    float itemSizeTranslatedy = PixelToGame(itemSize);

    Rectangle destRect = { loc, { loc.x + itemSizeTranslatedx, loc.y +itemSizeTranslatedy } };

    AddTextureToRender(blockRect, destRect, RenderPrio::Sprites, sprite, White, 0, {}, false, CoordinateSpace::World);
}


void SpriteMapRender(Sprite* sprite, const Block& block)
{
    SpriteMapRender(sprite, block.flags, blockSize, blockSize, block.location);
}



void DrawText(FontSprite* fontSprite, Color c, float size, 
              VectorInt loc, UIX XLayout, UIY YLayout, 
              RenderPrio prio, const char* fmt, ...)
{
    //ABC
    int32 CPR = fontSprite->charPerRow;

    //Upper Left corner and size
    Rectangle SRect = {};

    int32 w = fontSprite->xCharSize;
    int32 h = fontSprite->charSize;
    float width = (w * size);
    float height = (h * size);


    va_list args;
    va_start(args, fmt);

	char buffer[4096];
	int32 i = vsnprintf(buffer, ARRAY_COUNT(buffer), fmt, args);
	va_end(args);
	std::string text = std::string(buffer);

    for (int32 i = 0; i < text.size(); i++)
    {
        int32 charKey = text[i] - ' ';

		if (charKey >= 0 && charKey < 96)
		{

			SRect.botLeft.x = float(charKey % CPR * h + (h - w) / 2);
			SRect.botLeft.y = float(charKey / CPR * h);
			SRect.topRight.x = SRect.botLeft.x + w;
			SRect.topRight.y = SRect.botLeft.y + h;

			Rectangle DRectangle;
			DRectangle.botLeft.x = loc.x + i * width - (int32(XLayout) * width * int32(text.size())) / 2;
			DRectangle.botLeft.y = loc.y - (int32(YLayout) * height) / 2;
			DRectangle.topRight = { DRectangle.botLeft.x + width, DRectangle.botLeft.y + height };

			float height2 = DRectangle.Height();
			DRectangle.botLeft.y += height2;
			DRectangle.topRight.y -= height2;
			AddTextureToRender(SRect, DRectangle, prio, fontSprite->sprite, c, 0, {}, false, CoordinateSpace::UI);
		}
	}
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
    UIX XLayout, UIY YLayout, Color BC, Color TC,
    VectorInt mouseLoc, bool mousePressed, bool buttonSound)
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

    if (SDLPointRectangleCollision(mouseLoc, rect))
    {
        //PlayAudio("Button_Hover");
        AddRectToRender(RenderType::DebugFill, rect, { BC.r, BC.g, BC.b, BC.a / (uint32)2 }, RenderPrio::UI, CoordinateSpace::UI);
        if (mousePressed)
        {

            if (buttonSound)
                PlayAudio("Button_Confirm");
            AddRectToRender(RenderType::DebugFill, rect, BC, RenderPrio::UI, CoordinateSpace::UI);
            result = true;
        }
    }
    DrawText(textSprite, TC, 1.0f, { int32(rect.botLeft.x + rect.Width() / 2.0f), int32(rect.botLeft.y + rect.Height() / 2.0f) }, UIX::mid, UIY::mid, RenderPrio::UI, text.c_str());
	return result;
}

Sprite* GetSpriteFromAnimation(Actor* actor)
{
    assert(actor->currentSprite);
    return actor->currentSprite;
}

void RenderBlocks(TileMap* blocks)
{
    PROFILE_FUNCTION();

	Vector offset = { float(g_camera.size.x / 2) + 1, float(g_camera.size.y / 2) + 1 };
	for (float y = g_camera.position.y - offset.y; y < (g_camera.position.y + offset.y); y++)
	{
		for (float x = g_camera.position.x - offset.x; x < (g_camera.position.x + offset.x); x++)
		{
			Block* block = blocks->TryGetBlock({ x, y });
			if (block && block->tileType != TileType::invalid)
			{
				SpriteMapRender(g_sprites["spriteMap"], *block);

				if (g_debugList[DebugOptions::BlockCollision] && block->tileType != TileType::invalid)
                    AddRectToRender({ block->location, { block->location.x + 1 , block->location.y + 1 } }, lightRed, RenderPrio::Debug, CoordinateSpace::World);
			}
		}
	}
}

void RenderMovingPlatforms(Level* level)
{
    if (level)
	{
		for (int32 i = 0; i < level->movingPlatforms.size(); i++)
		{
			level->FindActor<MovingPlatform>(level->movingPlatforms[i])->Render();
		}
    }
}

void RenderLaser()
{
    if (laser.inUse)
    {//x, y, w, h
        Color PC = {};
        if (laser.paintType == TileType::filled)
            PC = Green;
        else
            PC = Red;
        Sprite* sprite = GetSpriteFromAnimation(&laser);

        Rectangle rectangle = { {laser.position}, {laser.position.x + Pythags(laser.destination - laser.position), laser.position.y + PixelToGame(sprite->height)} };
        AddTextureToRender({}, rectangle, RenderPrio::Sprites, sprite, PC, laser.rotation, { 0, rectangle.Height() / 2 }, false, CoordinateSpace::World);
    }
}


void RenderActor(Actor* actor, float rotation)
{
	bool flippage = false;
	int32 xPos;
	Sprite* sprite = GetSpriteFromAnimation(actor);

	if (actor->lastInputWasLeft && actor->allowRenderFlip)
	{
		flippage = true;
		xPos = sprite->width - actor->animationList->colRect.topRight.x;
	}
    else
		xPos = actor->animationList->colRect.bottomLeft.x;

    if (sprite == nullptr)
        sprite = actor->animationList->GetAnyValidAnimation()->anime[0];

    ActorType type = actor->GetActorType();
    Rectangle DR;
	DR.botLeft.x = actor->position.x - PixelToGame((int(xPos * actor->PixelToGameRatio())));
	DR.botLeft.y = actor->position.y - PixelToGame((int(actor->animationList->colRect.bottomLeft.y * actor->PixelToGameRatio())));
#if 1

	DR.topRight = DR.botLeft + PixelToGame({ (int)(actor->PixelToGameRatio() * sprite->width),
			                                 (int)(actor->PixelToGameRatio() * sprite->height) });
#else

    DR.topRight.x = DR.botLeft.x + actor->GameWidth();
    DR.topRight.y = DR.botLeft.y + actor->GameWidth();
#endif

	AddTextureToRender({}, DR, RenderPrio::Sprites, sprite, actor->colorMod, rotation, { actor->GameWidth() / 2.0f, actor->GameHeight() / 2.0f }, flippage, CoordinateSpace::World);
}
