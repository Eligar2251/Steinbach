/* game_logic.c — центральные игровые операции (без графики). */
#include "game.h"
#include "rng.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

GameState g;

/* объявления из towns.c (не выносятся в game.h ради краткости) */
const Brother* town_recruit_brother(int town_idx, int rec_idx);
int  town_recruit_price(int town_idx, int rec_idx);
bool town_recruit_hire(int town_idx, int rec_idx, Brother* out);

bool game_new(int world_w, int world_h, uint32_t seed, int difficulty) {
    game_free();
    memset(&g, 0, sizeof(g));
    g.difficulty = clampi(difficulty, 0, 2);
    for (int i = 0; i < MAX_ACTIVE; i++) g.active_contracts[i] = -1;
    g.day = 1;
    g.hour = 8;
    g.rng = seed ? seed : 0xC0FFEEull;
    rng_seed(g.rng);
    snprintf(g.company_name, sizeof(g.company_name), "%s", gen_company_name());

    if (!world_generate(world_w, world_h, seed)) return false;

    econ_start();

    /* стартовый отряд: 3 бойца */
    g.n_brothers = 0;
    int start_bg[3] = { 4, 0, 3 }; /* наёмник, фермер, охотник */
    for (int i = 0; i < 3; i++) {
        Brother* b = &g.brothers[g.n_brothers++];
        brother_generate(b, start_bg[i], 1);
        b->mood = 75;
    }
    /* стартовая экипировка */
    for (int i = 0; i < g.n_brothers; i++) {
        Brother* b = &g.brothers[i];
        const Background* bg = &g_backgrounds[b->background];
        if (bg->weapon != ITEM_NONE) {
            int it = game_add_item(bg->weapon);
            if (it != ITEM_NONE) b->equip[EQ_WEAPON] = it;
        }
        if (bg->offhand != ITEM_NONE) {
            int it = game_add_item(bg->offhand);
            if (it != ITEM_NONE) b->equip[EQ_OFFHAND] = it;
        }
        if (bg->head != ITEM_NONE) {
            int it = game_add_item(bg->head);
            if (it != ITEM_NONE) b->equip[EQ_HEAD] = it;
        }
        if (bg->body != ITEM_NONE) {
            int it = game_add_item(bg->body);
            if (it != ITEM_NONE) b->equip[EQ_BODY] = it;
        }
    }
    /* немного припасов */
    for (int i = 0; i < 3; i++) game_add_item(23);  /* аптечка */
    for (int i = 0; i < 2; i++) game_add_item(24);  /* точильный камень */

    /* отряд стоит у первого города */
    g.party_x = (float)g.towns[0].x + 0.5f;
    g.party_y = (float)g.towns[0].y + 1.6f;
    g.towns[0].visited = 1;

    contracts_refresh();
    return true;
}

void game_free(void) {
    world_free();
}

int game_add_item(int def) {
    return item_create(def);
}

void game_remove_item(int idx) {
    item_remove(idx);
}

int game_find_town_at(int tx, int ty) {
    for (int i = 0; i < g.n_towns; i++)
        if (g.towns[i].x == tx && g.towns[i].y == ty) return i;
    return -1;
}

int game_find_location_at(int tx, int ty) {
    for (int i = 0; i < g.n_locations; i++)
        if (g.locations[i].x == tx && g.locations[i].y == ty) return i;
    return -1;
}

bool game_try_buy(int town_idx, int stock_idx) {
    if (town_idx < 0 || town_idx >= g.n_towns) return false;
    Town* t = &g.towns[town_idx];
    if (stock_idx < 0 || stock_idx >= t->n_stock) return false;
    Item it = t->stock[stock_idx];
    if (it.def == ITEM_NONE) return false;
    int price = buy_price(t, it.def);
    if (g.crowns < price) return false;
    int slot = game_add_item(it.def);
    if (slot == ITEM_NONE) return false;
    g.items[slot] = it;
    g.crowns -= price;
    /* убираем из наличия */
    for (int i = stock_idx; i < t->n_stock - 1; i++) t->stock[i] = t->stock[i + 1];
    t->n_stock--;
    return true;
}

bool game_try_sell(int town_idx, int inv_idx) {
    if (town_idx < 0 || town_idx >= g.n_towns) return false;
    Town* t = &g.towns[town_idx];
    if (inv_idx < 0 || inv_idx >= g.n_items) return false;
    /* нельзя продать надетое */
    for (int b = 0; b < g.n_brothers; b++)
        for (int e = 0; e < EQ_COUNT; e++)
            if (g.brothers[b].equip[e] == inv_idx) return false;
    if (t->n_stock >= MAX_STOCK) return false;
    Item it = g.items[inv_idx];
    if (it.def == ITEM_NONE) return false;
    int price = sell_price(t, it.def);
    t->stock[t->n_stock++] = it;
    item_remove(inv_idx);
    g.crowns += price;
    return true;
}

