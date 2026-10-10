/* battle.c — логика тактического боя (без raylib, тестируемая). */
#include "battle.h"
#include "rng.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

BattleState bt;

bool occupied(int x, int y);
static bool bt_retreated_check(void);

/* ------------------------------------------------------------- утилиты -- */

static int absi(int v) { return v < 0 ? -v : v; }
static int dist_cheb(int x1, int y1, int x2, int y2) {
    int dx = absi(x1 - x2), dy = absi(y1 - y2);
    return dx > dy ? dx : dy;
}

void bt_logf(const char* fmt, ...) {
    if (bt.n_log >= BT_LOG) {
        memmove(&bt.log[0], &bt.log[1], (BT_LOG - 1) * sizeof(bt.log[0]));
        bt.n_log = BT_LOG - 1;
    }
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(bt.log[bt.n_log], sizeof(bt.log[0]), fmt, ap);
    va_end(ap);
    bt.n_log++;
}

void bt_launch(int ctx, int ctx_idx, int loc_idx, int faction, int enemy_power,
               const char* enemy_name, int terrain, bool scale) {
    bt_setup(ctx, ctx_idx, faction, enemy_power, enemy_name, terrain, scale);
    bt.loc_idx = loc_idx;
    bt_begin();
}

void bt_pre_add(const char* line) {
    if (!line || bt.n_pre >= 6) return;
    snprintf(bt.pre[bt.n_pre], sizeof(bt.pre[0]), "%s", line);
    bt.n_pre++;
}

int bt_alive_enemy(void) {
    int n = 0;
    for (int i = 0; i < bt.n; i++)
        if (bt.units[i].used && bt.units[i].alive && !bt.units[i].fled && bt.units[i].enemy) n++;
    return n;
}

int bt_alive_player(void) {
    int n = 0;
    for (int i = 0; i < bt.n; i++)
        if (bt.units[i].used && bt.units[i].alive && !bt.units[i].fled && !bt.units[i].enemy) n++;
    return n;
}

int bt_current(void) {
    if (bt.turn_i < 0 || bt.turn_i >= bt.n) return -1;
    int u = bt.order[bt.turn_i];
    if (u < 0 || u >= bt.n || !bt.units[u].used || !bt.units[u].alive || bt.units[u].fled) return -1;
    return u;
}

bool bt_is_player_turn(void) {
    int u = bt_current();
    return u >= 0 && !bt.units[u].enemy;
}

bool bt_walkable(int x, int y) {
    if (x < 0 || y < 0 || x >= BT_W || y >= BT_H) return false;
    return !bt.blocked[y][x];
}

/* ------------------------------------------------------------- генерация */

/* Имя и статы шаблонов врагов по фракции. sprite — база 0..23. */
typedef struct {
    const char* name;
    int hp, matk, ratk, mdef, rdef, ini, res;
    int w_min, w_max, w_pen, w_range;
    int sprite;
    int armor_head, armor_body;
} BTTemplate;

