/* test_core.c — автотесты игровой логики (компилируются без raylib). */
#include "../src/game.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int failures = 0;
static int checks = 0;

#define CHECK(cond, msg) do { \
    checks++; \
    if (!(cond)) { failures++; printf("FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); } \
    else printf("ok: %s\n", msg); \
} while (0)

int main(void) {
    printf("=== Steinbach core tests ===\n");

    /* 1. Генерация мира */
    CHECK(game_new(120, 80, 12345, 1), "game_new создаёт мир");
    CHECK(g.n_towns >= 6, "городов >= 6");
    CHECK(g.n_locations >= 10, "локаций >= 10");
    CHECK(g.n_brothers == 3, "стартовый отряд из 3 бойцов");
    CHECK(g.crowns == 2400, "стартовый капитал");
    CHECK(g.terrain != NULL, "карта сгенерирована");

    int land = 0, water = 0;
    for (int i = 0; i < g.world_w * g.world_h; i++) {
        if (g_terrain_cost[g.terrain[i]] > 0) land++;
        else water++;
    }
    CHECK(land > 2000 && water > 2000, "есть и суша, и вода");

    /* все города на суше и не в горах */
    int ok = 1;
    for (int i = 0; i < g.n_towns; i++) {
        int t = terrain_at(g.towns[i].x, g.towns[i].y);
        if (g_terrain_cost[t] == 0 || t == TERR_MOUNTAIN) ok = 0;
    }
    CHECK(ok, "города стоят на проходимой суше");

    /* 2. Путь */
    TilePos path[MAX_PATH];
    int plen = 0;
    int t0 = 0, t1 = g.n_towns - 1;
    CHECK(find_path(g.towns[t0].x, g.towns[t0].y, g.towns[t1].x, g.towns[t1].y, path, &plen),
          "A* строит путь между городами");
    CHECK(plen >= 2, "путь содержит точки");
    ok = 1;
    for (int i = 0; i < plen; i++) {
        if (tile_move_cost(path[i].x, path[i].y) == 0) ok = 0;
    }
    CHECK(ok, "весь путь проходим");

    /* 3. Бойцы */
    Brother b;
    brother_generate(&b, 4, 1);
    CHECK(b.base.hp > 30 && b.base.hp < 120, "здоровье в разумных пределах");
    CHECK(b.base.matk >= 30, "мастерство боя >= 30");
    CHECK(b.n_traits >= 1 && b.n_traits <= 2, "1-2 черты у бойца");
    int p0 = brother_power(&b);
    CHECK(p0 > 20, "сила бойца положительна");

    int xp0 = b.level;
    for (int i = 0; i < 40; i++) brother_add_xp(&b, 200);
    CHECK(b.level > xp0, "опыт даёт уровни");
    CHECK(b.stat_picks > 0, "есть очки характеристик");
    int picks[3] = { STAT_MATK, STAT_MDEF, STAT_HP };
    int lvl = b.level, sp = b.stat_picks;
    brother_levelup_apply(&b, picks, 0);
    CHECK(b.level == lvl && b.stat_picks == sp - 3, "прокачка расходует очки");
    CHECK((b.perks & 1u) != 0, "перк применён");

    /* 4. Предметы */
    int it = game_add_item(1);  /* меч */
    CHECK(it != ITEM_NONE, "предмет создаётся");
    g.brothers[0].equip[EQ_WEAPON] = it;
    CHECK(item_weapon_damage_max(&g.brothers[0]) == 20, "меч даёт урон 12-20");
    int sh = game_add_item(11);
    g.brothers[0].equip[EQ_OFFHAND] = sh;
    CHECK(item_total_armor(&g.brothers[0]) >= 8, "щит даёт броню");

    /* 5. Экономика */
    int crowns0 = g.crowns;
    int food0 = g.food;
    g.day = 5;
    game_daily_tick();
    CHECK(g.day == 6, "день увеличивается");
    CHECK(g.crowns < crowns0, "жалование списано");
    CHECK(g.food < food0 || food0 == 0, "провиант расходуется");

    /* 6. Автобой */
    BattleReport br;
    battle_autofight(&br, 100, "Бандиты", true);
    CHECK(br.active, "отчёт о бое создан");
    CHECK(br.n_lines > 0, "в отчёте есть строки");

    /* 7. Контракты */
    contracts_refresh();
    int found = 0;
    for (int i = 0; i < MAX_CONTRACTS; i++) if (g.contracts[i].active) found++;
    CHECK(found >= 3, "контракты сгенерированы");
    int cid = -1;
    for (int i = 0; i < MAX_CONTRACTS; i++) if (g.contracts[i].active) { cid = i; break; }
    if (cid >= 0) {
        CHECK(contracts_accept(cid), "контракт принимается");
        CHECK(g.n_active == 1, "контракт в активных");
    }

    /* 8. Сохранение/загрузка */
    char name_copy[NAME_LEN];
    snprintf(name_copy, sizeof(name_copy), "%s", g.company_name);
    int crowns = g.crowns, day = g.day, nb = g.n_brothers;
    CHECK(save_game("/tmp/steinbach_test.sav"), "сохранение пишется");
    /* меняем состояние */
    g.crowns = 1;
    g.n_brothers = 0;
    CHECK(load_game("/tmp/steinbach_test.sav"), "загрузка читает файл");
    CHECK(g.crowns == crowns, "кроны восстановлены");
    CHECK(g.day == day, "день восстановлен");
    CHECK(g.n_brothers == nb, "отряд восстановлен");
    CHECK(strcmp(g.company_name, name_copy) == 0, "имя компании восстановлено");
    CHECK(g.n_towns >= 6, "города восстановлены");

    /* 9. Детерминированность */
    GameState tmp;
    (void)tmp;
    game_new(120, 80, 777, 1);
    int n1 = g.n_towns, l1 = g.n_locations;
    uint8_t tv = g.terrain[50 * 120 + 50];
    game_new(120, 80, 777, 1);
    CHECK(g.n_towns == n1 && g.n_locations == l1 && g.terrain[50 * 120 + 50] == tv,
          "мир детерминирован по сиду");

    printf("=== %d checks, %d failures ===\n", checks, failures);
    return failures ? 1 : 0;
}
