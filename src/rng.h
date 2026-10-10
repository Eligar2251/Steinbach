/* rng.h — быстрый ГПСЧ (splitmix64 + xorshift) для логики игры. */
#ifndef RNG_H
#define RNG_H

#include <stdint.h>

static inline uint64_t rng_splitmix(uint64_t* s) {
    uint64_t z = (*s += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

static inline uint64_t rng_xorshift(uint64_t* s) {
    uint64_t x = *s;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *s = x ? x : 0x9E3779B97F4A7C15ull;
    return *s;
}

#endif
