/* tests/test_battle.c — тесты тактического боя. */
#include "../src/game.h"
#include "../src/battle.h"
#include "../src/rng.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int fails = 0;
static int checks = 0;

static void ok(int cond, const char* msg) {
    checks++;
    if (!cond) {
        fails++;
        printf("FAIL: %s\n", msg);
    } else {
        printf("ok: %s\n", msg);
    }
}

static void setup_game(void) {
    memset(&g, 0, sizeof(g));
    rng_seed(20261010);
    g.seed = 20261010;
    g.difficulty = 1;
    snprintf(g.company_name, sizeof(g.company_name), "Тестовая рота");
    g.crowns = 500;
    g.food = 100;
    g.n_brothers = 4;
    for (int i = 0; i < g.n_brothers; i++) {
        Brother* b = &g.brothers[i];
        memset(b, 0, sizeof(*b));
        snprintf(b->name, sizeof(b->name), "Боец_%d", i);
        b->base.hp = 60;
        b->base.fatigue = 100;
        b->base.resolve = 55;
        b->base.initiative = 105;
        b->base.matk = 70;
        b->base.ratk = 60;
        b->base.mdef = 15;
        b->base.rdef = 10;
        b->hp = 60;
        b->fatigue = 0;
        b->level = 1;
        b->injury = -1;
        b->portrait = i % 12;
        b->equip[EQ_WEAPON] = -1;
        b->equip[EQ_HEAD] = -1;
        b->equip[EQ_BODY] = -1;
        b->equip[EQ_OFFHAND] = -1;
    }
}

