/* render.c — все экраны игры: карта, города, отряд, модальные окна. */
#include "ui.h"
#include "game.h"
#include "render.h"
#include "battle.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

typedef enum { SC_MENU, SC_MAP, SC_TOWN, SC_PARTY, SC_BATTLE } Screen;

static Screen s_screen = SC_MENU;
static int s_town_idx = -1;
static int s_town_tab = 0;          /* 0 обзор, 1 таверна, 2 рынок, 3 храм, 4 гильдия, 5 кузница */
static int s_sel_brother = 0;
static int s_sel_item = -1;
static float s_cam_x = 0, s_cam_y = 0, s_zoom = 1.0f;
static int s_scroll_inv = 0, s_scroll_rec = 0, s_scroll_stock = 0, s_scroll_ctr = 0, s_scroll_log = 0;
static int s_level_picks[3] = { -1, -1, -1 };
static int s_level_perk = -1;
static int s_level_brother = 0;
static float s_time = 0.0f;
static bool s_quit_confirm = false;
void exit_request(void);   /* реализована в main.c */

static const int TILE_PX = 64;

static const Color TERR_MINI[TERR_COUNT] = {
    { 32, 52, 86, 255 }, { 48, 84, 122, 255 }, { 176, 154, 106, 255 },
    { 96, 122, 64, 255 }, { 58, 88, 48, 255 }, { 118, 128, 82, 255 },
    { 122, 116, 106, 255 }, { 62, 78, 56, 255 }, { 206, 216, 228, 255 },
};

static TexId terrain_tex[TERR_COUNT] = {
    TEX_DEEP, TEX_WATER, TEX_SAND, TEX_GRASS, TEX_FOREST,
    TEX_HILLS, TEX_MOUNTAIN, TEX_SWAMP, TEX_SNOW,
};

static TexId town_icon_tex[4] = { TEX_IC_VILLAGE, TEX_IC_TOWN, TEX_IC_CITY, TEX_IC_CASTLE };
static const char* loc_icon_spr[LOC_TYPE_COUNT] = {
    "mr_struct_12", "mr_env_21", "mr_env_11", "mr_struct_04", "mr_struct_19"
};
static const char* town_icon_spr[4] = {
    "mr_struct_01", "mr_struct_17", "mr_struct_20", "mr_struct_02"
};
static TexId loc_icon_tex[LOC_TYPE_COUNT] = {
    TEX_IC_RUINS, TEX_IC_CAMP, TEX_IC_MINE, TEX_IC_SHRINE, TEX_IC_FARM,
};

/* ------------------------------------------------------------- утилы --- */
static Vector2 world_to_screen(float wx, float wy) {
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    return (Vector2){
        (wx - s_cam_x) * s_zoom + sw / 2.0f,
        (wy - s_cam_y) * s_zoom + (sh - 36 + 54) / 2.0f
    };
}

static Vector2 screen_to_world(float sx, float sy) {
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    return (Vector2){
        (sx - sw / 2.0f) / s_zoom + s_cam_x,
        (sy - (sh - 36 + 54) / 2.0f) / s_zoom + s_cam_y
    };
}

void render_center_on(float wx, float wy) {
    s_cam_x = wx;
    s_cam_y = wy;
}

void render_init(void) {
    s_screen = SC_MENU;
    s_zoom = 1.0f;
    if (g.n_towns > 0) {
        s_cam_x = (float)g.towns[0].x * TILE_PX;
        s_cam_y = (float)g.towns[0].y * TILE_PX;
    }
}

int render_screen_id(void) { return (int)s_screen; }
void render_set_screen(int s) { s_screen = (Screen)s; }

/* ------------------------------------------------------------- меню ---- */
static void draw_menu(void) {
    if (g_tex[TEX_TITLE].id) {
        Rectangle src = { 0, 0, (float)g_tex[TEX_TITLE].width, (float)g_tex[TEX_TITLE].height };
        Rectangle dst = { 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() };
        DrawTexturePro(g_tex[TEX_TITLE], src, dst, (Vector2){ 0, 0 }, 0, WHITE);
    } else {
        ClearBackground(COL_BG);
    }

    float cx = GetScreenWidth() / 2.0f;
    ui_text_center(R(cx - 400, 50, 800, 70), "STEINBACH", 52, WHITE, g_font_title);
    ui_text_center(R(cx - 400, 122, 800, 44), "Сага о наёмниках", 28, COL_HEADER, g_font_title);
    ui_text_center(R(cx - 400, 176, 800, 30),
                   "Карта мира · Города · Ролевая система", 18, COL_TEXT, g_font_body);

    float bw = 320, bh = 52, by = 300;
    if (ui_button(R(cx - bw / 2, by, bw, bh), "Новая игра", true)) {
        unsigned int seed = (unsigned int)(GetTime() * 1000.0) ^ 0x5EED5EEDu;
        game_new(140, 90, seed, 1);
        render_init();
        s_screen = SC_MAP;
        render_center_on((float)g.towns[0].x * TILE_PX, (float)g.towns[0].y * TILE_PX);
    }
    if (ui_button(R(cx - bw / 2, by + 66, bw, bh), "Загрузить игру", true)) {
        char path[512];
        snprintf(path, sizeof(path), "%ssave1.sav", GetApplicationDirectory());
        if (load_game(path)) {
            render_init();
            s_screen = SC_MAP;
            render_center_on(g.party_x * TILE_PX, g.party_y * TILE_PX);
        }
    }
    if (ui_button(R(cx - bw / 2, by + 132, bw, bh), "Выход", true)) {
        s_quit_confirm = true;
    }

    if (s_quit_confirm) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){ 0, 0, 0, 160 });
        ui_panel(R(cx - 240, 280, 480, 170));
        ui_text_center(R(cx - 240, 300, 480, 30), "Выйти из игры?", 20, COL_HEADER, g_font_bold);
        if (ui_button(R(cx - 200, 360, 180, 46), "Да", true)) {
            /* выход обрабатывается в main через флаг */
            s_quit_confirm = false;
            exit_request();
        }
        if (ui_button(R(cx + 20, 360, 180, 46), "Нет", true)) s_quit_confirm = false;
    }

    ui_text(R(16, GetScreenHeight() - 30, 700, 24),
            "Управление: WASD/стрелки — карта, колесо — масштаб, ПКМ — панорама, F5/F9 — сохранение/загрузка",
            14, COL_TEXT_DIM);
}

