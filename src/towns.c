/* towns.c — города: постройки, рынок, наём, обновление по дням. */
#include "game.h"
#include "rng.h"
#include <string.h>
#include <stdio.h>

const char* g_contract_type_name[CONTRACT_TYPE_COUNT] = {
    "Доставка", "Охота", "Патруль", "Эскорт",
};

/* Встроенный справочник найма — временная таблица заявок. */
typedef struct {
    Brother b;
    int     price;
    bool    used;
} RecruitOffer;

static RecruitOffer s_recruit_pool[MAX_TOWNS * MAX_RECRUITS];

void town_generate(Town* t, int id, int x, int y, int type) {
    memset(t, 0, sizeof(*t));
    t->id = id;
    t->x = x;
    t->y = y;
    t->type = type;
    snprintf(t->name, sizeof(t->name), "%s", gen_town_name());
    t->population = 0;
    switch (type) {
        case TOWN_VILLAGE: t->population = rng_range(120, 600);  t->prosperity = rng_range(35, 75); break;
        case TOWN_TOWN:    t->population = rng_range(900, 3200); t->prosperity = rng_range(55, 105); break;
        case TOWN_CITY:    t->population = rng_range(5000, 14000); t->prosperity = rng_range(85, 145); break;
        case TOWN_CASTLE:  t->population = rng_range(300, 900);  t->prosperity = rng_range(50, 95); break;
        default: break;
    }
    switch (type) {
        case TOWN_VILLAGE: t->buildings = BLD_TAVERN | BLD_MARKET; break;
        case TOWN_TOWN:    t->buildings = BLD_TAVERN | BLD_MARKET | BLD_TEMPLE | BLD_GUILD; break;
        case TOWN_CITY:    t->buildings = BLD_TAVERN | BLD_MARKET | BLD_TEMPLE | BLD_GUILD | BLD_FORGE; break;
        case TOWN_CASTLE:  t->buildings = BLD_MARKET | BLD_TEMPLE | BLD_GUILD; break;
        default: break;
    }
    t->recruit_day = -1;
    t->contract_day = -1;
    towns_refresh_market(id);
    towns_refresh_recruits(id);
}

int town_price_mult(const Town* t) {
    /* чем богаче город, тем дороже */
    return 70 + t->prosperity * 3 / 5;
}

int buy_price(const Town* t, int item_def) {
    if (item_def < 0 || item_def >= g_item_def_count) return 0;
    int p = g_item_defs[item_def].price * town_price_mult(t) / 100;
    /* скидка перка «Торговец» у первого бойца-носителя? считаем по всему отряду: если кто-то с перком */
    for (int i = 0; i < g.n_brothers; i++)
        if (g.brothers[i].perks & (1u << 9)) { p = p * 90 / 100; break; }
    return clampi(p, 5, 50000);
}

int sell_price(const Town* t, int item_def) {
    int p = buy_price(t, item_def) * 55 / 100;
    for (int i = 0; i < g.n_brothers; i++)
        if (g.brothers[i].perks & (1u << 9)) { p = p * 110 / 100; break; }
    return clampi(p, 2, 50000);
}

static int roll_item_for_town(const Town* t) {
    /* вес по слотам и богатству */
    int r = rng_range(0, 99);
    int cap_price = 150 + t->prosperity * 14;
    for (int tries = 0; tries < 40; tries++) {
        int d = rng_range(0, g_item_def_count - 1);
        const ItemDef* def = &g_item_defs[d];
        if (def->slot == SLOT_SUPPLY && r < 22) return d;
        if (def->price > cap_price) continue;
        if (def->slot == SLOT_WEAPON && r < 55) return d;
        if ((def->slot == SLOT_HEAD || def->slot == SLOT_BODY) && r < 82) return d;
        if (def->slot == SLOT_OFFHAND && r < 94) return d;
    }
    return rng_range(0, 13); /* запасной вариант: оружие/щиты */
}