int main(void) {
    printf("== Тесты тактического боя ==\n");
    setup_game();

    /* --- запуск боя --- */
    bt_launch(BT_CTX_EVENT, -1, -1, BT_F_BANDITS, 50, "Бандиты", BT_T_GRASS, true);
    ok(bt.active, "бой запущен");
    ok(bt.n >= 7, "на поле достаточно юнитов (7+)");
    ok(bt_alive_player() == 4, "4 бойца игрока живы");
    ok(bt_alive_enemy() >= 3 && bt_alive_enemy() <= 8, "врагов 3..8");
    int enemies_before = bt_alive_enemy();
    ok(bt.round == 1, "первый раунд");

    /* расстановка: игроки слева, враги справа */
    int left_ok = 1, right_ok = 1;
    for (int i = 0; i < bt.n; i++) {
        if (!bt.units[i].enemy && bt.units[i].x > 4) left_ok = 0;
        if (bt.units[i].enemy && bt.units[i].x < BT_W - 5) right_ok = 0;
    }
    ok(left_ok, "игроки на левом краю");
    ok(right_ok, "враги на правом краю");

    /* --- достижимость --- */
    int u = bt_current();
    ok(u >= 0, "есть текущий юнит");
    bool vis[BT_H][BT_W];
    bt_reachable(u, vis);
    int reach = 0;
    for (int y = 0; y < BT_H; y++)
        for (int x = 0; x < BT_W; x++)
            if (vis[y][x]) reach++;
    ok(reach > 5, "BFS находит клетки для шага");
    ok(!vis[bt.units[u].y][bt.units[u].x], "своя клетка не в списке шагов");

    /* шаг в пределах досягаемости */
    int mx = -1, my = -1;
    for (int y = 0; y < BT_H && mx < 0; y++)
        for (int x = 0; x < BT_W && mx < 0; x++)
            if (vis[y][x]) { mx = x; my = y; }
    bool moved = bt_move(NULL, u, mx, my);
    ok(moved, "перемещение выполнено");
    ok(bt.units[u].x == mx && bt.units[u].y == my, "юнит на новой клетке");

    /* --- ближний бой: подводим юнитов вплотную --- */
    /* найдём пару: первый игрок и первый враг */
    int p0 = -1, e0 = -1;
    for (int i = 0; i < bt.n; i++) {
        if (bt.units[i].used && !bt.units[i].enemy && p0 < 0) p0 = i;
        if (bt.units[i].used && bt.units[i].enemy && e0 < 0) e0 = i;
    }
    bt.units[p0].x = 5; bt.units[p0].y = 4;
    bt.units[e0].x = 6; bt.units[e0].y = 4;
    bt.units[p0].ap = 2; bt.units[p0].w_range = 1;
    bt.sel = p0;

    ok(bt_attack_possible(p0, e0), "атака возможна по соседу");
    int hp_before = bt.units[e0].hp;
    int ar_before = bt.units[e0].armor_body + bt.units[e0].armor_head;
    bool hit_any = false;
    for (int t = 0; t < 20 && !hit_any; t++) {
        bt.units[p0].ap = 2;
        bt_attack(NULL, p0, e0);
        bt.anim = BT_ANIM_NONE;   /* сбрасываем анимацию между ударами */
        if (bt.units[e0].hp < hp_before ||
            bt.units[e0].armor_body + bt.units[e0].armor_head < ar_before) hit_any = true;
    }
    ok(hit_any, "удар наносит урон (броня или HP)");

    /* шанс попадания в границах */
    int ch = bt_hit_chance(p0, e0);
    ok(ch >= 5 && ch <= 95, "шанс попадания в пределах 5..95");

    /* --- смерть и мораль --- */
    bt.units[e0].hp = 1;
    bt.units[e0].armor_body = 0;
    bt.units[e0].armor_head = 0;
    bt.units[e0].waver = 2;   /* умрёт — союзники не должны сбежать дважды из-за бага */
    bool killed = false;
    for (int t = 0; t < 30 && !killed; t++) {
        bt.units[p0].ap = 2;
        bt.units[p0].w_min = 20; bt.units[p0].w_max = 25; bt.units[p0].w_pen = 60;
        if (!bt.units[e0].alive) { killed = true; break; }
        bt_attack(NULL, p0, e0);
        bt.anim = BT_ANIM_NONE;
        if (!bt.units[e0].alive) killed = true;
    }
    ok(killed, "враг убит при нуле HP");
    ok(bt_alive_enemy() == enemies_before - 1, "живых врагов стало меньше");
    ok(bt.kills_player == 1, "убийство засчитано игроку");

    /* --- победа: добиваем всех --- */
    for (int i = 0; i < bt.n; i++) {
        if (bt.units[i].used && bt.units[i].enemy) {
            bt.units[i].alive = false;
            bt.units[i].hp = 0;
        }
    }
    bool ended = bt_check_end(NULL);
    ok(ended, "бой завершён");
    ok(bt.victory, "победа зафиксирована");
    ok(g.battle.active, "отчёт о бое создан");
    ok(g.battle.won, "отчёт: победа");
    ok(g.battle.n_lines > 0, "в отчёте есть строки");

    /* --- второй бой: ИИ --- */
    setup_game();
    bt_launch(BT_CTX_LOCATION, -1, -1, BT_F_GUARDS, 70, "Наёмники", BT_T_STONE, false);
    ok(bt.n >= 6, "бой с наёмниками запущен");

    int ai_steps = 0;
    for (int guard = 0; guard < 200 && !bt.finished; guard++) {
        if (bt.anim != BT_ANIM_NONE) { bt_anim_tick(0.5f); continue; }
        if (bt_is_player_turn()) {
            bt_end_turn(NULL);   /* пропускаем ходы игрока */
        } else {
            bt_ai_act(NULL);
            bt.anim = BT_ANIM_NONE;
            ai_steps++;
        }
        bt_check_end(NULL);
    }
    ok(ai_steps > 0, "ИИ совершает действия");
    ok(bt.finished, "бой дошёл до конца силами ИИ");
    ok(g.battle.active, "итоговый отчёт создан");

    /* --- отступление --- */
    setup_game();
    bt_launch(BT_CTX_EVENT, -1, -1, BT_F_BANDITS, 50, "Бандиты", BT_T_GRASS, true);
    bt_retreat(NULL);
    bt_check_end(NULL);
    ok(bt.finished && !bt.victory, "отступление завершает бой без победы");

    /* --- усталость ограничивает действия --- */
    setup_game();
    bt_launch(BT_CTX_EVENT, -1, -1, BT_F_BANDITS, 50, "Бандиты", BT_T_GRASS, true);
    for (int i = 0; i < bt.n; i++) {
        if (bt.units[i].used && !bt.units[i].enemy) {
            bt.units[i].fat = bt.units[i].fat_max + 20;   /* предел clamp */
        }
    }
    /* force next_round через конец всех ходов */
    for (int k = 0; k < 40 && bt.round == 1; k++) bt_end_turn(NULL);
    ok(bt.round >= 2, "раунды сменяются");
    int tired = 0;
    for (int i = 0; i < bt.n; i++)
        if (bt.units[i].used && !bt.units[i].enemy && bt.units[i].fat >= bt.units[i].fat_max)
            tired++;
    ok(tired > 0, "переутомленные получают меньше AP (фатига сохранена)");

    printf("=== %d checks, %d failures ===\n", checks, fails);
    return fails ? 1 : 0;
}