static const BTTemplate k_templates[BT_F_COUNT][5] = {
    /* Бандиты: красная команда mr_unit_07..12 */
    {
        {"Бандит",        55, 62, 45, 12, 8, 105, 40,  8, 14, 25, 1, 6,  30, 55},
        {"Головорез",     70, 70, 40, 16, 10, 95, 50, 12, 20, 30, 1, 7,  55, 85},
        {"Лучник бандитов",50, 45, 68, 10, 12, 110, 40,  6, 12, 15, 6, 8, 25, 45},
        {"Вожак бандитов", 80, 76, 50, 20, 14, 100, 55, 14, 22, 35, 1, 9, 70, 110},
        {"Скиталец",      55, 58, 42, 12, 10, 112, 42,  8, 13, 20, 1, 10, 30, 50}
    },
    /* Разбойники/рейдеры: зелёная mr_unit_13..18 */
    {
        {"Разбойник",     58, 64, 45, 13, 9, 108, 42,  9, 15, 25, 1, 12, 30, 55},
        {"Налётчик",      72, 72, 42, 17, 11, 98, 50, 13, 21, 30, 1, 13, 55, 90},
        {"Метатель",      52, 46, 70, 11, 13, 112, 42,  7, 13, 15, 5, 14, 25, 45},
        {"Атаман",        85, 78, 52, 21, 15, 102, 58, 15, 24, 35, 1, 15, 75, 115},
        {"Следопыт",      60, 60, 55, 14, 12, 114, 45,  9, 14, 20, 4, 16, 35, 60}
    },
    /* Стража/наёмники: серая mr_unit_19..24 */
    {
        {"Стражник",      65, 66, 48, 16, 12, 100, 50, 10, 16, 30, 1, 18, 55, 90},
        {"Ветеран",       80, 74, 48, 20, 14, 98, 55, 14, 22, 35, 1, 19, 80, 120},
        {"Арбалетчик",    55, 48, 72, 12, 14, 106, 46,  9, 15, 25, 7, 20, 35, 60},
        {"Капитан",       90, 80, 55, 22, 16, 104, 60, 16, 25, 40, 1, 21, 95, 140},
        {"Рекрут",        55, 58, 42, 12, 10, 105, 42,  8, 13, 20, 1, 22, 40, 65}
    },
    /* Культисты: серая + фиолет mr_unit_19..24 */
    {
        {"Сектант",       52, 60, 42, 10, 8, 108, 55,  9, 14, 25, 1, 18, 25, 45},
        {"Фанатик",       62, 70, 40, 12, 8, 112, 60, 12, 19, 30, 1, 19, 35, 60},
        {"Заклинатель",   50, 42, 66, 10, 12, 106, 65,  8, 14, 20, 5, 20, 25, 40},
        {"Жрец тьмы",     78, 72, 50, 18, 14, 100, 70, 15, 23, 35, 1, 21, 55, 90},
        {"Послушник",     48, 55, 40, 10, 8, 110, 50,  8, 12, 20, 1, 22, 20, 40}
    },
    /* Дезертиры: красная + тень */
    {
        {"Дезертир",      62, 65, 48, 15, 11, 102, 45, 10, 16, 30, 1, 6, 50, 85},
        {"Забияка", 76, 72, 45, 18, 12, 96, 50, 13, 21, 35, 1, 7, 65, 100},
        {"Стрелок",       55, 46, 70, 12, 14, 108, 45,  8, 14, 25, 6, 8, 35, 55},
        {"Сержант-мятежник",85, 78, 52, 21, 15, 100, 55, 15, 24, 40, 1, 9, 85, 125},
        {"Мародёр",       58, 62, 45, 13, 10, 106, 42,  9, 15, 25, 1, 10, 35, 60}
    },
    /* Ополчение: жёлтые */
    {
        {"Ополченец",     50, 56, 42, 11, 9, 104, 38,  7, 12, 15, 1, 4, 25, 45},
        {"Мызник",        62, 64, 42, 14, 10, 98, 44, 10, 17, 25, 1, 5, 45, 75},
        {"Охотник",       48, 42, 66, 10, 12, 110, 40,  6, 11, 15, 5, 3, 20, 40},
        {"Староста",      72, 70, 48, 18, 13, 96, 50, 12, 20, 30, 1, 2, 60, 95},
        {"Парень",        46, 52, 40, 10, 8, 108, 36,  7, 11, 15, 1, 1, 20, 35}
    }
};

/* Спрайты игрока: синяя команда mr_unit_01..06 по оружию. */
static int player_sprite(const Brother* b) {
    const ItemDef* wd = NULL;
    if (b->equip[EQ_WEAPON] >= 0) {
        int def = g.items[b->equip[EQ_WEAPON]].def;
        if (def >= 0) wd = &g_item_defs[def];
    }
    if (wd && (wd->wtype == WTYPE_BOW || wd->wtype == WTYPE_CROSSBOW)) return 2;
    if (wd && wd->wtype == WTYPE_SPEAR) return 3;
    if (wd && wd->two_handed) return 0;
    if (b->equip[EQ_OFFHAND] >= 0) return 1;
    return 0;
}

static void terrain_gen(void) {
    int base = bt.terrain;
    for (int y = 0; y < BT_H; y++) {
        for (int x = 0; x < BT_W; x++) {
            bt.cell[y][x] = (uint8_t)base;
            bt.blocked[y][x] = false;
        }
    }
    /* случайные препятствия: деревья/камни по краям и островками */
    int n_obst = 6 + rng_range(0, 6);
    for (int i = 0; i < n_obst; i++) {
        int x = rng_range(0, BT_W - 1), y = rng_range(0, BT_H - 1);
        if (x < 2 || x >= BT_W - 2) continue;   /* края свободны для дебаркации */
        if (y < 1 || y >= BT_H - 1) continue;
        bt.blocked[y][x] = true;
    }
    if (bt.terrain == BT_T_FOREST) {
        for (int i = 0; i < 8; i++) {
            int x = rng_range(2, BT_W - 3), y = rng_range(1, BT_H - 2);
            bt.blocked[y][x] = true;
        }
    }
}