/* ----------------------------------------------------------- верхняя --- */
static void draw_topbar(void) {
    float w = (float)GetScreenWidth();
    DrawRectangle(0, 0, (int)w, 54, (Color){ 22, 18, 15, 255 });
    DrawRectangle(0, 53, (int)w, 2, COL_ACCENT);
    if (g_tex[TEX_WOOD].id) {
        Rectangle src = { 0, 0, 128, 128 };
        Rectangle dst = { 0, 0, w, 54 };
        DrawTexturePro(g_tex[TEX_WOOD], src, dst, (Vector2){ 0, 0 }, 0, (Color){ 255, 255, 255, 22 });
    }

    ui_textf(R(14, 8, 340, 22), 15, COL_TEXT_DIM, "Компания");
    ui_textf(R(14, 26, 340, 24), 19, COL_HEADER, "%s", g.company_name);

    float x = 320;
    ui_icon(R(x, 12, 26, 26), TEX_IC_DAY);
    ui_textf(R(x + 32, 16, 150, 22), 17, COL_TEXT, "День %d, %02d:00", g.day, g.hour);
    x += 190;
    ui_icon(R(x, 12, 26, 26), TEX_IC_COIN);
    ui_textf(R(x + 32, 16, 140, 22), 17, COL_GOLD, "%d крон", g.crowns);
    x += 170;
    ui_icon(R(x, 12, 26, 26), TEX_IC_FOOD);
    ui_textf(R(x + 32, 16, 120, 22), 17, COL_TEXT, "Еда %d", g.food);
    x += 120;
    ui_icon(R(x, 12, 26, 26), TEX_IC_STAR);
    ui_textf(R(x + 32, 16, 140, 22), 17, COL_TEXT, "Репутация %d", g.renown);

    float bw = 108;
    float bx = w - bw - 12;
    if (ui_button(R(bx, 10, bw, 34), "Меню", true)) { s_screen = SC_MENU; s_quit_confirm = false; }
    bx -= bw + 8;
    if (ui_button(R(bx, 10, bw, 34), "Загрузить", true)) {
        char path[512];
        snprintf(path, sizeof(path), "%ssave1.sav", GetApplicationDirectory());
        if (load_game(path)) {
            render_init();
            s_screen = SC_MAP;
            render_center_on(g.party_x * TILE_PX, g.party_y * TILE_PX);
        }
    }
    bx -= bw + 8;
    if (ui_button(R(bx, 10, bw, 34), "Сохранить", true)) {
        char path[512];
        snprintf(path, sizeof(path), "%ssave1.sav", GetApplicationDirectory());
        save_game(path);
    }
    bx -= bw + 8;
    if (ui_button(R(bx, 10, bw, 34), "Отряд", true)) s_screen = SC_PARTY;
    bx -= bw + 8;
    if (ui_button(R(bx, 10, bw, 34), "Карта", true)) s_screen = SC_MAP;
}

/* -------------------------------------------------------------- карта -- */
static void draw_minimap(Rect r) {
    ui_panel(r);
    float sx = r.width / (float)g.world_w;
    float sy = r.height / (float)g.world_h;
    for (int y = 0; y < g.world_h; y++) {
        for (int x = 0; x < g.world_w; x++) {
            int t = g.terrain[y * g.world_w + x];
            DrawRectangle((int)(r.x + x * sx), (int)(r.y + y * sy),
                          (int)ceilf(sx), (int)ceilf(sy), TERR_MINI[t]);
        }
    }
    for (int i = 0; i < g.n_towns; i++) {
        DrawRectangle((int)(r.x + g.towns[i].x * sx) - 2, (int)(r.y + g.towns[i].y * sy) - 2,
                      5, 5, g.towns[i].visited ? COL_GOLD : (Color){ 120, 100, 60, 255 });
    }
    int px = (int)(r.x + g.party_x * sx), py = (int)(r.y + g.party_y * sy);
    DrawRectangle(px - 3, py - 3, 7, 7, (Color){ 240, 240, 245, 255 });

    Vector2 m = GetMousePosition();
    if (!ui_blocked() && ui_point_in(r, m) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        float wx = (m.x - r.x) / sx * TILE_PX;
        float wy = (m.y - r.y) / sy * TILE_PX;
        render_center_on(wx, wy);
    }
}

