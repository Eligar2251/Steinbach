/* render_battle.c — экран тактического боя. */
#include "game.h"
#include "battle.h"
#include "render.h"
#include "ui.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define BT_FX 12.0f
#define BT_FY 62.0f
#define BT_TS 64.0f            /* размер клетки */

static const char* k_terrain_tex[BT_T_TYPES] = {
    "t_grass", "t_dirt", "t_stone", "t_sand", "t_forest"
};

static const char* k_env_obst[8] = {
    "mr_env_02", "mr_env_03", "mr_env_04", "mr_env_08",
    "mr_env_09", "mr_env_13", "mr_env_10", "mr_env_20"
};

static int s_hover_x = -1, s_hover_y = -1;
static bool s_retreat_confirm = false;

static Vector2 cell_center(int x, int y) {
    return (Vector2){ BT_FX + x * BT_TS + BT_TS / 2, BT_FY + y * BT_TS + BT_TS / 2 };
}

static void draw_unit_sprite(const BTUnit* u) {
    float px = BT_FX + u->fx * BT_TS;
    float py = BT_FY + u->fy * BT_TS;

    /* смещение выпада при ударе */
    if (bt.anim == BT_ANIM_LUNGE && bt.anim_a >= 0 && &bt.units[bt.anim_a] == u) {
        BTUnit* t = (bt.anim_b >= 0) ? &bt.units[bt.anim_b] : NULL;
        if (t) {
            float dx = (float)t->x - u->fx, dy = (float)t->y - u->fy;
            float l = sqrtf(dx * dx + dy * dy);
            if (l > 0.01f) {
                float k = sinf(bt.anim_t * 3.14159f) * 18.0f;
                px += dx / l * k;
                py += dy / l * k;
            }
        }
    }

    char name[32];
    snprintf(name, sizeof(name), "mr_unit_%02d", u->sprite + 1);

    /* тень */
    DrawEllipse((int)(px + BT_TS / 2), (int)(py + BT_TS - 6), 20, 7, (Color){ 0, 0, 0, 60 });

    Color tint = WHITE;
    if (!u->alive || u->fled) {
        tint = (Color){ 120, 120, 120, u->fled ? 90 : (unsigned char)(170 * (1.0f - bt.anim_t)) };
    } else if (u->enemy) {
        /* фиолетовые культисты — подтон */
        if (bt.faction == BT_F_CULTISTS) tint = (Color){ 210, 180, 255, 255 };
        else if (bt.faction == BT_F_DESERTERS) tint = (Color){ 190, 175, 175, 255 };
    }

    /* подсветка выбранного */
    if (bt.sel >= 0 && &bt.units[bt.sel] == u && u->alive && !u->fled) {
        DrawCircle((int)(px + BT_TS / 2), (int)(py + BT_TS / 2), BT_TS * 0.46f,
                   (Color){ 240, 220, 120, 60 });
    }

    sprite_draw_fit(name, R(px + 2, py + 2, BT_TS - 4, BT_TS - 4), tint);

    /* полоски HP и усталости */
    if (u->alive && !u->fled) {
        float frac = u->hp_max > 0 ? (float)u->hp / (float)u->hp_max : 0;
        float ffrac = u->fat_max > 0 ? (float)u->fat / (float)u->fat_max : 0;
        Color hp_c = frac > 0.6f ? COL_GOOD : (frac > 0.3f ? COL_GOLD : COL_BAD);
        if (u->enemy) hp_c = frac > 0.5f ? (Color){ 200, 90, 80, 255 } : (Color){ 160, 50, 40, 255 };
        DrawRectangle((int)(px + 8), (int)(py + BT_TS - 7), 48, 5, (Color){ 20, 16, 12, 200 });
        DrawRectangle((int)(px + 8), (int)(py + BT_TS - 7), (int)(48 * frac), 5, hp_c);
        if (ffrac > 0.02f) {
            DrawRectangle((int)(px + 8), (int)(py + BT_TS - 3), 48, 3, (Color){ 20, 16, 12, 200 });
            DrawRectangle((int)(px + 8), (int)(py + BT_TS - 3), (int)(48 * ffrac), 3,
                          (Color){ 150, 120, 60, 255 });
        }
    }
}