static void unit_place(BTUnit* u, bool enemy, int idx) {
    memset(u, 0, sizeof(*u));
    u->used = true;
    u->alive = true;
    u->enemy = enemy;
    u->brother = -1;
    u->team = enemy ? 1 : 0;

    if (!enemy) {
        Brother* b = &g.brothers[idx];
        u->brother = idx;
        snprintf(u->name, sizeof(u->name), "%s", b->name);
        u->hp_max = b->base.hp;
        u->hp = b->hp > 0 ? b->hp : 1;
        u->fat_max = b->base.fatigue;
        u->fat = b->fatigue > 0 ? b->fatigue : 0;
        u->matk = b->base.matk; u->ratk = b->base.ratk;
        u->mdef = b->base.mdef; u->rdef = b->base.rdef;
        u->ini = b->base.initiative; u->res = b->base.resolve;
        u->sprite = player_sprite(b);

        /* снаряжение */
        const ItemDef* hd = NULL, *bd = NULL, *wd = NULL, *od = NULL;
        if (b->equip[EQ_HEAD] >= 0 && g.items[b->equip[EQ_HEAD]].def >= 0)
            hd = &g_item_defs[g.items[b->equip[EQ_HEAD]].def];
        if (b->equip[EQ_BODY] >= 0 && g.items[b->equip[EQ_BODY]].def >= 0)
            bd = &g_item_defs[g.items[b->equip[EQ_BODY]].def];
        if (b->equip[EQ_WEAPON] >= 0 && g.items[b->equip[EQ_WEAPON]].def >= 0)
            wd = &g_item_defs[g.items[b->equip[EQ_WEAPON]].def];
        if (b->equip[EQ_OFFHAND] >= 0 && g.items[b->equip[EQ_OFFHAND]].def >= 0)
            od = &g_item_defs[g.items[b->equip[EQ_OFFHAND]].def];

        u->armor_head_max = hd ? hd->armor * 8 : 10;
        u->armor_body_max = bd ? bd->armor * 8 : 15;
        if (wd) {
            u->w_min = wd->dmg_min; u->w_max = wd->dmg_max;
            u->w_pen = wd->armor_pen; u->w_range = wd->range > 0 ? wd->range : 1;
            u->wtype = wd->wtype;
        } else {
            u->w_min = 4; u->w_max = 9; u->w_pen = 10; u->w_range = 1; u->wtype = WTYPE_NONE;
        }
        u->shield = od ? od->shield_def : 0;
    } else {
        const BTTemplate* t = &k_templates[bt.faction][idx % 5];
        snprintf(u->name, sizeof(u->name), "%s", t->name);
        /* масштаб под силу отряда */
        int p = bt.enemy_power;
        int scale_hp = 100 + clampi(p / 6, -10, 40);
        u->hp_max = t->hp * scale_hp / 100;
        u->hp = u->hp_max;
        u->fat_max = 90;
        u->fat = 0;
        u->matk = t->matk + clampi(p / 30, -4, 12);
        u->ratk = t->ratk + clampi(p / 30, -4, 12);
        u->mdef = t->mdef + clampi(p / 40, -3, 10);
        u->rdef = t->rdef + clampi(p / 40, -3, 10);
        u->ini = t->ini;
        u->res = t->res + clampi(p / 40, -4, 12);
        u->w_min = t->w_min; u->w_max = t->w_max;
        u->w_pen = t->w_pen; u->w_range = t->w_range; u->wtype = WTYPE_SWORD;
        u->sprite = t->sprite;
        u->armor_head_max = t->armor_head * (100 + p / 8) / 100;
        u->armor_body_max = t->armor_body * (100 + p / 8) / 100;
        u->shield = (idx % 3 == 1) ? 12 : 0;
    }
    u->armor_head = u->armor_head_max;
    u->armor_body = u->armor_body_max;
    u->start_hp = u->hp;
    u->move_max = clampi(4 + u->ini / 40 - u->armor_body_max / 60, 3, 6);
    u->move_left = u->move_max;
    u->ap = 2;
}

static void squad_order(void) {
    int idx[BT_MAX_UNITS], n = 0;
    for (int i = 0; i < bt.n; i++)
        if (bt.units[i].used) idx[n++] = i;
    /* сортировка по эффективной инициативе (пузырьком — юнитов мало) */
    for (int a = 0; a < n; a++) {
        for (int b = a + 1; b < n; b++) {
            int ia = bt.units[idx[a]].ini - bt.units[idx[a]].fat / 2;
            int ib = bt.units[idx[b]].ini - bt.units[idx[b]].fat / 2;
            if (ib > ia) { int t = idx[a]; idx[a] = idx[b]; idx[b] = t; }
        }
    }
    for (int i = 0; i < n; i++) bt.order[i] = idx[i];
    for (int i = n; i < BT_MAX_UNITS; i++) bt.order[i] = -1;
    bt.turn_i = 0;
}

