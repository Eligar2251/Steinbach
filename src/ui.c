/* ui.c — реализация виджетов и загрузка ресурсов. */
#include "ui.h"
#include "assets_gen.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

Font g_font_body;
Font g_font_bold;
Font g_font_title;
Texture2D g_tex[TEX_COUNT];

static bool s_blocked = false;

void ui_set_blocked(bool blocked) { s_blocked = blocked; }
bool ui_blocked(void) { return s_blocked; }

/* --------------------------------------------------- загрузка ассетов --- */
static const char* tex_names[TEX_COUNT] = {
    [TEX_DEEP] = "t_deep", [TEX_WATER] = "t_water", [TEX_SAND] = "t_sand",
    [TEX_GRASS] = "t_grass", [TEX_FOREST] = "t_forest", [TEX_HILLS] = "t_hills",
    [TEX_MOUNTAIN] = "t_mountain", [TEX_SWAMP] = "t_swamp", [TEX_SNOW] = "t_snow",
    [TEX_DIRT] = "t_dirt",
    [TEX_IC_VILLAGE] = "ic_village", [TEX_IC_TOWN] = "ic_town", [TEX_IC_CITY] = "ic_city",
    [TEX_IC_CASTLE] = "ic_castle", [TEX_IC_RUINS] = "ic_ruins", [TEX_IC_CAMP] = "ic_camp",
    [TEX_IC_MINE] = "ic_mine", [TEX_IC_SHRINE] = "ic_shrine", [TEX_IC_PARTY] = "ic_party",
    [TEX_IC_FARM] = "ic_farm", [TEX_IC_BANDIT] = "ic_bandit",
    [TEX_WOOD] = "ui_wood", [TEX_PARCHMENT] = "ui_parchment", [TEX_STONE] = "ui_stone",
    [TEX_IC_HP] = "ic_hp", [TEX_IC_FATIGUE] = "ic_fatigue", [TEX_IC_RESOLVE] = "ic_resolve",
    [TEX_IC_INITIATIVE] = "ic_initiative", [TEX_IC_MATK] = "ic_matk", [TEX_IC_RATK] = "ic_ratk",
    [TEX_IC_MDEF] = "ic_mdef", [TEX_IC_RDEF] = "ic_rdef",
    [TEX_IC_ARMOR_HEAD] = "ic_armor_head", [TEX_IC_ARMOR_BODY] = "ic_armor_body",
    [TEX_IC_COIN] = "ic_coin", [TEX_IC_FOOD] = "ic_food", [TEX_IC_MED] = "ic_med",
    [TEX_IC_STAR] = "ic_star", [TEX_IC_DAY] = "ic_day", [TEX_IC_MOOD] = "ic_mood",
    [TEX_IC_WEIGHT] = "ic_weight",
    [TEX_TITLE] = "title",
    [TEX_PORTRAIT_0] = "portrait_0", [TEX_PORTRAIT_1] = "portrait_1",
    [TEX_PORTRAIT_2] = "portrait_2", [TEX_PORTRAIT_3] = "portrait_3",
    [TEX_PORTRAIT_4] = "portrait_4", [TEX_PORTRAIT_5] = "portrait_5",
    [TEX_PORTRAIT_6] = "portrait_6", [TEX_PORTRAIT_7] = "portrait_7",
    [TEX_PORTRAIT_8] = "portrait_8", [TEX_PORTRAIT_9] = "portrait_9",
    [TEX_PORTRAIT_10] = "portrait_10", [TEX_PORTRAIT_11] = "portrait_11",
};

static int* build_codepoints(int* count) {
    /* ASCII + Latin-1 (включая «») + кириллица + типографика */
    static int cps[700];
    int n = 0;
    for (int c = 0x20; c <= 0x17F; c++) cps[n++] = c;
    for (int c = 0x400; c <= 0x4FF; c++) cps[n++] = c;
    cps[n++] = 0x2013; cps[n++] = 0x2014;
    cps[n++] = 0x2018; cps[n++] = 0x2019;
    cps[n++] = 0x201C; cps[n++] = 0x201D;
    cps[n++] = 0x2026;
    *count = n;
    return cps;
}

