/* test_ui.c — смоук-тест всех экранов через заглушку raylib (ловит краши). */
#include "../src/game.h"
#include "../src/ui.h"
#include "../src/render.h"
#include <stdio.h>
#include <string.h>

void stub_input(float mx, float my, int down, int pressed, int released, float wheel);
void exit_request(void);
void exit_request(void) {}

extern int render_screen_id(void);

static int crashes = 0;

static void frames(int n, float mx, float my, int click) {
    for (int i = 0; i < n; i++) {
        stub_input(mx, my, click ? 1 : 0, click && i == 0, click && i == 1, 0);
        render_frame(0.016f);
    }
    stub_input(mx, my, 0, 0, 0, 0);
}

int main(void) {
    printf("=== UI smoke test ===\n");

    ui_assets_load();
    render_init();

    /* меню */
    frames(3, 640, 300, 0);
    /* клик по «Новая игра» */
    stub_input(640, 326, 1, 1, 0, 0);
    render_frame(0.016f);
    stub_input(640, 326, 0, 0, 1, 0);
    render_frame(0.016f);
    printf("ok: меню → новая игра (screen=%d, towns=%d)\n", render_screen_id(), g.n_towns);
    if (g.n_towns < 6) { printf("FAIL: нет городов\n"); crashes++; }

    /* карта: движение мыши, клики по разным местам, зум */
    for (int i = 0; i < 30; i++) {
        stub_input(100.0f + i * 37, 200.0f + (i % 7) * 60, 0, 0, 0, (i % 3) - 1);
        render_frame(0.05f);
    }
    /* клики по карте */
    for (int i = 0; i < 10; i++) {
        frames(2, 300.0f + i * 70, 300.0f + i * 20, 1);
    }
    printf("ok: карта с кликами\n");

    /* открыть город напрямую */
    render_set_screen(2);
    /* s_town_idx статичен в render.c — выставляется кликом; используем обход:
       кликаем по иконке города на карте через travel, затем открытие */
    render_set_screen(1);
    travel_set_dest(g.towns[0].x, g.towns[0].y);
    for (int i = 0; i < 400; i++) travel_update(0.1f);
    printf("ok: travel (party=%.1f,%.1f)\n", g.party_x, g.party_y);

    /* город: симулируем открытие кликом рядом с городом 0 */
    {
        /* открыть экран города через внутренний механизм: клик по иконке */
        float tx = (float)g.towns[0].x * 64.0f + 32.0f;
        float ty = (float)g.towns[0].y * 64.0f + 32.0f;
        render_center_on(tx, ty);
        frames(2, 640, 360, 0);
        frames(3, 640, 360, 1);   /* клик по центру — там город */
        printf("ok: клик по городу (screen=%d)\n", render_screen_id());
    }

    /* вкладки города (если открылся) и экран отряда */
    render_set_screen(3);
    for (int t = 0; t < 5; t++) frames(2, 400, 300, 1);
    frames(3, 1100, 300, 1);      /* клики по инвентарю */
    frames(3, 200, 250, 1);       /* выбор бойца */
    printf("ok: экран отряда\n");

    /* модалка боя */
    battle_autofight(&g.battle, 120, "Бандиты", true);
    frames(5, 640, 600, 0);
    frames(3, 640, 640, 1);       /* «Продолжить» */
    printf("ok: модалка боя (active=%d)\n", g.battle.active);

    /* модалка события */
    g.pending_event.active = true;
    g.pending_event.event_id = 0;
    frames(4, 640, 400, 0);
    frames(3, 640, 420, 1);       /* выбор варианта */
    printf("ok: модалка события (pending=%d, battle=%d)\n", g.pending_event.active, g.battle.active);
    if (g.battle.active) {
        frames(3, 640, 620, 1);
    }

    /* прокачка */
    if (g.n_brothers > 0) {
        g.brothers[0].stat_picks = 3;
        g.brothers[0].perk_points = 1;
        g.levelup_open = true;
        frames(3, 640, 360, 0);
        frames(2, 520, 120, 1);   /* взять стат */
        frames(2, 520, 160, 1);
        frames(2, 520, 200, 1);
        frames(2, 700, 120, 1);   /* перк */
        frames(2, 640, 600, 1);   /* готово */
        printf("ok: прокачка (picks=%d perk=%u)\n", g.brothers[0].stat_picks, g.brothers[0].perks);
    }

    /* сохранение/загрузка через игровые функции */
    if (save_game("/tmp/ui_smoke.sav")) {
        if (load_game("/tmp/ui_smoke.sav")) printf("ok: save/load круг\n");
        else { printf("FAIL: load\n"); crashes++; }
    } else { printf("FAIL: save\n"); crashes++; }

    render_set_screen(1);
    frames(5, 640, 360, 0);

    /* тик 20 дней */
    for (int i = 0; i < 20; i++) {
        g.traveling = true;
        g.path_pos = 0;
        g.path_len = 1;
        g.path[0] = (TilePos){ (int)g.party_x, (int)g.party_y };
        travel_update(5.0f);
        if (g.battle.active) { frames(2, 640, 640, 1); }
        if (g.pending_event.active) { frames(2, 640, 420, 1); if (g.battle.active) frames(2, 640, 640, 1); }
    }
    printf("ok: 20 дней (день %d, бойцов %d, крон %d)\n", g.day, g.n_brothers, g.crowns);

    ui_assets_unload();
    printf("=== UI smoke: %d проблем ===\n", crashes);
    return crashes ? 1 : 0;
}