void bt_setup(int ctx, int ctx_idx, int faction, int enemy_power,
              const char* enemy_name, int terrain, bool scale) {
    memset(&bt, 0, sizeof(bt));
    bt.active = true;
    bt.ctx = ctx; bt.ctx_idx = ctx_idx;
    bt.faction = faction % BT_F_COUNT;
    bt.enemy_power = enemy_power;
    bt.scale = scale;
    snprintf(bt.enemy_name, sizeof(bt.enemy_name), "%s", enemy_name);
    bt.terrain = terrain % BT_T_TYPES;
    bt.sel = -1;
    bt.anim = BT_ANIM_NONE;
}

void bt_begin(void) {
    terrain_gen();

    int n_players = 0;
    for (int i = 0; i < g.n_brothers; i++)
        if (g.brothers[i].hp > 0 && g.brothers[i].injury < 0) n_players++;
        else if (g.brothers[i].hp > 0) n_players++; /* раненые тоже дерутся */
    if (n_players > 8) n_players = 8;

    int n_enemies = clampi(3 + bt.enemy_power / 25, 3, 8);
    if (bt.scale) {
        /* подгоняем количество под отряд */
        int want = clampi(n_players + rng_range(-1, 2), 3, 8);
        if (want * 22 > bt.enemy_power) n_enemies = want;
    }
    if (n_players + n_enemies > BT_MAX_UNITS) n_enemies = BT_MAX_UNITS - n_players;

    bt.n = 0;
    int placed = 0;
    for (int i = 0; i < g.n_brothers && placed < n_players; i++) {
        if (g.brothers[i].hp <= 0) continue;
        BTUnit* u = &bt.units[bt.n];
        unit_place(u, false, i);
        /* расстановка: левый край, две колонны */
        int col = placed / 5, row = placed % 5;
        u->x = 1 + col; u->y = 2 + row;
        for (int guard = 0; guard < BT_H && (!bt_walkable(u->x, u->y) || occupied(u->x, u->y)); guard++)
            u->y = (u->y + 1) % BT_H;
        bt.blocked[u->y][u->x] = false;
        u->fx = (float)u->x; u->fy = (float)u->y;
        bt.n++; placed++;
    }
    int eplaced = 0;
    for (int i = 0; i < n_enemies; i++) {
        BTUnit* u = &bt.units[bt.n];
        unit_place(u, true, i);
        int col = eplaced / 5, row = eplaced % 5;
        u->x = BT_W - 2 - col; u->y = 2 + row;
        for (int guard = 0; guard < BT_H && (!bt_walkable(u->x, u->y) || occupied(u->x, u->y)); guard++)
            u->y = (u->y + 1) % BT_H;
        bt.blocked[u->y][u->x] = false;
        u->fx = (float)u->x; u->fy = (float)u->y;
        bt.n++; eplaced++;
    }

    squad_order();
    bt.round = 1;
    bt_logf("Бой начинается! Враг: %s.", bt.enemy_name);
    bt_logf("Раунд %d.", bt.round);
}

bool occupied(int x, int y) {
    for (int i = 0; i < bt.n; i++) {
        BTUnit* u = &bt.units[i];
        if (u->used && u->alive && !u->fled && u->x == x && u->y == y) return true;
    }
    return false;
}

/* ------------------------------------------------- поиск пути (BFS) ----- */

void bt_reachable(int u, bool vis[BT_H][BT_W]) {
    memset(vis, 0, BT_H * BT_W * sizeof(bool));
    if (u < 0 || u >= bt.n) return;
    BTUnit* s = &bt.units[u];
    if (!s->alive || s->fled) return;

    int head = 0, tail = 0;
    static int qx[BT_W * BT_H], qy[BT_W * BT_H], qd[BT_W * BT_H];
    static uint8_t seen[BT_H][BT_W];
    memset(seen, 0, sizeof(seen));
    qx[tail] = s->x; qy[tail] = s->y; qd[tail] = 0; tail++;
    seen[s->y][s->x] = 1;
    static const int DX[8] = { 1,-1, 0, 0, 1, 1,-1,-1 };
    static const int DY[8] = { 0, 0, 1,-1, 1,-1, 1,-1 };

    while (head < tail) {
        int x = qx[head], y = qy[head], d = qd[head];
        head++;
        if (d > 0) vis[y][x] = true;
        if (d >= s->move_left) continue;
        for (int k = 0; k < 8; k++) {
            int nx = x + DX[k], ny = y + DY[k];
            if (!bt_walkable(nx, ny) || seen[ny][nx]) continue;
            if (occupied(nx, ny) && !(nx == s->x && ny == s->y)) continue;
            /* по диагонали нельзя «срезать» угол стены */
            if (DX[k] && DY[k] && (!bt_walkable(x + DX[k], y) || !bt_walkable(x, y + DY[k]))) continue;
            seen[ny][nx] = 1;
            qx[tail] = nx; qy[tail] = ny; qd[tail] = d + 1; tail++;
        }
    }
}

