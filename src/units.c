/* units.c — наёмники: генерация, характеристики, черты, перки, уровни. */
#include "game.h"
#include "rng.h"
#include <string.h>
#include <stdio.h>

const char* g_stat_name[STAT_COUNT] = {
    "Здоровье", "Выносливость", "Решимость", "Инициатива",
    "Мастерство боя", "Стрельба", "Защита", "Уклонение",
};

/* Имена оружия-заглушек для стартовой экипировки берутся из items.c */
#define DEF_KNIFE 0
#define DEF_SWORD 1
#define DEF_LSWORD 2
#define DEF_AXE 3
#define DEF_MACE 4
#define DEF_SPEAR 5
#define DEF_POLE 6
#define DEF_HAXE 7
#define DEF_BOW 8
#define DEF_LBOW 9
#define DEF_XBOW 10
#define DEF_SSHIELD 11
#define DEF_KSHIELD 12
#define DEF_TSHIELD 13
#define DEF_HLEATHER 14
#define DEF_HMAIL 15
#define DEF_HSTEEL 16
#define DEF_BJACK 18
#define DEF_BPADDED 19
#define DEF_BMAIL 20
#define DEF_BPLATE 21

const Background g_backgrounds[N_BACKGROUNDS] = {
    /* name, desc, min-stats, max-stats, wage, price_mult, weapon, offhand, head, body, bias */
    { "Фермер", "Привык к тяжёлой работе и непогоде.",
      { 55, 95, 38, 95, 48, 40, 2, 0 }, { 65, 115, 52, 110, 62, 55, 8, 5 },
      12, 80, DEF_SPEAR, ITEM_NONE, ITEM_NONE, DEF_BJACK, -1 },
    { "Пастух", "Волки не страшны после ночного дежурства.",
      { 52, 100, 40, 100, 46, 48, 3, 2 }, { 62, 120, 55, 115, 58, 62, 8, 8 },
      12, 85, DEF_BOW, ITEM_NONE, ITEM_NONE, DEF_BJACK, 0 },
    { "Мещанин", "Ремесленник с крепкими руками.",
      { 55, 95, 42, 95, 50, 42, 3, 1 }, { 65, 110, 56, 108, 62, 56, 8, 6 },
      15, 100, DEF_MACE, ITEM_NONE, DEF_HLEATHER, DEF_BPADDED, 0 },
    { "Охотник", "Стреляет в зайца с сотни шагов.",
      { 52, 105, 40, 105, 45, 62, 3, 4 }, { 60, 120, 54, 118, 56, 75, 8, 10 },
      18, 110, DEF_BOW, ITEM_NONE, ITEM_NONE, DEF_BJACK, 1 },
    { "Наемник", "Видал и не такое. Берёт за работу дорого.",
      { 62, 105, 50, 95, 62, 48, 8, 4 }, { 72, 120, 64, 110, 74, 60, 15, 10 },
      32, 160, DEF_SWORD, DEF_SSHIELD, DEF_HMAIL, DEF_BPADDED, 2 },
    { "Дезертир", "Бежал из армии. Зато умеет воевать.",
      { 58, 100, 42, 100, 58, 45, 6, 3 }, { 70, 118, 56, 112, 70, 58, 12, 9 },
      24, 130, DEF_SWORD, ITEM_NONE, DEF_HLEATHER, DEF_BJACK, 2 },
    { "Бандит", "Тёмное прошлое и быстрый клинок.",
      { 58, 100, 44, 105, 58, 50, 7, 5 }, { 68, 115, 58, 120, 70, 62, 12, 12 },
      22, 125, DEF_AXE, ITEM_NONE, ITEM_NONE, DEF_BJACK, 2 },
    { "Монах", "Молится за вас и за себя.",
      { 50, 90, 58, 90, 42, 38, 2, 1 }, { 60, 105, 72, 102, 52, 50, 6, 5 },
      14, 95, DEF_MACE, ITEM_NONE, ITEM_NONE, DEF_BJACK, -1 },
    { "Дворянин", "Кровь благородная, спесь великая.",
      { 58, 100, 55, 100, 60, 55, 7, 5 }, { 68, 115, 68, 112, 72, 68, 13, 11 },
      40, 200, DEF_LSWORD, DEF_KSHIELD, DEF_HSTEEL, DEF_BMAIL, 2 },
    { "Пират", "Привык качаться на палубе и в таверне.",
      { 58, 105, 45, 100, 55, 55, 6, 5 }, { 68, 120, 58, 112, 66, 68, 11, 11 },
      24, 130, DEF_AXE, DEF_SSHIELD, ITEM_NONE, DEF_BJACK, 1 },
    { "Кузнец", "Руки как молоты.",
      { 62, 105, 46, 85, 58, 38, 5, 1 }, { 74, 122, 58, 96, 70, 48, 10, 5 },
      20, 120, DEF_MACE, ITEM_NONE, DEF_HLEATHER, DEF_BPADDED, 1 },
    { "Лесоруб", "Знает, как валить толстых.",
      { 62, 110, 42, 92, 56, 40, 4, 1 }, { 74, 126, 54, 104, 68, 50, 9, 6 },
      16, 105, DEF_HAXE, ITEM_NONE, ITEM_NONE, DEF_BJACK, 1 },
    { "Шахтёр", "Тесно и темно — его стихия.",
      { 60, 112, 46, 88, 55, 38, 5, 1 }, { 72, 128, 58, 98, 66, 48, 10, 5 },
      16, 105, DEF_MACE, ITEM_NONE, DEF_HLEATHER, DEF_BJACK, 0 },
    { "Цирюльник", "И режет, и шьёт.",
      { 50, 95, 48, 102, 50, 55, 4, 3 }, { 58, 110, 60, 115, 60, 68, 9, 8 },
      18, 110, DEF_KNIFE, ITEM_NONE, ITEM_NONE, DEF_BJACK, 0 },
    { "Студент", "Книжное дело, а не военное. Но учится быстро.",
      { 48, 90, 52, 100, 42, 45, 2, 3 }, { 56, 102, 66, 112, 52, 58, 6, 8 },
      14, 95, DEF_SPEAR, ITEM_NONE, ITEM_NONE, DEF_BJACK, -1 },
    { "Ветеран", "Седой, шрамастый, опасный.",
      { 65, 108, 56, 92, 66, 50, 10, 5 }, { 78, 124, 70, 105, 78, 62, 18, 12 },
      46, 230, DEF_LSWORD, DEF_KSHIELD, DEF_HSTEEL, DEF_BMAIL, 3 },
};