static void draw_map_screen(float dt) {
    (void)dt;
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    float view_top = 54, view_bot = sh - 36;

    /* --- камера --- */
    bool modal = ui_blocked();
    if (!modal) {
        float spd = 620.0f / s_zoom * dt;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    s_cam_y -= spd;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  s_cam_y += spd;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  s_cam_x -= spd;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) s_cam_x += spd;
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 d = GetMouseDelta();
            s_cam_x -= d.x / s_zoom;
            s_cam_y -= d.y / s_zoom;
        }
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            Vector2 m = GetMousePosition();
            Vector2 before = screen_to_world(m.x, m.y);
            s_zoom = clampf(s_zoom * (1.0f + wheel * 0.12f), 0.45f, 2.2f);
            Vector2 after = screen_to_world(m.x, m.y);
            s_cam_x += before.x - after.x;
            s_cam_y += before.y - after.y;
        }
        float map_w = (float)g.world_w * TILE_PX, map_h = (float)g.world_h * TILE_PX;
        s_cam_x = clampf(s_cam_x, 0, map_w);
        s_cam_y = clampf(s_cam_y, -100, map_h + 100);
    }

    /* --- тайлы --- */
    DrawRectangle(0, (int)view_top, (int)sw, (int)(view_bot - view_top), (Color){ 18, 24, 32, 255 });
    Vector2 wtl = screen_to_world(0, view_top);
    Vector2 wbr = screen_to_world(sw, view_bot);
    int x0 = (int)floorf(wtl.x / TILE_PX) - 1, x1 = (int)ceilf(wbr.x / TILE_PX) + 1;
    int y0 = (int)floorf(wtl.y / TILE_PX) - 1, y1 = (int)ceilf(wbr.y / TILE_PX) + 1;
    x0 = clampi(x0, -1, g.world_w);
    y0 = clampi(y0, -1, g.world_h);
    x1 = clampi(x1, -1, g.world_w + 1);
    y1 = clampi(y1, -1, g.world_h + 1);

    BeginScissorMode(0, (int)view_top, (int)sw, (int)(view_bot - view_top));

    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            float wx = (float)x * TILE_PX, wy = (float)y * TILE_PX;
            Vector2 sp = world_to_screen(wx, wy);
            float ts = TILE_PX * s_zoom;
            if (x < 0 || y < 0 || x >= g.world_w || y >= g.world_h) {
                DrawRectangle((int)sp.x, (int)sp.y, (int)ts + 1, (int)ts + 1, (Color){ 24, 34, 48, 255 });
                continue;
            }
            int t = g.terrain[y * g.world_w + x];
            Texture2D tex = g_tex[terrain_tex[t]];
            if (tex.id) {
                float flip = (g.decor[y * g.world_w + x] & 1) ? -1.0f : 1.0f;
                Rectangle src = { flip < 0 ? (float)tex.width : 0, 0,
                                  (float)tex.width * flip, (float)tex.height };
                Rectangle dst = { sp.x, sp.y, ts + 1.0f, ts + 1.0f };
                DrawTexturePro(tex, src, dst, (Vector2){ 0, 0 }, 0, WHITE);
            }
        }
    }

    /* --- декор: деревья и камни на тайлах --- */
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            if (x < 0 || y < 0 || x >= g.world_w || y >= g.world_h) continue;
            int t = g.terrain[y * g.world_w + x];
            int dv = g.decor[y * g.world_w + x];
            const char* spr = NULL;
            if (t == TERR_FOREST) {
                static const char* trees[3] = { "mr_env_02", "mr_env_03", "mr_env_04" };
                spr = trees[dv % 3];
            } else if (t == TERR_HILLS || t == TERR_MOUNTAIN) {
                static const char* rocks[3] = { "mr_env_08", "mr_env_09", "mr_env_10" };
                if ((dv & 3) != 0) continue;   /* не на каждом тайле */
                spr = rocks[(dv >> 2) % 3];
            } else if (t == TERR_SWAMP) {
                static const char* bush[2] = { "mr_env_13", "mr_env_20" };
                if ((dv & 1) == 0) continue;
                spr = bush[(dv >> 1) % 2];
            }
            if (!spr) continue;
            Vector2 sp = world_to_screen(x * TILE_PX, y * TILE_PX);
            float ts = TILE_PX * 1.0f * s_zoom;
            sprite_draw_fit(spr, R(sp.x, sp.y, ts, ts), WHITE);
        }
    }

    /* --- дороги --- */
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            if (x < 0 || y < 0 || x >= g.world_w || y >= g.world_h) continue;
            if (!g.road[y * g.world_w + x]) continue;
            Vector2 sp = world_to_screen((x + 0.5f) * TILE_PX, (y + 0.5f) * TILE_PX);
            float r = TILE_PX * 0.75f * s_zoom;
            DrawCircleV(sp, r, (Color){ 118, 92, 62, 200 });
            DrawCircleV(sp, r * 0.62f, (Color){ 136, 108, 74, 210 });
        }
    }

    /* --- локации --- */
    Vector2 mouse_w = screen_to_world(GetMouseX(), GetMouseY());
    int hover_town = -1, hover_loc = -1;
    for (int i = 0; i < g.n_locations; i++) {
        Location* L = &g.locations[i];
        float cx = (L->x + 0.5f) * TILE_PX, cy = (L->y + 0.5f) * TILE_PX;
        Vector2 sp = world_to_screen(cx, cy);
        float sz = TILE_PX * 1.15f * s_zoom;
        Color tint = L->cleared ? (Color){ 150, 150, 150, 210 } : WHITE;
        sprite_draw_fit(loc_icon_spr[L->type], R(sp.x - sz / 2, sp.y - sz / 2, sz, sz), tint);
        float d = sqrtf((mouse_w.x - cx) * (mouse_w.x - cx) + (mouse_w.y - cy) * (mouse_w.y - cy));
        if (d < TILE_PX * 0.7f && !modal) hover_loc = i;
    }

    /* --- города --- */
    for (int i = 0; i < g.n_towns; i++) {
        Town* t = &g.towns[i];
        float cx = (t->x + 0.5f) * TILE_PX, cy = (t->y + 0.5f) * TILE_PX;
        Vector2 sp = world_to_screen(cx, cy);
        float sz = TILE_PX * 1.8f * s_zoom;
        sprite_draw_fit(town_icon_spr[t->type], R(sp.x - sz / 2, sp.y - sz / 2, sz, sz), WHITE);
        float d = sqrtf((mouse_w.x - cx) * (mouse_w.x - cx) + (mouse_w.y - cy) * (mouse_w.y - cy));
        if (d < TILE_PX * 0.9f && !modal) {
            hover_town = i;
            DrawCircleLines((int)sp.x, (int)sp.y, sz * 0.52f, COL_GOLD);
        }
        /* название */
        const char* nm = t->name;
        Vector2 tsz = MeasureTextEx(g_font_body, nm, 14, 0);
        DrawRectangle((int)(sp.x - tsz.x / 2 - 4), (int)(sp.y + sz * 0.42f),
                      (int)tsz.x + 8, 18, (Color){ 16, 12, 10, 165 });
        DrawTextEx(g_font_body, nm,
                   (Vector2){ sp.x - tsz.x / 2, sp.y + sz * 0.42f + 1 }, 14, 0,
                   t->visited ? COL_TEXT : COL_TEXT_DIM);
    }

    /* --- отряд --- */
    {
        float bob = sinf(s_time * 3.0f) * 3.0f;
        Vector2 sp = world_to_screen(g.party_x * TILE_PX, g.party_y * TILE_PX + bob);
        float sz = TILE_PX * 1.15f * s_zoom;
        DrawCircle((int)sp.x, (int)(sp.y + sz * 0.28f), sz * 0.34f, (Color){ 230, 210, 120, 70 });
        sprite_draw_fit("mr_unit_04", R(sp.x - sz / 2, sp.y - sz / 2, sz, sz), WHITE);
        DrawCircleLines((int)sp.x, (int)sp.y, sz * 0.62f, (Color){ 230, 220, 190, 130 });
    }

    /* --- клики --- */
    if (!modal) {
        if (hover_town >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Town* t = &g.towns[hover_town];
            float dx = g.party_x - (t->x + 0.5f), dy = g.party_y - (t->y + 0.5f);
            if (dx * dx + dy * dy < 2.2f * 2.2f) {
                s_town_idx = hover_town;
                s_town_tab = 0;
                s_screen = SC_TOWN;
            } else {
                travel_set_dest(t->x, t->y);
            }
        } else if (hover_loc >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            travel_set_dest(g.locations[hover_loc].x, g.locations[hover_loc].y);
        } else if (hover_town < 0 && hover_loc < 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            int tx = (int)floorf(mouse_w.x / TILE_PX), ty = (int)floorf(mouse_w.y / TILE_PX);
            if (tx >= 0 && ty >= 0 && tx < g.world_w && ty < g.world_h &&
                tile_move_cost(tx, ty) > 0) {
                travel_set_dest(tx, ty);
            }
        }
    }

    EndScissorMode();

    /* --- подсказки --- */
    if (hover_town >= 0) {
        Town* t = &g.towns[hover_town];
        char tip[256];
        snprintf(tip, sizeof(tip), "%s — %s · население %d · %s",
                 t->name, g_town_type_name[t->type], t->population,
                 t->visited ? "известен" : "неизвестен");
        ui_tooltip(R((float)GetMouseX() + 14, (float)GetMouseY() + 8, 0, 0), tip);
    } else if (hover_loc >= 0) {
        Location* L = &g.locations[hover_loc];
        char tip[256];
        snprintf(tip, sizeof(tip), "%s — %s · сила %d%s",
                 L->name, g_loc_name[L->type], L->enemy_power,
                 L->cleared ? " · зачищено" : "");
        ui_tooltip(R((float)GetMouseX() + 14, (float)GetMouseY() + 8, 0, 0), tip);
    } else {
        int tx = (int)floorf(mouse_w.x / TILE_PX), ty = (int)floorf(mouse_w.y / TILE_PX);
        if (tx >= 0 && ty >= 0 && tx < g.world_w && ty < g.world_h && !modal) {
            int t = g.terrain[ty * g.world_w + tx];
            char tip[128];
            snprintf(tip, sizeof(tip), "%s%s", g_terrain_name[t], road_at(tx, ty) ? " · дорога" : "");
            ui_tooltip(R((float)GetMouseX() + 14, (float)GetMouseY() + 8, 0, 0), tip);
        }
    }

    /* --- миникарта --- */
    draw_minimap(R(sw - 236, 66, 220, 150));

    /* --- нижняя панель --- */
    DrawRectangle(0, (int)sh - 36, (int)sw, 36, (Color){ 22, 18, 15, 255 });
    DrawRectangle(0, (int)sh - 38, (int)sw, 2, COL_ACCENT);
    const char* hint = travel_is_traveling()
        ? "Отряд в пути…  WASD — камера, колесо — масштаб, ЛКМ по карте — идти, по городу — войти"
        : "WASD — камера, колесо — масштаб, ПКМ — панорама, ЛКМ — идти/войти, P — отряд, F5/F9 — сохранение";
    ui_text(R(14, sh - 28, sw - 28, 22), hint, 14, COL_TEXT_DIM);

    /* статус пути */
    if (travel_is_traveling()) {
        ui_panel(R(14, 66, 250, 40));
        int left = g.path_len - g.path_pos;
        ui_textf(R(26, 76, 230, 22), 15, COL_TEXT, "В пути: %d тайл.", left);
    }
}