/* ------------------------------------------------------------- бой ------ */

static bool los_clear(int x1, int y1, int x2, int y2) {
    int dx = absi(x2 - x1), dy = absi(y2 - y1);
    int sx = x1 < x2 ? 1 : -1, sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;
    int x = x1, y = y1;
    while (!(x == x2 && y == y2)) {
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 < dx) { err += dx; y += sy; }
        if (x == x2 && y == y2) return true;
        if (bt.blocked[y][x]) return false;
    }
    return true;
}

bool bt_attack_possible(int a, int b) {
    if (a < 0 || b < 0 || a >= bt.n || b >= bt.n) return false;
    BTUnit* A = &bt.units[a];
    BTUnit* B = &bt.units[b];
    if (!A->alive || !B->alive || A->fled || B->fled) return false;
    if (A->ap < 1) return false;
    int d = dist_cheb(A->x, A->y, B->x, B->y);
    if (d > A->w_range) return false;
    if (d > 1 && !los_clear(A->x, A->y, B->x, B->y)) return false;
    return true;
}

int bt_hit_chance(int a, int b) {
    BTUnit* A = &bt.units[a];
    BTUnit* B = &bt.units[b];
    int atk = A->w_range > 1 ? A->ratk : A->matk;
    int def = A->w_range > 1 ? B->rdef : B->mdef;
    if (B->defending) def += 15;
    if (B->fat > B->fat_max * 3 / 4) def -= 5;
    if (A->fat > A->fat_max * 3 / 4) atk -= 5;
    return clampi(75 + atk - def, 5, 95);
}

bool bt_move(GameState* gs, int u, int x, int y) {
    (void)gs;
    if (u < 0 || u >= bt.n || bt.anim != BT_ANIM_NONE) return false;
    BTUnit* s = &bt.units[u];
    if (!s->alive || s->fled || s->ap < 1) return false;
    bool vis[BT_H][BT_W];
    bt_reachable(u, vis);
    if (!vis[y][x]) return false;
    int d = dist_cheb(s->x, s->y, x, y);
    if (d > s->move_left) return false;

    s->move_left -= d;
    s->x = x; s->y = y;
    /* усталость за движение */
    s->fat = clampi(s->fat + d * 4, 0, s->fat_max + 20);
    bt.anim = BT_ANIM_MOVE;
    bt.anim_t = 0;
    bt.anim_a = u;
    return true;
}

bool bt_attack(GameState* gs, int a, int b) {
    (void)gs;
    if (bt.anim != BT_ANIM_NONE) return false;
    if (!bt_attack_possible(a, b)) return false;
    BTUnit* A = &bt.units[a];
    BTUnit* B = &bt.units[b];

    A->ap--;
    A->fat = clampi(A->fat + (A->w_range > 1 ? 10 : 15), 0, A->fat_max + 20);

    int chance = bt_hit_chance(a, b);
    int roll = rng_range(1, 100);
    bt.anim = A->w_range > 1 ? BT_ANIM_SHOOT : BT_ANIM_LUNGE;
    bt.anim_t = 0;
    bt.anim_a = a;
    bt.anim_b = b;

    if (roll > chance) {
        bt.float_dmg = 0;
        bt.float_t = 1.0f;
        bt.float_x = B->x; bt.float_y = B->y;
        bt_logf("%s промахивается по %s.", A->name, B->name);
        return true;
    }

    /* урон */
    int dmg = rng_range(A->w_min, A->w_max);
    bool crit = roll <= chance / 4;   /* раннее попадание — «крит» */
    if (crit) dmg = dmg * 3 / 2;

    int head = rng_range(0, 99) < 25;
    int* armor = head ? &B->armor_head : &B->armor_body;
    int pen = clampi(A->w_pen, 0, 80);
    int absorb = dmg * (100 - pen) / 100;
    int hp_dmg = dmg - absorb;
    if (absorb > *armor) {
        hp_dmg += absorb - *armor;
        *armor = 0;
    } else {
        *armor -= absorb;
    }
    if (B->shield > 0 && !crit) hp_dmg = hp_dmg * (100 - clampi(B->shield, 0, 30)) / 100;
    if (hp_dmg < 1) hp_dmg = 1;
    B->hp -= hp_dmg;

    bt.float_dmg = hp_dmg;
    bt.float_crit = crit;
    bt.float_t = 1.0f;
    bt.float_x = B->x; bt.float_y = B->y;

    bt_logf("%s %s %s: %d урона%s.", A->name,
            A->w_range > 1 ? "стреляет в" : "бьёт",
            B->name, hp_dmg, head ? " (в голову)" : "");

    if (B->hp <= 0) {
        B->hp = 0;
        B->alive = false;
        bt.anim = BT_ANIM_DIE;
        bt_logf("%s падает замертво!", B->name);
        if (A->enemy) bt.kills_enemy++; else { bt.kills_player++; A->kills++; }
        /* мораль союзников */
        for (int i = 0; i < bt.n; i++) {
            BTUnit* u = &bt.units[i];
            if (!u->used || !u->alive || u->fled || u->enemy != B->enemy) continue;
            if (rng_range(1, 100) > u->res) {
                u->waver++;
                if (u->waver >= 2) {
                    u->fled = true;
                    bt_logf("%s в ужасе бежит с поля боя!", u->name);
                } else {
                    bt_logf("%s потрясён гибелью товарища.", u->name);
                }
            }
        }
    }
    return true;
}

