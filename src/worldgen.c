/* worldgen.c — генерация мира: рельеф, биомы, города, локации, дороги, A*. */
#include "game.h"
#include "rng.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

const int g_terrain_cost[TERR_COUNT] = {
    0,  /* глубокая вода */
    0,  /* вода          */
    3,  /* песок         */
    3,  /* трава         */
    4,  /* лес           */
    5,  /* холмы         */
    7,  /* горы          */
    6,  /* болото        */
    5,  /* снег          */
};

const char* g_terrain_name[TERR_COUNT] = {
    "Глубокая вода", "Вода", "Пляж", "Равнины", "Лес",
    "Холмы", "Горы", "Болото", "Снега",
};

const char* g_town_type_name[TOWN_TYPE_COUNT] = {
    "Деревня", "Город", "Крепость", "Замок",
};

const char* g_loc_name[LOC_TYPE_COUNT] = {
    "Руины", "Логово", "Шахта", "Святилище", "Хутор",
};

/* --------------------------------------------------------------- шум --- */
static uint64_t noise_seed;

static float lattice(int x, int y, uint64_t s) {
    uint64_t n = (uint64_t)x * 374761393ull + (uint64_t)y * 668265263ull + s * 951274213ull;
    n = (n ^ (n >> 13)) * 1274126177ull;
    n = n ^ (n >> 16);
    return (float)(n & 0xFFFFFF) / (float)0xFFFFFF;
}

static float smoothstep(float t) { return t * t * (3.0f - 2.0f * t); }

static float vnoise(float x, float y, uint64_t s) {
    int x0 = (int)floorf(x), y0 = (int)floorf(y);
    float fx = smoothstep(x - x0), fy = smoothstep(y - y0);
    float a = lattice(x0, y0, s), b = lattice(x0 + 1, y0, s);
    float c = lattice(x0, y0 + 1, s), d = lattice(x0 + 1, y0 + 1, s);
    float top = a + (b - a) * fx;
    float bot = c + (d - c) * fx;
    return top + (bot - top) * fy;
}

static float fbm(float x, float y, int octaves, uint64_t s) {
    float sum = 0, amp = 1, tot = 0, sc = 1;
    for (int i = 0; i < octaves; i++) {
        sum += amp * vnoise(x * sc, y * sc, s + (uint64_t)i * 17);
        tot += amp;
        amp *= 0.5f;
        sc *= 2.0f;
    }
    return sum / tot;
}

/* ------------------------------------------------------------- доступ --- */
int terrain_at(int x, int y) {
    if (x < 0 || y < 0 || x >= g.world_w || y >= g.world_h) return TERR_DEEP;
    return g.terrain[y * g.world_w + x];
}

bool road_at(int x, int y) {
    if (x < 0 || y < 0 || x >= g.world_w || y >= g.world_h) return false;
    return g.road[y * g.world_w + x] != 0;
}

int tile_move_cost(int x, int y) {
    int t = terrain_at(x, y);
    if (g_terrain_cost[t] == 0) return 0;
    if (road_at(x, y)) return 2;
    return g_terrain_cost[t];
}

/* --------------------------------------------------- генерация мира ----- */
static uint8_t* s_mainland = NULL;

