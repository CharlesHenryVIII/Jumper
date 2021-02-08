
#include "Console.h"
#include "Math.h"
#include "Rendering.h"
#include "WinUtilities.h"

#include "SDL.h"
#include <cstdint>

#if 1// TODO: Utility stuff, could be shared

#include <algorithm>
#include <cassert>


/**********************************************
 *
 * Defer
 *
 ***************/

template <typename T>
struct ExitScope
{
    T lambda;
    ExitScope(T lambda): lambda(lambda){ }
    ~ExitScope(){ lambda();}
};

struct ExitScopeHelp
{
    template<typename T>
    ExitScope<T> operator+(T t) { return t; }
};

#define _CG_CONCAT(a, b) a ## b
#define CG_CONCAT(a, b) _CG_CONCAT(a, b)

#define Defer auto CG_CONCAT(defer__, __LINE__) = ExitScopeHelp() + [&]()

#define ARRAY_COUNT(arr__) ((sizeof(arr__) / sizeof(arr__[0])))
#define REF(a) ((void)a)

enum class TweenStyle
{
    Linear,
    Square,
    Cube,
    InverseSquare,
    InverseCube,
};

struct Tween
{
    TweenStyle  style;
    float       v0;
    float       v1;
    double      start_time;
    float       duration;
    bool        finished;
};


#include <chrono>

static double ToSeconds(std::chrono::duration<float> duration)
{
    return duration.count();
}

double TimeNow()
{
    static auto base_time = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    return ToSeconds(now - base_time);
}


float TweenValue(Tween& tween)
{
    if (tween.duration <= 0.0f)
        return 0;
    if (tween.finished)
        return tween.v1;

    float t = std::clamp((static_cast<float>(TimeNow() - tween.start_time)) / tween.duration, 0.0f, 1.0f);
    if (t == 1.0f)
    {
        tween.finished = true;
        return tween.v1;
    }
    float inv_t = 1.0f - t;

    switch (tween.style)
    {
    case TweenStyle::Linear: break;
    case TweenStyle::Square: t = t * t;  break;
    case TweenStyle::Cube:   t = t * t * t;  break;

    case TweenStyle::InverseSquare: t = 1.0f - (inv_t * inv_t);  break;
    case TweenStyle::InverseCube:   t = 1.0f - (inv_t * inv_t * inv_t);  break;
    default: assert(0);
    }

    return Lerp(tween.v0, tween.v1, t);
}

Tween TweenBegin(TweenStyle style, float duration, float start, float end)
{
    Tween result = {
        .style = style,
        .v0 = start,
        .v1 = end,
        .duration = duration,
        .finished = false,
    };
    result.start_time = TimeNow();
    return result;
}




size_t str_copy(char* dest, size_t size, const char* source)
{
    assert(dest && size);
    if (!source)
    {
        dest[0] = 0;
        return 0;
    }

    size_t len = Min(strlen(source), size - 1);
    dest[len] = 0;
    memcpy(dest, source, len);
    return len;
}
template <size_t size>
size_t str_copy(char(&dest)[size], const char* source) { return str_copy(dest, size, source); }

bool str_equals(const char* lhs, const char* rhs, bool case_sensitive)
{
    assert(lhs && rhs);
    if (case_sensitive)
        return strcmp(lhs, rhs) == 0;
    else
        return _stricmp(lhs, rhs) == 0;
}

bool str_equals(const char* lhs, const char* rhs, int num_chars, bool case_sensitive)
{
    assert(lhs && rhs);
    if (case_sensitive)
        return strncmp(lhs, rhs, num_chars) == 0;
    else
        return _strnicmp(lhs, rhs, num_chars) == 0;
}


inline bool is_whitespace(char c) { return c == ' '; }

const char* str_eat_whitespace(const char* input)
{
    while (*input && is_whitespace(*input))
        input++;

    return input;
}

const char* str_eat_word(const char* word)
{
    while (*word && !is_whitespace(*word))
        word++;

    return word;
}

const char* str_eat_word_and_whitespace(const char* input)
{
    const char* word_end = str_eat_word(input);
    const char* whitespace_end = str_eat_whitespace(word_end);
    return whitespace_end;
}

bool str_get_token(char* buf, size_t size, const char** input)
{
    if (size == 0)
        return 0;
    else if (!input)
    {
        buf[0] = 0;
        return 0;
    }

    *input = str_eat_whitespace(*input);
    const char* word_end = str_eat_word(*input);
    size_t len = size_t(word_end - *input);
    len = Min(size - 1, len);
    memcpy(buf, *input, len);
    buf[len] = 0;
    *input = word_end;
    return int(len) > 0;
}
template <size_t SIZE>
bool str_get_token(char(&buf)[SIZE], const char** input) { return str_get_token(buf, SIZE, input); }

#endif // TODO: MOVE


typedef uint32_t Pos;
#define NOT_IMPLEMENTED assert(0)

// STB TextEdit implementation defines
#define STB_TEXTEDIT_CHARTYPE char
#define STB_TEXTEDIT_POSITIONTYPE int
#define STB_TEXTEDIT_UNDOSTATECOUNT 32
#define STB_TEXTEDIT_UNDOCHARCOUNT 1024

// Include in header mode to get struct definitions
#include "stb/stb_textedit.h"

typedef std::string TextEditString;