void bt_defend(GameState* gs, int u) {
    (void)gs;
    if (u < 0 || u >= bt.n) return;
    BTUnit* s = &bt.units[u];
    if (!s->alive || s->fled || s->ap < 1) return;
    s->defending = true;
    s->ap = 0;
    bt_logf("%s занимает оборону.", s->name);
}

void bt_wait(GameState* gs, int u) {
    (void)gs;
    if (u < 0 || u >= bt.n) return;
    BTUnit* s = &bt.units[u];
    if (!s->alive || s->fled) return;
    s->ap = 0;
    s->move_left = 0;
}

void bt_retreat(GameState* gs) {
    (void)gs;
    bt.retreated = true;
    bt.finished = true;
    bt.victory = false;
    bt_logf("Отряд отступает с поля боя!");
}

static void next_round(void) {
    bt.round++;
    for (int i = 0; i < bt.n; i++) {
        BTUnit* u = &bt.units[i];
        if (!u->used || !u->alive) continue;
        u->defending = false;
        u->move_left = u->move_max;
        u->ap = 2;
        u->fat = clampi(u->fat - 15, 0, u->fat_max + 20);
        if (u->fat >= u->fat_max) {
            u->ap = 1;
            bt_logf("%s выдохся.", u->name);
        }
    }
    squad_order();
    bt_logf("Раунд %d.", bt.round);
}

void bt_end_turn(GameState* gs) {
    (void)gs;
    if (bt.finished) return;
    for (;;) {
        bt.turn_i++;
        if (bt.turn_i >= bt.n) {
            next_round();
            /* после пересортировки order[0] — первый юнит нового раунда */
            if (bt.n <= 0) return;
            bt.turn_i = 0;
        }
        int u = bt.order[bt.turn_i];
        BTUnit* p = (u >= 0 && u < bt.n) ? &bt.units[u] : NULL;
        if (p && p->used && p->alive && !p->fled) break;
    }
    bt.ai_delay = 0.35f;
}

/* ------------------------------------------------------------- ИИ ------- */

static int pick_target(BTUnit* e) {
    int best = -1, best_score = 1 << 29;
    for (int i = 0; i < bt.n; i++) {
        BTUnit* t = &bt.units[i];
        if (!t->used || !t->alive || t->fled || t->enemy == e->enemy) continue;
        int d = dist_cheb(e->x, e->y, t->x, t->y);
        int score = d * 10;
        score += t->hp;                    /* добивать раненых */
        if (e->w_range > 1 && d <= e->w_range && los_clear(e->x, e->y, t->x, t->y))
            score -= 60;                   /* уже в зоне огня */
        if (d == 1) score -= 40;
        if (score < best_score) { best_score = score; best = i; }
    }
    return best;
}