void ui_assets_load(void) {
    int cp_count = 0;
    int* cps = build_codepoints(&cp_count);

    unsigned int len;
    const unsigned char* d;
    d = asset_get("font_sans", &len);
    if (d) g_font_body = LoadFontFromMemory(".ttf", d, (int)len, 17, cps, cp_count);
    d = asset_get("font_sans_bold", &len);
    if (d) g_font_bold = LoadFontFromMemory(".ttf", d, (int)len, 21, cps, cp_count);
    d = asset_get("font_serif_bold", &len);
    if (d) g_font_title = LoadFontFromMemory(".ttf", d, (int)len, 36, cps, cp_count);

    for (int i = 0; i < TEX_COUNT; i++) {
        g_tex[i].id = 0;
        if (!tex_names[i]) continue;
        d = asset_get(tex_names[i], &len);
        if (!d) continue;
        Image img = LoadImageFromMemory(".png", d, (int)len);
        g_tex[i] = LoadTextureFromImage(img);
        UnloadImage(img);
        SetTextureFilter(g_tex[i], TEXTURE_FILTER_POINT);
    }
}

void ui_assets_unload(void) {
    for (int i = 0; i < TEX_COUNT; i++)
        if (g_tex[i].id) UnloadTexture(g_tex[i]);
    UnloadFont(g_font_body);
    UnloadFont(g_font_bold);
    UnloadFont(g_font_title);
}

/* --------------------------------------------------------- виджеты ----- */
bool ui_button(Rect r, const char* label, bool enabled) {
    Vector2 m = GetMousePosition();
    bool hover = enabled && !s_blocked && ui_point_in(r, m);
    bool down = hover && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool click = hover && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);

    Color base = COL_PANEL_LT;
    if (!enabled) base = (Color){ 38, 32, 28, 255 };
    else if (down) base = (Color){ 92, 72, 48, 255 };
    else if (hover) base = (Color){ 84, 66, 46, 255 };

    DrawRectangleRounded(R(r.x, r.y, r.width, r.height), 0.18f, 4, (Color){ 18, 14, 12, 255 });
    DrawRectangleRounded(R(r.x + 2, r.y + 2, r.width - 4, r.height - 4), 0.16f, 4, base);
    if (enabled) DrawRectangleRoundedLines(R(r.x + 2, r.y + 2, r.width - 4, r.height - 4), 0.16f, 4, COL_ACCENT);

    Color tc = enabled ? COL_TEXT : COL_TEXT_DIM;
    Vector2 sz = MeasureTextEx(g_font_body, label, 17, 0);
    DrawTextEx(g_font_body, label,
               (Vector2){ r.x + (r.width - sz.x) / 2.0f, r.y + (r.height - sz.y) / 2.0f }, 17, 0, tc);
    return click;
}

bool ui_button_small(Rect r, const char* label, bool enabled) {
    Vector2 m = GetMousePosition();
    bool hover = enabled && !s_blocked && ui_point_in(r, m);
    bool down = hover && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool click = hover && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);

    Color base = COL_PANEL_LT;
    if (!enabled) base = (Color){ 38, 32, 28, 255 };
    else if (down) base = (Color){ 92, 72, 48, 255 };
    else if (hover) base = (Color){ 84, 66, 46, 255 };
    DrawRectangleRounded(R(r.x, r.y, r.width, r.height), 0.25f, 3, (Color){ 18, 14, 12, 255 });
    DrawRectangleRounded(R(r.x + 1.5f, r.y + 1.5f, r.width - 3, r.height - 3), 0.22f, 3, base);

    Color tc = enabled ? COL_TEXT : COL_TEXT_DIM;
    Vector2 sz = MeasureTextEx(g_font_body, label, 15, 0);
    DrawTextEx(g_font_body, label,
               (Vector2){ r.x + (r.width - sz.x) / 2.0f, r.y + (r.height - sz.y) / 2.0f }, 15, 0, tc);
    return click;
}