static void gen_terrain(uint32_t seed) {
    int w = g.world_w, h = g.world_h;
    int total = w * h;
    float* elev = malloc(sizeof(float) * total);
    float* moist = malloc(sizeof(float) * total);
    /* высота: fBm + радиальное затухание к краям */
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float nx = (float)x / (float)w, ny = (float)y / (float)h;
            float e = fbm(nx * 5.2f, ny * 5.2f, 5, seed);
            e += (fbm(nx * 14.0f, ny * 14.0f, 3, seed + 991) - 0.5f) * 0.14f;
            float dx = (nx - 0.5f) * 2.05f, dy = (ny - 0.5f) * 2.25f;
            float fall = 1.0f - clampf(sqrtf(dx * dx + dy * dy), 0.0f, 1.0f);
            e = e * 0.72f + fall * 0.28f;
            elev[y * w + x] = e;
            moist[y * w + x] = fbm(nx * 6.4f + 3.7f, ny * 6.4f + 1.3f, 4, seed + 7717);
        }
    }
    /* уровень моря — 42-й процентиль: суша ~58% карты */
    int hist[256] = { 0 };
    for (int i = 0; i < total; i++) {
        int b = (int)(elev[i] * 255.0f);
        hist[clampi(b, 0, 255)]++;
    }
    int want = total * 42 / 100, acc = 0, sea_i = 0;
    for (int i = 0; i < 256; i++) {
        acc += hist[i];
        if (acc >= want) { sea_i = i; break; }
    }
    float sea = (float)sea_i / 255.0f;
    /* сглаживание порога: глубокая вода/мелководье/пляж */
    for (int i = 0; i < total; i++) {
        float e = elev[i], m = moist[i];
        int x = i % w, y = i / w;
        float ny = (float)y / (float)h;
        int t;
        if (e < sea - 0.06f)       t = TERR_DEEP;
        else if (e < sea)          t = TERR_WATER;
        else if (e < sea + 0.022f) t = TERR_SAND;
        else if (e > sea + 0.30f)  t = TERR_MOUNTAIN;
        else if (e > sea + 0.22f)  t = TERR_HILLS;
        else if (ny < 0.10f)       t = TERR_SNOW;
        else if (m > 0.60f && e < sea + 0.16f) t = TERR_SWAMP;
        else if (m > 0.52f)        t = TERR_FOREST;
        else                       t = TERR_GRASS;
        if (t == TERR_MOUNTAIN && m > 0.66f) t = TERR_HILLS;
        g.terrain[i] = (uint8_t)t;
        g.decor[i] = (uint8_t)(lattice(x, y, seed + 4242) * 3.0f);
    }
    free(elev);
    free(moist);

    /* --- крупнейший массив суши (flood fill) --- */
    if (s_mainland) { free(s_mainland); s_mainland = NULL; }
    s_mainland = calloc(total, 1);
    static int* stack = NULL;
    static int cap = 0;
    if (cap < total) { free(stack); stack = malloc(sizeof(int) * total); cap = total; }
    int best_size = 0, best_label = 0;
    uint8_t* labels = calloc(total, 1);
    int label = 0;
    for (int i = 0; i < total; i++) {
        if (labels[i] || g_terrain_cost[g.terrain[i]] == 0) continue;
        label++;
        int sp = 0, size = 0;
        stack[sp++] = i;
        labels[i] = (uint8_t)label;
        while (sp > 0) {
            int c = stack[--sp];
            size++;
            int cx = c % w, cy = c / w;
            static const int DX[4] = { 1, -1, 0, 0 };
            static const int DY[4] = { 0, 0, 1, -1 };
            for (int d = 0; d < 4; d++) {
                int nx = cx + DX[d], ny2 = cy + DY[d];
                if (nx < 0 || ny2 < 0 || nx >= w || ny2 >= h) continue;
                int ni = ny2 * w + nx;
                if (labels[ni] || g_terrain_cost[g.terrain[ni]] == 0) continue;
                labels[ni] = (uint8_t)label;
                stack[sp++] = ni;
            }
        }
        if (size > best_size) { best_size = size; best_label = label; }
    }
    for (int i = 0; i < total; i++)
        if (labels[i] == best_label) s_mainland[i] = 1;
    free(labels);
}

bool mainland_at(int x, int y) {
    if (x < 0 || y < 0 || x >= g.world_w || y >= g.world_h) return false;
    if (!s_mainland) return false;
    return s_mainland[y * g.world_w + x] != 0;
}

static bool land_at(int x, int y) {
    return g_terrain_cost[terrain_at(x, y)] > 0;
}