void bt_ai_act(GameState* gs) {
    int u = bt_current();
    if (u < 0 || bt.finished) return;
    BTUnit* e = &bt.units[u];
    if (!e->enemy) return;

    int target = pick_target(e);
    if (target < 0) {
        bt_end_turn(gs);
        return;
    }
    BTUnit* t = &bt.units[target];

    /* трусость: почти без HP — уходим */
    if (e->hp < e->hp_max / 5 && rng_range(0, 99) < 35) {
        int bx = e->x, by = e->y;
        for (int y = 0; y < BT_H; y++)
            for (int x = 0; x < BT_W; x++) {
                bool vis[BT_H][BT_W];
                bt_reachable(u, vis);
                if (!vis[y][x]) continue;
                int d = dist_cheb(x, y, t->x, t->y);
                if (d > dist_cheb(bx, by, t->x, t->y)) { bx = x; by = y; }
                goto moved;   /* один проход достаточно */
            }
        moved:
        if (bx != e->x || by != e->y) bt_move(gs, u, bx, by);
        bt_end_turn(gs);
        return;
    }

    /* стрелок: стреляем, если цель в радиусе с линией огня */
    if (e->w_range > 1 && bt_attack_possible(u, target)) {
        bt_attack(gs, u, target);
        if (e->ap > 0 && t->alive && bt_attack_possible(u, target)) bt_attack(gs, u, target);
        bt_end_turn(gs);
        return;
    }

    /* ищем клетку, с которой можно атаковать */
    int bestx = e->x, besty = e->y, best_d = dist_cheb(e->x, e->y, t->x, t->y);
    bool vis[BT_H][BT_W];
    bt_reachable(u, vis);
    for (int y = 0; y < BT_H; y++) {
        for (int x = 0; x < BT_W; x++) {
            if (!vis[y][x]) continue;
            int d = dist_cheb(x, y, t->x, t->y);
            bool can_hit = (e->w_range > 1) ? (d <= e->w_range && los_clear(x, y, t->x, t->y))
                                           : (d == 1);
            if (can_hit && (best_d > 1 || d < best_d)) { bestx = x; besty = y; best_d = d; }
            else if (!can_hit && d < best_d) { bestx = x; besty = y; best_d = d; }
        }
    }
    if (bestx != e->x || besty != e->y) {
        bt_move(gs, u, bestx, besty);
        /* после анимации движения атакуем — но проще сразу (позиция уже обновлена) */
        if (e->ap > 0 && bt_attack_possible(u, target)) bt_attack(gs, u, target);
    } else if (e->ap > 0 && bt_attack_possible(u, target)) {
        bt_attack(gs, u, target);
    }
    bt_end_turn(gs);
}

/* --------------------------------------------------------- анимация ----- */

bool bt_anim_tick(float dt) {
    if (bt.anim == BT_ANIM_NONE) return false;
    if (bt.anim == BT_ANIM_MOVE) {
        BTUnit* s = &bt.units[bt.anim_a];
        float speed = 6.0f;
        float dx = (float)s->x - s->fx, dy = (float)s->y - s->fy;
        float d2 = dx * dx + dy * dy;
        if (d2 < 0.001f) {
            s->fx = (float)s->x; s->fy = (float)s->y;
            bt.anim = BT_ANIM_NONE;
            return true;
        }
        float step = speed * dt;
        float len = d2 > 0 ? __builtin_sqrtf(d2) : 1.0f;
        if (step >= len) {
            s->fx = (float)s->x; s->fy = (float)s->y;
            bt.anim = BT_ANIM_NONE;
            return true;
        }
        s->fx += dx / len * step;
        s->fy += dy / len * step;
        return false;
    }
    bt.anim_t += dt * 3.2f;
    if (bt.float_t > 0) bt.float_t -= dt * 1.1f;
    if (bt.anim_t >= 1.0f) {
        /* после смерти — возвращаемся к прошлой анимации, если нужно */
        if (bt.anim == BT_ANIM_DIE) {
            BTUnit* d = &bt.units[bt.anim_b >= 0 ? bt.anim_b : bt.anim_a];
            (void)d;
        }
        bt.anim = BT_ANIM_NONE;
        bt.anim_t = 0;
        return true;
    }
    return false;
}

/* ------------------------------------------------------- завершение ----- */

bool bt_check_end(GameState* gs) {
    if (!gs) gs = &g;
    if (bt.finished) return true;
    if (bt_retreated_check()) return true;
    int ae = bt_alive_enemy(), ap = bt_alive_player();
    if (ae == 0) {
        bt.finished = true;
        bt.victory = true;
        bt_logf("Победа! Враг разбит.");
        bt_finish(gs);
        return true;
    }
    if (ap == 0) {
        bt.finished = true;
        bt.victory = false;
        bt_logf("Поражение... Отряд уничтожен или бежал.");
        bt_finish(gs);
        return true;
    }
    return false;
}

static bool bt_retreated_check(void) {
    return bt.retreated;
}

