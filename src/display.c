#include "display.h"
#include "config.h"
#include "keyicon.h"
#include "raylib.h"
#include "search.h"
#include "structures.h"
#include "theme.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WITH_VALGRIND
#include <valgrind/valgrind.h>
#endif

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define ROW_HEIGHT 22
#define COL_KEYS 20
#define COL_DESC 400
/*Padding before start of text in container */
#define PADDING 20

#define TOP_SECTION_HEIGHT (PADDING + ROW_HEIGHT)

#define BODY_SECTION_POS TOP_SECTION_HEIGHT
#define BOTTOM_SECTION_HEIGHT TOP_SECTION_HEIGHT
#define BODY_SECTION_HEIGHT                                                    \
    (WINDOW_HEIGHT - TOP_SECTION_HEIGHT - BOTTOM_SECTION_HEIGHT)

#define BOTTOM_SECTION_POS (WINDOW_HEIGHT - BOTTOM_SECTION_HEIGHT)

static const Color COLOR_LINE = {60, 60, 60, 255};

typedef struct {
    Font font;
    DisplayError error;
} font_container_t;

typedef struct {
    const KeyMap *km;
    const intlist_t *positions;
} visible_item_t;

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
static Scroll scroll_create(int content_height, int window_height);
static void scroll_update(Scroll *s);
static void draw_top(theme_color_t color, theme_color_t text_color, Font font,
                     float font_size, float spacing);
static void draw_row(theme_color_t text_color, theme_color_t highlight_color,
                     theme_color_t highlight_text_color, Font font,
                     float font_size, float spacing, visible_item_t item,
                     float y);
static void draw_body(theme_color_t color, theme_color_t text_color,
                      theme_color_t highlight_color,
                      theme_color_t highlight_text_color, Font font,
                      float font_size, float spacing, visible_item_t *items,
                      const size_t count, float offset_y);

static font_container_t load_font_from_paths(const stringlist_t *paths,
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
            printf("Loading font %s\n", path);
            return (font_container_t){.font = font, .error = DISPLAY_SUCCESS};
        }
    }

    return (font_container_t){.font = GetFontDefault(),
                              .error = DISPLAY_FONT_LOAD_ERROR};
}