static void  stbLayoutRow(StbTexteditRow* row, TextEditString* string, Pos pos);
static float stbCharWidth(TextEditString* string, int start, int i);
static int   stbKeyToText(int c);
static void  stbDeleteChars(TextEditString* string, int start, int count);
static bool  stbInsertChars(TextEditString* string, int start, char* characters, int count);


#define STB_TEXTEDIT_STRING                     TextEditString
#define STB_TEXTEDIT_STRINGLEN(obj)             ((int)(obj)->size())
#define STB_TEXTEDIT_LAYOUTROW(r, obj, n)       stbLayoutRow(r, obj, n)
#define STB_TEXTEDIT_GETWIDTH(obj,n,i)          stbCharWidth(obj, n, i)
#define STB_TEXTEDIT_KEYTOTEXT(k)               stbKeyToText(k)
#define STB_TEXTEDIT_GETCHAR(obj,i)             ((*obj)[i])
#define STB_TEXTEDIT_NEWLINE                    ('\n')
#define STB_TEXTEDIT_DELETECHARS(obj,i,n)       stbDeleteChars(obj, i, n)
#define STB_TEXTEDIT_INSERTCHARS(obj,i,c,n)     stbInsertChars(obj, i, c, n)


#define STB_TEXTEDIT_K_SHIFT       (KMOD_SHIFT << 16)
#define STB_TEXTEDIT_K_LEFT        SDLK_LEFT
#define STB_TEXTEDIT_K_RIGHT       SDLK_RIGHT
#define STB_TEXTEDIT_K_UP          SDLK_UP
#define STB_TEXTEDIT_K_DOWN        SDLK_DOWN
#define STB_TEXTEDIT_K_LINESTART   SDLK_HOME
#define STB_TEXTEDIT_K_LINEEND     SDLK_END
#define STB_TEXTEDIT_K_TEXTSTART   (SDLK_HOME | (KMOD_CTRL << 16))
#define STB_TEXTEDIT_K_TEXTEND     (SDLK_END| (KMOD_CTRL  << 16))
#define STB_TEXTEDIT_K_DELETE      SDLK_DELETE
#define STB_TEXTEDIT_K_BACKSPACE   SDLK_BACKSPACE
#define STB_TEXTEDIT_K_UNDO        (SDLK_z | (KMOD_CTRL << 16))
#define STB_TEXTEDIT_K_REDO        (SDLK_z | (KMOD_CTRL << 16) | (KMOD_SHIFT << 16))

// Optional:
#define STB_TEXTEDIT_K_INSERT              SDLK_INSERT
#define STB_TEXTEDIT_IS_SPACE(ch)          ((ch) == ' ')
//    STB_TEXTEDIT_MOVEWORDLEFT(obj,i)   custom handler for WORDLEFT, returns index to move cursor to
//    STB_TEXTEDIT_MOVEWORDRIGHT(obj,i)  custom handler for WORDRIGHT, returns index to move cursor to
#define STB_TEXTEDIT_K_WORDLEFT            SDLK_LEFT  | (KMOD_CTRL << 16)
#define STB_TEXTEDIT_K_WORDRIGHT           SDLK_RIGHT | (KMOD_CTRL << 16)
//    STB_TEXTEDIT_K_LINESTART2          secondary keyboard input to move cursor to start of line
//    STB_TEXTEDIT_K_LINEEND2            secondary keyboard input to move cursor to end of line
//    STB_TEXTEDIT_K_TEXTSTART2          secondary keyboard input to move cursor to start of text
//    STB_TEXTEDIT_K_TEXTEND2            secondary keyboard input to move cursor to end of text


#define STB_TEXTEDIT_IMPLEMENTATION
#include "stb/stb_textedit.h"

// CONFIG:
static const float OPEN_TIME          = 0.5f;  // Seconds to fully open the console
static const float OPEN_STANDARD      = 0.3f;  // Screen ratio when opening with `
static const float OPEN_LARGE         = 0.8f;  // Screen ratio when opening with shift+`
static const float SCROLL_SPEED       = 3.0f;  // Number of lines to jump with the mouse scroll wheel
static const float CARET_BLINK        = 0.5f;  // Input cursor blink time
static const float SCROLLBAR_WIDTH    = 15.0f; // Width of the clickable scrollbar region
static const float DEFAULT_FONT_SCALE = 0.5f;  // Controls the size of the bitmap font. TODO: Could switch to setting a pixel height, which would be better for TTF anyway.

static const Color console_color              = Color{0.1f, 0.1f, 0.1f, 0.95f};
static const Color input_color                = Color{0.2f, 0.2f, 0.2f, 0.95f};
static const Color caret_color                = Color{0.8f, 0.8f, 0.8f, 0.6f};
static const Color selection_color            = Color{0.8f, 0.8f, 0.8f, 0.4f};
static const Color font_color                 = Color{0.5f, 0.9f, 0.5f, 1.0f};
static const Color scroll_background_color    = Color{0.5f, 0.5f, 0.5f, 0.8f};
static const Color scroll_handle_color        = Color{0.7f, 0.7f, 1.0f, 0.8f};
static const Color scroll_handle_active_color = Color{0.8f, 0.8f, 1.0f, 1.0f};