void bt_finish(GameState* gs) {
    if (!gs) gs = &g;
    BattleReport* br = &gs->battle;
    memset(br, 0, sizeof(*br));
    br->active = true;
    br->won = bt.victory && !bt.retreated;
    snprintf(br->title, sizeof(br->title), "%s: %s",
             br->won ? "Победа" : (bt.retreated ? "Отступление" : "Поражение"),
             bt.enemy_name);

    int lines = 0;
    for (int i = 0; i < bt.n_pre && lines < MAX_BATTLE_LINES; i++)
        snprintf(br->lines[lines++], 128, "%s", bt.pre[i]);
    if (bt.n_pre > 0 && lines < MAX_BATTLE_LINES)
        snprintf(br->lines[lines++], 128, "--- Схватка ---");

    int xp = 0, gold = 0;

    if (br->won) {
        xp = 25 + bt.kills_player * 12 + bt.enemy_power / 3;
        gold = 20 + bt.enemy_power * (2 + rng_range(0, 3)) / 2 + bt.kills_player * 5;
    } else {
        xp = 8 + bt.kills_player * 6;
        gold = 0;
    }

    /* судьба бойцов */
    int casualties = 0;
    for (int i = 0; i < bt.n; i++) {
        BTUnit* u = &bt.units[i];
        if (!u->used || u->enemy || u->brother < 0) continue;
        Brother* b = &gs->brothers[u->brother];
        b->battles++;
        b->kills += u->kills;
        b->fatigue = clampi(u->fat, 0, b->base.fatigue);
        if (u->hp <= 0) {
            /* последний бросок: выжил ли */
            if (rng_range(1, 100) <= b->base.resolve / 2) {
                b->hp = 1;
                b->injury = 1;          /* тяжёлая травма */
                b->injury_days = 8 + rng_range(0, 10);
                if (lines < MAX_BATTLE_LINES)
                    snprintf(br->lines[lines++], 128, "%s чудом выжил, но тяжело ранен.", b->name);
            } else {
                b->hp = 0;
                casualties++;
                if (lines < MAX_BATTLE_LINES)
                    snprintf(br->lines[lines++], 128, "%s погиб в бою.", b->name);
            }
        } else {
            b->hp = u->hp;
            if (lines < MAX_BATTLE_LINES && u->start_hp - u->hp >= u->hp_max / 3)
                snprintf(br->lines[lines++], 128, "%s получил серьёзные ранения.", b->name);
        }
        b->xp += xp;
        /* уровень */
        while (b->xp >= b->level * 100 + 100) {
            b->xp -= b->level * 100 + 100;
            b->level++;
            b->stat_picks += 3;
            b->perk_points++;
            if (lines < MAX_BATTLE_LINES)
                snprintf(br->lines[lines++], 128, "%s получает уровень %d!", b->name, b->level);
        }
    }

    if (br->won) {
        gs->crowns += gold;
        gs->renown += 1 + (bt.enemy_power > 60 ? 1 : 0);
        gs->battles_won++;
    } else {
        gs->battles_lost++;
    }
    gs->day += 1;   /* бой съедает день */

    /* контракты */
    if (bt.ctx == BT_CTX_CONTRACT && bt.ctx_idx >= 0 && bt.ctx_idx < MAX_CONTRACTS) {
        Contract* c = &gs->contracts[bt.ctx_idx];
        if (br->won && c->active && c->type == CONTRACT_HUNT) {
            if (lines < MAX_BATTLE_LINES)
                snprintf(br->lines[lines++], 128, "Контракт «%s»: цель уничтожена.", c->title);
            gs->crowns += c->reward_gold;
            for (int i = 0; i < bt.n; i++) {
                BTUnit* u = &bt.units[i];
                if (u->used && !u->enemy && u->brother >= 0)
                    gs->brothers[u->brother].xp += c->reward_xp;
            }
            if (lines < MAX_BATTLE_LINES)
                snprintf(br->lines[lines++], 128, "Контракт выполнен! Награда: %d крон.", c->reward_gold);
            c->active = false;
            for (int k = 0; k < gs->n_active; k++)
                if (gs->active_contracts[k] == bt.ctx_idx) gs->active_contracts[k] = -1;
        }
    }
    /* локация (зачистка + лут) */
    if (bt.loc_idx >= 0 && bt.loc_idx < gs->n_locations) {
        Location* L = &gs->locations[bt.loc_idx];
        if (br->won) {
            L->cleared = 1;
            L->enemy_power = 0;
            gs->crowns += L->loot_gold;
            if (L->loot_item != ITEM_NONE) game_add_item(L->loot_item);
            if (lines < MAX_BATTLE_LINES)
                snprintf(br->lines[lines++], 128, "Локация «%s» зачищена: +%d крон.",
                         L->name, L->loot_gold);
        }
    }

    br->gold = gold;
    br->xp = xp;
    br->casualties = casualties;
    br->n_lines = lines;
    if (lines < MAX_BATTLE_LINES) {
        snprintf(br->lines[lines++], 128, "Убито врагов: %d. Потери: %d.", bt.kills_player, casualties);
        br->n_lines = lines;
    }

    bt.active = false;
}