const TraitDef g_traits[N_TRAITS] = {
    { "Крепыш",   "Здоровье выше обычного.",           { 12, 0, 0, 0, 0, 0, 0, 0 },  25, 0 },
    { "Тощий",    "Хилый и слабый.",                    { -10, -8, 0, 0, 0, 0, 0, 0 }, -20, -1 },
    { "Храбрый",  "Не знает страха.",                   { 0, 0, 12, 0, 0, 0, 0, 0 },  20, 1 },
    { "Трус",     "Сердце подводит.",                   { 0, 0, -12, 0, 0, 0, 0, 0 }, -25, -2 },
    { "Быстрый",  "Движется молниеносно.",              { 0, 0, 0, 12, 0, 0, 0, 0 },  20, 0 },
    { "Медлительный", "Всё делает не спеша.",           { 0, 0, 0, -12, 0, 0, 0, 0 }, -15, 0 },
    { "Силач",    "Удар как у кузнечного молота.",      { 0, 0, 0, 0, 8, 0, 0, 0 },   30, 0 },
    { "Шустрой",  "Метко стреляет.",                    { 0, 0, 0, 0, 0, 8, 0, 0 },   30, 0 },
    { "Неповоротливый", "Тяжёл на подъём.",             { 0, 0, 0, 0, 0, 0, -6, -3 }, -20, 0 },
    { "Ловкач",   "Уходит от ударов.",                  { 0, 0, 0, 0, 0, 0, 8, 4 },   35, 0 },
    { "Красивый", "Приятная внешность открывает двери.", { 0, 0, 4, 0, 0, 0, 0, 0 },  30, 2 },
    { "Уродливый","Взгляд отводят.",                    { 0, 0, -4, 0, 0, 0, 0, 0 }, -15, -1 },
    { "Пьяница",  "Любит выпить. Каждый день.",         { 0, 0, -6, 0, 0, 0, 0, 0 }, -15, -3 },
    { "Трезвенник", "Ясная голова.",                    { 0, 0, 6, 0, 0, 0, 0, 0 },   15, 1 },
    { "Ветеран",  "Быстро набирается опыта.",           { 0, 0, 0, 0, 0, 0, 0, 0 },   40, 1 },
    { "Тупица",   "Учится с трудом.",                   { 0, 0, -4, 0, 0, 0, 0, 0 }, -25, 0 },
    { "Выносливый", "Дольше держится в строю.",         { 0, 14, 0, 0, 0, 0, 0, 0 },  25, 0 },
    { "Слабак",   "Быстро выдыхается.",                 { 0, -12, 0, 0, 0, 0, 0, 0 }, -20, 0 },
};

