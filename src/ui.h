/* ui.h — виджеты интерфейса (immediate mode) поверх raylib. */
#ifndef UI_H
#define UI_H

#include "raylib.h"

typedef Rectangle Rect;

#define R(x, y, w, h) ((Rectangle){ (x), (y), (w), (h) })
static inline bool ui_point_in(Rect r, Vector2 p) {
    return p.x >= r.x && p.x < r.x + r.width && p.y >= r.y && p.y < r.y + r.height;
}

/* Палитра */
#define COL_BG        (Color){ 24, 20, 18, 255 }
#define COL_PANEL     (Color){ 44, 36, 30, 255 }
#define COL_PANEL_LT  (Color){ 62, 50, 40, 255 }
#define COL_TEXT      (Color){ 222, 204, 168, 255 }
#define COL_TEXT_DIM  (Color){ 156, 138, 112, 255 }
#define COL_TEXT_DARK (Color){ 46, 34, 24, 255 }
#define COL_GOLD      (Color){ 222, 184, 92, 255 }
#define COL_GOOD      (Color){ 126, 186, 108, 255 }
#define COL_BAD       (Color){ 196, 92, 76, 255 }
#define COL_HEADER    (Color){ 236, 212, 160, 255 }
#define COL_ACCENT    (Color){ 168, 124, 62, 255 }

/* Шрифты и текстуры (загружаются в main) */
extern Font g_font_body;    /* 17 */
extern Font g_font_bold;    /* 21 */
extern Font g_font_title;   /* 36 */

/* Идентификаторы текстур */
typedef enum {
    TEX_DEEP = 0, TEX_WATER, TEX_SAND, TEX_GRASS, TEX_FOREST, TEX_HILLS,
    TEX_MOUNTAIN, TEX_SWAMP, TEX_SNOW, TEX_DIRT,
    TEX_IC_VILLAGE, TEX_IC_TOWN, TEX_IC_CITY, TEX_IC_CASTLE, TEX_IC_RUINS,
    TEX_IC_CAMP, TEX_IC_MINE, TEX_IC_SHRINE, TEX_IC_PARTY, TEX_IC_FARM, TEX_IC_BANDIT,
    TEX_WOOD, TEX_PARCHMENT, TEX_STONE,
    TEX_IC_HP, TEX_IC_FATIGUE, TEX_IC_RESOLVE, TEX_IC_INITIATIVE, TEX_IC_MATK,
    TEX_IC_RATK, TEX_IC_MDEF, TEX_IC_RDEF, TEX_IC_ARMOR_HEAD, TEX_IC_ARMOR_BODY,
    TEX_IC_COIN, TEX_IC_FOOD, TEX_IC_MED, TEX_IC_STAR, TEX_IC_DAY, TEX_IC_MOOD,
    TEX_IC_WEIGHT,
    TEX_TITLE,
    TEX_PORTRAIT_0, TEX_PORTRAIT_1, TEX_PORTRAIT_2, TEX_PORTRAIT_3,
    TEX_PORTRAIT_4, TEX_PORTRAIT_5, TEX_PORTRAIT_6, TEX_PORTRAIT_7,
    TEX_PORTRAIT_8, TEX_PORTRAIT_9, TEX_PORTRAIT_10, TEX_PORTRAIT_11,
    TEX_COUNT
} TexId;

extern Texture2D g_tex[TEX_COUNT];

void ui_assets_load(void);
void ui_assets_unload(void);

/* Блокировка ввода под модальными окнами */
void ui_set_blocked(bool blocked);
bool ui_blocked(void);

/* Виджеты */
bool ui_button(Rect r, const char* label, bool enabled);
bool ui_button_small(Rect r, const char* label, bool enabled);
bool ui_button_icon(Rect r, const char* label, bool enabled, int icon_tex);
void  ui_panel(Rect r);
void  ui_panel_parchment(Rect r);
void  ui_header(Rect r, const char* title);
void  ui_text(Rect r, const char* text, int size, Color c);
void  ui_text_wrap(Rect r, const char* text, int size, Color c, float line_h);
void  ui_text_center(Rect r, const char* text, int size, Color c, Font f);
void  ui_text_right(Rect r, const char* text, int size, Color c);
void  ui_bar(Rect r, float frac, Color c, const char* label);
void  ui_icon(Rect r, TexId tex);
void  ui_portrait(Rect r, int idx);
void  ui_separator(Rect r);
void  ui_tooltip(Rect at, const char* text);
void  ui_textf(Rect r, int size, Color c, const char* fmt, ...);
void  ui_textf_right(Rect r, int size, Color c, const char* fmt, ...);
void  ui_textf_center(Rect r, int size, Color c, const char* fmt, ...);

#endif