static font_container_t load_system_font(int size) {
    stringlist_t list;
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
        stringlist_t list;
        stringlist_init(&list);
        stringlist_append(&list, font->file);

        font_container_t result = load_font_from_paths(&list, font->size);

        stringlist_free(&list);

        if (result.error == DISPLAY_SUCCESS) {
            return result.font;
        }
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

static void draw_top(theme_color_t color, theme_color_t text_color, Font font,
                     float font_size, float spacing) {
    Color tcolor = to_raylib_color(text_color);
    DrawRectangle(0, 0, WINDOW_WIDTH, TOP_SECTION_HEIGHT,
                  to_raylib_color(color));

    DrawTextEx(font, "Keybinding", (Vector2){COL_KEYS, PADDING}, font_size,
               spacing, tcolor);
    DrawTextEx(font, "Action", (Vector2){COL_DESC, PADDING}, font_size, spacing,
               tcolor);
    DrawLine(COL_KEYS, TOP_SECTION_HEIGHT, WINDOW_WIDTH - COL_KEYS,
             TOP_SECTION_HEIGHT, COLOR_LINE);
}

bool is_contiguous(size_t prev, size_t current) {
    return (current - 1) == prev;
}

static void draw_text_highlighted(Font font, visible_item_t item,
                                  Vector2 position, float font_size,
                                  float spacing, Color text_color,
                                  Color highlight_text_color,
                                  Color highlight_color) {
    const char *text = item.km->description;
    if (text == NULL)
        return;

    // No highlights → just draw normally
    if (item.positions == NULL || item.positions->count == 0) {
        DrawTextEx(font, text, position, font_size, spacing, text_color);
        return;
    }

    segmentlist_t *segments = malloc(sizeof(segmentlist_t));
    if (segments == NULL) {
        DrawTextEx(font, text, position, font_size, spacing, text_color);
        return;
    }
    segmentlist_init(segments);

    size_t n_segments = get_segments(text, item.positions, segments);

    Vector2 current_pos = position;
    float line_height = MeasureTextEx(font, "A", font_size, spacing).y;

    for (size_t i = 0; i < n_segments; i++) {
        segment_t seg = segments->items[i];
        size_t seg_len = seg.end - seg.start;

        if (seg_len == 0)
            continue;

        char buffer[seg_len + 1];
        memcpy(buffer, text + seg.start, seg_len);
        buffer[seg_len] = '\0';

        Vector2 text_size = MeasureTextEx(font, buffer, font_size, spacing);

        if (seg.highlighted) {
            DrawRectangleV(current_pos, (Vector2){text_size.x, line_height},
                           highlight_color);
        }

        Color draw_color = seg.highlighted ? highlight_text_color : text_color;

        DrawTextEx(font, buffer, current_pos, font_size, spacing, draw_color);

        // Advance position for next segment
        current_pos.x += text_size.x;
    }

    segmentlist_free(segments);
    free(segments);
}

static void draw_row(theme_color_t text_color, theme_color_t highlight_color,
                     theme_color_t highlight_text_color, Font font,
                     float font_size, float spacing, visible_item_t item,
                     float y) {
    char keybuf[256];

    format_keys(item.km, keybuf, sizeof(keybuf));
    Color tc = to_raylib_color(text_color);
    DrawTextEx(font, keybuf, (Vector2){COL_KEYS, y}, font_size, spacing, tc);

    draw_text_highlighted(font, item, (Vector2){COL_DESC, y}, font_size,
                          spacing, tc, to_raylib_color(highlight_text_color),
                          to_raylib_color(highlight_color));
}

static void draw_body(theme_color_t color, theme_color_t text_color,
                      theme_color_t highlight_color,
                      theme_color_t highlight_text_color, Font font,
                      float font_size, float spacing, visible_item_t *items,
                      const size_t count, float offset_y) {
    DrawRectangle(0, BODY_SECTION_POS, WINDOW_WIDTH, BODY_SECTION_HEIGHT,
                  to_raylib_color(color));
    float y = offset_y;

    for (size_t i = 0; i < count; i++) {
        if (y + ROW_HEIGHT < (TOP_SECTION_HEIGHT + PADDING)) {
            y += ROW_HEIGHT;
            continue;
        }
        if (y > (BODY_SECTION_HEIGHT + PADDING))
            break;
        draw_row(text_color, highlight_color, highlight_text_color, font,
                 font_size, spacing, items[i], y);
        y += ROW_HEIGHT;
    }
}

static void draw_bottom(theme_color_t color, theme_color_t text_color,
                        Font font, float font_size, float spacing,
                        const char *text, const labels_t labels) {
    Color tc = to_raylib_color(text_color);

    DrawRectangle(0, BOTTOM_SECTION_POS, WINDOW_WIDTH, BOTTOM_SECTION_HEIGHT,
                  to_raylib_color(color));

    DrawTextEx(font, labels.search, (Vector2){COL_KEYS - 5, BOTTOM_SECTION_POS},
               font_size, spacing, tc);
    float search_x_offset =
        MeasureTextEx(font, labels.search, font_size, spacing).x;
    DrawTextEx(font, text,
               (Vector2){COL_KEYS + search_x_offset, BOTTOM_SECTION_POS},
               font_size, spacing, tc);
    float info_x_offset =
        MeasureTextEx(font, labels.info, font_size, spacing).x;
    DrawTextEx(
        font, labels.info,
        (Vector2){WINDOW_WIDTH - COL_KEYS - info_x_offset, BOTTOM_SECTION_POS},
        font_size, spacing, tc);
}

size_t filter_keymaps(visible_item_t *visible, const KeyMapList *kml,
                      const search_result_t *result) {
    size_t write = 0;
    for (size_t i = 0; i < kml->count; i++) {
        if (result->mask[i]) {
            visible[write].km = &kml->items[i];
            visible[write].positions = result->positions[i];
            write++;
        }
    }
    return write;
}

void display(const KeyMapList *kml, const theme_t *theme) {
    SetConfigFlags(FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_UNDECORATED);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "swindings");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    float spacing = 1.0f;

    int content_height = PADDING + (int)kml->count * ROW_HEIGHT + PADDING;
    Scroll scroll = scroll_create(content_height, WINDOW_HEIGHT);

    Font top_font = load_best_font(&theme->top.font);
    Font body_font = load_best_font(&theme->body.font);
    Font bottom_font = load_best_font(&theme->bottom.font);
    search_state_t search_state;
    char search_buffer[256];
    init_search_state(&search_state, search_buffer, 256);

    visible_item_t *visible = calloc(kml->count, sizeof(visible_item_t));
    size_t visible_count = kml->count;
    const char *descriptions[kml->count + 1];
    for (size_t i = 0; i < kml->count; i++)
        descriptions[i] = kml->items[i].description;
    descriptions[kml->count] = NULL;

    search_result_t search_result;
    if (init_search_result(&search_result, kml->count) != 0) {
        // TODO: Manage errors
    }
    while (!WindowShouldClose()) {
        float body_offset = TOP_SECTION_HEIGHT + scroll.y;
        scroll_update(&scroll);
        update_search_input(&search_state);

        if (search_state.active && search_state.query_changed) {
            search(search_state.text, descriptions, search_state.type,
                   &search_result);
            visible_count = filter_keymaps(visible, kml, &search_result);
            search_state.query_changed = false;
        } else if (!search_state.active) {
            for (size_t i = 0; i < kml->count; i++) {
                visible[i].km = &kml->items[i];
                visible[i].positions = NULL;
            }

            visible_count = kml->count;
        }

        BeginDrawing();
        ClearBackground(BLANK);

        draw_top(theme->top.background.color, theme->top.font.color, top_font,
                 theme->top.font.size, spacing);
        draw_body(theme->body.background.color, theme->body.font.color,
                  theme->highlight.background.color,
                  theme->highlight.font.color, body_font, theme->body.font.size,
                  spacing, visible, visible_count, body_offset);
        if (search_state.active) {
            labels_t labels = {.search = " Search: ",
                               .info = "esc to escape"};

            draw_bottom(theme->bottom.background.color,
                        theme->bottom.font.color, bottom_font,
                        theme->bottom.font.size, spacing, search_state.text,
                        labels);
        } else {
            for (size_t i = 0; i < kml->count; i++) {
                visible[i].km = &kml->items[i];
                visible[i].positions = NULL;
            }
            labels_t labels = {.search = " Search: ",
                               .info = "⌃+f to search | ⌃+k to fuzzy search"};
            draw_bottom(theme->bottom.background.color,
                        theme->bottom.font.color, bottom_font,
                        theme->bottom.font.size, spacing, "", labels);
        }
        EndDrawing();
#ifdef WITH_VALGRIND
        if (RUNNING_ON_VALGRIND)
            break;
#endif
    }
    free(visible);
    free_search_result(&search_result);
    UnloadFont(top_font);
    UnloadFont(body_font);
    UnloadFont(bottom_font);
    CloseWindow();
}