const PerkDef g_perks[PERK_COUNT] = {
    { "Крепкий удар",    "+15% урона в ближнем бою." },
    { "Мастер выстрелов","+15% урона из лука и арбалета." },
    { "Каменная стена",  "+10 защиты при щите в руке." },
    { "Железные латы",   "-25% штрафа выносливости от брони." },
    { "Храброе сердце",  "+10 решимости отряду в бою." },
    { "Лёгкие ноги",     "+10 инициативы." },
    { "Крепкое здоровье","+15 здоровья." },
    { "Берсерк",         "После победы в бою — восстановление выносливости." },
    { "Целитель",        "Раны заживают вдвое быстрее." },
    { "Торговец",        "Скидка 10% в магазинах, наценка при продаже." },
    { "Командир",        "+5 решимости и морали всем бойцам." },
    { "Вояка",           "+5 мастерства боя и +5 стрельбы." },
};

const InjuryDef g_injuries[N_INJURIES] = {
    { "Раненая рука",     { 0, 0, 0, 0, -8, 0, 0, 0 }, 3, 6 },
    { "Раненая нога",     { 0, -12, 0, -8, 0, 0, -3, -3 }, 3, 7 },
    { "Сотрясение",       { -8, -8, -8, -6, -4, -4, -2, -2 }, 2, 5 },
    { "Порез на лице",    { 0, 0, -6, 0, 0, 0, 0, 0 }, 2, 4 },
    { "Сломанные рёбра",  { -12, -10, 0, -5, -5, 0, -4, -4 }, 4, 9 },
    { "Раненое плечо",    { 0, -6, 0, 0, -10, -6, -3, 0 }, 3, 7 },
    { "Кровопотеря",      { -10, -10, -5, -6, -4, -4, -2, -2 }, 1, 3 },
    { "Потрескавшийся череп", { -6, 0, -10, -4, -4, -4, -2, -2 }, 5, 10 },
};

/* ------------------------------------------------------------- базовые --- */
void brother_clear(Brother* b) {
    memset(b, 0, sizeof(*b));
    b->injury = -1;
    for (int i = 0; i < EQ_COUNT; i++) b->equip[i] = ITEM_NONE;
    b->mood = 70;
}

int xp_needed(int level) {
    return 180 + level * level * 130;
}