static void place_towns(uint32_t seed) {
    (void)seed;
    int w = g.world_w, h = g.world_h;
    int want = 11;
    int tries = 0;
    g.n_towns = 0;
    while (g.n_towns < want && tries < 60000) {
        tries++;
        int x = rng_range(6, w - 7);
        int y = rng_range(6, h - 7);
        if (!mainland_at(x, y)) continue;
        int t = terrain_at(x, y);
        if (t == TERR_MOUNTAIN || t == TERR_SWAMP) continue;
        bool near_water = false, ok = true;
        for (int dy = -4; dy <= 4 && ok; dy++) {
            for (int dx = -4; dx <= 4; dx++) {
                int tt = terrain_at(x + dx, y + dy);
                if (tt == TERR_WATER || tt == TERR_DEEP) near_water = true;
                if (g_terrain_cost[tt] == 0 && abs(dx) <= 1 && abs(dy) <= 1) { ok = false; break; }
            }
        }
        if (!ok) continue;
        /* разнесение городов */
        for (int i = 0; i < g.n_towns; i++) {
            int ddx = g.towns[i].x - x, ddy = g.towns[i].y - y;
            if (ddx * ddx + ddy * ddy < 16 * 16) { ok = false; break; }
        }
        if (!ok) continue;
        (void)near_water;

        int type;
        int idx = g.n_towns;
        if (idx == 0)      type = TOWN_CITY;
        else if (idx == 1) type = TOWN_CITY;
        else if (idx < 5)  type = TOWN_TOWN;
        else if (idx < 8)  type = TOWN_CASTLE;
        else               type = TOWN_VILLAGE;

        town_generate(&g.towns[idx], idx, x, y, type);
        g.n_towns++;
    }
    /* гарантируем минимальный набор */
    while (g.n_towns < 6) {
        int x = rng_range(8, w - 9), y = rng_range(8, h - 9);
        if (!mainland_at(x, y)) continue;
        bool ok = true;
        for (int i = 0; i < g.n_towns; i++) {
            int ddx = g.towns[i].x - x, ddy = g.towns[i].y - y;
            if (ddx * ddx + ddy * ddy < 12 * 12) { ok = false; break; }
        }
        if (!ok) continue;
        town_generate(&g.towns[g.n_towns], g.n_towns, x, y,
                      g.n_towns < 2 ? TOWN_TOWN : TOWN_VILLAGE);
        g.n_towns++;
    }
}

static void place_locations(void) {
    int w = g.world_w, h = g.world_h;
    g.n_locations = 0;
    int per_type[LOC_TYPE_COUNT] = { 5, 5, 3, 3, 3 };
    for (int t = 0; t < LOC_TYPE_COUNT; t++) {
        for (int k = 0; k < per_type[t]; k++) {
            int tries = 0;
            while (tries++ < 4000) {
                int x = rng_range(4, w - 5), y = rng_range(4, h - 5);
                if (!mainland_at(x, y)) continue;
                bool ok = true;
                for (int i = 0; i < g.n_towns; i++) {
                    int dx = g.towns[i].x - x, dy = g.towns[i].y - y;
                    if (dx * dx + dy * dy < 7 * 7) { ok = false; break; }
                }
                for (int i = 0; ok && i < g.n_locations; i++) {
                    int dx = g.locations[i].x - x, dy = g.locations[i].y - y;
                    if (dx * dx + dy * dy < 6 * 6) { ok = false; break; }
                }
                if (!ok) continue;
                Location* L = &g.locations[g.n_locations];
                L->id = g.n_locations;
                L->x = x;
                L->y = y;
                L->type = t;
                snprintf(L->name, sizeof(L->name), "%s", gen_loc_name((LocType)t));
                L->enemy_power = 30 + rng_range(0, 60) + t * 8;
                L->cleared = 0;
                L->loot_gold = rng_range(40, 220);
                L->loot_item = (rng_float() < 0.55f) ? rng_range(0, g_item_def_count - 1) : ITEM_NONE;
                g.n_locations++;
                break;
            }
        }
    }
}