static void draw_field(void) {
    /* поле */
    for (int y = 0; y < BT_H; y++) {
        for (int x = 0; x < BT_W; x++) {
            float px = BT_FX + x * BT_TS, py = BT_FY + y * BT_TS;
            const char* tex = k_terrain_tex[bt.cell[y][x]];
            Texture2D* t = sprite_get(tex);
            if (t && t->id) {
                Rectangle src = { 0, 0, (float)t->width, (float)t->height };
                DrawTexturePro(*t, src, R(px, py, BT_TS, BT_TS), (Vector2){ 0, 0 }, 0, WHITE);
            } else {
                DrawRectangle((int)px, (int)py, (int)BT_TS, (int)BT_TS, (Color){ 80, 120, 70, 255 });
            }
            /* лёгкая сетка */
            DrawRectangleLinesEx(R(px, py, BT_TS, BT_TS), 1, (Color){ 0, 0, 0, 26 });

            if (bt.blocked[y][x]) {
                sprite_draw_fit(k_env_obst[(x * 7 + y * 3) % 8], R(px, py, BT_TS, BT_TS), WHITE);
            }
        }
    }

    /* достижимые клетки */
    if (bt.sel >= 0 && bt_is_player_turn() && bt.anim == BT_ANIM_NONE) {
        bool vis[BT_H][BT_W];
        bt_reachable(bt.sel, vis);
        for (int y = 0; y < BT_H; y++)
            for (int x = 0; x < BT_W; x++)
                if (vis[y][x])
                    DrawRectangle((int)(BT_FX + x * BT_TS), (int)(BT_FY + y * BT_TS),
                                  (int)BT_TS, (int)BT_TS, (Color){ 230, 220, 140, 34 });

        /* доступные цели */
        for (int i = 0; i < bt.n; i++) {
            if (!bt.units[i].used || !bt.units[i].alive || bt.units[i].fled) continue;
            if (bt_attack_possible(bt.sel, i)) {
                Rectangle r = R(BT_FX + bt.units[i].x * BT_TS, BT_FY + bt.units[i].y * BT_TS,
                                BT_TS, BT_TS);
                DrawRectangleLinesEx(r, 2, (Color){ 200, 70, 50, 220 });
            }
        }
    }

    /* юниты: сначала мёртвые, потом живые (порядок перекрытия) */
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < bt.n; i++) {
            BTUnit* u = &bt.units[i];
            if (!u->used) continue;
            bool dead = !u->alive || u->fled;
            if ((pass == 0) != dead) continue;
            draw_unit_sprite(u);
        }

    /* анимация выстрела */
    if (bt.anim == BT_ANIM_SHOOT && bt.anim_a >= 0 && bt.anim_b >= 0) {
        Vector2 a = cell_center(bt.units[bt.anim_a].x, bt.units[bt.anim_a].y);
        Vector2 b = cell_center(bt.units[bt.anim_b].x, bt.units[bt.anim_b].y);
        float t = bt.anim_t;
        Vector2 p = { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
        DrawLineEx(a, p, 2.5f, (Color){ 240, 230, 180, 220 });
        DrawCircleV(p, 4, (Color){ 220, 200, 150, 255 });
    }

    /* всплывающий урон */
    if (bt.float_t > 0 && bt.float_dmg != 0) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%s%d", bt.float_crit ? "!" : "", bt.float_dmg);
        Vector2 c = cell_center(bt.float_x, bt.float_y);
        float rise = (1.0f - bt.float_t) * 34.0f;
        Color fc = bt.float_crit ? (Color){ 255, 120, 60, 255 } : (Color){ 255, 235, 200, 255 };
        DrawTextEx(g_font_bold, buf, (Vector2){ c.x - 14, c.y - 40 - rise + 1 }, 22, 1, (Color){ 0, 0, 0, 180 });
        DrawTextEx(g_font_bold, buf, (Vector2){ c.x - 15, c.y - 41 - rise }, 22, 1, fc);
    } else if (bt.float_t > 0 && bt.float_dmg == 0 && bt.anim != BT_ANIM_NONE) {
        Vector2 c = cell_center(bt.float_x, bt.float_y);
        float rise = (1.0f - bt.float_t) * 30.0f;
        DrawTextEx(g_font_bold, "мимо", (Vector2){ c.x - 22, c.y - 40 - rise }, 19, 1,
                   (Color){ 210, 210, 210, 220 });
    }

    /* рамка поля */
    DrawRectangleLinesEx(R(BT_FX - 1, BT_FY - 1, BT_W * BT_TS + 2, BT_H * BT_TS + 2), 2,
                         (Color){ 20, 14, 10, 200 });

    /* курсор: клетка */
    if (s_hover_x >= 0) {
        Rectangle r = R(BT_FX + s_hover_x * BT_TS, BT_FY + s_hover_y * BT_TS, BT_TS, BT_TS);
        DrawRectangleLinesEx(r, 2, (Color){ 255, 255, 255, 150 });
    }
}

