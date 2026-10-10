/* rng.c — реализация ГПСЧ для логики. */
#include "game.h"
#include "rng.h"

static uint64_t s_state = 0x9E3779B97F4A7C15ull;

void rng_seed(uint64_t s) {
    s_state = s ? s : 0x9E3779B97F4A7C15ull;
    /* прогрев */
    for (int i = 0; i < 4; i++) rng_next();
}

uint64_t rng_next(void) {
    return rng_splitmix(&s_state);
}

int rng_range(int lo, int hi) {
    if (hi < lo) { int t = lo; lo = hi; hi = t; }
    uint64_t span = (uint64_t)(hi - lo) + 1;
    return lo + (int)(rng_next() % span);
}

float rng_float(void) {
    return (float)(rng_next() >> 11) / (float)(1ull << 53);
}

int rng_pick(const int* arr, int n) {
    if (n <= 0) return 0;
    return arr[rng_range(0, n - 1)];
}
