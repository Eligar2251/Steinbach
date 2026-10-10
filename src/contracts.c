/* contracts.c — контракты: доставка, охота, патруль, эскорт. */
#include "game.h"
#include "rng.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static void gen_title_desc(Contract* c) {
    const char* from = g.towns[c->town_from].name;
    switch (c->type) {
        case CONTRACT_DELIVERY:
            snprintf(c->title, sizeof(c->title), "Доставка в %s", g.towns[c->town_to].name);
            snprintf(c->desc, sizeof(c->desc),
                     "Купцы из %s просят доставить груз в %s. Платят по прибытии.",
                     from, g.towns[c->town_to].name);
            break;
        case CONTRACT_HUNT: {
            Location* L = &g.locations[c->loc];
            snprintf(c->title, sizeof(c->title), "Зачистить: %s", L->name);
            snprintf(c->desc, sizeof(c->desc),
                     "В %s обосновались твари и бандиты. Горожане %s платят за их головы.",
                     L->name, from);
            break;
        }
        case CONTRACT_PATROL:
            snprintf(c->title, sizeof(c->title), "Патруль окрестностей");
            snprintf(c->desc, sizeof(c->desc),
                     "Нужно объехать %d точек и проверить их. Старт из %s.",
                     c->patrol_left, from);
            break;
        case CONTRACT_ESCORT:
            snprintf(c->title, sizeof(c->title), "Эскорт каравана в %s", g.towns[c->town_to].name);
            snprintf(c->desc, sizeof(c->desc),
                     "Караван из %s идёт в %s. Идти придётся медленно, но платят хорошо.",
                     from, g.towns[c->town_to].name);
            break;
        default: break;
    }
}

void contracts_refresh(void) {
    for (int i = 0; i < MAX_CONTRACTS; i++) {
        if (g.contracts[i].active) continue;
        Contract* c = &g.contracts[i];
        memset(c, 0, sizeof(*c));
        int town_from = rng_range(0, g.n_towns - 1);
        int type = rng_range(0, CONTRACT_TYPE_COUNT - 1);
        if (g.n_towns < 2 && type <= CONTRACT_ESCORT) type = CONTRACT_HUNT;

        c->active = true;
        c->type = type;
        c->town_from = town_from;
        c->town_to = -1;
        c->loc = -1;
        c->day_taken = g.day;
        c->days_limit = 0;
        c->reward_item = ITEM_NONE;

        switch (type) {
            case CONTRACT_DELIVERY:
            case CONTRACT_ESCORT: {
                int t2 = rng_range(0, g.n_towns - 1);
                int guard = 0;
                while ((t2 == town_from || g.towns[t2].type == TOWN_VILLAGE) && guard++ < 20)
                    t2 = rng_range(0, g.n_towns - 1);
                c->town_to = t2;
                int dx = g.towns[t2].x - g.towns[town_from].x;
                int dy = g.towns[t2].y - g.towns[town_from].y;
                int dist = (int)((dx * dx + dy * dy) > 0 ? 1 : 1);
                (void)dist;
                int reward = 120 + (abs(dx) + abs(dy)) * 6;
                if (type == CONTRACT_ESCORT) reward = reward * 16 / 10;
                c->reward_gold = reward + rng_range(0, reward / 3);
                c->reward_xp = 60 + reward / 6;
                break;
            }
            case CONTRACT_HUNT: {
                if (g.n_locations == 0) { c->active = false; continue; }
                c->loc = rng_range(0, g.n_locations - 1);
                c->reward_gold = 150 + g.locations[c->loc].enemy_power * 3 + rng_range(0, 120);
                c->reward_xp = 100 + g.locations[c->loc].enemy_power;
                if (rng_float() < 0.4f) c->reward_item = rng_range(11, 22);
                break;
            }
            case CONTRACT_PATROL: {
                c->patrol_left = rng_range(2, 3);
                for (int k = 0; k < 4; k++) {
                    c->patrol_target[k] = rng_range(0, g.n_locations - 1);
                }
                c->reward_gold = 100 + c->patrol_left * 90 + rng_range(0, 80);
                c->reward_xp = 70 + c->patrol_left * 40;
                break;
            }
        }
        c->days_limit = 0;
        gen_title_desc(c);
    }
}

Contract* contract_get(int id) {
    if (id < 0 || id >= MAX_CONTRACTS) return NULL;
    return &g.contracts[id];
}

