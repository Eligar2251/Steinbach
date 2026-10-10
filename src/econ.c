/* econ.c — ежедневные расходы: жалование, провиант, настроение, раны. */
#include "game.h"
#include "rng.h"
#include <string.h>

/* Поведение отряда при нехватке крон. */
void econ_daily(void) {
    int wages = 0;
    for (int i = 0; i < g.n_brothers; i++)
        wages += g.brothers[i].wage;

    bool paid = g.crowns >= wages;
    if (paid) {
        g.crowns -= wages;
    } else {
        g.crowns -= g.crowns; /* всё, что есть */
    }

    /* еда: 1 единица на бойца */
    bool ate = g.food >= g.n_brothers;
    if (ate) g.food -= g.n_brothers;
    else g.food = 0;

    for (int i = 0; i < g.n_brothers; i++) {
        Brother* b = &g.brothers[i];
        b->days_hired++;

        int dmood = 0;
        if (!paid) dmood -= 6;
        if (!ate)  dmood -= 5;
        if (paid && ate) dmood += 1;
        if (b->hp < brother_stat(b, STAT_HP) / 3) dmood -= 2;

        /* черты */
        for (int t = 0; t < b->n_traits; t++)
            dmood += g_traits[b->traits[t]].mood_mod;

        /* настроение медленно возвращается к норме */
        if (b->mood < 60) dmood += 1;
        if (b->mood > 75) dmood -= 1;

        b->mood = clampi(b->mood + dmood, 0, 100);

        /* раны заживают */
        if (b->injury >= 0) {
            int heal = 1;
            if (b->perks & (1u << 8)) heal = 2;   /* «Целитель» */
            b->injury_days -= heal;
            if (b->injury_days <= 0) {
                b->injury = -1;
                b->injury_days = 0;
            }
        }

        /* восстановление */
        int hp_max = brother_stat(b, STAT_HP);
        int fat_max = brother_stat(b, STAT_FATIGUE);
        if (b->hp < hp_max) b->hp = clampi(b->hp + 4, 0, hp_max);
        if (b->fatigue < fat_max) b->fatigue = fat_max;

        /* дезертирование в тяжёлый час */
        if (b->mood <= 5 && rng_float() < 0.35f) {
            /* боец уходит — освобождаем слот */
            for (int e = 0; e < EQ_COUNT; e++)
                if (b->equip[e] != ITEM_NONE) {
                    /* вещь остаётся в инвентаре */
                    b->equip[e] = ITEM_NONE;
                }
            for (int j = i; j < g.n_brothers - 1; j++) g.brothers[j] = g.brothers[j + 1];
            g.n_brothers--;
            i--;
        }
    }
}

void econ_start(void) {
    g.crowns = 2400;
    g.food = 30;
}