/* ------------------------------------------------------------- город --- */
static void draw_item_line(Rect r, const Item* it, bool selected) {
    if (selected) DrawRectangleRounded(r, 0.15f, 3, (Color){ 84, 66, 46, 255 });
    if (!it || it->def == ITEM_NONE) return;
    const ItemDef* d = &g_item_defs[it->def];
    Color c = COL_TEXT;
    if (d->slot == SLOT_SUPPLY) c = (Color){ 180, 190, 160, 255 };
    const char* icon = item_icon_name(it->def);
    if (icon) {
        sprite_draw_fit(icon, R(r.x + 4, r.y + 1, 26, 26), WHITE);
        DrawRectangleLinesEx(R(r.x + 4, r.y + 1, 26, 26), 1, (Color){ 90, 72, 52, 160 });
    }
    ui_textf(R(r.x + 36, r.y + 4, r.width - 152, 22), 16, c, "%s", d->name);
    if (it->durability < 100)
        ui_textf_right(R(r.x, r.y + 4, r.width - 10, 22), 14, COL_BAD, "%d%%", it->durability);
}

static void draw_town_screen(void) {
    if (s_town_idx < 0 || s_town_idx >= g.n_towns) { s_screen = SC_MAP; return; }
    Town* t = &g.towns[s_town_idx];

    /* фон: каменная стена */
    if (g_tex[TEX_STONE].id) {
        Rectangle src = { 0, 0, 128, 128 };
        Rectangle dst = { 0, 54, (float)GetScreenWidth(), (float)GetScreenHeight() - 54 };
        DrawTexturePro(g_tex[TEX_STONE], src, dst, (Vector2){ 0, 0 }, 0, (Color){ 255, 255, 255, 60 });
    }

    char title[128];
    snprintf(title, sizeof(title), "%s — %s", t->name, g_town_type_name[t->type]);
    ui_header(R(120, 62, GetScreenWidth() - 240, 46), title);

    /* левая колонка — здания */
    ui_panel(R(16, 120, 200, GetScreenHeight() - 170));
    float by = 136;
    struct { int tab; const char* name; int flag; } tabs[] = {
        { 0, "Обзор", 0 },
        { 1, "Таверна", BLD_TAVERN },
        { 2, "Рынок", BLD_MARKET },
        { 3, "Храм", BLD_TEMPLE },
        { 4, "Гильдия", BLD_GUILD },
        { 5, "Кузница", BLD_FORGE },
    };
    for (int i = 0; i < 6; i++) {
        if (tabs[i].flag && !(t->buildings & tabs[i].flag)) continue;
        bool cur = s_town_tab == tabs[i].tab;
        if (ui_button(R(28, by, 176, 42), tabs[i].name, true)) s_town_tab = tabs[i].tab;
        if (cur) DrawRectangleLinesEx((Rectangle){ 26, by - 2, 180, 46 }, 2, COL_GOLD);
        by += 50;
    }
    if (ui_button(R(28, GetScreenHeight() - 130, 176, 42), "Покинуть город", true)) {
        s_screen = SC_MAP;
    }

    /* правая часть */
    Rect content = R(232, 120, GetScreenWidth() - 248, GetScreenHeight() - 170);
    ui_panel_parchment(content);
    Rect inner = R(content.x + 18, content.y + 14, content.width - 36, content.height - 28);

    if (s_town_tab == 0) {
        ui_textf(inner, 22, COL_TEXT_DARK, "%s", t->name);
        ui_textf(R(inner.x, inner.y + 34, inner.width, 22), 16, (Color){ 90, 70, 50, 255 },
                 "%s · население %d · процветание %d", g_town_type_name[t->type], t->population, t->prosperity);
        ui_separator(R(inner.x, inner.y + 64, inner.width, 2));
        const char* desc =
            "Здешние места живут по древним обычаям. Наёмники здесь — привычное дело, "
            "а кроны открыто правят миром. Загляните в таверну за свежими слухами, "
            "на рынок за снаряжением или в гильдию за работой.";
        ui_text_wrap(R(inner.x, inner.y + 80, inner.width, 120), desc, 17, (Color){ 80, 62, 44, 255 }, 24);

        ui_textf(R(inner.x, inner.y + 210, inner.width, 22), 18, COL_TEXT_DARK, "Постройки:");
        float px = inner.x;
        float py = inner.y + 240;
        struct { const char* n; int f; } bl[] = {
            { "Таверна", BLD_TAVERN }, { "Рынок", BLD_MARKET }, { "Храм", BLD_TEMPLE },
            { "Гильдия", BLD_GUILD }, { "Кузница", BLD_FORGE },
        };
        for (int i = 0; i < 5; i++) {
            if (!(t->buildings & bl[i].f)) continue;
            DrawRectangleRounded(R(px, py, 150, 36), 0.2f, 3, (Color){ 96, 74, 46, 230 });
            ui_textf_center(R(px, py, 150, 36), 16, COL_TEXT, "%s", bl[i].n);
            px += 162;
            if (px + 150 > inner.x + inner.width) { px = inner.x; py += 44; }
        }

        ui_textf(R(inner.x, inner.y + 320, inner.width, 22), 18, COL_TEXT_DARK, "Активные контракты:");
        float cy2 = inner.y + 350;
        for (int i = 0; i < MAX_ACTIVE; i++) {
            int id = g.active_contracts[i];
            if (id < 0) continue;
            Contract* c = contract_get(id);
            if (!c) continue;
            ui_textf(R(inner.x, cy2, inner.width, 22), 16, (Color){ 80, 62, 44, 255 },
                     "• %s — награда %d крон", c->title, c->reward_gold);
            cy2 += 26;
        }
    } else if (s_town_tab == 1) {
        /* таверна */
        ui_text(R(inner.x, inner.y, inner.width, 24), "Наём новобранцев", 22, COL_TEXT_DARK);
        ui_textf(R(inner.x, inner.y + 32, inner.width, 22), 15, (Color){ 100, 80, 58, 255 },
                 "Свежие лица в таверне. Обновление — каждые 3 дня. В отряде: %d/%d",
                 g.n_brothers, MAX_BROTHERS);
        float ry = inner.y + 62;
        for (int i = 0; i < t->n_recruits && ry + 74 < inner.y + inner.height; i++) {
            const Brother* b = town_recruit_brother(s_town_idx, i);
            int price = town_recruit_price(s_town_idx, i);
            if (!b) continue;
            Rect row = R(inner.x, ry, inner.width, 70);
            DrawRectangleRounded(row, 0.12f, 3, (Color){ 70, 54, 38, 40 });
            ui_portrait(R(row.x + 8, row.y + 6, 58, 58), b->portrait);
            ui_textf(R(row.x + 76, row.y + 6, 300, 22), 17, COL_TEXT_DARK, "%s", b->name);
            ui_textf(R(row.x + 76, row.y + 30, 300, 20), 14, (Color){ 100, 80, 58, 255 },
                     "%s · уровень %d · жалование %d/день", background_name(b->background), b->level, b->wage);
            ui_textf(R(row.x + 76, row.y + 50, 460, 20), 14, (Color){ 80, 62, 44, 255 },
                     "ЗД %d  ВЫН %d  РЕШ %d  ИНЦ %d  БОЙ %d  СТР %d  ЗАЩ %d  УКЛ %d",
                     brother_stat(b, STAT_HP), brother_stat(b, STAT_FATIGUE), brother_stat(b, STAT_RESOLVE),
                     brother_stat(b, STAT_INITIATIVE), brother_stat(b, STAT_MATK), brother_stat(b, STAT_RATK),
                     brother_stat(b, STAT_MDEF), brother_stat(b, STAT_RDEF));
            bool afford = g.crowns >= price && g.n_brothers < MAX_BROTHERS;
            if (ui_button(R(row.x + row.width - 220, row.y + 18, 200, 40), "Нанять", afford)) {
                game_try_recruit(s_town_idx, i);
            }
            ui_textf_right(R(row.x + row.width - 230, row.y + 2, 220, 20), 15, COL_GOLD, "%d крон", price);
            ry += 78;
        }
        if (t->n_recruits == 0)
            ui_text(R(inner.x, ry + 10, inner.width, 24), "Сейчас в таверне никого нет.", 17, (Color){ 100, 80, 58, 255 });
    } else if (s_town_tab == 2) {
        /* рынок */
        ui_text(R(inner.x, inner.y, inner.width, 24), "Рынок", 22, COL_TEXT_DARK);
        ui_textf(R(inner.x, inner.y + 32, inner.width, 22), 15, (Color){ 100, 80, 58, 255 },
                 "Покупка и продажа снаряжения. Ваши кроны: %d", g.crowns);
        float col_w = (inner.width - 24) / 2;
        ui_text(R(inner.x, inner.y + 62, col_w, 22), "В продаже:", 18, COL_TEXT_DARK);
        float ry = inner.y + 92;
        for (int i = 0; i < t->n_stock && ry + 34 < inner.y + inner.height - 50; i++) {
            Item* it = &t->stock[i];
            if (it->def == ITEM_NONE) continue;
            Rect row = R(inner.x, ry, col_w, 30);
            draw_item_line(row, it, false);
            int price = buy_price(t, it->def);
            bool afford = g.crowns >= price;
            char lbl[32];
            snprintf(lbl, sizeof(lbl), "%d кр", price);
            if (ui_button_small(R(row.x + col_w - 90, ry + 1, 86, 28), lbl, afford)) {
                game_try_buy(s_town_idx, i);
                break;
            }
            ry += 34;
        }
        ui_text(R(inner.x + col_w + 24, inner.y + 62, col_w, 22), "Ваше снаряжение:", 18, COL_TEXT_DARK);
        ry = inner.y + 92;
        for (int i = 0; i < g.n_items && ry + 34 < inner.y + inner.height - 50; i++) {
            Item* it = &g.items[i];
            if (it->def == ITEM_NONE) continue;
            bool equipped = false;
            for (int b = 0; b < g.n_brothers && !equipped; b++)
                for (int e = 0; e < EQ_COUNT; e++)
                    if (g.brothers[b].equip[e] == i) equipped = true;
            Rect row = R(inner.x + col_w + 24, ry, col_w, 30);
            draw_item_line(row, it, false);
            if (equipped) {
                ui_textf_right(R(row.x, ry + 4, row.width - 10, 22), 13, COL_TEXT_DIM, "надето");
            } else {
                int price = sell_price(t, it->def);
                char lbl[32];
                snprintf(lbl, sizeof(lbl), "%d кр", price);
                if (ui_button_small(R(row.x + col_w - 90, ry + 1, 86, 28), lbl, t->n_stock < MAX_STOCK)) {
                    game_try_sell(s_town_idx, i);
                    break;
                }
            }
            ry += 34;
        }
    } else if (s_town_tab == 3) {
        /* храм */
        ui_text(R(inner.x, inner.y, inner.width, 24), "Храм и лекарь", 22, COL_TEXT_DARK);
        ui_text(R(inner.x, inner.y + 32, inner.width, 22),
                "Здесь лечат раны и восстанавливают силы. Плата — пожертвование.", 15, (Color){ 100, 80, 58, 255 });
        float ry = inner.y + 70;
        for (int i = 0; i < g.n_brothers && ry + 64 < inner.y + inner.height; i++) {
            Brother* b = &g.brothers[i];
            Rect row = R(inner.x, ry, inner.width, 60);
            DrawRectangleRounded(row, 0.12f, 3, (Color){ 70, 54, 38, 40 });
            ui_portrait(R(row.x + 8, row.y + 5, 50, 50), b->portrait);
            ui_textf(R(row.x + 68, row.y + 6, 320, 22), 17, COL_TEXT_DARK, "%s", b->name);
            int cost = 120 + b->level * 60;
            const char* status;
            Color sc;
            if (b->injury >= 0) {
                status = g_injuries[b->injury].name;
                sc = COL_BAD;
            } else if (b->hp < brother_stat(b, STAT_HP)) {
                status = "потерял здоровье";
                sc = (Color){ 160, 120, 60, 255 };
            } else {
                status = "здоров";
                sc = (Color){ 70, 110, 60, 255 };
            }
            ui_textf(R(row.x + 68, row.y + 32, 320, 20), 14, sc, "%s · лечение %d крон", status, cost);
            bool need = b->injury >= 0 || b->hp < brother_stat(b, STAT_HP);
            if (ui_button(R(row.x + row.width - 200, row.y + 12, 180, 38), "Лечить", need && g.crowns >= cost)) {
                game_try_heal(s_town_idx, i);
            }
            ry += 68;
        }
    } else if (s_town_tab == 4) {
        /* гильдия */
        ui_text(R(inner.x, inner.y, inner.width, 24), "Гильдия наёмников", 22, COL_TEXT_DARK);
        ui_textf(R(inner.x, inner.y + 32, inner.width, 22), 15, (Color){ 100, 80, 58, 255 },
                 "Контракты: %d из %d активных.", g.n_active, MAX_ACTIVE);
        float ry = inner.y + 62;
        for (int i = 0; i < MAX_CONTRACTS && ry + 86 < inner.y + inner.height; i++) {
            Contract* c = contract_get(i);
            if (!c || !c->active) continue;
            bool is_active = false;
            for (int k = 0; k < MAX_ACTIVE; k++) if (g.active_contracts[k] == i) is_active = true;
            Rect row = R(inner.x, ry, inner.width, 82);
            DrawRectangleRounded(row, 0.1f, 3, is_active ? (Color){ 96, 74, 46, 70 } : (Color){ 70, 54, 38, 40 });
            ui_textf(R(row.x + 10, row.y + 6, 300, 22), 17, COL_TEXT_DARK, "%s", c->title);
            ui_textf(R(row.x + 10, row.y + 30, 90, 20), 14, (Color){ 120, 90, 50, 255 },
                     "[%s]", g_contract_type_name[c->type]);
            ui_text_wrap(R(row.x + 10, row.y + 52, row.width - 250, 26), c->desc, 13, (Color){ 90, 70, 50, 255 }, 16);
            ui_textf_right(R(row.x + row.width - 240, row.y + 8, 230, 22), 15, COL_GOLD, "%d крон", c->reward_gold);
            ui_textf_right(R(row.x + row.width - 240, row.y + 30, 230, 22), 14, (Color){ 100, 80, 58, 255 },
                           "опыт %d", c->reward_xp);
            if (is_active) {
                if (ui_button_small(R(row.x + row.width - 180, row.y + 50, 160, 28), "Отказаться", true))
                    contracts_abandon(i);
            } else {
                bool can = g.n_active < MAX_ACTIVE && g.n_brothers > 0;
                if (ui_button_small(R(row.x + row.width - 180, row.y + 50, 160, 28), "Принять", can))
                    contracts_accept(i);
            }
            ry += 90;
        }
    } else if (s_town_tab == 5) {
        /* кузница */
        ui_text(R(inner.x, inner.y, inner.width, 24), "Кузница", 22, COL_TEXT_DARK);
        ui_text(R(inner.x, inner.y + 32, inner.width, 22),
                "Ремонт снаряжения. Цену смотрите напротив каждой вещи.", 15, (Color){ 100, 80, 58, 255 });
        float ry = inner.y + 70;
        for (int i = 0; i < g.n_items && ry + 34 < inner.y + inner.height; i++) {
            Item* it = &g.items[i];
            if (it->def == ITEM_NONE || it->durability >= 100) continue;
            Rect row = R(inner.x, ry, inner.width, 30);
            draw_item_line(row, it, false);
            int cost = (100 - it->durability) * g_item_defs[it->def].price / 400;
            if (cost < 5) cost = 5;
            char lbl[32];
            snprintf(lbl, sizeof(lbl), "Ремонт %d кр", cost);
            if (ui_button_small(R(row.x + row.width - 160, ry + 1, 156, 28), lbl, g.crowns >= cost)) {
                game_try_repair(s_town_idx, i);
                break;
            }
            ry += 34;
        }
        if (ry <= inner.y + 70)
            ui_text(R(inner.x, ry + 10, inner.width, 24), "Всё снаряжение в порядке.", 17, (Color){ 100, 80, 58, 255 });
    }
}