/* Дороги: остовное дерево городов + пара дополнительных веток. */
static void lay_road(TilePos* pts, int n) {
    for (int i = 0; i + 1 < n; i++) {
        int x0 = pts[i].x, y0 = pts[i].y, x1 = pts[i + 1].x, y1 = pts[i + 1].y;
        int steps = abs(x1 - x0) > abs(y1 - y0) ? abs(x1 - x0) : abs(y1 - y0);
        if (steps < 1) steps = 1;
        for (int s = 0; s <= steps; s++) {
            float tt = (float)s / (float)steps;
            /* лёгкая кривизна */
            int x = x0 + (int)((x1 - x0) * tt + 0.5f);
            int y = y0 + (int)((y1 - y0) * tt + 0.5f);
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int xx = x + dx, yy = y + dy;
                    if (xx < 1 || yy < 1 || xx >= g.world_w - 1 || yy >= g.world_h - 1) continue;
                    if (terrain_at(xx, yy) == TERR_DEEP || terrain_at(xx, yy) == TERR_WATER) continue;
                    /* дорога сглаживает горы/болота */
                    if (terrain_at(xx, yy) == TERR_MOUNTAIN) g.terrain[yy * g.world_w + xx] = TERR_HILLS;
                    if (terrain_at(xx, yy) == TERR_SWAMP && dx == 0 && dy == 0)
                        g.terrain[yy * g.world_w + xx] = TERR_GRASS;
                    g.road[yy * g.world_w + xx] = 1;
                }
            }
        }
    }
}

static void make_roads(void) {
    /* Прим: MST по городам */
    bool in_tree[MAX_TOWNS] = { false };
    TilePos pts[512];
    in_tree[0] = true;
    for (int e = 0; e < g.n_towns - 1; e++) {
        int best_i = -1, best_j = -1, best_d = 1 << 30;
        for (int i = 0; i < g.n_towns; i++) {
            if (!in_tree[i]) continue;
            for (int j = 0; j < g.n_towns; j++) {
                if (in_tree[j]) continue;
                int dx = g.towns[i].x - g.towns[j].x, dy = g.towns[i].y - g.towns[j].y;
                int d = dx * dx + dy * dy;
                if (d < best_d) { best_d = d; best_i = i; best_j = j; }
            }
        }
        if (best_j < 0) break;
        in_tree[best_j] = true;
        /* ломаная между городами с промежуточной точкой */
        int n = 0;
        pts[n++] = (TilePos){ g.towns[best_i].x, g.towns[best_i].y };
        int mx = (g.towns[best_i].x + g.towns[best_j].x) / 2;
        int my = (g.towns[best_i].y + g.towns[best_j].y) / 2;
        mx += rng_range(-4, 4);
        my += rng_range(-4, 4);
        pts[n++] = (TilePos){ mx, my };
        pts[n++] = (TilePos){ g.towns[best_j].x, g.towns[best_j].y };
        lay_road(pts, n);
    }
    /* пара «кольцевых» дорог */
    for (int k = 0; k < 3; k++) {
        int i = rng_range(0, g.n_towns - 1), j = rng_range(0, g.n_towns - 1);
        if (i == j) continue;
        pts[0] = (TilePos){ g.towns[i].x, g.towns[i].y };
        pts[1] = (TilePos){ g.towns[j].x, g.towns[j].y };
        lay_road(pts, 2);
    }
    /* площади городов */
    for (int i = 0; i < g.n_towns; i++) {
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++)
                g.road[(g.towns[i].y + dy) * g.world_w + (g.towns[i].x + dx)] = 1;
    }
}

bool world_generate(int w, int h, uint32_t seed) {
    world_free();
    g.world_w = w;
    g.world_h = h;
    g.seed = seed;
    g.terrain = calloc((size_t)w * h, 1);
    g.road = calloc((size_t)w * h, 1);
    g.decor = calloc((size_t)w * h, 1);
    if (!g.terrain || !g.road || !g.decor) return false;
    rng_seed(seed);
    noise_seed = seed;
    gen_terrain(seed);
    place_towns(seed);
    place_locations();
    make_roads();
    return g.n_towns > 0;
}

void world_free(void) {
    free(s_mainland); s_mainland = NULL;
    free(g.terrain); g.terrain = NULL;
    free(g.road);    g.road = NULL;
    free(g.decor);   g.decor = NULL;
}