bool ui_button_icon(Rect r, const char* label, bool enabled, int icon_tex) {
    bool click = ui_button(r, label, enabled);
    if (icon_tex >= 0 && icon_tex < TEX_COUNT && g_tex[icon_tex].id) {
        Rectangle src = { 0, 0, (float)g_tex[icon_tex].width, (float)g_tex[icon_tex].height };
        Rectangle dst = { r.x + 10, r.y + r.height / 2 - 11, 22, 22 };
        DrawTexturePro(g_tex[icon_tex], src, dst, (Vector2){ 0, 0 }, 0, WHITE);
    }
    return click;
}

void ui_panel(Rect r) {
    DrawRectangleRounded(R(r.x - 3, r.y - 3, r.width + 6, r.height + 6), 0.06f, 3, (Color){ 12, 10, 8, 235 });
    DrawRectangleRounded(R(r.x, r.y, r.width, r.height), 0.05f, 3, COL_PANEL);
    /* текстура дерева поверх */
    if (g_tex[TEX_WOOD].id) {
        Rectangle src = { 0, 0, 128, 128 };
        Rectangle dst = { r.x, r.y, r.width, r.height };
        DrawTexturePro(g_tex[TEX_WOOD], src, dst, (Vector2){ 0, 0 }, 0, (Color){ 255, 255, 255, 26 });
    }
    DrawRectangleRoundedLines(R(r.x, r.y, r.width, r.height), 0.05f, 3, COL_ACCENT);
}

void ui_panel_parchment(Rect r) {
    DrawRectangleRounded(R(r.x, r.y, r.width, r.height), 0.04f, 3, (Color){ 206, 182, 140, 255 });
    if (g_tex[TEX_PARCHMENT].id) {
        Rectangle src = { 0, 0, 128, 128 };
        Rectangle dst = { r.x, r.y, r.width, r.height };
        DrawTexturePro(g_tex[TEX_PARCHMENT], src, dst, (Vector2){ 0, 0 }, 0, (Color){ 255, 255, 255, 210 });
    }
    DrawRectangleRoundedLines(R(r.x, r.y, r.width, r.height), 0.04f, 3, (Color){ 96, 74, 46, 255 });
}

void ui_header(Rect r, const char* title) {
    DrawRectangleRounded(R(r.x, r.y, r.width, r.height), 0.12f, 4, (Color){ 30, 24, 20, 255 });
    if (g_tex[TEX_STONE].id) {
        Rectangle src = { 0, 0, 128, 128 };
        Rectangle dst = { r.x, r.y, r.width, r.height };
        DrawTexturePro(g_tex[TEX_STONE], src, dst, (Vector2){ 0, 0 }, 0, (Color){ 255, 255, 255, 40 });
    }
    DrawRectangleRoundedLines(R(r.x, r.y, r.width, r.height), 0.12f, 4, COL_ACCENT);
    Vector2 sz = MeasureTextEx(g_font_title, title, 26, 0);
    DrawTextEx(g_font_title, title,
               (Vector2){ r.x + (r.width - sz.x) / 2.0f, r.y + (r.height - sz.y) / 2.0f - 1 }, 26, 0, COL_HEADER);
}

void ui_text(Rect r, const char* text, int size, Color c) {
    DrawTextEx(g_font_body, text, (Vector2){ r.x, r.y }, (float)size, 0, c);
}

void ui_text_right(Rect r, const char* text, int size, Color c) {
    Vector2 sz = MeasureTextEx(g_font_body, text, (float)size, 0);
    DrawTextEx(g_font_body, text, (Vector2){ r.x + r.width - sz.x, r.y }, (float)size, 0, c);
}

void ui_text_center(Rect r, const char* text, int size, Color c, Font f) {
    Vector2 sz = MeasureTextEx(f, text, (float)size, 0);
    DrawTextEx(f, text, (Vector2){ r.x + (r.width - sz.x) / 2.0f, r.y + (r.height - sz.y) / 2.0f },
               (float)size, 0, c);
}

void ui_textf(Rect r, int size, Color c, const char* fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    ui_text(r, buf, size, c);
}

void ui_textf_right(Rect r, int size, Color c, const char* fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    ui_text_right(r, buf, size, c);
}