/* -------------------------------------------------------------- отряд -- */
static void draw_party_screen(void) {
    DrawRectangle(0, 54, GetScreenWidth(), GetScreenHeight() - 54, (Color){ 30, 24, 20, 255 });
    ui_header(R(120, 62, GetScreenWidth() - 240, 46), g.company_name);

    /* колонка 1: список */
    ui_panel(R(16, 120, 300, GetScreenHeight() - 170));
    ui_textf(R(30, 132, 270, 22), 17, COL_HEADER, "Отряд: %d / %d", g.n_brothers, MAX_BROTHERS);
    float ry = 162;
    for (int i = 0; i < g.n_brothers && ry + 62 < GetScreenHeight() - 60; i++) {
        Brother* b = &g.brothers[i];
        Rect row = R(28, ry, 276, 58);
        bool sel = s_sel_brother == i;
        DrawRectangleRounded(row, 0.12f, 3, sel ? (Color){ 96, 74, 46, 255 } : COL_PANEL_LT);
        ui_portrait(R(row.x + 6, row.y + 6, 46, 46), b->portrait);
        ui_textf(R(row.x + 60, row.y + 6, 210, 20), 15, COL_TEXT, "%s", b->name);
        ui_textf(R(row.x + 60, row.y + 28, 120, 18), 13, COL_TEXT_DIM, "ур. %d", b->level);
        ui_bar(R(row.x + 130, row.y + 30, 138, 12),
               (float)b->hp / (float)brother_stat(b, STAT_HP),
               b->hp * 3 < brother_stat(b, STAT_HP) ? COL_BAD : COL_GOOD, "");
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && !ui_blocked() && ui_point_in(row, GetMousePosition()))
            s_sel_brother = i;
        ry += 64;
    }

    if (g.n_brothers == 0) {
        ui_text(R(40, 200, 260, 24), "В отряде никого нет.", 16, COL_TEXT_DIM);
        return;
    }
    if (s_sel_brother >= g.n_brothers) s_sel_brother = 0;
    Brother* b = &g.brothers[s_sel_brother];

    /* колонка 2: карточка бойца */
    ui_panel(R(332, 120, 460, GetScreenHeight() - 170));
    ui_portrait(R(348, 136, 96, 96), b->portrait);
    ui_textf(R(456, 136, 320, 24), 19, COL_HEADER, "%s", b->name);
    ui_textf(R(456, 164, 320, 20), 15, COL_TEXT_DIM, "%s · уровень %d", background_name(b->background), b->level);
    ui_bar(R(456, 190, 310, 14), (float)b->xp / (float)xp_needed(b->level), COL_ACCENT, "");
    ui_textf(R(456, 208, 320, 18), 13, COL_TEXT_DIM, "опыт %d / %d", b->xp, xp_needed(b->level));
    ui_textf(R(456, 228, 320, 18), 13, b->mood > 60 ? COL_GOOD : (b->mood > 35 ? COL_GOLD : COL_BAD),
             "настроение: %d", b->mood);

    if (b->stat_picks > 0 || b->perk_points > 0) {
        if (ui_button(R(348, 250, 140, 38), "Прокачка!", true)) {
            s_level_brother = s_sel_brother;
            s_level_perk = -1;
            for (int k = 0; k < 3; k++) s_level_picks[k] = -1;
            g.levelup_open = true;
        }
    }

    /* характеристики */
    static const TexId stat_icons[STAT_COUNT] = {
        TEX_IC_HP, TEX_IC_FATIGUE, TEX_IC_RESOLVE, TEX_IC_INITIATIVE,
        TEX_IC_MATK, TEX_IC_RATK, TEX_IC_MDEF, TEX_IC_RDEF,
    };
    float sy = 300;
    for (int s = 0; s < STAT_COUNT; s++) {
        int v = brother_stat(b, (StatId)s);
        ui_icon(R(348, sy, 22, 22), stat_icons[s]);
        ui_textf(R(376, sy + 1, 150, 20), 15, COL_TEXT, "%s", g_stat_name[s]);
        ui_textf_right(R(520, sy + 1, 90, 20), 16, COL_HEADER, "%d", v);
        sy += 28;
    }
    /* броня */
    ui_icon(R(348, sy + 2, 22, 22), TEX_IC_ARMOR_BODY);
    ui_text(R(376, sy + 3, 150, 20), "Броня всего", 15, COL_TEXT);
    ui_textf_right(R(520, sy + 3, 90, 20), 16, COL_HEADER, "%d", item_total_armor(b));
    sy += 32;
    ui_icon(R(348, sy + 2, 22, 22), TEX_IC_WEIGHT);
    ui_text(R(376, sy + 3, 150, 20), "Урон оружия", 15, COL_TEXT);
    ui_textf_right(R(520, sy + 3, 90, 20), 16, COL_HEADER, "%d–%d",
                   item_weapon_damage_min(b), item_weapon_damage_max(b));

    /* черты */
    float ty = 300;
    ui_text(R(630, ty, 150, 20), "Черты:", 15, COL_HEADER);
    ty += 24;
    for (int i = 0; i < b->n_traits; i++) {
        int tr = b->traits[i];
        ui_textf(R(630, ty, 150, 18), 13, COL_TEXT, "• %s", g_traits[tr].name);
        ty += 20;
    }
    /* перки */
    ty += 8;
    ui_text(R(630, ty, 150, 20), "Перки:", 15, COL_HEADER);
    ty += 24;
    for (int i = 0; i < PERK_COUNT; i++) {
        if (!(b->perks & (1u << i))) continue;
        ui_textf(R(630, ty, 150, 18), 13, COL_TEXT, "• %s", g_perks[i].name);
        ty += 20;
    }

    /* снаряжение */
    ui_text(R(348, 470, 200, 20), "Снаряжение:", 15, COL_HEADER);
    static const char* eq_names[EQ_COUNT] = { "Голова", "Тело", "Оружие", "Левая рука" };
    float ey = 496;
    for (int e = 0; e < EQ_COUNT; e++) {
        Rect row = R(348, ey, 430, 30);
        DrawRectangleRounded(row, 0.15f, 3, (Color){ 54, 44, 36, 255 });
        ui_textf(R(row.x + 8, row.y + 5, 100, 20), 13, COL_TEXT_DIM, "%s:", eq_names[e]);
        int it = b->equip[e];
        if (it != ITEM_NONE && it < g.n_items) {
            ui_textf(R(row.x + 110, row.y + 5, 240, 20), 15, COL_TEXT, "%s", g_item_defs[g.items[it].def].name);
            if (ui_button_small(R(row.x + row.width - 90, row.y + 2, 86, 26), "Снять", true))
                game_unequip(s_sel_brother, e);
        } else {
            ui_text(R(row.x + 110, row.y + 5, 240, 20), "—", 15, COL_TEXT_DIM);
        }
        ey += 34;
    }

    /* колонка 3: инвентарь */
    ui_panel(R(808, 120, GetScreenWidth() - 824, GetScreenHeight() - 170));
    ui_text(R(822, 132, 300, 22), "Инвентарь (клик — надеть):", 16, COL_HEADER);
    float iy = 162;
    int shown = 0;
    int max_show = (int)((GetScreenHeight() - 220 - iy) / 32);
    for (int i = s_scroll_inv; i < g.n_items && shown < max_show; i++) {
        Item* it = &g.items[i];
        if (it->def == ITEM_NONE) continue;
        Rect row = R(822, iy, GetScreenWidth() - 852, 28);
        bool equipped = false;
        for (int bb = 0; bb < g.n_brothers && !equipped; bb++)
            for (int e = 0; e < EQ_COUNT; e++)
                if (g.brothers[bb].equip[e] == i) equipped = true;
        draw_item_line(row, it, s_sel_item == i);
        if (equipped) {
            for (int bb = 0; bb < g.n_brothers; bb++) {
                bool used = false;
                for (int e = 0; e < EQ_COUNT; e++) if (g.brothers[bb].equip[e] == i) used = true;
                if (used) {
                    ui_textf_right(R(row.x, row.y + 4, row.width, 22), 12, COL_TEXT_DIM, "%s", g.brothers[bb].name);
                    break;
                }
            }
        }
        if (!ui_blocked() && IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && ui_point_in(row, GetMousePosition())) {
            if (!equipped) {
                game_equip(s_sel_brother, i);
            }
            s_sel_item = i;
        }
        iy += 32;
        shown++;
    }
    /* скролл */
    if (!ui_blocked() && ui_point_in(R(822, 162, GetScreenWidth() - 852, GetScreenHeight() - 280), GetMousePosition())) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            s_scroll_inv -= (int)(wheel * 3);
            if (s_scroll_inv < 0) s_scroll_inv = 0;
            if (s_scroll_inv > g.n_items) s_scroll_inv = g.n_items;
        }
    }
}

