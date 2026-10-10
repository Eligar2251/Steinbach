/* stub_raylib.c — заглушка raylib для смоук-теста интерфейса на Linux. */
#include "raylib.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* --- состояние «ввода» --- */
static Vector2 s_mouse = { 640, 360 };
static int s_down = 0, s_pressed = 0, s_released = 0;
static int s_keys_pressed = 0;
static float s_wheel = 0;
static Vector2 s_delta = { 0, 0 };

void stub_input(float mx, float my, int down, int pressed, int released, float wheel) {
    s_mouse.x = mx;
    s_mouse.y = my;
    s_down = down;
    s_pressed = pressed;
    s_released = released;
    s_wheel = wheel;
}

/* --- окно --- */
static int s_frame = 0;
void InitWindow(int w, int h, const char* t) { (void)w; (void)h; (void)t; }
void CloseWindow(void) {}
bool WindowShouldClose(void) { return s_frame > 5000; }
void SetConfigFlags(unsigned int f) { (void)f; }
void SetTargetFPS(int fps) { (void)fps; }
void SetWindowMinSize(int w, int h) { (void)w; (void)h; }
void SetTraceLogLevel(int l) { (void)l; }
int GetScreenWidth(void) { return 1280; }
int GetScreenHeight(void) { return 720; }
double GetTime(void) { return (double)s_frame / 60.0; }
float GetFrameTime(void) { return 0.016f; }
void BeginDrawing(void) { s_frame++; }
void EndDrawing(void) {}
void ClearBackground(Color c) { (void)c; }
void BeginScissorMode(int x, int y, int w, int h) { (void)x; (void)y; (void)w; (void)h; }
void EndScissorMode(void) {}

/* --- ввод --- */
Vector2 GetMousePosition(void) { return s_mouse; }
Vector2 GetMouseDelta(void) { return s_delta; }
float GetMouseWheelMove(void) { return s_wheel; }
int GetMouseX(void) { return (int)s_mouse.x; }
int GetMouseY(void) { return (int)s_mouse.y; }
bool IsMouseButtonDown(int b) { return b == MOUSE_BUTTON_LEFT ? s_down != 0 : 0; }
bool IsMouseButtonPressed(int b) { return b == MOUSE_BUTTON_LEFT ? s_pressed != 0 : 0; }
bool IsMouseButtonReleased(int b) { return b == MOUSE_BUTTON_LEFT ? s_released != 0 : 0; }
bool IsKeyPressed(int k) { (void)k; return 0; }
bool IsKeyDown(int k) { (void)k; return 0; }

/* --- отрисовка (no-op, только проверка аргументов) --- */
void DrawRectangle(int x, int y, int w, int h, Color c) { (void)x; (void)y; (void)w; (void)h; (void)c; }
void DrawRectangleLines(int x, int y, int w, int h, Color c) { (void)x; (void)y; (void)w; (void)h; (void)c; }
void DrawRectangleLinesEx(Rectangle r, float t, Color c) { (void)r; (void)t; (void)c; }
void DrawRectangleRounded(Rectangle r, float ro, int se, Color c) {
    if (r.width != r.width || r.height != r.height) { fprintf(stderr, "NaN rect!\n"); exit(2); }
    (void)ro; (void)se; (void)c;
}
void DrawRectangleRoundedLines(Rectangle r, float ro, int se, Color c) { (void)r; (void)ro; (void)se; (void)c; }
void DrawRectangleRoundedLinesEx(Rectangle r, float ro, int se, float t, Color c) { (void)r; (void)ro; (void)se; (void)t; (void)c; }
void DrawCircleV(Vector2 c, float r, Color col) { (void)c; (void)r; (void)col; }
void DrawCircle(int cx, int cy, float r, Color col) { (void)cx; (void)cy; (void)r; (void)col; }
void DrawEllipse(int cx, int cy, float rh, float rv, Color col) { (void)cx; (void)cy; (void)rh; (void)rv; (void)col; }
void DrawLineEx(Vector2 a, Vector2 b, float t, Color col) { (void)a; (void)b; (void)t; (void)col; }
void DrawCircleLines(int cx, int cy, float r, Color c) { (void)cx; (void)cy; (void)r; (void)c; }
void DrawTexturePro(Texture2D t, Rectangle s, Rectangle d, Vector2 o, float rot, Color c) {
    if (t.id == 0) { /* допустимо, raylib бы залогировал */ }
    (void)s; (void)d; (void)o; (void)rot; (void)c;
}
void DrawTextEx(Font f, const char* text, Vector2 pos, float size, float spacing, Color c) {
    if (!text) { fprintf(stderr, "NULL text!\n"); exit(2); }
    (void)f; (void)pos; (void)size; (void)spacing; (void)c;
}
Vector2 MeasureTextEx(Font f, const char* text, float size, float spacing) {
    (void)f; (void)spacing;
    Vector2 v = { text ? (float)strlen(text) * size * 0.52f : 0, size * 1.2f };
    return v;
}

/* --- ресурсы --- */
Font LoadFontFromMemory(const char* t, const unsigned char* d, int n, int size, int* cp, int cpc) {
    (void)t; (void)d; (void)n; (void)cp; (void)cpc;
    Font f;
    memset(&f, 0, sizeof(f));
    f.baseSize = size;
    f.texture.id = 1;
    f.texture.width = 256;
    f.texture.height = 256;
    f.glyphCount = 100;
    f.glyphs = calloc(100, sizeof(GlyphInfo));
    f.recs = calloc(100, sizeof(Rectangle));
    return f;
}
void UnloadFont(Font f) { free(f.glyphs); free(f.recs); }
Image LoadImageFromMemory(const char* t, const unsigned char* d, int n) {
    (void)t; (void)d; (void)n;
    Image img;
    memset(&img, 0, sizeof(img));
    img.width = 64;
    img.height = 64;
    img.data = (void*)1;
    return img;
}
void UnloadImage(Image img) { (void)img; }
Texture2D LoadTextureFromImage(Image img) {
    Texture2D t;
    memset(&t, 0, sizeof(t));
    t.id = 1;
    t.width = img.width;
    t.height = img.height;
    return t;
}
void UnloadTexture(Texture2D t) { (void)t; }
void SetTextureFilter(Texture2D t, int f) { (void)t; (void)f; }

const char* GetApplicationDirectory(void) { return "/tmp/"; }