static const Color log_colors[] = {
    Color{0.9f, 0.9f, 0.9f, 1.0f}, // LogLevel_Info
    Color{0.9f, 0.9f, 0.2f, 1.0f}, // LogLevel_Warning
    Color{0.9f, 0.1f, 0.1f, 1.0f}, // LogLevel_Error
    Color{0.5f, 0.5f, 0.8f, 1.0f}, // LogLevel_Internal
};
static_assert(ARRAY_COUNT(log_colors) == LogLevel_Count);
std::vector<char*> logStrings;
std::vector<LogLevel> logLevels;
// :CONFIG


struct ConsoleCommand
{
    char            name[128];
    CommandFunc     callback;
    CommandFuncArgs callback_args;
};

struct ConsoleItem
{
    LogLevel     level;
    Color        color;
    std::string  text;
    char         preamble[4] = {};
};

struct Console
{
    bool                        initialized = false;

    TextEditString              input_buf;
    std::vector<ConsoleItem>    items;
    std::vector<std::string>    history;
    int                         history_pos = 0;    // -1: new line, 0..History.Size-1 browsing history.
    LogLevel                    log_level = LogLevel_Info;

    std::vector<ConsoleCommand> commands;

    float                       visible_height = 0;
    float                       delta = 0;
    Tween                       tween = {};
    Tween                       caret_tween;

    float                       font_scale = DEFAULT_FONT_SCALE;
    Vector                      window_size;

    // Scrollbar
    float                       scroll_position = 0.0f;
    float                       scroll_target = 0.0f;
    float                       mouse_scroll_handle_t = 0.0f;
    bool                        mouse_scrolling = false;

    // Autocomplete
    int                         ac_index;
    std::vector<std::string>    ac_current_matches;
    bool                        ac_active;
    // The string we had before starting autocomplete (hitting backspace returns to this)
    std::string                 ac_pre_string;

    //ConsoleInputHandler*        input_handler = nullptr;
    bool                        wants_input;

    // STB:
    STB_TexteditState           te_state = {};
};
static Console s_console;


static Color LogColor(LogLevel level)
{
    int index = std::clamp(static_cast<int>(level), 0, static_cast<int>(ARRAY_COUNT(log_colors)));
    assert(index == level); // Request was out of bounds
    return log_colors[index];
}


static Vector GetWindowSize()
{
    int w, h;
    SDL_GetWindowSize(g_windowInfo.SDLWindow, &w, &h);
    Vector window_size = {
        static_cast<float>(w),
        static_cast<float>(h),
    };
    return window_size;
}

static Vector GetMousePosition()
{
    int x, y;
    SDL_GetMouseState(&x, &y);
    return { static_cast<float>(x), static_cast<float>(y) };
}

static void ConsoleClearInput()
{
    s_console.input_buf.clear();
    s_console.te_state.select_end = 0;
    s_console.te_state.select_start = 0;
    s_console.te_state.cursor = 0;
}

static Rectangle ConsoleRect()
{
    Vector window_size = s_console.window_size;// GetWindowSize();

    Rectangle console_rect;
    console_rect.botLeft  = { 0.0f,          s_console.visible_height };
    console_rect.topRight = { window_size.x, 0.0f };
    return console_rect;
}

FontSprite* ConsoleFont()
{
	return g_fonts["Main"];
}

static float ItemHeight()
{
    return static_cast<float>(ConsoleFont()->charSize) * s_console.font_scale;
    //return AppDefaultFont()->AdvanceY();
}

// The input rect is the bottom portion of the console rect
static Rectangle InputRect()
{
    Rectangle console_rect = ConsoleRect();
    Vector max = console_rect.topRight;
    float line_height = ItemHeight();
    Rectangle input_rect;
    input_rect.botLeft    = console_rect.botLeft;
    input_rect.topRight.x = console_rect.topRight.x;
    input_rect.topRight.y = console_rect.botLeft.y - line_height;
    return input_rect;
}

static Rectangle LogRect()
{
    Rectangle console_rect = ConsoleRect();
    Vector min = console_rect.botLeft;
    Vector max = console_rect.topRight;
    float line_height = ItemHeight();
    Rectangle log_rect;
    log_rect.botLeft = console_rect.botLeft;
    log_rect.botLeft.y -= line_height;
    log_rect.topRight = console_rect.topRight;
    return log_rect;
}

static float NumVisibleItems()
{
    Rectangle log_rect = LogRect();
    float visible_items = std::fabsf(log_rect.Height()) / ItemHeight();
    return visible_items;
}

static float NumItems()
{
    return static_cast<float>(s_console.items.size());
}

static float MaxScroll()
{
    float max_scroll = Max(NumItems() - NumVisibleItems(), 0.0f);
    return max_scroll;
}

static Rectangle ScrollBackgroundRect()
{
    Rectangle log_rect = LogRect();
    float visible_items = NumVisibleItems();
    if (NumItems() < visible_items)
        return {};

    Vector min = log_rect.botLeft;
    Vector max = log_rect.topRight;
    min.x = max.x - SCROLLBAR_WIDTH;

    Rectangle result;
    result.botLeft = min;
    result.topRight = max;
    return result;
}