int brother_stat(const Brother* b, StatId s) {
    int v = 0;
    switch (s) {
        case STAT_HP:         v = b->base.hp; break;
        case STAT_FATIGUE:    v = b->base.fatigue; break;
        case STAT_RESOLVE:    v = b->base.resolve; break;
        case STAT_INITIATIVE: v = b->base.initiative; break;
        case STAT_MATK:       v = b->base.matk; break;
        case STAT_RATK:       v = b->base.ratk; break;
        case STAT_MDEF:       v = b->base.mdef; break;
        case STAT_RDEF:       v = b->base.rdef; break;
        default: return 0;
    }
    /* перки */
    if (s == STAT_HP && (b->perks & (1u << 6))) v += 15;
    if (s == STAT_INITIATIVE && (b->perks & (1u << 5))) v += 10;
    if (s == STAT_RESOLVE && (b->perks & (1u << 4))) v += 10;
    if ((s == STAT_MATK || s == STAT_RATK) && (b->perks & (1u << 11))) v += 5;
    /* ранение */
    if (b->injury >= 0 && b->injury < N_INJURIES) {
        const Stats* m = &g_injuries[b->injury].mod;
        switch (s) {
            case STAT_HP: v += m->hp; break;
            case STAT_FATIGUE: v += m->fatigue; break;
            case STAT_RESOLVE: v += m->resolve; break;
            case STAT_INITIATIVE: v += m->initiative; break;
            case STAT_MATK: v += m->matk; break;
            case STAT_RATK: v += m->ratk; break;
            case STAT_MDEF: v += m->mdef; break;
            case STAT_RDEF: v += m->rdef; break;
            default: break;
        }
    }
    if (v < 1) v = 1;
    return v;
}

int brother_power(const Brother* b) {
    int hp = brother_stat(b, STAT_HP);
    int matk = brother_stat(b, STAT_MATK);
    int ratk = brother_stat(b, STAT_RATK);
    int mdef = brother_stat(b, STAT_MDEF);
    int rdef = brother_stat(b, STAT_RDEF);
    int arm = item_total_armor(b);
    float p = matk * 1.1f + ratk * 0.55f + mdef * 1.6f + rdef * 0.6f +
              hp * 0.32f + arm * 0.075f + b->level * 4.0f;
    int mood_factor = 85 + b->mood / 8;
    return (int)(p * mood_factor / 100.0f);
}

void brother_apply_traits(Brother* b) {
    for (int i = 0; i < b->n_traits; i++) {
        const TraitDef* t = &g_traits[b->traits[i]];
        b->base.hp += t->mod.hp;
        b->base.fatigue += t->mod.fatigue;
        b->base.resolve += t->mod.resolve;
        b->base.initiative += t->mod.initiative;
        b->base.matk += t->mod.matk;
        b->base.ratk += t->mod.ratk;
        b->base.mdef += t->mod.mdef;
        b->base.rdef += t->mod.rdef;
    }
}

static int rrange(const Stats* lo, const Stats* hi, int off) {
    const int* a = (const int*)lo;
    const int* b = (const int*)hi;
    return rng_range(a[off], b[off]);
}