/* ------------------------------------------------------------------ A* --- */
typedef struct { int x, y, f, g; } ANode;

static int astar_h(int x0, int y0, int x1, int y1) {
    int dx = abs(x0 - x1), dy = abs(y0 - y1);
    return (dx > dy ? dx : dy) + (dx < dy ? dx : dy) / 2;
}


bool find_path(int x0, int y0, int x1, int y1, TilePos* out, int* out_len) {
    int w = g.world_w, h = g.world_h;
    if (x0 < 0 || y0 < 0 || x0 >= w || y0 >= h) return false;
    if (x1 < 0 || y1 < 0 || x1 >= w || y1 >= h) return false;
    if (tile_move_cost(x1, y1) == 0) {
        /* цель в воде — идём к ближайшей проходимой соседней клетке */
        int best = -1, bd = 1 << 30;
        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                if (tile_move_cost(x1 + dx, y1 + dy) == 0) continue;
                int d = dx * dx + dy * dy;
                if (d < bd) { bd = d; best = (y1 + dy) * w + (x1 + dx); }
            }
        }
        if (best < 0) return false;
        x1 = best % w;
        y1 = best / w;
    }

    static int* cost = NULL;
    static int* came = NULL;
    static uint8_t* closed = NULL;
    static int cap = 0;
    if (cap < w * h) {
        free(cost); free(came); free(closed);
        cap = w * h;
        cost = malloc(sizeof(int) * cap);
        came = malloc(sizeof(int) * cap);
        closed = malloc(cap);
    }
    for (int i = 0; i < w * h; i++) { cost[i] = 1 << 30; came[i] = -1; closed[i] = 0; }

    /* открытый список — простой массив (карты небольшие) */
    static int open[32768];
    static int open_f[32768];
    int n_open = 0;

    int start = y0 * w + x0, goal = y1 * w + x1;
    cost[start] = 0;
    open[n_open] = start;
    open_f[n_open] = astar_h(x0, y0, x1, y1);
    n_open++;

    static const int DX[8] = { 1, -1, 0, 0, 1, 1, -1, -1 };
    static const int DY[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };

    int guard = 0;
    while (n_open > 0 && guard++ < 200000) {
        /* выбор минимума */
        int bi = 0;
        for (int i = 1; i < n_open; i++)
            if (open_f[i] < open_f[bi]) bi = i;
        int cur = open[bi];
        open[bi] = open[--n_open];

        if (cur == goal) {
            /* восстановление */
            int n = 0, c = cur;
            while (c != -1 && n < MAX_PATH) {
                out[n].x = c % w;
                out[n].y = c / w;
                n++;
                if (c == start) break;
                c = came[c];
            }
            /* разворот */
            for (int i = 0; i < n / 2; i++) {
                TilePos t = out[i];
                out[i] = out[n - 1 - i];
                out[n - 1 - i] = t;
            }
            *out_len = n;
            return n > 0;
        }
        closed[cur] = 1;
        int cx = cur % w, cy = cur / w;
        for (int d = 0; d < 8; d++) {
            int nx = cx + DX[d], ny = cy + DY[d];
            if (nx < 0 || ny < 0 || nx >= w || ny >= h) continue;
            int ni = ny * w + nx;
            if (closed[ni]) continue;
            int step = tile_move_cost(nx, ny);
            if (step == 0) continue;
            /* по диагонали дороже, и нельзя резать воду */
            if (d >= 4) {
                if (tile_move_cost(cx, ny) == 0 || tile_move_cost(nx, cy) == 0) continue;
                step = step * 14 / 10;
            }
            int ng = cost[cur] + step;
            if (ng < cost[ni]) {
                cost[ni] = ng;
                came[ni] = cur;
                if (n_open < 32768) {
                    open[n_open] = ni;
                    open_f[n_open] = ng + astar_h(nx, ny, x1, y1) * 2;
                    n_open++;
                }
            }
        }
    }
    return false;
}
