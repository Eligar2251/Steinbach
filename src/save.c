/* save.c — сохранение/загрузка в текстовом формате. */
#include "game.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void w_stats(FILE* f, const Stats* s) {
    fprintf(f, " %d %d %d %d %d %d %d %d", s->hp, s->fatigue, s->resolve, s->initiative,
            s->matk, s->ratk, s->mdef, s->rdef);
}

static bool r_stats(FILE* f, Stats* s) {
    return fscanf(f, " %d %d %d %d %d %d %d %d", &s->hp, &s->fatigue, &s->resolve, &s->initiative,
                  &s->matk, &s->ratk, &s->mdef, &s->rdef) == 8;
}

/* Пропускает пробелы и ожидает точную строку-тег. */
static bool expect_tag(FILE* f, const char* tag) {
    int c;
    do {
        c = fgetc(f);
        if (c == EOF) return false;
    } while (c == ' ' || c == '\n' || c == '\r' || c == '\t');
    while (*tag) {
        if (c != (unsigned char)*tag) return false;
        c = fgetc(f);
        if (c == EOF && tag[1]) return false;
        tag++;
    }
    if (c != EOF) ungetc(c, f);
    return true;
}

static bool read_line_field(FILE* f, const char* tag, char* out, int n) {
    if (!expect_tag(f, tag)) return false;
    int c = fgetc(f);
    if (c == ' ') c = fgetc(f);
    int i = 0;
    while (c != EOF && c != '\n' && i < n - 1) {
        out[i++] = (char)c;
        c = fgetc(f);
    }
    out[i] = 0;
    return true;
}

bool save_game(const char* path) {
    FILE* f = fopen(path, "w");
    if (!f) return false;
    fprintf(f, "STEINBACH_SAVE %d\n", SAVE_VERSION);
    fprintf(f, "META %u %d %d %d %d %d %llu %d %d %d\n",
            g.seed, g.day, g.hour, g.crowns, g.food, g.renown,
            (unsigned long long)g.rng, g.difficulty, g.battles_won, g.battles_lost);
    fprintf(f, "COMPANY %s\n", g.company_name);
    fprintf(f, "PARTY %.2f %.2f %d %d %d %d\n",
            g.party_x, g.party_y, g.path_len, g.path_pos, g.traveling ? 1 : 0, g.n_active);
    fprintf(f, "WORLD %d %d\n", g.world_w, g.world_h);
    fprintf(f, "TERRAIN ");
    for (int i = 0; i < g.world_w * g.world_h; ) {
        uint8_t v = g.terrain[i];
        int run = 1;
        while (i + run < g.world_w * g.world_h && g.terrain[i + run] == v && run < 255) run++;
        fprintf(f, "%d.%d,", v, run);
        i += run;
    }
    fprintf(f, "\nROAD ");
    for (int i = 0; i < g.world_w * g.world_h; ) {
        uint8_t v = g.road[i];
        int run = 1;
        while (i + run < g.world_w * g.world_h && g.road[i + run] == v && run < 255) run++;
        fprintf(f, "%d.%d,", v, run);
        i += run;
    }
    fprintf(f, "\n");

    fprintf(f, "TOWNS %d\n", g.n_towns);
    for (int i = 0; i < g.n_towns; i++) {
        const Town* t = &g.towns[i];
        fprintf(f, "T %d %d %d %d %d %d %d %d %d %d %d\n",
                t->id, t->x, t->y, t->type, t->population, t->prosperity,
                t->buildings, t->n_stock, t->recruit_day, t->contract_day, t->visited);
        fprintf(f, "N %s\n", t->name);
        fprintf(f, "S");
        for (int k = 0; k < t->n_stock; k++)
            fprintf(f, " %d %d %d", t->stock[k].def, t->stock[k].durability, t->stock[k].ammo);
        fprintf(f, "\n");
    }

    fprintf(f, "LOCS %d\n", g.n_locations);
    for (int i = 0; i < g.n_locations; i++) {
        const Location* L = &g.locations[i];
        fprintf(f, "L %d %d %d %d %d %d %d %d\n", L->id, L->x, L->y, L->type,
                L->enemy_power, L->cleared, L->loot_gold, L->loot_item);
        fprintf(f, "N %s\n", L->name);
    }

    fprintf(f, "ITEMS %d\n", g.n_items);
    for (int i = 0; i < g.n_items; i++)
        fprintf(f, "I %d %d %d\n", g.items[i].def, g.items[i].durability, g.items[i].ammo);

    fprintf(f, "BROTHERS %d\n", g.n_brothers);
    for (int i = 0; i < g.n_brothers; i++) {
        const Brother* b = &g.brothers[i];
        fprintf(f, "B %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %u\n",
                b->background, b->level, b->xp, b->stat_picks, b->perk_points,
                b->hp, b->fatigue, b->mood, b->injury, b->injury_days,
                b->portrait, b->wage, b->days_hired, b->kills, b->battles,
                b->n_traits, b->perks);
        fprintf(f, "N %s\n", b->name);
        fprintf(f, "S");
        w_stats(f, &b->base);
        fprintf(f, "\nE");
        for (int e = 0; e < EQ_COUNT; e++) fprintf(f, " %d", b->equip[e]);
        fprintf(f, "\nT");
        for (int t = 0; t < b->n_traits; t++) fprintf(f, " %d", b->traits[t]);
        fprintf(f, "\n");
    }

    fprintf(f, "CONTRACTS\n");
    for (int i = 0; i < MAX_CONTRACTS; i++) {
        const Contract* c = &g.contracts[i];
        fprintf(f, "C %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
                c->active ? 1 : 0, c->type, c->town_from, c->town_to, c->loc,
                c->patrol_left, c->reward_gold, c->reward_xp, c->reward_item,
                c->day_taken, c->days_limit,
                c->patrol_target[0], c->patrol_target[1], c->patrol_target[2], c->patrol_target[3]);
        fprintf(f, "N %s\n", c->title);
        fprintf(f, "D %s\n", c->desc);
    }
    fprintf(f, "ACTIVE");
    for (int i = 0; i < MAX_ACTIVE; i++) fprintf(f, " %d", g.active_contracts[i]);
    fprintf(f, "\nEND\n");
    fclose(f);
    return true;
}

