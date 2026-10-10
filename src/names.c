/* names.c — генераторы имён (немецко-фэнтезийный колорит Battle Brothers). */
#include "game.h"
#include "rng.h"
#include <string.h>
#include <stdio.h>

static const char* FIRST_NAMES[] = {
    "Ганс", "Отто", "Вильгельм", "Фридрих", "Конрад", "Готфрид", "Альбрехт",
    "Дитрих", "Эберхард", "Каспар", "Людвиг", "Матиас", "Ульрих", "Филипп",
    "Рудольф", "Стефан", "Тилль", "Вальтер", "Вернер", "Бертольд", "Генрих",
    "Курт", "Максимилиан", "Рейнхард", "Зигфрид", "Теодор", "Эрнст", "Франц",
    "Георг", "Йорг", "Лоренц", "Нильс", "Петер", "Роланд", "Себастьян", "Юстус",
    "Ансельм", "Бруно", "Вендель", "Готхард",
};
static const char* NICKNAMES[] = {
    "Ворон", "Молот", "Тень", "Кривой", "Тихий", "Железный", "Седой",
    "Весёлый", "Хмурый", "Волк", "Медведь", "Ястреб", "Топор", "Клык",
    "Сокол", "Камень", "Бритва", "Пес", "Орёл", "Пламя", "Лис", "Вепрь",
    "Цепь", "Скала", "Гром", "Дуб", "Пепел", "Кость", "Сова", "Призрак",
    "Пьяный", "Слон", "Крот", "Шрам", "Кузнец", "Лютый", "Мясник", "Дровосек",
    "Хитрый", "Простак",
};

static const char* TOWN_PREFIX[] = {
    "Штайн", "Альтен", "Ной", "Крон", "Айзен", "Вильден", "Грайфен",
    "Ротен", "Дункель", "Хаген", "Лихтен", "Мюль", "Обер", "Нидер",
    "Фогель", "Вассер", "Эбен", "Хоэн", "Финстер", "Зильбер", "Винцен",
    "Райхен", "Блюмен", "Занд", "Таль", "Кирхен", "Марк", "Лауфен",
    "Кальк", "Розен", "Айхен", "Фалькен",
};
static const char* TOWN_SUFFIX[] = {
    "бах", "дорф", "штадт", "берг", "бург", "фельс", "вальд", "штайн",
    "фурт", "брунн", "хаузен", "фельд", "берг", "рода", "маркт", "флёр",
    "хайм", "вайль", "таль", "экк",
};

static const char* LOC_ADJ[] = {
    "Забытые", "Проклятые", "Старые", "Разбитые", "Тёмные", "Пустые",
    "Каменные", "Ветхие", "Заброшенные", "Чёрные", "Ледяные", "Дальние",
};
static const char* LOC_NOUN[][3] = {
    /* RUINS */   { "Руины", "Башни", "Стены", },
    /* CAMP */    { "Лагерь", "Стоянка", "Логово", },
    /* MINE */    { "Шахта", "Рудник", "Копь", },
    /* SHRINE */  { "Святилище", "Капище", "Часовня", },
    /* FARM */    { "Хутор", "Ферма", "Мельница", },
};

static const char* COMPANY[] = {
    "Серые Псы", "Железная Длань", "Волки Штайнбаха", "Кровавое Солнце",
    "Вороний Клинок", "Братья Пепла", "Свободная Рота", "Костяной Щит",
    "Чёрные Знамёна", "Стальные Сердца", "Наёмный Братство", "Северный Ветер",
};

const char* gen_first_name(void) {
    return FIRST_NAMES[rng_range(0, (int)(sizeof(FIRST_NAMES) / sizeof(FIRST_NAMES[0])) - 1)];
}

const char* gen_nickname(void) {
    return NICKNAMES[rng_range(0, (int)(sizeof(NICKNAMES) / sizeof(NICKNAMES[0])) - 1)];
}

const char* gen_brother_name(char* buf, int n) {
    if (rng_float() < 0.62f)
        snprintf(buf, n, "%s «%s»", gen_first_name(), gen_nickname());
    else
        snprintf(buf, n, "%s", gen_first_name());
    return buf;
}

const char* gen_town_name(void) {
    static char buf[NAME_LEN];
    const char* a = TOWN_PREFIX[rng_range(0, (int)(sizeof(TOWN_PREFIX) / sizeof(TOWN_PREFIX[0])) - 1)];
    const char* b = TOWN_SUFFIX[rng_range(0, (int)(sizeof(TOWN_SUFFIX) / sizeof(TOWN_SUFFIX[0])) - 1)];
    if (rng_float() < 0.22f) {
        const char* c = TOWN_PREFIX[rng_range(0, (int)(sizeof(TOWN_PREFIX) / sizeof(TOWN_PREFIX[0])) - 1)];
        snprintf(buf, sizeof(buf), "%s%s-%s", a, b, c);
    } else {
        snprintf(buf, sizeof(buf), "%s%s", a, b);
    }
    return buf;
}

const char* gen_loc_name(LocType t) {
    static char buf[NAME_LEN];
    const char* adj = LOC_ADJ[rng_range(0, (int)(sizeof(LOC_ADJ) / sizeof(LOC_ADJ[0])) - 1)];
    const char* noun = LOC_NOUN[t][rng_range(0, 2)];
    snprintf(buf, sizeof(buf), "%s %s", adj, noun);
    return buf;
}

const char* gen_company_name(void) {
    return COMPANY[rng_range(0, (int)(sizeof(COMPANY) / sizeof(COMPANY[0])) - 1)];
}