/* --------------------------------------------------- модальные окна ---- */
static void draw_battle_modal(void) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){ 0, 0, 0, 165 });
    float w = 720, h = 520;
    float x = (GetScreenWidth() - w) / 2, y = (GetScreenHeight() - h) / 2;
    ui_panel(R(x, y, w, h));
    ui_header(R(x + 30, y + 16, w - 60, 44), g.battle.title);

    ui_panel_parchment(R(x + 30, y + 76, w - 60, h - 170));
    float ly = y + 92;
    int start = s_scroll_log;
    for (int i = start; i < g.battle.n_lines && ly < y + h - 110; i++) {
        Color c = COL_TEXT_DARK;
        if (strstr(g.battle.lines[i], "ПОБЕДА")) c = (Color){ 40, 110, 40, 255 };
        if (strstr(g.battle.lines[i], "ПОРАЖЕНИЕ") || strstr(g.battle.lines[i], "ранен")) c = (Color){ 140, 50, 40, 255 };
        ui_text_wrap(R(x + 48, ly, w - 96, 22), g.battle.lines[i], 15, c, 20);
        ly += 21;
    }
    if (g.battle.n_lines > 18 && !ui_blocked()) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            s_scroll_log -= (int)wheel * 3;
            if (s_scroll_log < 0) s_scroll_log = 0;
            if (s_scroll_log > g.battle.n_lines - 1) s_scroll_log = g.battle.n_lines - 1;
        }
    }

    if (ui_button(R(x + w / 2 - 110, y + h - 72, 220, 48), "Продолжить", true)) {
        g.battle.active = false;
        s_scroll_log = 0;
        if (g.game_over) {
            s_screen = SC_MENU;
            g.game_over = false;
        }
    }
}