static Rectangle ScrollbarRect()
{
    Rectangle result = {};

    Rectangle log_rect = ScrollBackgroundRect();
    float visible_items = NumVisibleItems();
    float items = NumItems();
    if (items == 0 || items < visible_items)
        return {};

    float min_height = 10.0f;
    float visible_ratio = visible_items / items;
    float size = std::fabsf(log_rect.Height()) * visible_ratio;
    float height = Max(size, min_height);

    Vector min = log_rect.botLeft;
    Vector max = log_rect.topRight;

    // Positioning
    // We have the position calculated for the end, do a lerp from the start
    float top = max.y + height;
    min.y = Lerp(min.y, top, s_console.scroll_target / MaxScroll());
    max.y = min.y - height;

    result.botLeft = min;
    result.topRight = max;
    return result;
}

static void sConsoleLog(LogLevel level, const char* string, const char* preamble = nullptr, Color* opt_color = nullptr)
{
    Color color = opt_color != nullptr ? *opt_color : LogColor(level);
    s_console.items.push_back(ConsoleItem{ level, color, std::string(string) });
    if (preamble && preamble[0])
    {
        ConsoleItem& item = s_console.items.back();
        str_copy(item.preamble, preamble);
    }

    // NOTE: This keeps the scroll position the same if we are not scrolled to the bottom, but doesn't handle multiple
    // lines being inserted at the same time.
    const float MIN_VALUE = 0.01f;
    float max_scroll = MaxScroll();
    if (s_console.scroll_target > max_scroll)
    {
        s_console.scroll_target = max_scroll;
    }
    else if (s_console.scroll_target >= MIN_VALUE)
    {
        s_console.scroll_target += 1.0f;
        s_console.scroll_position += 1.0f;
    }
}

static void AddLog(const char* preamble, const char* fmt, ...)
{
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, ARRAY_COUNT(buf), fmt, args);
    buf[ARRAY_COUNT(buf) - 1] = 0;
    va_end(args);
    sConsoleLog(LogLevel_Internal, buf, preamble, nullptr);
}

static void ConsoleClearAutoComplete()
{
    s_console.ac_index = -1;
    s_console.ac_current_matches.clear();
    s_console.ac_active = false;
    s_console.ac_pre_string.clear();
}

static void ConsoleMoveAutoCompleteIndex(bool next_alphabetically)
{
    Console* console = &s_console;

    if (next_alphabetically)
    {
        console->ac_index = (console->ac_index + 1) % static_cast<int>(console->ac_current_matches.size());
    }
    else
    {
        console->ac_index -= 1;
        if (console->ac_index < 0)
        {
            console->ac_index = int(console->ac_current_matches.size()) - 1;
        }
    }

    console->input_buf = console->ac_current_matches[console->ac_index];
    console->te_state.cursor = static_cast<int>(console->input_buf.length());
}

static void ConsoleBeginAutocomplete()
{
    Console* console = &s_console;
    std::string& current = console->input_buf;
    int current_len = static_cast<int>(current.length());

    std::vector<std::string> all_matches;
    const int matches_per_line = 6;
    int current_matches = 0;
    for (auto& command : console->commands)
    {
        if (str_equals(current.c_str(), command.name, current_len, false))
        {
            if (current_matches++ % matches_per_line == 0)
            {
                all_matches.push_back("");
            }

            std::string& line = all_matches.back();
            console->ac_current_matches.push_back(command.name);
            line += " " + console->ac_current_matches.back();
        }
    }

    if (console->ac_current_matches.size() >= 1)
    {
        if (console->ac_current_matches.size() > 1)
        {
            console->ac_pre_string = current;
            console->ac_active = true;

            AddLog("@ ", "Matches:");
            for (auto& line : all_matches)
                AddLog("   ", "%s", line.c_str());
        }
        else
        {
            console->ac_pre_string.clear();
        }

        console->ac_index = -1;
        ConsoleMoveAutoCompleteIndex(true);
    }
    else
    {
        AddLog("! ", "No matches for %s", current.c_str());
    }
}

void ExecCommand(const char* command_line)
{
    command_line = str_eat_whitespace(command_line);
    if (command_line[0] == 0)
        return;
    s_console.scroll_target = 0.0f;
    AddLog("# ", "%s", command_line);

    // Insert into history. First find match and delete it so it can be pushed to the back. This isn't trying to be smart or optimal.
    s_console.history_pos = -1;
    for (int i = static_cast<int>(s_console.history.size()); i--;)
    {
        if (str_equals(s_console.history[i].c_str(), command_line, false))
        {
            s_console.history.erase(s_console.history.begin() + i);
            break;
        }
    }
    s_console.history.push_back(command_line);

    char command[512] = {};
    {
        const char* tc = command_line;
        int len = str_get_token(command, &tc);
        if (!len)
            return;
    }

    for (auto& cmd : s_console.commands)
    {
        if (str_equals(cmd.name, command, false))
        {
            if (cmd.callback_args)
            {
                std::vector<std::string> args;
                char arg[512] = {};
                const char* command_arguments = str_eat_word_and_whitespace(command_line);
                while (str_get_token(arg, ARRAY_COUNT(arg), &command_arguments))
                    args.push_back(arg);
                cmd.callback_args(args);
            }
            else if (cmd.callback)
            {
                cmd.callback();
            }
            else
            {
                assert(!"No function was registered with the command");
            }
            return;
        }
    }
    ConsoleLog(LogLevel_Error, "Unknown command: %s", command);
}

CONSOLE_FUNCTION(ShowHelp)
{
    for (auto& command : s_console.commands)
    {
        ConsoleLog(LogLevel_Internal, command.name);
    }
}

CONSOLE_FUNCTION(ConsoleClear)
{
    s_console.items.clear();
}