void brother_generate(Brother* b, int background, int level_hint) {
    if (background < 0 || background >= N_BACKGROUNDS) background = 0;
    const Background* bg = &g_backgrounds[background];
    brother_clear(b);
    b->background = background;
    gen_brother_name(b->name, (int)sizeof(b->name));
    b->portrait = rng_range(0, 11);

    Stats* s = &b->base;
    s->hp         = rrange(&bg->stats_min, &bg->stats_max, 0);
    s->fatigue    = rrange(&bg->stats_min, &bg->stats_max, 1);
    s->resolve    = rrange(&bg->stats_min, &bg->stats_max, 2);
    s->initiative = rrange(&bg->stats_min, &bg->stats_max, 3);
    s->matk       = rrange(&bg->stats_min, &bg->stats_max, 4);
    s->ratk       = rrange(&bg->stats_min, &bg->stats_max, 5);
    s->mdef       = rrange(&bg->stats_min, &bg->stats_max, 6);
    s->rdef       = rrange(&bg->stats_min, &bg->stats_max, 7);

    /* 1-2 черты */
    b->n_traits = rng_float() < 0.55f ? 2 : 1;
    for (int i = 0; i < b->n_traits; i++) {
        int id = rng_range(0, N_TRAITS - 1);
        bool dup;
        do {
            dup = false;
            for (int j = 0; j < i; j++) if (b->traits[j] == id) dup = true;
            if (dup) id = rng_range(0, N_TRAITS - 1);
        } while (dup);
        b->traits[i] = id;
    }
    brother_apply_traits(b);

    b->level = 1;
    b->xp = 0;
    for (int lv = 1; lv < level_hint; lv++) {
        /* авто-прокачка для «ветеранов» */
        b->level++;
        int pick = rng_range(0, 7);
        int* fields[8] = { &s->hp, &s->fatigue, &s->resolve, &s->initiative,
                           &s->matk, &s->ratk, &s->mdef, &s->rdef };
        *fields[pick] += 2 + rng_range(0, 3);
    }

    b->hp = brother_stat(b, STAT_HP);
    b->fatigue = brother_stat(b, STAT_FATIGUE);
    b->wage = brother_wage(b);
}

void brother_recalc_current(Brother* b) {
    int hp = brother_stat(b, STAT_HP);
    int fat = brother_stat(b, STAT_FATIGUE);
    if (b->hp > hp) b->hp = hp;
    if (b->fatigue > fat) b->fatigue = fat;
}

int brother_price(const Brother* b) {
    int base = 60 + b->level * 90;
    base += (b->base.matk + b->base.ratk) * 2 + b->base.hp;
    base += b->base.mdef * 6 + b->base.rdef * 3;
    base = base * g_backgrounds[b->background].price_mult / 100;
    for (int i = 0; i < b->n_traits; i++)
        base = base * (100 + g_traits[b->traits[i]].price_mod) / 100;
    return clampi(base, 40, 9000);
}

int brother_wage(const Brother* b) {
    int w = g_backgrounds[b->background].wage + b->level * 4;
    return w;
}

void brother_add_xp(Brother* b, int xp) {
    for (int i = 0; i < b->n_traits; i++)
        if (b->traits[i] == 14) xp = xp * 12 / 10;   /* «Ветеран» */
        else if (b->traits[i] == 15) xp = xp * 8 / 10; /* «Тупица» */
    b->xp += xp;
    while (b->xp >= xp_needed(b->level) && b->level < 11) {
        b->xp -= xp_needed(b->level);
        b->level++;
        b->stat_picks += 3;
        b->perk_points += 1;
    }
}

void brother_levelup_apply(Brother* b, const int stat_picks[3], int perk) {
    int* fields[STAT_COUNT] = {
        &b->base.hp, &b->base.fatigue, &b->base.resolve, &b->base.initiative,
        &b->base.matk, &b->base.ratk, &b->base.mdef, &b->base.rdef
    };
    for (int i = 0; i < 3; i++) {
        int s = stat_picks[i];
        if (s < 0 || s >= STAT_COUNT) continue;
        static const int gain[STAT_COUNT] = { 4, 6, 4, 5, 3, 3, 3, 3 };
        *fields[s] += gain[s];
    }
    if (perk >= 0 && perk < PERK_COUNT) b->perks |= (1u << perk);
    if (b->stat_picks >= 3) b->stat_picks -= 3;
    if (b->perk_points > 0) b->perk_points--;
    brother_recalc_current(b);
    b->hp = brother_stat(b, STAT_HP);
    b->fatigue = brother_stat(b, STAT_FATIGUE);
}

const char* background_name(int idx) {
    if (idx < 0 || idx >= N_BACKGROUNDS) return "—";
    return g_backgrounds[idx].name;
}