void ui_textf_center(Rect r, int size, Color c, const char* fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    ui_text_center(r, buf, size, c, g_font_body);
}

void ui_text_wrap(Rect r, const char* text, int size, Color c, float line_h) {
    const char* p = text;
    float y = r.y;
    char line[512];
    line[0] = 0;
    while (*p) {
        /* очередное слово */
        while (*p == ' ') p++;
        if (!*p) break;
        const char* start = p;
        while (*p && *p != ' ') p++;
        int wlen = (int)(p - start);
        char word[256];
        if (wlen > 255) wlen = 255;
        memcpy(word, start, (size_t)wlen);
        word[wlen] = 0;
        char trial[512];
        if (line[0]) snprintf(trial, sizeof(trial), "%s %s", line, word);
        else snprintf(trial, sizeof(trial), "%s", word);
        if (MeasureTextEx(g_font_body, trial, (float)size, 0).x > r.width && line[0]) {
            DrawTextEx(g_font_body, line, (Vector2){ r.x, y }, (float)size, 0, c);
            y += line_h;
            snprintf(line, sizeof(line), "%s", word);
        } else {
            snprintf(line, sizeof(line), "%s", trial);
        }
    }
    if (line[0]) DrawTextEx(g_font_body, line, (Vector2){ r.x, y }, (float)size, 0, c);
}

void ui_bar(Rect r, float frac, Color c, const char* label) {
    frac = frac < 0 ? 0 : (frac > 1 ? 1 : frac);
    DrawRectangleRounded(R(r.x, r.y, r.width, r.height), 0.35f, 3, (Color){ 18, 14, 12, 255 });
    if (frac > 0.01f)
        DrawRectangleRounded(R(r.x + 1.5f, r.y + 1.5f, (r.width - 3) * frac, r.height - 3), 0.35f, 3, c);
    if (label && label[0]) {
        Vector2 sz = MeasureTextEx(g_font_body, label, 13, 0);
        DrawTextEx(g_font_body, label,
                   (Vector2){ r.x + (r.width - sz.x) / 2.0f, r.y + (r.height - sz.y) / 2.0f - 1 }, 13, 0,
                   (Color){ 235, 225, 205, 255 });
    }
}

void ui_icon(Rect r, TexId tex) {
    if (tex < 0 || tex >= TEX_COUNT || !g_tex[tex].id) return;
    Rectangle src = { 0, 0, (float)g_tex[tex].width, (float)g_tex[tex].height };
    Rectangle dst = { r.x, r.y, r.width, r.height };
    DrawTexturePro(g_tex[tex], src, dst, (Vector2){ 0, 0 }, 0, WHITE);
}

void ui_portrait(Rect r, int idx) {
    if (idx < 0 || idx > 11) return;
    Rectangle src = { 0, 0, 96, 96 };
    Rectangle dst = { r.x, r.y, r.width, r.height };
    DrawTexturePro(g_tex[TEX_PORTRAIT_0 + idx], src, dst, (Vector2){ 0, 0 }, 0, WHITE);
    DrawRectangleLinesEx((Rectangle){ r.x, r.y, r.width, r.height }, 2, (Color){ 96, 74, 46, 255 });
}

void ui_separator(Rect r) {
    DrawRectangle((int)r.x, (int)r.y, (int)r.width, 2, (Color){ 96, 74, 46, 160 });
}

void ui_tooltip(Rect at, const char* text) {
    if (!text || !text[0]) return;
    Vector2 sz = MeasureTextEx(g_font_body, text, 15, 0);
    Rect r = R(at.x, at.y - sz.y - 12, sz.x + 18, sz.y + 12);
    if (r.x + r.width > GetScreenWidth()) r.x = GetScreenWidth() - r.width - 4;
    DrawRectangleRounded(r, 0.2f, 3, (Color){ 18, 14, 12, 235 });
    DrawRectangleRoundedLines(r, 0.2f, 3, COL_ACCENT);
    ui_text(R(r.x + 9, r.y + 4, r.width, r.height), text, 15, COL_TEXT);
}