CONSOLE_FUNCTIONA(ConsoleFontScale)
{
    float scale = DEFAULT_FONT_SCALE;
    if (args.size() >= 1)
        scale = static_cast<float>(atof(args[0].c_str()));

    s_console.font_scale = std::clamp(scale, 0.25f, 1.0f);
    ConsoleLog(LogLevel_Info, "Setting console font scale to %0.3f", s_console.font_scale);
}

CONSOLE_FUNCTION(Logo)
{
    Color logo_color = Mint;

    auto WriteLine = [&logo_color](const char* line) {
        sConsoleLog(LogLevel_Internal, line, nullptr, &logo_color);
    };

    WriteLine(R"(        __   __    __  .___  ___. .______    _______ .______      )");
    WriteLine(R"(       |  | |  |  |  | |   \/   | |   _  \  |   ____||   _  \     )");
    WriteLine(R"(       |  | |  |  |  | |  \  /  | |  |_)  | |  |__   |  |_)  |    )");
    WriteLine(R"( .--.  |  | |  |  |  | |  |\/|  | |   ___/  |   __|  |      /     )");
    WriteLine(R"( |  `--'  | |  `--'  | |  |  |  | |  |      |  |____ |  |\  \----.)");
    WriteLine(R"(  \______/   \______/  |__|  |__| | _|      |_______|| _| `._____|)");
    WriteLine("");
}

void ConsoleInit()
{
    if (s_console.initialized)
        return;
    s_console.window_size = GetWindowSize();
    s_console.initialized = true;
    s_console.items.reserve(1000);
    ConsoleAddCommand("help", ShowHelp);
    ConsoleAddCommand("clear", ConsoleClear);
    ConsoleAddCommand("font_scale", ConsoleFontScale);
    ConsoleAddCommand("logo", Logo);

    stb_textedit_initialize_state(&s_console.te_state, true);

    s_console.caret_tween = TweenBegin(TweenStyle::InverseSquare, CARET_BLINK, 0.1f, 1.0f);

    if (0)
    {
        // Silence unused function warnings
        stb_textedit_click(nullptr, nullptr, 0, 0);
        stb_textedit_drag(nullptr, nullptr, 0, 0);
    }

    Logo();
    
    assert(logStrings.size() == logLevels.size());
    for (int i = 0; i < logStrings.size(); i++)
    {
        sConsoleLog(logLevels[i], logStrings[i]);
        free(logStrings[i]);
    }
    logLevels.clear();
    logStrings.clear();
}

void DrawString(Vector location, Color color, const char* text, ...)
{
    va_list count_args, write_args;
    va_start(count_args, text);
    va_copy(write_args, count_args);
    auto count = vsnprintf(nullptr, 0, text, count_args);
    va_end(count_args);

    if (count)
    {
        std::string buffer;
        buffer.resize(count);
        vsnprintf(&buffer[0], buffer.size() + 1, text, write_args);
        assert(*(buffer.data() + buffer.size()) == 0);
        DrawText(ConsoleFont(), color, s_console.font_scale, { int32(location.x), int32(location.y) }, UIX::left, UIY::bot, RenderPrio::Console, buffer.c_str());
    }
}