static void draw_order_queue(void) {
    float x = 930, y = 70;
    ui_text(R(x, y, 340, 22), "Порядок ходов", 15, COL_HEADER);
    y += 26;
    int shown = 0;
    for (int i = bt.turn_i; i < bt.n && shown < 14; i++) {
        int u = bt.order[i];
        if (u < 0 || u >= bt.n) continue;
        BTUnit* p = &bt.units[u];
        if (!p->used || !p->alive || p->fled) continue;
        char name[32];
        snprintf(name, sizeof(name), "mr_unit_%02d", p->sprite + 1);
        Color tint = p->enemy ? (Color){ 255, 190, 190, 255 } : (Color){ 190, 220, 255, 255 };
        if (i == bt.turn_i) {
            DrawRectangle((int)(x + shown * 25 - 2), (int)y - 2, 28, 28, (Color){ 230, 210, 110, 90 });
        }
        sprite_draw_fit(name, R(x + shown * 25, y, 24, 24), tint);
        shown++;
    }
}

static void draw_unit_info(void) {
    float x = 930, y = 128;
    int u = bt.sel;
    if (u < 0 && s_hover_x >= 0) {
        for (int i = 0; i < bt.n; i++)
            if (bt.units[i].used && bt.units[i].x == s_hover_x && bt.units[i].y == s_hover_y)
                u = i;
    }
    ui_panel(R(x, y, 338, 260));
    if (u < 0 || u >= bt.n || !bt.units[u].used) {
        ui_text(R(x + 16, y + 16, 300, 24), "Выберите бойца", 16, COL_TEXT_DIM);
        ui_text(R(x + 16, y + 48, 300, 120),
                "ЛКМ — выбор бойца и перемещение.\n"
                "Клик по врагу — атака.\n"
                "«Конец хода» — пропустить ход.", 14, COL_TEXT_DIM);
        return;
    }
    BTUnit* p = &bt.units[u];
    char buf[128];
    snprintf(buf, sizeof(buf), "%s%s", p->name, p->enemy ? " (враг)" : "");
    ui_text(R(x + 14, y + 10, 310, 24), buf, 17, COL_HEADER);

    float frac = p->hp_max ? (float)p->hp / p->hp_max : 0;
    ui_bar(R(x + 14, y + 40, 310, 18), frac, frac > 0.5f ? COL_GOOD : COL_BAD, "HP");
    float ff = p->fat_max ? (float)p->fat / p->fat_max : 0;
    ui_bar(R(x + 14, y + 64, 310, 14), ff, (Color){ 170, 140, 70, 255 }, "Уст.");

    snprintf(buf, sizeof(buf), "HP %d/%d   Броня: %d/%d   Шлем: %d/%d",
             p->hp, p->hp_max, p->armor_body, p->armor_body_max,
             p->armor_head, p->armor_head_max);
    ui_text(R(x + 14, y + 86, 310, 20), buf, 13, COL_TEXT_DIM);

    snprintf(buf, sizeof(buf), "Атака %d   Стрельба %d   Защита %d/%d",
             p->matk, p->ratk, p->mdef, p->rdef);
    ui_text(R(x + 14, y + 108, 310, 20), buf, 13, COL_TEXT);

    snprintf(buf, sizeof(buf), "Иниц. %d   Решимость %d   Щит +%d",
             p->ini, p->res, p->shield);
    ui_text(R(x + 14, y + 130, 310, 20), buf, 13, COL_TEXT);

    snprintf(buf, sizeof(buf), "Оружие: урон %d–%d, пробитие %d%%%s",
             p->w_min, p->w_max, p->w_pen,
             p->w_range > 1 ? ", дальнобойное" : "");
    ui_text(R(x + 14, y + 152, 310, 20), buf, 13, COL_TEXT);

    snprintf(buf, sizeof(buf), "Ход: шагов %d, действий %d", p->move_left, p->ap);
    ui_text(R(x + 14, y + 174, 310, 20), buf, 14, COL_GOLD);

    if (p->defending)
        ui_text(R(x + 14, y + 196, 310, 20), "В обороне: +15 защиты", 13, COL_GOOD);

    /* шанс попадания по цели под курсором */
    if (bt.sel >= 0 && s_hover_x >= 0) {
        for (int i = 0; i < bt.n; i++) {
            BTUnit* t = &bt.units[i];
            if (!t->used || !t->alive || t->fled || t->enemy == bt.units[bt.sel].enemy) continue;
            if (t->x == s_hover_x && t->y == s_hover_y && bt_attack_possible(bt.sel, i)) {
                int ch = bt_hit_chance(bt.sel, i);
                snprintf(buf, sizeof(buf), "Шанс попасть: %d%%", ch);
                ui_text(R(x + 14, y + 218, 310, 22), buf, 15,
                        ch >= 65 ? COL_GOOD : (ch >= 40 ? COL_GOLD : COL_BAD));
            }
        }
    }
}