static bool read_rle(FILE* f, uint8_t* out, int total) {
    int i = 0;
    int v, run;
    char ch = ',';
    while (i < total && ch == ',') {
        if (fscanf(f, "%d.%d%c", &v, &run, &ch) != 3) return false;
        for (int k = 0; k < run && i < total; k++) out[i++] = (uint8_t)v;
    }
    return i == total;
}

bool load_game(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    int ver = 0;
    if (fscanf(f, "STEINBACH_SAVE %d", &ver) != 1 || ver != SAVE_VERSION) {
        fclose(f);
        return false;
    }
    game_free();
    memset(&g, 0, sizeof(g));
    for (int i = 0; i < MAX_ACTIVE; i++) g.active_contracts[i] = -1;

    unsigned int seed;
    unsigned long long rngv;
    int traveling, n_active;
    if (fscanf(f, " META %u %d %d %d %d %d %llu %d %d %d",
               &seed, &g.day, &g.hour, &g.crowns, &g.food, &g.renown,
               &rngv, &g.difficulty, &g.battles_won, &g.battles_lost) != 10) { fclose(f); return false; }
    g.seed = seed;
    g.rng = rngv;

    if (!read_line_field(f, "COMPANY", g.company_name, NAME_LEN)) { fclose(f); return false; }

    if (fscanf(f, " PARTY %f %f %d %d %d %d",
               &g.party_x, &g.party_y, &g.path_len, &g.path_pos, &traveling, &n_active) != 6) {
        fclose(f); return false;
    }
    g.traveling = traveling != 0;
    g.n_active = n_active;

    if (fscanf(f, " WORLD %d %d", &g.world_w, &g.world_h) != 2) { fclose(f); return false; }
    g.terrain = calloc((size_t)g.world_w * g.world_h, 1);
    g.road = calloc((size_t)g.world_w * g.world_h, 1);
    g.decor = calloc((size_t)g.world_w * g.world_h, 1);
    if (!g.terrain || !g.road || !g.decor) { fclose(f); return false; }
    if (!expect_tag(f, "TERRAIN")) { fclose(f); return false; }
    if (!read_rle(f, g.terrain, g.world_w * g.world_h)) { fclose(f); return false; }
    if (!expect_tag(f, "ROAD")) { fclose(f); return false; }
    if (!read_rle(f, g.road, g.world_w * g.world_h)) { fclose(f); return false; }

    int n;
    if (fscanf(f, " TOWNS %d", &n) != 1) { fclose(f); return false; }
    g.n_towns = n;
    for (int i = 0; i < n; i++) {
        Town* t = &g.towns[i];
        if (fscanf(f, " T %d %d %d %d %d %d %d %d %d %d %d",
                   &t->id, &t->x, &t->y, &t->type, &t->population, &t->prosperity,
                   &t->buildings, &t->n_stock, &t->recruit_day, &t->contract_day, &t->visited) != 11) {
            fclose(f); return false;
        }
        if (!read_line_field(f, "N", t->name, NAME_LEN)) { fclose(f); return false; }
        if (!expect_tag(f, "S")) { fclose(f); return false; }
        for (int k = 0; k < t->n_stock; k++) {
            if (fscanf(f, " %d %d %d", &t->stock[k].def, &t->stock[k].durability, &t->stock[k].ammo) != 3) {
                fclose(f); return false;
            }
        }
    }

    if (fscanf(f, " LOCS %d", &n) != 1) { fclose(f); return false; }
    g.n_locations = n;
    for (int i = 0; i < n; i++) {
        Location* L = &g.locations[i];
        if (fscanf(f, " L %d %d %d %d %d %d %d %d", &L->id, &L->x, &L->y, &L->type,
                   &L->enemy_power, &L->cleared, &L->loot_gold, &L->loot_item) != 8) {
            fclose(f); return false;
        }
        if (!read_line_field(f, "N", L->name, NAME_LEN)) { fclose(f); return false; }
    }

    if (fscanf(f, " ITEMS %d", &n) != 1) { fclose(f); return false; }
    g.n_items = n;
    for (int i = 0; i < n; i++) {
        if (fscanf(f, " I %d %d %d", &g.items[i].def, &g.items[i].durability, &g.items[i].ammo) != 3) {
            fclose(f); return false;
        }
    }

    if (fscanf(f, " BROTHERS %d", &n) != 1) { fclose(f); return false; }
    g.n_brothers = n;
    for (int i = 0; i < n; i++) {
        Brother* b = &g.brothers[i];
        brother_clear(b);
        unsigned int perks;
        if (fscanf(f, " B %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %u",
                   &b->background, &b->level, &b->xp, &b->stat_picks, &b->perk_points,
                   &b->hp, &b->fatigue, &b->mood, &b->injury, &b->injury_days,
                   &b->portrait, &b->wage, &b->days_hired, &b->kills, &b->battles,
                   &b->n_traits, &perks) != 17) { fclose(f); return false; }
        b->perks = perks;
        if (!read_line_field(f, "N", b->name, NAME_LEN)) { fclose(f); return false; }
        if (!expect_tag(f, "S")) { fclose(f); return false; }
        if (!r_stats(f, &b->base)) { fclose(f); return false; }
        if (fscanf(f, " E %d %d %d %d", &b->equip[0], &b->equip[1], &b->equip[2], &b->equip[3]) != 4) {
            fclose(f); return false;
        }
        if (!expect_tag(f, "T")) { fclose(f); return false; }
        for (int t = 0; t < b->n_traits; t++) {
            if (fscanf(f, " %d", &b->traits[t]) != 1) { fclose(f); return false; }
        }
    }

    if (!expect_tag(f, "CONTRACTS")) { fclose(f); return false; }
    for (int i = 0; i < MAX_CONTRACTS; i++) {
        Contract* c = &g.contracts[i];
        int act;
        if (fscanf(f, " C %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                   &act, &c->type, &c->town_from, &c->town_to, &c->loc,
                   &c->patrol_left, &c->reward_gold, &c->reward_xp, &c->reward_item,
                   &c->day_taken, &c->days_limit,
                   &c->patrol_target[0], &c->patrol_target[1], &c->patrol_target[2], &c->patrol_target[3]) != 15) {
            fclose(f); return false;
        }
        c->active = act != 0;
        if (!read_line_field(f, "N", c->title, TEXT_LEN)) { fclose(f); return false; }
        if (!read_line_field(f, "D", c->desc, TEXT_LEN)) { fclose(f); return false; }
    }
    if (!expect_tag(f, "ACTIVE")) { fclose(f); return false; }
    for (int i = 0; i < MAX_ACTIVE; i++) {
        if (fscanf(f, " %d", &g.active_contracts[i]) != 1) g.active_contracts[i] = -1;
    }
    fclose(f);
    return true;
}