void ConsoleRun()
{
    ConsoleInit();

    if (s_console.mouse_scrolling)
    {
        float pos = GetMousePosition().y;
        Rectangle scroll = ScrollBackgroundRect();
        Rectangle handle = ScrollbarRect();
        float height = std::fabsf(handle.Height());

        // Consider a region that doesn't include the handle for the mouse scroll so that it is fixed to the initial click position.
        float top_delete = height * (1.0f - s_console.mouse_scroll_handle_t);
        float bot_delete = height * s_console.mouse_scroll_handle_t;
        scroll.topRight.y += top_delete;
        scroll.botLeft.y -= bot_delete;

        float t = (pos - scroll.botLeft.y) / scroll.Height();
        s_console.scroll_target = gb_clamp01(t) * MaxScroll();
    }

    const float lerp_t = 0.2f;
    if (s_console.scroll_target < 0)
        s_console.scroll_target = Lerp(s_console.scroll_target, 0.0f, lerp_t);
    float max_scroll = MaxScroll();
    if (s_console.scroll_target > max_scroll)
        s_console.scroll_target = Lerp(s_console.scroll_target, max_scroll, lerp_t);
    s_console.scroll_position = Lerp(s_console.scroll_position, s_console.scroll_target, lerp_t);
    //s_console.scroll_position = Clamp(s_console.scroll_position, 0.0f, NumItems());

    Console* console = &s_console;
    console->visible_height = TweenValue(console->tween);
    if (console->visible_height == 0 && console->delta == 0)
        return;

    //Font* font = AppDefaultFont();
    FontSprite* font = ConsoleFont();

    Rectangle log_rect = LogRect();
    AddRectToRender(RenderType::DebugFill, log_rect, console_color, RenderPrio::Console, CoordinateSpace::UI);

    // Input rect
    Rectangle input_rect = InputRect();
    AddRectToRender(RenderType::DebugFill, input_rect, input_color, RenderPrio::Console, CoordinateSpace::UI);

    const char* terminal_prompt = "> ";
    float charWidth = font->xCharSize * s_console.font_scale;
    float prompt_width = static_cast<float>(charWidth * strlen(terminal_prompt)); // font->StringWidth(terminal_prompt); // TODO:
    DrawString(input_rect.botLeft, font_color, "%s%s", terminal_prompt, s_console.input_buf.c_str());

    STB_TexteditState& state = s_console.te_state;

    if (state.select_end == state.select_start)
    {
        float alpha = TweenValue(s_console.caret_tween);
        if (s_console.caret_tween.finished)
        {
            std::swap(s_console.caret_tween.v0, s_console.caret_tween.v1);
            s_console.caret_tween.start_time = TimeNow();
            s_console.caret_tween.finished = false;
            // Flipping style keeps it bright for most of the time:
            if (s_console.caret_tween.v0 < s_console.caret_tween.v1)
                s_console.caret_tween.style = TweenStyle::InverseSquare;
            else
                s_console.caret_tween.style = TweenStyle::Square;
        }

        // Nothing selected, draw the cursor
        float caret_x = static_cast<float>(state.cursor * charWidth);// TODO: font->StringWidth(s_console.input_buf.c_str(), state.cursor);
        caret_x += prompt_width;
        Rectangle caret;
        caret.botLeft.x = caret_x - 1.0f;
        caret.topRight.x = caret_x + 1.0f;

        float center_y = (input_rect.botLeft.y + input_rect.topRight.y) / 2.0f;
        caret.botLeft.y = center_y + ItemHeight() * 0.5f;
        caret.topRight.y = center_y - ItemHeight() * 0.5f;
        Color c = caret_color;
        c.a = alpha;
        AddRectToRender(RenderType::DebugFill, caret, c, RenderPrio::Console, CoordinateSpace::UI);
    }
    else
    {
        // Draw selection rectangle
        int start = Min(state.select_start, state.select_end);
        int end = Max(state.select_start, state.select_end);

        float select_start = static_cast<float>(start * charWidth); // TODO: font->StringWidth(s_console.input_buf.c_str(), start);
        float select_end = static_cast<float>(end * charWidth); // TODO: font->StringWidth(s_console.input_buf.c_str(), end);
        select_start += prompt_width;
        select_end += prompt_width;
        float select_width = select_end - select_start;
        Rectangle selection;
        selection.botLeft.x = select_start;
        selection.topRight.x = select_end;

        float center_y = (input_rect.botLeft.y + input_rect.topRight.y) / 2.0f;
        selection.botLeft.y = center_y + ItemHeight() * 0.5f;
        selection.topRight.y = center_y - ItemHeight() * 0.5f;

        AddRectToRender(RenderType::DebugFill, selection, selection_color, RenderPrio::Console, CoordinateSpace::UI);
    }

    // Draw log
    const float text_x_offset = 2.0f;
    Vector min = log_rect.botLeft;
    min.x += text_x_offset;

    // Scissor coordinates need to be framebuffer relative: 0, 0 is the bottom left;
    Rectangle scissor_rect;
    scissor_rect.botLeft.x = std::min(log_rect.botLeft.x, log_rect.topRight.x);
    scissor_rect.botLeft.y = s_console.window_size.y - std::max(log_rect.botLeft.y, log_rect.topRight.y);
    scissor_rect.topRight.x = std::max(log_rect.botLeft.x, log_rect.topRight.x);
    scissor_rect.topRight.y = s_console.window_size.y - std::min(log_rect.botLeft.y, log_rect.topRight.y);
    PushScissor(scissor_rect);
    Defer{ PopScissor(); };

    for (size_t i = 0; i < s_console.items.size(); i++)
    {
        float offset = float((NumItems() - 1) - i) * ItemHeight() - s_console.scroll_position * ItemHeight();
        min.y = log_rect.botLeft.y - offset;
        auto& item = s_console.items[i];

        if (min.y < 0.0f)
            continue;
        else if (min.y > log_rect.botLeft.y + ItemHeight())
            continue;

        DrawString(min, item.color, "%s%s", item.preamble, item.text.c_str());
        min.y += ItemHeight();//font->AdvanceY();
    }

    // Scrollbar
    {
        Rectangle scroll = ScrollBackgroundRect();
        AddRectToRender(RenderType::DebugFill, scroll, scroll_background_color, RenderPrio::Console, CoordinateSpace::UI);

        Rectangle bar = ScrollbarRect();
        Color color = s_console.mouse_scrolling ? scroll_handle_active_color : scroll_handle_color;
        // The current scissor rect will still clip the y-coord here:
        AddRectToRender(RenderType::DebugFill, bar, color, RenderPrio::Console, CoordinateSpace::UI);
    }
}

void ConsoleSetLogLevel(LogLevel level)
{
    s_console.log_level = Clamp(level, LogLevel(0), LogLevel(LogLevel_Count - 1));
}

void AddLogToList(char* buf, size_t size, LogLevel level = LogLevel_Info)
{
    char* copy = (char*)malloc(size);
    memcpy(copy, buf, size);

	logStrings.push_back(copy);
	logLevels.push_back(level);
}

void ConsoleLog(LogLevel level, const char* fmt, ...)
{
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, ARRAY_COUNT(buf), fmt, args);
    buf[ARRAY_COUNT(buf) - 1] = 0;
    va_end(args);

    if (s_console.initialized)
        sConsoleLog(level, buf);
    else
        AddLogToList(buf, sizeof(buf), level);
    DebugPrint("%s\n", buf);
}