static void draw_log(void) {
    float x = 930, y = 400;
    ui_panel(R(x, y, 338, 226));
    ui_text(R(x + 12, y + 8, 310, 22), "Журнал боя", 15, COL_HEADER);
    int start = bt.n_log > 8 ? bt.n_log - 8 : 0;
    int ly = (int)y + 34;
    for (int i = start; i < bt.n_log && ly < y + 210; i++) {
        Color c = COL_TEXT;
        if (strstr(bt.log[i], "погиб") || strstr(bt.log[i], "бежит") || strstr(bt.log[i], "Поражение"))
            c = COL_BAD;
        else if (strstr(bt.log[i], "Победа") || strstr(bt.log[i], "урона"))
            c = (Color){ 200, 220, 170, 255 };
        ui_text_wrap(R(x + 12, ly, 314, 20), bt.log[i], 13, c, 18);
        ly += 19;
    }
}

void render_battle_screen(float dt) {
    ClearBackground((Color){ 28, 24, 20, 255 });

    /* таймер ИИ и анимации */
    if (bt.anim != BT_ANIM_NONE) {
        bt_anim_tick(dt);
    } else if (!bt.finished && !bt_is_player_turn()) {
        bt.ai_delay -= dt;
        if (bt.ai_delay <= 0) {
            bt_ai_act(&g);
            bt.ai_delay = 0.55f;
        }
    }

    /* завершение боя */
    if (!bt.finished) bt_check_end(&g);
    if (bt.finished) {
        render_set_screen(1);   /* SC_MAP: отчёт покажется модальным окном */
        return;
    }

    /* шапка */
    char buf[160];
    snprintf(buf, sizeof(buf), "Бой: %s", bt.enemy_name);
    ui_header(R(0, 0, 1280, 54), buf);
    snprintf(buf, sizeof(buf), "Раунд %d", bt.round);
    ui_text(R(1000, 16, 260, 24), buf, 17, COL_TEXT_DIM);

    /* поле */
    draw_field();

    /* курсорная клетка */
    Vector2 m = GetMousePosition();
    s_hover_x = s_hover_y = -1;
    if (m.x >= BT_FX && m.y >= BT_FY &&
        m.x < BT_FX + BT_W * BT_TS && m.y < BT_FY + BT_H * BT_TS) {
        s_hover_x = (int)((m.x - BT_FX) / BT_TS);
        s_hover_y = (int)((m.y - BT_FY) / BT_TS);
    }

    draw_order_queue();
    draw_unit_info();
    draw_log();

    /* кнопки действий */
    float bx = BT_FX, by = BT_FY + BT_H * BT_TS + 10;
    bool my_turn = bt_is_player_turn() && bt.anim == BT_ANIM_NONE;

    if (ui_button(R(bx, by, 150, 36), "Оборона", my_turn && bt.sel >= 0 && bt.units[bt.sel].ap > 0))
        bt_defend(&g, bt.sel);
    if (ui_button(R(bx + 160, by, 150, 36), "Ждать", my_turn && bt.sel >= 0))
        bt_wait(&g, bt.sel);
    if (ui_button(R(bx + 320, by, 180, 36), "Конец хода", my_turn))
        bt_end_turn(&g);

    if (!s_retreat_confirm) {
        if (ui_button(R(bx + 520, by, 170, 36), "Отступить", my_turn))
            s_retreat_confirm = true;
    } else {
        if (ui_button(R(bx + 520, by, 170, 36), "Точно бежим?", true)) {
            bt_retreat(&g);
            s_retreat_confirm = false;
        }
    }

    if (!my_turn && !bt.finished) {
        ui_text(R(bx + 720, by + 8, 190, 24), "Ход противника…", 15, COL_TEXT_DIM);
    }

    /* ввод: клики по полю */
    if (my_turn && !ui_blocked() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        s_hover_x >= 0) {
        int clicked_unit = -1;
        for (int i = 0; i < bt.n; i++) {
            BTUnit* p = &bt.units[i];
            if (p->used && p->alive && !p->fled && p->x == s_hover_x && p->y == s_hover_y) {
                clicked_unit = i;
                break;
            }
        }
        if (clicked_unit >= 0 && !bt.units[clicked_unit].enemy) {
            bt.sel = clicked_unit;
        } else if (clicked_unit >= 0 && bt.sel >= 0 &&
                   bt_attack_possible(bt.sel, clicked_unit)) {
            bt_attack(&g, bt.sel, clicked_unit);
        } else if (bt.sel >= 0 && !bt.blocked[s_hover_y][s_hover_x]) {
            bool vis[BT_H][BT_W];
            bt_reachable(bt.sel, vis);
            if (vis[s_hover_y][s_hover_x])
                bt_move(&g, bt.sel, s_hover_x, s_hover_y);
            else
                bt.sel = -1;
        } else {
            bt.sel = -1;
        }
    }

    /* горячие клавиши */
    if (my_turn && !ui_blocked()) {
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) bt_end_turn(&g);
        if (IsKeyPressed(KEY_D)) bt_defend(&g, bt.sel);
    }
}
