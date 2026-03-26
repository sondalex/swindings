#include "display.h"
#include "keyicon.h"
#include "raylib.h"
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
static const Color COLOR_HEADER = {160, 160, 160, 255};
static const Color COLOR_ROW = {130, 130, 130, 255};
static const Color COLOR_LINE = {60, 60, 60, 255};

// ── Font loading ──────────────────────────────────────────────

static const char *font_paths[] = {
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf",
    "/usr/share/fonts/TTF/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    "/usr/share/fonts/liberation-sans/LiberationSans-Regular.ttf",
    "/usr/share/fonts/noto/NotoSans-Regular.ttf",
    NULL,
};

static Font load_system_font(int size) {
    int ascii_count = 126 - 32 + 1;
    int total = ascii_count + key_codepoints_count;

    int codepoints[total];
    for (int i = 0; i < ascii_count; i++)
        codepoints[i] = 32 + i;
    memcpy(&codepoints[ascii_count], key_codepoints,
           key_codepoints_count * sizeof(int));

    for (const char **p = font_paths; *p; p++) {
        if (FileExists(*p))
            return LoadFontEx(*p, size, codepoints, total);
    }
    return GetFontDefault();
}

// ── Text helpers ──────────────────────────────────────────────

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

// ── Scroll ────────────────────────────────────────────────────

typedef struct {
    float y;
    float min;
} Scroll;

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

// ── Drawing ───────────────────────────────────────────────────

static void draw_header(Font font, float spacing, float y) {
    DrawTextEx(font, "Keybinding", (Vector2){COL_KEYS, y}, FONT_SIZE, spacing,
               COLOR_HEADER);
    DrawTextEx(font, "Action", (Vector2){COL_DESC, y}, FONT_SIZE, spacing,
               COLOR_HEADER);
    DrawLine(COL_KEYS, y + FONT_SIZE + 4, WINDOW_WIDTH - COL_KEYS,
             y + FONT_SIZE + 4, COLOR_LINE);
}

static void draw_row(Font font, float spacing, const KeyMap *km, float y) {
    char keybuf[256];
    char descbuf[256];

    format_keys(km, keybuf, sizeof(keybuf));
    capitalize_into(km->description, descbuf, sizeof(descbuf));

    DrawTextEx(font, keybuf, (Vector2){COL_KEYS, y}, FONT_SIZE, spacing,
               COLOR_ROW);
    DrawTextEx(font, descbuf, (Vector2){COL_DESC, y}, FONT_SIZE, spacing,
               COLOR_ROW);
}

static void draw_rows(Font font, float spacing, const KeyMapList *kml,
                      float offset_y) {
    float y = PADDING + ROW_HEIGHT + 8 + offset_y;

    for (size_t i = 0; i < kml->count; i++) {
        if (y + ROW_HEIGHT < 0) {
            y += ROW_HEIGHT;
            continue;
        }
        if (y > WINDOW_HEIGHT)
            break;

        draw_row(font, spacing, &kml->items[i], y);
        y += ROW_HEIGHT;
    }
}

// ── Public entry point ────────────────────────────────────────

void display(const KeyMapList *kml) {
    SetConfigFlags(FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_UNDECORATED);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "swindings");
    SetTargetFPS(60);

    Font font = load_system_font(FONT_SIZE);
    float spacing = 1.0f;

    int content_height = PADDING + (int)kml->count * ROW_HEIGHT + PADDING;
    Scroll scroll = scroll_create(content_height, WINDOW_HEIGHT);

    while (!WindowShouldClose()) {
        scroll_update(&scroll);

        BeginDrawing();
        ClearBackground(BLANK);
        DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, COLOR_BG);

        draw_header(font, spacing, PADDING + scroll.y);
        draw_rows(font, spacing, kml, scroll.y);

        EndDrawing();
    }

    UnloadFont(font);
    CloseWindow();
}