bool game_try_recruit(int town_idx, int rec_idx) {
    if (g.n_brothers >= MAX_BROTHERS) return false;
    if (town_idx < 0 || town_idx >= g.n_towns) return false;
    int price = town_recruit_price(town_idx, rec_idx);
    if (g.crowns < price) return false;
    Brother nb;
    if (!town_recruit_hire(town_idx, rec_idx, &nb)) return false;
    g.crowns -= price;
    Brother* b = &g.brothers[g.n_brothers++];
    *b = nb;
    b->mood = 65;
    b->days_hired = 0;
    /* стартовые вещи из фона */
    const Background* bg = &g_backgrounds[b->background];
    if (bg->weapon != ITEM_NONE) {
        int it = game_add_item(bg->weapon);
        if (it != ITEM_NONE) b->equip[EQ_WEAPON] = it;
    }
    if (bg->offhand != ITEM_NONE) {
        int it = game_add_item(bg->offhand);
        if (it != ITEM_NONE) b->equip[EQ_OFFHAND] = it;
    }
    if (bg->body != ITEM_NONE) {
        int it = game_add_item(bg->body);
        if (it != ITEM_NONE) b->equip[EQ_BODY] = it;
    }
    return true;
}

bool game_try_heal(int town_idx, int brother_idx) {
    (void)town_idx;
    if (brother_idx < 0 || brother_idx >= g.n_brothers) return false;
    Brother* b = &g.brothers[brother_idx];
    if (b->injury < 0 && b->hp >= brother_stat(b, STAT_HP)) return false;
    int cost = 120 + b->level * 60;
    if (g.crowns < cost) return false;
    g.crowns -= cost;
    b->injury = -1;
    b->injury_days = 0;
    b->hp = brother_stat(b, STAT_HP);
    b->fatigue = brother_stat(b, STAT_FATIGUE);
    b->mood = clampi(b->mood + 6, 0, 100);
    return true;
}

bool game_try_repair(int town_idx, int inv_idx) {
    (void)town_idx;
    if (inv_idx < 0 || inv_idx >= g.n_items) return false;
    Item* it = &g.items[inv_idx];
    if (it->durability >= 100) return false;
    int cost = (100 - it->durability) * g_item_defs[it->def].price / 400;
    if (cost < 5) cost = 5;
    if (g.crowns < cost) return false;
    g.crowns -= cost;
    it->durability = 100;
    return true;
}

void game_equip(int brother_idx, int inv_idx) {
    if (brother_idx < 0 || brother_idx >= g.n_brothers) return;
    if (inv_idx < 0 || inv_idx >= g.n_items) return;
    Brother* b = &g.brothers[brother_idx];
    const ItemDef* def = &g_item_defs[g.items[inv_idx].def];
    int slot;
    switch (def->slot) {
        case SLOT_HEAD: slot = EQ_HEAD; break;
        case SLOT_BODY: slot = EQ_BODY; break;
        case SLOT_WEAPON: slot = EQ_WEAPON; break;
        case SLOT_OFFHAND: slot = EQ_OFFHAND; break;
        default: return;
    }
    /* уже надето на кого-то? */
    for (int i = 0; i < g.n_brothers; i++)
        for (int e = 0; e < EQ_COUNT; e++)
            if (g.brothers[i].equip[e] == inv_idx) {
                g.brothers[i].equip[e] = ITEM_NONE;
            }
    int old = b->equip[slot];
    b->equip[slot] = inv_idx;
    (void)old;
    /* двуручное оружие снимает щит */
    if (slot == EQ_WEAPON && def->two_handed) b->equip[EQ_OFFHAND] = ITEM_NONE;
    if (slot == EQ_OFFHAND && def->wtype == WTYPE_SHIELD) {
        int w = b->equip[EQ_WEAPON];
        if (w != ITEM_NONE && g_item_defs[g.items[w].def].two_handed) b->equip[EQ_WEAPON] = ITEM_NONE;
    }
    brother_recalc_current(b);
}

void game_unequip(int brother_idx, int slot) {
    if (brother_idx < 0 || brother_idx >= g.n_brothers) return;
    if (slot < 0 || slot >= EQ_COUNT) return;
    g.brothers[brother_idx].equip[slot] = ITEM_NONE;
}

void game_daily_tick(void) {
    if (g.game_over) return;
    g.day++;
    econ_daily();
    towns_daily();
    contracts_check_daily();
    events_roll_daily();
    if (g.n_brothers <= 0) {
        g.game_over = true;
        g.battle.active = true;
        g.battle.won = false;
        snprintf(g.battle.title, sizeof(g.battle.title), "Компания распалась");
        snprintf(g.battle.lines[0], sizeof(g.battle.lines[0]),
                 "Последний наёмник покинул вас. Компании %s больше нет.", g.company_name);
        g.battle.n_lines = 1;
    }
}

void game_arrive_at(int x, int y) {
    int t = game_find_town_at(x, y);
    if (t >= 0) {
        g.towns[t].visited = 1;
    }
    int loc = game_find_location_at(x, y);
    if (loc >= 0 && !g.locations[loc].cleared) {
        Location* L = &g.locations[loc];
        char ename[64];
        static const char* ENEMY_NAMES[N_ENEMIES] = {
            "Бандиты", "Волки", "Скелеты", "Гоблины", "Орки", "Нежить",
        };
        int ei = rng_range(0, N_ENEMIES - 1);
        if (L->type == LOC_RUINS) ei = 2;
        else if (L->type == LOC_CAMP) ei = 0;
        else if (L->type == LOC_FARM) ei = 1;
        snprintf(ename, sizeof(ename), "%s", ENEMY_NAMES[ei]);
        battle_autofight(&g.battle, L->enemy_power, ename, true);
        if (g.battle.won) {
            L->cleared = 1;
            g.battle.gold += L->loot_gold;
            g.crowns += L->loot_gold;
            if (L->loot_item != ITEM_NONE) game_add_item(L->loot_item);
        }
    }
    contracts_check_arrival();
}