void ConsoleLog(const char* fmt, ...)
{

	char buf[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, ARRAY_COUNT(buf), fmt, args);
	buf[ARRAY_COUNT(buf) - 1] = 0;
	va_end(args);

    if (s_console.initialized)
        sConsoleLog(LogLevel_Info, buf);
    else
        AddLogToList(buf, sizeof(buf));
    DebugPrint("%s\n", buf);
}

static ConsoleCommand& AddCommand(const char* name)
{
    ConsoleInit();
    ConsoleCommand command = {};
    strcpy_s(command.name, name);

    auto it = s_console.commands.begin();
    for (; it != s_console.commands.end(); ++it)
    {
        if (_strcmpi(name, it->name) < 0)
        {
            break;
        }
    }

    auto result = s_console.commands.insert(it, command);
    return *result;
}

void ConsoleAddCommand(const char* name, CommandFunc func)
{
    assert(name && func);
    ConsoleCommand& command = AddCommand(name);
    command.callback = func;
}

void ConsoleAddCommand(const char* name, CommandFuncArgs func)
{
    assert(name && func);
    ConsoleCommand& command = AddCommand(name);
    command.callback_args = func;
}

void ConsoleClose()
{
    Console* console = &s_console;
    console->tween = TweenBegin(TweenStyle::InverseCube, OPEN_TIME, console->visible_height, 0.0f);
    s_console.wants_input = false;
    ConsoleClearInput();
    ConsoleClearAutoComplete();
}

static float TargetHeight(bool large)
{
    Vector size = s_console.window_size;
    float target = large ? size.y * OPEN_LARGE : size.y * OPEN_STANDARD;
    return target;
}

void ConsoleOpen(bool large)
{
    float target = TargetHeight(large);
    s_console.tween = TweenBegin(TweenStyle::InverseCube, OPEN_TIME, s_console.visible_height, target);
    s_console.wants_input = true;
}

static bool FloatEquals(float a, float b, float epsilon = FLT_EPSILON)
{
    float abs_diff = std::fabsf(b - a);
    return abs_diff < epsilon;
}

void ConsoleToggle(bool shift_pressed)
{
    float cur = s_console.tween.v1;
    bool is_open = cur > 0.0f;
    bool is_large = FloatEquals(cur, TargetHeight(true));

    if (!is_open)
        ConsoleOpen(shift_pressed);
    else if (shift_pressed)
        ConsoleOpen(!is_large);
    else
        ConsoleClose();
}

//
// STB helper functions:
//

// NOTE: Used for mouse input, probably don't need/want it.
static void stbLayoutRow(StbTexteditRow* row, TextEditString* string, Pos pos)
{
    REF(row); REF(string); REF(pos);
    NOT_IMPLEMENTED;
}

static float stbCharWidth(TextEditString* string, int start, int i)
{
    REF(string); REF(start); REF(i);
    // Return the size of the character, takking Kerning into account
    NOT_IMPLEMENTED;
    return 1.0f;
}

static int stbKeyToText(int c)
{
    if (c >= 32 && c <= 126)
        return c;
    return -1;
}

static void stbDeleteChars(TextEditString* string, int start, int count)
{
    string->erase(size_t(start), size_t(count));
}

static bool stbInsertChars(TextEditString* string, int start, char* characters, int count)
{
    string->insert(size_t(start), characters, size_t(count));
    return true;
}

static void CycleHistory(bool reverse)
{
    Console* console = &s_console;

    if (console->ac_active)
    {
        ConsoleMoveAutoCompleteIndex(reverse);
    }
    else
    {
        const int prev_history_pos = console->history_pos;

        if (reverse)
        {
            if (console->history_pos != -1)
                if (++console->history_pos >= (int)console->history.size())
                    console->history_pos = -1;
        }
        else
        {
            if (console->history_pos == -1)
                console->history_pos = static_cast<int>(console->history.size()) - 1;
            else if (console->history_pos > 0)
                console->history_pos--;
        }

        // A better implementation would preserve the data on the current input line along with cursor position.
        if (prev_history_pos != console->history_pos)
        {
            if (console->history_pos == -1)
                console->input_buf = "";
            else
                console->input_buf = console->history[console->history_pos];
            console->te_state.select_end = console->te_state.select_start = 0;
            console->te_state.cursor = static_cast<int>(console->input_buf.size());
        }
    }
}

static void CopySelection()
{
    STB_TexteditState* state = &s_console.te_state;
    if (state->select_start != state->select_end)
    {
        size_t start = Min<size_t>(state->select_start, state->select_end);
        size_t end = Max<size_t>(state->select_start, state->select_end);
        std::string text = s_console.input_buf.substr(start, end - start);
        SDL_SetClipboardText(text.c_str());
    }
}

//
// Input Handler
//

bool ConsoleWantsInput()
{
    return s_console.wants_input;
}

bool Console_OnCharacter(int c)
{
    constexpr uint32_t TILDE_KEY = 126; // Not in SDL documentation?
    if (c == SDLK_BACKQUOTE || c == TILDE_KEY) return false;
    if (!ConsoleWantsInput()) return false;
    STB_TEXTEDIT_KEYTYPE key = c;
    stb_textedit_key(&s_console.input_buf, &s_console.te_state, key);
    ConsoleClearAutoComplete();
    return true;
}