bool contracts_accept(int contract_id) {
    Contract* c = contract_get(contract_id);
    if (!c || !c->active || c->days_limit < 0) return false;
    if (g.n_active >= MAX_ACTIVE) return false;
    /* один контракт — один слот */
    for (int i = 0; i < MAX_ACTIVE; i++) {
        if (g.active_contracts[i] == contract_id) return false;
    }
    for (int i = 0; i < MAX_ACTIVE; i++) {
        if (g.active_contracts[i] < 0) {
            g.active_contracts[i] = contract_id;
            g.n_active++;
            c->day_taken = g.day;
            if (c->type == CONTRACT_DELIVERY || c->type == CONTRACT_ESCORT) {
                travel_set_dest(g.towns[c->town_to].x, g.towns[c->town_to].y);
                if (c->type == CONTRACT_ESCORT) g.speed_mult = 0.72f;
            } else if (c->type == CONTRACT_HUNT) {
                travel_set_dest(g.locations[c->loc].x, g.locations[c->loc].y);
            }
            return true;
        }
    }
    return false;
}

bool contracts_abandon(int contract_id) {
    for (int i = 0; i < MAX_ACTIVE; i++) {
        if (g.active_contracts[i] == contract_id) {
            g.active_contracts[i] = -1;
            g.n_active--;
            Contract* c = contract_get(contract_id);
            if (c) c->active = false;
            g.speed_mult = 1.0f;
            contracts_refresh();
            return true;
        }
    }
    return false;
}

static void complete_contract(int slot, int contract_id) {
    Contract* c = contract_get(contract_id);
    if (!c) return;
    g.crowns += c->reward_gold;
    for (int i = 0; i < g.n_brothers; i++) {
        brother_add_xp(&g.brothers[i], c->reward_xp);
        g.brothers[i].mood = clampi(g.brothers[i].mood + 5, 0, 100);
    }
    if (c->reward_item != ITEM_NONE) game_add_item(c->reward_item);
    g.renown += 2 + c->type;

    g.battle.active = true;
    g.battle.won = true;
    snprintf(g.battle.title, sizeof(g.battle.title), "Контракт выполнен");
    snprintf(g.battle.lines[0], sizeof(g.battle.lines[0]),
             "«%s» — выполнено! Награда: %d крон.", c->title, c->reward_gold);
    snprintf(g.battle.lines[1], sizeof(g.battle.lines[1]),
             "Опыт: %d. Репутация компании растёт.", c->reward_xp);
    g.battle.n_lines = 2;
    g.battle.gold = c->reward_gold;
    g.battle.xp = c->reward_xp;

    c->active = false;
    g.active_contracts[slot] = -1;
    g.n_active--;
    g.speed_mult = 1.0f;
    contracts_refresh();
}

void contracts_check_arrival(void) {
    int px = (int)g.party_x, py = (int)g.party_y;
    for (int i = 0; i < MAX_ACTIVE; i++) {
        int id = g.active_contracts[i];
        if (id < 0) continue;
        Contract* c = contract_get(id);
        if (!c || !c->active) continue;
        switch (c->type) {
            case CONTRACT_DELIVERY:
            case CONTRACT_ESCORT:
                if (px == g.towns[c->town_to].x && py == g.towns[c->town_to].y)
                    complete_contract(i, id);
                break;
            case CONTRACT_HUNT:
                if (c->loc >= 0 && g.locations[c->loc].cleared)
                    complete_contract(i, id);
                break;
            case CONTRACT_PATROL: {
                int li = game_find_location_at(px, py);
                if (li >= 0) {
                    bool is_target = false;
                    for (int k = 0; k < 4; k++)
                        if (c->patrol_target[k] == li) is_target = true;
                    if (is_target && c->patrol_left > 0) {
                        c->patrol_left--;
                        if (c->patrol_left <= 0) complete_contract(i, id);
                        else {
                            g.battle.active = true;
                            g.battle.won = true;
                            snprintf(g.battle.title, sizeof(g.battle.title), "Патруль");
                            snprintf(g.battle.lines[0], sizeof(g.battle.lines[0]),
                                     "Точка проверена. Осталось: %d.", c->patrol_left);
                            g.battle.n_lines = 1;
                        }
                    }
                }
                break;
            }
        }
    }
}

void contracts_check_daily(void) {
    contracts_refresh();
}