void towns_refresh_market(int town_idx) {
    if (town_idx < 0 || town_idx >= g.n_towns) return;
    Town* t = &g.towns[town_idx];
    t->n_stock = 0;
    int n = 4 + t->type * 2 + rng_range(0, 3);
    if (n > MAX_STOCK) n = MAX_STOCK;
    for (int i = 0; i < n; i++) {
        int def = roll_item_for_town(t);
        if (def < 0) continue;
        Item it;
        it.def = def;
        it.durability = 100;
        it.ammo = (g_item_defs[def].wtype == WTYPE_BOW) ? 20 :
                  (g_item_defs[def].wtype == WTYPE_CROSSBOW) ? 12 : 0;
        t->stock[t->n_stock++] = it;
    }
    /* провиант всегда есть */
    for (int i = 0; i < 3 && t->n_stock < MAX_STOCK; i++) {
        Item it = { 26, 100, 0 }; /* мешок провианта */
        t->stock[t->n_stock++] = it;
    }
    t->recruit_day = g.day;
}

void towns_refresh_recruits(int town_idx) {
    if (town_idx < 0 || town_idx >= g.n_towns) return;
    Town* t = &g.towns[town_idx];
    int pool_base = town_idx * MAX_RECRUITS;
    t->n_recruits = 0;
    int n = 2 + t->type + rng_range(0, 2);
    if (n > MAX_RECRUITS) n = MAX_RECRUITS;
    for (int i = 0; i < n; i++) {
        RecruitOffer* o = &s_recruit_pool[pool_base + i];
        /* фонды: в деревне простые, в городе/замке — боевые */
        int bg;
        int r = rng_range(0, 99);
        if (t->type >= TOWN_CITY && r < 35)      bg = rng_range(4, 6);   /* наёмник/дезертир/бандит */
        else if (t->type == TOWN_TOWN && r < 25) bg = rng_range(4, 7);
        else if (r < 20)                         bg = rng_range(8, 15);  /* дворянин/ветеран и пр. */
        else                                     bg = rng_range(0, 7);
        int level_hint = 1;
        if (t->type >= TOWN_CITY && rng_float() < 0.35f) level_hint = rng_range(2, 3);
        else if (rng_float() < 0.15f) level_hint = 2;
        brother_generate(&o->b, bg, level_hint);
        /* стартовая экипировка (только для найма — кладём в снаряжение при найме) */
        o->price = brother_price(&o->b);
        o->used = false;
        t->recruits[t->n_recruits++] = pool_base + i;
    }
    t->contract_day = g.day;
}

const Brother* town_recruit_brother(int town_idx, int rec_idx);
const Brother* town_recruit_brother(int town_idx, int rec_idx) {
    if (town_idx < 0 || town_idx >= g.n_towns) return NULL;
    Town* t = &g.towns[town_idx];
    if (rec_idx < 0 || rec_idx >= t->n_recruits) return NULL;
    RecruitOffer* o = &s_recruit_pool[t->recruits[rec_idx]];
    return o->used ? NULL : &o->b;
}

int town_recruit_price(int town_idx, int rec_idx);
int town_recruit_price(int town_idx, int rec_idx) {
    if (town_idx < 0 || town_idx >= g.n_towns) return 0;
    Town* t = &g.towns[town_idx];
    if (rec_idx < 0 || rec_idx >= t->n_recruits) return 0;
    return s_recruit_pool[t->recruits[rec_idx]].price;
}

bool town_recruit_hire(int town_idx, int rec_idx, Brother* out);
bool town_recruit_hire(int town_idx, int rec_idx, Brother* out) {
    if (town_idx < 0 || town_idx >= g.n_towns) return false;
    Town* t = &g.towns[town_idx];
    if (rec_idx < 0 || rec_idx >= t->n_recruits) return false;
    RecruitOffer* o = &s_recruit_pool[t->recruits[rec_idx]];
    if (o->used) return false;
    *out = o->b;
    o->used = true;
    return true;
}

void towns_daily(void) {
    for (int i = 0; i < g.n_towns; i++) {
        if (g.day - g.towns[i].recruit_day >= 3) {
            towns_refresh_recruits(i);
            towns_refresh_market(i);
        }
    }
}
