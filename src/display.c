#include "display.h"
#include "keyicon.h"
#include "raylib.h"
#include "stringlist.h"
#include "theme.h"
#include <ctype.h>
#include <string.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define FONT_SIZE 14
#define ROW_HEIGHT 22
#define COL_KEYS 20
#define COL_DESC 400
#define PADDING 20

static const Color COLOR_BG = {20, 20, 20, 180};
static const Color COLOR_LINE = {60, 60, 60, 255};

typedef struct {
    Font font;
    DisplayError error;
} font_container_t;

static const char *font_paths[] = {
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf",
    "/usr/share/fonts/TTF/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    "/usr/share/fonts/liberation-sans/LiberationSans-Regular.ttf",
    "/usr/share/fonts/noto/NotoSans-Regular.ttf",
    NULL,
};
size_t font_paths_len(const char *arr[]) {
    size_t i = 0;
    while (arr[i] != NULL) {
        i++;
    }
    return i;
}

static font_container_t load_system_font(int size);
static void format_keys(const KeyMap *km, char *buf, size_t bufsize);
static void capitalize_into(const char *src, char *buf, size_t bufsize);
static Scroll scroll_create(int content_height, int window_height);
static Scroll scroll_create(int content_height, int window_height);
static void scroll_update(Scroll *s);
static void draw_top(theme_color_t text_color, Font font, float spacing,
                     float y);
static void draw_row(theme_color_t text_color, Font font, float spacing,
                     const KeyMap *km, float y);
static void draw_body(theme_color_t text_color, Font font, float spacing,
                      const KeyMapList *kml, float offset_y);

static font_container_t load_font_from_paths(const StringList *paths,
                                             int size) {
    if (!paths || paths->count == 0) {
        return (font_container_t){.font = GetFontDefault(),
                                  .error = DISPLAY_STRINGLIST_UNINITIALIZED};
    }

    int ascii_count = 126 - 32 + 1;
    int total = ascii_count + key_codepoints_count;

    int codepoints[total];

    for (int i = 0; i < ascii_count; i++)
        codepoints[i] = 32 + i;

    memcpy(&codepoints[ascii_count], key_codepoints,
           key_codepoints_count * sizeof(int));

    for (size_t i = 0; i < paths->count; i++) {
        const char *path = paths->items[i];

        if (FileExists(path)) {
            Font font = LoadFontEx(path, size, codepoints, total);

            return (font_container_t){.font = font, .error = DISPLAY_SUCCESS};
        }
    }

    return (font_container_t){.font = GetFontDefault(),
                              .error = DISPLAY_FONT_LOAD_ERROR};
}

static font_container_t load_system_font(int size) {
    StringList list;
    stringlist_init(&list);

    size_t len = font_paths_len(font_paths);
    for (size_t i = 0; i < len; i++) {
        stringlist_append(&list, font_paths[i]);
    }

    font_container_t result = load_font_from_paths(&list, size);

    stringlist_free(&list);
    return result;
}

static Font load_best_font(const theme_font_t *font) {
    if (!font)
        return GetFontDefault();

    if (font->file) {
        StringList list;
        stringlist_init(&list);
        stringlist_append(&list, font->file);

        font_container_t result = load_font_from_paths(&list, font->size);

        stringlist_free(&list);

        if (result.error == DISPLAY_SUCCESS)
            return result.font;
    }

    font_container_t system_font = load_system_font(font->size);
    if (system_font.error == DISPLAY_SUCCESS)
        return system_font.font;

    return GetFontDefault();
}

static void format_keys(const KeyMap *km, char *buf, size_t bufsize) {
    buf[0] = '\0';
    for (size_t j = 0; j < km->modifier_count; j++) {
        strncat(buf, key_to_symbol(km->modifiers[j]),
                bufsize - strlen(buf) - 1);
        strncat(buf, " + ", bufsize - strlen(buf) - 1);
    }
    strncat(buf, key_to_symbol(km->main_key), bufsize - strlen(buf) - 1);
}

