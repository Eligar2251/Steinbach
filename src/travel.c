/* travel.c — движение отряда по карте, течение времени в пути. */
#include "game.h"
#include <math.h>
#include <stdio.h>

float travel_hours_per_tile(int x, int y) {
    int c = tile_move_cost(x, y);
    if (c <= 0) return 99.0f;
    return (float)c * 0.85f;
}

bool travel_is_traveling(void) {
    return g.traveling && g.path_pos < g.path_len;
}

void travel_stop(void) {
    g.traveling = false;
    g.path_len = 0;
    g.path_pos = 0;
    g.move_progress = 0.0f;
}

void travel_set_dest(int x, int y) {
    int px = (int)g.party_x, py = (int)g.party_y;
    if (px == x && py == y) return;
    TilePos path[MAX_PATH];
    int n = 0;
    if (!find_path(px, py, x, y, path, &n)) return;
    if (n < 2) return;
    for (int i = 0; i < n; i++) g.path[i] = path[i];
    g.path_len = n;
    g.path_pos = 1;
    g.move_progress = 0.0f;
    g.traveling = true;
}

static void advance_hours(float hours) {
    while (hours > 0.0f) {
        float to_midnight = (float)(24 - g.hour);
        float step = hours < to_midnight ? hours : to_midnight;
        g.hour += (int)step;
        if (g.hour >= 24) {
            g.hour = 0;
            game_daily_tick();
        }
        hours -= step;
    }
}

void travel_update(float dt) {
    if (!travel_is_traveling()) {
        if (g.traveling) g.traveling = false;
        return;
    }
    TilePos* t = &g.path[g.path_pos];
    float tx = (float)t->x + 0.5f, ty = (float)t->y + 0.5f;
    float dx = tx - g.party_x, dy = ty - g.party_y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist < 0.01f) {
        g.party_x = tx;
        g.party_y = ty;
        g.path_pos++;
        g.move_progress = 0.0f;
        if (g.path_pos >= g.path_len) {
            g.traveling = false;
            game_arrive_at((int)g.party_x, (int)g.party_y);
            return;
        }
        return;
    }
    /* сколько «тайловых единиц» проходим за секунду: зависит от часов на тайл */
    float hours_per_tile = travel_hours_per_tile(t->x, t->y) * g.speed_mult;
    if (hours_per_tile < 0.4f) hours_per_tile = 0.4f;
    float tiles_per_sec = 2.2f / hours_per_tile;   /* игровой скорости */
    float step = tiles_per_sec * dt;
    if (step > dist) {
        /* прибыли на тайл */
        float frac = dist / (tiles_per_sec > 0 ? tiles_per_sec : 1);
        advance_hours(hours_per_tile * frac);
        g.party_x = tx;
        g.party_y = ty;
        g.path_pos++;
        g.move_progress = 0.0f;
        if (g.path_pos >= g.path_len) {
            g.traveling = false;
            game_arrive_at((int)g.party_x, (int)g.party_y);
        }
        return;
    }
    g.party_x += dx / dist * step;
    g.party_y += dy / dist * step;
    g.move_progress += step / dist;
    advance_hours(hours_per_tile * step / (dist > 0 ? dist : 1));
}