bool Console_OnKeyboard(int c, int mods, bool pressed, bool repeat)
{
    if (c == SDLK_BACKQUOTE)
    {
        if (pressed)
        {
            ConsoleToggle(mods & KMOD_SHIFT);
            return true;
        }
        return false;
    }

    STB_TEXTEDIT_KEYTYPE key = c | (mods << 16);
    if (!ConsoleWantsInput()) return false;
    if (stbKeyToText(key) >= 0) return true;
    if (!pressed && !repeat) return true;

    STB_TexteditState* state = &s_console.te_state;
    bool control = !!(mods & KMOD_CTRL);
    bool shift = !!(mods & KMOD_SHIFT);
    bool clear_autocomplete = false;

    if (c == SDLK_RETURN || c == SDLK_KP_ENTER)
    {
        ExecCommand(s_console.input_buf.c_str());
        clear_autocomplete = true;
        ConsoleClearInput();
    }
    else if (control && c == SDLK_a)
    {
        state->select_start = 0;
        state->select_end = STB_TEXTEDIT_STRINGLEN(&s_console.input_buf);
        clear_autocomplete = true;
    }
    else if (control && (c == SDLK_BACKSPACE || c == SDLK_w))
    {
        stb_textedit_key(&s_console.input_buf, state, SDLK_LEFT | (KMOD_SHIFT << 16) | (KMOD_CTRL << 16));
        stb_textedit_key(&s_console.input_buf, state, SDLK_BACKSPACE);
        clear_autocomplete = true;
    }
    else if (s_console.ac_active && c == SDLK_BACKSPACE)
    {
        s_console.input_buf = s_console.ac_pre_string;
        s_console.te_state.cursor = static_cast<int>(s_console.input_buf.length());
        clear_autocomplete = true;
    }
    else if (control && c == SDLK_DELETE)
    {
        stb_textedit_key(&s_console.input_buf, state, SDLK_RIGHT | (KMOD_SHIFT << 16) | (KMOD_CTRL << 16));
        stb_textedit_key(&s_console.input_buf, state, SDLK_DELETE);
        clear_autocomplete = true;
    }
    else if (control && (c == SDLK_c || c == SDLK_x))
    {
        CopySelection();
        if (c == SDLK_x)
            stb_textedit_cut(&s_console.input_buf, state);
    }
    else if (control && c == SDLK_v)
    {
        if (const char* clip_text = SDL_GetClipboardText())
        {
            stb_textedit_paste(&s_console.input_buf, state, clip_text, static_cast<int>(strlen(clip_text)));
        }

        clear_autocomplete = true;
    }
    else if (control && (c == SDLK_HOME || c == SDLK_END))
    {
        if (c == SDLK_HOME)
            s_console.scroll_target = MaxScroll();
        else
            s_console.scroll_target = 0.0f;
        clear_autocomplete = true;
    }
    else if (c == SDLK_UP || c == SDLK_DOWN)
    {
        CycleHistory(c == SDLK_DOWN);
    }
    else if (c == SDLK_PAGEDOWN || c == SDLK_PAGEUP)
    {
        float offset = NumVisibleItems();
        s_console.scroll_target += offset * ((c == SDLK_PAGEDOWN) ? -1.0f : 1.0f);
        s_console.scroll_target = Clamp(s_console.scroll_target, 0.0f, MaxScroll());
    }
    else if (c == SDLK_TAB)
    {
        if (s_console.ac_active)
            ConsoleMoveAutoCompleteIndex(!shift);
        else
            ConsoleBeginAutocomplete();
    }
    else
    {
        stb_textedit_key(&s_console.input_buf, state, key);
    }

    if (clear_autocomplete)
        ConsoleClearAutoComplete();

    return true;
}


void Console_OnWindowSize(int width, int height)
{
    if (width * height == 0)
        return;
    ConsoleInit(); // Paranoid check to avoid divide by zero

    // Preserve the old ratio that the tween was targeting.
    float ratio = s_console.tween.v1 / s_console.window_size.y;
    s_console.tween.v1 = height * ratio;
    s_console.window_size = {
        static_cast<float>(width),
        static_cast<float>(height),
    };
}

static bool Contains(Rectangle r, Vector point)
{
    bool x = point.x > r.botLeft.x && point.x < r.topRight.x;
    bool y = point.y < r.botLeft.y && point.y > r.topRight.y;
    return x && y;
}

static bool MouseIsOverConsole()
{
    Vector pos = GetMousePosition();
    Rectangle console_rect = ConsoleRect();
    return Contains(console_rect, pos);
}

bool Console_OnMouseButton(int button, bool pressed)
{
    if (button == SDL_BUTTON_LEFT && !pressed)
        s_console.mouse_scrolling = false;

    if (!ConsoleWantsInput())
        return false;
    if (!MouseIsOverConsole())
        return false;

    if (button == SDL_BUTTON_LEFT && pressed)
    {
        Vector pos = GetMousePosition();
        Rectangle rect = ScrollbarRect();
        if (Contains(rect, pos))
        {
            s_console.mouse_scrolling = true;
            s_console.mouse_scroll_handle_t = (pos.y - rect.botLeft.y) / rect.Height();
        }
    }

    return true;
}

bool Console_OnMouseWheel(float scroll)
{
    if (!ConsoleWantsInput())
        return false;
    if (!MouseIsOverConsole())
        return false;

    s_console.scroll_target += scroll * SCROLL_SPEED;
    return true;
}