static void capitalize_into(const char *src, char *buf, size_t bufsize) {
    if (!src || !src[0] || bufsize < 2) {
        buf[0] = '\0';
        return;
    }
    size_t len = strlen(src);
    if (len >= bufsize)
        len = bufsize - 1;
    memcpy(buf, src, len);
    buf[len] = '\0';
    buf[0] = toupper((unsigned char)buf[0]);
}

static Scroll scroll_create(int content_height, int window_height) {
    float min = -(content_height - window_height);
    return (Scroll){.y = 0, .min = min > 0 ? 0 : min};
}

static void scroll_update(Scroll *s) {
    s->y += GetMouseWheelMove() * ROW_HEIGHT;
    if (s->y > 0)
        s->y = 0;
    if (s->y < s->min)
        s->y = s->min;
}

static Color to_raylib_color(theme_color_t color) {
    Color c = {.a = color.a, .b = color.b, .g = color.g, .r = color.r};
    return c;
}

static void draw_top(theme_color_t text_color, Font font, float spacing,
                     float y) {
    Color color = to_raylib_color(text_color);

    DrawTextEx(font, "Keybinding", (Vector2){COL_KEYS, y}, FONT_SIZE, spacing,
               color);
    DrawTextEx(font, "Action", (Vector2){COL_DESC, y}, FONT_SIZE, spacing,
               color);
    DrawLine(COL_KEYS, y + FONT_SIZE + 4, WINDOW_WIDTH - COL_KEYS,
             y + FONT_SIZE + 4, COLOR_LINE);
}

static void draw_row(theme_color_t text_color, Font font, float spacing,
                     const KeyMap *km, float y) {
    char keybuf[256];
    char descbuf[256];

    format_keys(km, keybuf, sizeof(keybuf));
    capitalize_into(km->description, descbuf, sizeof(descbuf));
    Color tc = to_raylib_color(text_color);
    DrawTextEx(font, keybuf, (Vector2){COL_KEYS, y}, FONT_SIZE, spacing, tc);
    DrawTextEx(font, descbuf, (Vector2){COL_DESC, y}, FONT_SIZE, spacing, tc);
}

static void draw_body(theme_color_t text_color, Font font, float spacing,
                      const KeyMapList *kml, float offset_y) {
    float y = PADDING + ROW_HEIGHT + 8 + offset_y;

    for (size_t i = 0; i < kml->count; i++) {
        if (y + ROW_HEIGHT < 0) {
            y += ROW_HEIGHT;
            continue;
        }
        if (y > WINDOW_HEIGHT)
            break;
        draw_row(text_color, font, spacing, &kml->items[i], y);
        y += ROW_HEIGHT;
    }
}

void display(const KeyMapList *kml, const theme_t *theme) {
    SetConfigFlags(FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_UNDECORATED);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "swindings");
    SetTargetFPS(60);

    float spacing = 1.0f;

    int content_height = PADDING + (int)kml->count * ROW_HEIGHT + PADDING;
    Scroll scroll = scroll_create(content_height, WINDOW_HEIGHT);

    Font top_font = load_best_font(&theme->top.font);
    Font body_font = load_best_font(&theme->body.font);
    // Font bottom_font = load_best_font(&theme->bottom.font);

    while (!WindowShouldClose()) {
        scroll_update(&scroll);

        BeginDrawing();
        ClearBackground(BLANK);
        DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, COLOR_BG);
        // TODO: Move hardcoded color to theme
        theme_color_t top_color = {
            .r = 160, .g = 160, .b = 160, .a = 255, .has_alpha = true};
        draw_top(top_color, top_font, spacing, PADDING + scroll.y);

        theme_color_t body_color = {
            .r = 130,
            .g = 130,
            .b = 130,
            .a = 255,
            .has_alpha = true,
        };
        draw_body(body_color, body_font, spacing, kml, scroll.y);
        EndDrawing();
    }
    UnloadFont(top_font);
    UnloadFont(body_font);
    CloseWindow();
}
