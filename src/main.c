/* main.c — точка входа, окно, игровой цикл. */
#include "raylib.h"
#include "game.h"
#include "ui.h"
#include "render.h"
#include <stdio.h>
#include <string.h>
#include "assets_gen.h"

static bool s_exit = false;

void exit_request(void) { s_exit = true; }

int main(void) {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(1280, 720, "Steinbach — Сага о наёмниках");
    SetWindowMinSize(1100, 640);
    SetTargetFPS(60);

    FILE* logf = fopen("steinbach.log", "w");
    if (logf) { fprintf(logf, "init ok, window created\n"); fflush(logf); }
    ui_assets_load();
    if (logf) {
        fprintf(logf, "assets loaded: %d embedded, fonts body=%u bold=%u title=%u\n",
                g_assets_count, g_font_body.texture.id, g_font_bold.texture.id, g_font_title.texture.id);
        for (int i = 0; i < TEX_COUNT; i++)
            if (!g_tex[i].id) fprintf(logf, "  texture missing id=%d\n", i);
        fflush(logf);
    }
    render_init();

    while (!WindowShouldClose() && !s_exit) {
        float dt = GetFrameTime();
        if (dt > 0.1f) dt = 0.1f;

        /* --- обновление --- */
        if (!g.game_over && !g.pending_event.active && !g.battle.active && !g.levelup_open)
            travel_update(dt);

        /* горячие клавиши */
        if (IsKeyPressed(KEY_F5)) {
            char path[512];
            snprintf(path, sizeof(path), "%ssave1.sav", GetApplicationDirectory());
            save_game(path);
        }
        if (IsKeyPressed(KEY_F9)) {
            char path[512];
            snprintf(path, sizeof(path), "%ssave1.sav", GetApplicationDirectory());
            if (load_game(path)) {
                render_init();
                render_set_screen(1);
                render_center_on(g.party_x * 64.0f, g.party_y * 64.0f);
            }
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            if (g.levelup_open) {
                g.levelup_open = false;
            } else if (g.battle.active) {
                g.battle.active = false;
                if (g.game_over) {
                    render_set_screen(0);
                    g.game_over = false;
                }
            } else if (render_screen_id() == 2 || render_screen_id() == 3) {
                render_set_screen(1);   /* из города/отряда — на карту */
            }
        }
        if (IsKeyPressed(KEY_P) && !g.levelup_open && !g.battle.active && !g.pending_event.active) {
            render_set_screen(render_screen_id() == 3 ? 1 : 3);
        }

        /* --- отрисовка --- */
        BeginDrawing();
        ClearBackground(COL_BG);
        render_frame(dt);
        EndDrawing();
    }

    ui_assets_unload();
    CloseWindow();
    if (logf) { fprintf(logf, "exit clean\n"); fclose(logf); }
    return 0;
}
