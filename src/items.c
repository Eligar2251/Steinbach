/* items.c — определения предметов и работа с инвентарём. */
#include "game.h"
#include <string.h>

/* name, desc, slot, wtype, price, armor, dmg_min, dmg_max, fat_cost, armor_pen,
   range, shield_def, init_penalty, fat_penalty, two_handed */
const ItemDef g_item_defs[] = {
    /* ---- оружие ближнего боя ---- */
    { "Нож", "Простой нож. Всегда лучше, чем кулаки.",
      SLOT_WEAPON, WTYPE_SWORD, 40, 0, 5, 10, 8, 5, 0, 0, 0, 0, 0 },
    { "Меч", "Надёжный короткий меч.",
      SLOT_WEAPON, WTYPE_SWORD, 220, 0, 12, 20, 12, 15, 0, 0, 0, 0, 0 },
    { "Длинный меч", "Тяжёлый клинок для обученного бойца.",
      SLOT_WEAPON, WTYPE_SWORD, 620, 0, 18, 30, 16, 20, 0, 0, -5, 0, 1 },
    { "Боевой топор", "Рубит и плоть, и доспех.",
      SLOT_WEAPON, WTYPE_AXE, 340, 0, 16, 26, 15, 30, 0, 0, 0, 0, 0 },
    { "Боевой молот", "Ломает щиты и кости.",
      SLOT_WEAPON, WTYPE_MACE, 300, 0, 14, 24, 18, 45, 0, 0, -5, 0, 0 },
    { "Копьё", "Достаёт врага первым.",
      SLOT_WEAPON, WTYPE_SPEAR, 180, 0, 10, 18, 12, 10, 0, 0, 5, 0, 0 },
    { "Алебарда", "Оружие ополчения и наёмников.",
      SLOT_WEAPON, WTYPE_POLEARM, 520, 0, 20, 32, 20, 25, 0, 0, -10, 0, 1 },
    { "Секира ополченца", "Тяжёлая и беспощадная.",
      SLOT_WEAPON, WTYPE_AXE, 260, 0, 18, 28, 22, 35, 0, 0, -8, 0, 1 },
    /* ---- стрелковое ---- */
    { "Короткий лук", "Быстрый и лёгкий.",
      SLOT_WEAPON, WTYPE_BOW, 200, 0, 8, 14, 10, 5, 6, 0, 0, 0, 1 },
    { "Длинный лук", "Дальний выстрел, страшная сила.",
      SLOT_WEAPON, WTYPE_BOW, 520, 0, 12, 20, 14, 10, 7, 0, -5, 0, 1 },
    { "Арбалет", "Медленно, но пробивает доспех.",
      SLOT_WEAPON, WTYPE_CROSSBOW, 700, 0, 18, 28, 18, 40, 7, 0, -10, 0, 1 },
    /* ---- щиты ---- */
    { "Деревянный щит", "Простая доска, обитая кожей.",
      SLOT_OFFHAND, WTYPE_SHIELD, 140, 8, 0, 0, 0, 0, 0, 15, 0, 3, 0 },
    { "Кованый щит", "Тяжёлый, зато крепкий.",
      SLOT_OFFHAND, WTYPE_SHIELD, 380, 16, 0, 0, 0, 0, 0, 22, -5, 6, 0 },
    { "Башенный щит", "Стена из дерева и железа.",
      SLOT_OFFHAND, WTYPE_SHIELD, 720, 28, 0, 0, 0, 0, 0, 30, -12, 12, 0 },
    /* ---- голова ---- */
    { "Кожаный шлем", "Мягкая защита головы.",
      SLOT_HEAD, WTYPE_NONE, 120, 35, 0, 0, 0, 0, 0, 0, 0, 1, 0 },
    { "Кольчужный капюшон", "Хлопотно надевать, полезно носить.",
      SLOT_HEAD, WTYPE_NONE, 320, 65, 0, 0, 0, 0, 0, 0, 0, 2, 0 },
    { "Стальной шлем", "Классика наёмных войн.",
      SLOT_HEAD, WTYPE_NONE, 680, 110, 0, 0, 0, 0, 0, 0, -3, 4, 0 },
    { "Шишак с бармицей", "Тяжёлая сталь, полная защита.",
      SLOT_HEAD, WTYPE_NONE, 1250, 165, 0, 0, 0, 0, 0, 0, -6, 8, 0 },
    /* ---- тело ---- */
    { "Кожаная куртка", "Лёгкая и удобная.",
      SLOT_BODY, WTYPE_NONE, 150, 50, 0, 0, 0, 0, 0, 0, 0, 2, 0 },
    { "Стёганный доспех", "Кольчуга под тканью.",
      SLOT_BODY, WTYPE_NONE, 420, 95, 0, 0, 0, 0, 0, 0, -3, 5, 0 },
    { "Кольчуга", "Гибкий сетчатый доспех.",
      SLOT_BODY, WTYPE_NONE, 900, 160, 0, 0, 0, 0, 0, 0, -6, 10, 0 },
    { "Латный доспех", "Тяжёлые пластины стали.",
      SLOT_BODY, WTYPE_NONE, 1900, 235, 0, 0, 0, 0, 0, 0, -12, 18, 0 },
    { "Латный доспех ветерана", "Переделан под владельца.",
      SLOT_BODY, WTYPE_NONE, 3200, 300, 0, 0, 0, 0, 0, 0, -16, 24, 0 },
    /* ---- снаряжение ---- */
    { "Стрелы", "Пучок стрел (20 шт.).",
      SLOT_SUPPLY, WTYPE_NONE, 25, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { "Аптечка", "Перевязка и зелья. Лечит раны.",
      SLOT_SUPPLY, WTYPE_NONE, 90, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { "Точильный камень", "Восстанавливает прочность оружия.",
      SLOT_SUPPLY, WTYPE_NONE, 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { "Мешок провианта", "Еда для отряда на несколько дней.",
      SLOT_SUPPLY, WTYPE_NONE, 55, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
};

const int g_item_def_count = (int)(sizeof(g_item_defs) / sizeof(g_item_defs[0]));

const char* slot_name(ItemSlot s) {
    switch (s) {
        case SLOT_HEAD:   return "Голова";
        case SLOT_BODY:   return "Тело";
        case SLOT_WEAPON: return "Оружие";
        case SLOT_OFFHAND:return "Левая рука";
        case SLOT_SUPPLY: return "Снаряжение";
        default:          return "—";
    }
}

int item_create(int def) {
    if (def < 0 || def >= g_item_def_count) return ITEM_NONE;
    if (g.n_items >= MAX_ITEMS) return ITEM_NONE;
    int idx = g.n_items++;
    g.items[idx].def = def;
    g.items[idx].durability = g_item_defs[def].price > 400 ? 100 : 100;
    g.items[idx].ammo = (g_item_defs[def].wtype == WTYPE_BOW) ? 20 :
                        (g_item_defs[def].wtype == WTYPE_CROSSBOW) ? 12 : 0;
    return idx;
}

void item_remove(int idx) {
    if (idx < 0 || idx >= g.n_items) return;
    /* корректируем ссылки бойцов */
    for (int b = 0; b < g.n_brothers; b++) {
        for (int e = 0; e < EQ_COUNT; e++) {
            if (g.brothers[b].equip[e] == idx) g.brothers[b].equip[e] = ITEM_NONE;
            else if (g.brothers[b].equip[e] > idx) g.brothers[b].equip[e]--;
        }
    }
    for (int i = idx; i < g.n_items - 1; i++) g.items[i] = g.items[i + 1];
    g.n_items--;
}

/* ------------------------------------------------------------ запросы --- */

static int equip_def(const Brother* b, int slot) {
    int idx = b->equip[slot];
    if (idx < 0 || idx >= g.n_items) return ITEM_NONE;
    return g.items[idx].def;
}

int item_total_armor(const Brother* b) {
    int a = 0;
    int h = equip_def(b, EQ_HEAD), t = equip_def(b, EQ_BODY);
    if (h != ITEM_NONE) a += g_item_defs[h].armor;
    if (t != ITEM_NONE) a += g_item_defs[t].armor;
    int o = equip_def(b, EQ_OFFHAND);
    if (o != ITEM_NONE && g_item_defs[o].wtype == WTYPE_SHIELD) a += g_item_defs[o].armor;
    return a;
}

int item_weapon_damage_min(const Brother* b) {
    int w = equip_def(b, EQ_WEAPON);
    return w == ITEM_NONE ? 3 : g_item_defs[w].dmg_min;
}

int item_weapon_damage_max(const Brother* b) {
    int w = equip_def(b, EQ_WEAPON);
    return w == ITEM_NONE ? 8 : g_item_defs[w].dmg_max;
}

int item_weapon_range(const Brother* b) {
    int w = equip_def(b, EQ_WEAPON);
    return w == ITEM_NONE ? 0 : g_item_defs[w].range;
}

bool item_is_ranged(const Brother* b) {
    int w = equip_def(b, EQ_WEAPON);
    if (w == ITEM_NONE) return false;
    return g_item_defs[w].wtype == WTYPE_BOW || g_item_defs[w].wtype == WTYPE_CROSSBOW;
}