static void draw_event_modal(void) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){ 0, 0, 0, 165 });
    float w = 680, h = 380;
    float x = (GetScreenWidth() - w) / 2, y = (GetScreenHeight() - h) / 2;
    ui_panel(R(x, y, w, h));
    ui_header(R(x + 30, y + 16, w - 60, 44), event_title());
    ui_panel_parchment(R(x + 30, y + 76, w - 60, 130));
    ui_text_wrap(R(x + 50, y + 94, w - 100, 110), event_text(), 17, (Color){ 80, 62, 44, 255 }, 24);

    float by = y + 230;
    for (int i = 0; i < event_choice_count(); i++) {
        const char* lbl = event_choice_label(i);
        if (!lbl || !lbl[0]) continue;
        if (ui_button(R(x + 60, by, w - 120, 44), lbl, true)) {
            events_resolve_choice(i);
        }
        by += 54;
    }
}

static void draw_levelup_modal(void) {
    if (s_level_brother < 0 || s_level_brother >= g.n_brothers) {
        g.levelup_open = false;
        return;
    }
    Brother* b = &g.brothers[s_level_brother];
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){ 0, 0, 0, 165 });
    float w = 860, h = 560;
    float x = (GetScreenWidth() - w) / 2, y = (GetScreenHeight() - h) / 2;
    ui_panel(R(x, y, w, h));
    char title[160];
    snprintf(title, sizeof(title), "Прокачка: %s (уровень %d)", b->name, b->level);
    ui_header(R(x + 30, y + 14, w - 60, 42), title);

    int picks_used = 0;
    for (int i = 0; i < 3; i++) if (s_level_picks[i] >= 0) picks_used++;

    ui_textf(R(x + 40, y + 70, w - 80, 22), 16, COL_TEXT,
             "Выберите %d улучшения характеристик:", 3 - picks_used);

    static const TexId stat_icons[STAT_COUNT] = {
        TEX_IC_HP, TEX_IC_FATIGUE, TEX_IC_RESOLVE, TEX_IC_INITIATIVE,
        TEX_IC_MATK, TEX_IC_RATK, TEX_IC_MDEF, TEX_IC_RDEF,
    };
    static const int gain[STAT_COUNT] = { 4, 6, 4, 5, 3, 3, 3, 3 };
    float sy = y + 100;
    for (int s = 0; s < STAT_COUNT; s++) {
        bool picked_here = false;
        for (int i = 0; i < 3; i++) if (s_level_picks[i] == s) picked_here = true;
        Rect row = R(x + 40, sy, 380, 34);
        DrawRectangleRounded(row, 0.18f, 3, picked_here ? (Color){ 96, 74, 46, 255 } : COL_PANEL_LT);
        ui_icon(R(row.x + 8, row.y + 6, 22, 22), stat_icons[s]);
        ui_textf(R(row.x + 38, row.y + 7, 220, 22), 15, COL_TEXT, "%s", g_stat_name[s]);
        ui_textf_right(R(row.x, row.y + 7, row.width - 12, 22), 15, COL_GOOD, "+%d", gain[s]);
        if (!picked_here && picks_used < 3 &&
            ui_button_small(R(row.x + row.width + 8, row.y + 2, 80, 30), "Взять", true)) {
            for (int i = 0; i < 3; i++) {
                if (s_level_picks[i] < 0) { s_level_picks[i] = s; break; }
            }
        }
        sy += 38;
    }

    ui_text(R(x + 560, y + 70, 260, 22), "Перк:", 16, COL_TEXT);
    float py = y + 98;
    for (int i = 0; i < PERK_COUNT && py < y + h - 130; i++) {
        bool picked = s_level_perk == i;
        Rect row = R(x + 480, py, 340, 34);
        DrawRectangleRounded(row, 0.15f, 3, picked ? (Color){ 96, 74, 46, 255 } : COL_PANEL_LT);
        ui_textf(R(row.x + 8, row.y + 7, 320, 22), 14, picked ? COL_HEADER : COL_TEXT, "%s", g_perks[i].name);
        if (ui_point_in(row, GetMousePosition()) && !ui_blocked()) {
            ui_tooltip(R((float)GetMouseX() + 12, (float)GetMouseY() + 8, 0, 0), g_perks[i].desc);
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) s_level_perk = i;
        }
        py += 38;
    }

    bool can_done = picks_used >= 3 && (b->perk_points <= 0 || s_level_perk >= 0);
    if (ui_button(R(x + w / 2 - 110, y + h - 66, 220, 46), "Готово", can_done)) {
        brother_levelup_apply(b, s_level_picks, s_level_perk);
        g.levelup_open = false;
    }
    if (ui_button(R(x + 30, y + h - 66, 140, 46), "Отмена", true)) {
        g.levelup_open = false;
    }
}

/* ------------------------------------------------------------- кадр ---- */
void render_frame(float dt) {
    s_time += dt;

    /* запущен тактический бой — показываем его */
    if (bt.active && !bt.finished && s_screen != SC_BATTLE && s_screen != SC_MENU)
        s_screen = SC_BATTLE;

    bool modal = g.battle.active || g.pending_event.active || g.levelup_open;

    switch (s_screen) {
        case SC_MENU:
            ui_set_blocked(false);
            draw_menu();
            break;
        case SC_MAP:
            ui_set_blocked(modal);
            draw_topbar();
            draw_map_screen(dt);
            break;
        case SC_TOWN:
            ui_set_blocked(modal);
            draw_topbar();
            draw_town_screen();
            break;
        case SC_PARTY:
            ui_set_blocked(modal);
            draw_topbar();
            draw_party_screen();
            break;
        case SC_BATTLE:
            ui_set_blocked(false);
            render_battle_screen(dt);
            break;
    }

    if (modal) {
        ui_set_blocked(false);
        if (g.battle.active) draw_battle_modal();
        else if (g.pending_event.active) draw_event_modal();
        else if (g.levelup_open) draw_levelup_modal();
    }
}
