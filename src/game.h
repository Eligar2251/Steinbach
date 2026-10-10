/* ==========================================================================
   Steinbach — Saga of Mercenaries
   Тактико-ролевая игра в духе Battle Brothers: мир, города, отряд, RPG.
   game.h — общие типы и состояние игры.
   ========================================================================== */
#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ------------------------------------------------------------- лимиты --- */
#define MAX_BROTHERS      12
#define MAX_ITEMS         240
#define MAX_STOCK         14
#define MAX_RECRUITS      6
#define MAX_TOWNS         14
#define MAX_LOCATIONS     28
#define MAX_PATH          2048
#define MAX_CONTRACTS     24
#define MAX_ACTIVE        3
#define MAX_TRAITS        3
#define MAX_EVENT_CHOICES 3
#define MAX_BATTLE_LINES  48
#define NAME_LEN          64
#define TEXT_LEN          512

#define PERK_COUNT        12
#define N_BACKGROUNDS     16
#define N_TRAITS          18
#define N_INJURIES        8
#define N_EVENTS          12
#define N_ENEMIES         6

#define SAVE_VERSION      3

/* ---------------------------------------------------------- местность --- */
typedef enum {
    TERR_DEEP = 0,
    TERR_WATER,
    TERR_SAND,
    TERR_GRASS,
    TERR_FOREST,
    TERR_HILLS,
    TERR_MOUNTAIN,
    TERR_SWAMP,
    TERR_SNOW,
    TERR_COUNT
} TerrainType;

/* Стоимость передвижения в часах на тайл (0 — непроходимо). */
extern const int   g_terrain_cost[TERR_COUNT];
extern const char* g_terrain_name[TERR_COUNT];

/* ------------------------------------------------------------- города --- */
typedef enum {
    TOWN_VILLAGE = 0,
    TOWN_TOWN,
    TOWN_CITY,
    TOWN_CASTLE,
    TOWN_TYPE_COUNT
} TownType;

extern const char* g_town_type_name[TOWN_TYPE_COUNT];

enum {
    BLD_TAVERN   = 1 << 0,
    BLD_MARKET   = 1 << 1,
    BLD_TEMPLE   = 1 << 2,
    BLD_GUILD    = 1 << 3,
    BLD_FORGE    = 1 << 4,
};

/* ------------------------------------------------------------- локации -- */
typedef enum {
    LOC_RUINS = 0,
    LOC_CAMP,
    LOC_MINE,
    LOC_SHRINE,
    LOC_FARM,
    LOC_TYPE_COUNT
} LocType;

extern const char* g_loc_name[LOC_TYPE_COUNT];

/* -------------------------------------------------------------- предметы */
typedef enum {
    SLOT_NONE = 0,
    SLOT_HEAD,
    SLOT_BODY,
    SLOT_WEAPON,
    SLOT_OFFHAND,
    SLOT_SUPPLY,
    SLOT_COUNT
} ItemSlot;

typedef enum {
    WTYPE_NONE = 0,
    WTYPE_SWORD,
    WTYPE_AXE,
    WTYPE_MACE,
    WTYPE_SPEAR,
    WTYPE_POLEARM,
    WTYPE_BOW,
    WTYPE_CROSSBOW,
    WTYPE_SHIELD
} WeaponType;

typedef struct {
    const char* name;
    const char* desc;
    ItemSlot    slot;
    WeaponType  wtype;
    int         price;          /* базовая цена в кронах              */
    int         armor;          /* броня (шлем/тело)                   */
    int         dmg_min, dmg_max;
    int         fat_cost;       /* усталость за удар                   */
    int         armor_pen;      /* пробитие брони, %                   */
    int         range;          /* дальность (лук/арбалет)             */
    int         shield_def;     /* бонус защиты (щит)                  */
    int         init_penalty;   /* штраф инициативы                    */
    int         fat_penalty;    /* штраф выносливости (броня)          */
    int         two_handed;
} ItemDef;

#define ITEM_NONE (-1)

typedef struct {
    int def;         /* индекс ItemDef или ITEM_NONE                  */
    int durability;  /* текущая прочность                            */
    int ammo;        /* стрелы для луков                              */
} Item;

/* --------------------------------------------------------------- бойцы -- */
typedef struct {
    int hp;          /* здоровье                                       */
    int fatigue;     /* выносливость                                   */
    int resolve;     /* решимость                                      */
    int initiative;  /* инициатива                                     */
    int matk;        /* мастерство рукопашного боя                     */
    int ratk;        /* мастерство стрельбы                            */
    int mdef;        /* защита в рукопашной                            */
    int rdef;        /* защита от стрел                                */
} Stats;

typedef enum {
    STAT_HP = 0, STAT_FATIGUE, STAT_RESOLVE, STAT_INITIATIVE,
    STAT_MATK, STAT_RATK, STAT_MDEF, STAT_RDEF,
    STAT_COUNT
} StatId;

extern const char* g_stat_name[STAT_COUNT];

/* Слоты снаряжения бойца */
typedef enum {
    EQ_HEAD = 0, EQ_BODY, EQ_WEAPON, EQ_OFFHAND, EQ_COUNT
} EquipSlot;

typedef struct {
    char    name[NAME_LEN];
    int     background;                    /* индекс фона               */
    Stats   base;                          /* базовые характеристики    */
    int     level, xp;
    int     stat_picks;                    /* нераспределённые очки     */
    int     perk_points;
    uint32_t perks;                        /* битовая маска PERK_COUNT  */
    int     traits[MAX_TRAITS];
    int     n_traits;
    int     hp, fatigue;                   /* текущие                   */
    int     mood;                          /* 0..100                    */
    int     injury;                        /* -1 нет, иначе id          */
    int     injury_days;
    int     equip[EQ_COUNT];               /* индексы в инвентаре       */
    int     portrait;
    int     wage;                          /* жалование в день          */
    int     days_hired;
    int     kills, battles;
    bool    used_this_battle;
} Brother;

/* Фоны найма */
typedef struct {
    const char* name;
    const char* desc;
    Stats   stats_min, stats_max;
    int     wage;          /* базовое жалование                       */
    int     price_mult;    /* множитель цены найма /100               */
    int     weapon;        /* стартовое оружие (def)                  */
    int     offhand;
    int     head;
    int     body;
    int     trait_bias;    /* смещение к «военным» чертам             */
} Background;

/* Черты характера */
typedef struct {
    const char* name;
    const char* desc;
    Stats   mod;           /* модификатор характеристик              */
    int     price_mod;     /* % к цене найма                          */
    int     mood_mod;      /* ежедневный модификатор настроения       */
} TraitDef;

/* Перки */
typedef struct {
    const char* name;
    const char* desc;
} PerkDef;

/* Ранения */
typedef struct {
    const char* name;
    Stats mod;
    int   days_min, days_max;
} InjuryDef;

/* ------------------------------------------------------------ контракты - */
typedef enum {
    CONTRACT_DELIVERY = 0,  /* доставка груза в город                  */
    CONTRACT_HUNT,          /* зачистить логово (автобой)              */
    CONTRACT_PATROL,        /* посетить несколько точек                */
    CONTRACT_ESCORT,        /* сопроводить караван                     */
    CONTRACT_TYPE_COUNT
} ContractType;

extern const char* g_contract_type_name[CONTRACT_TYPE_COUNT];

typedef struct {
    bool  active;
    int   type;
    int   town_from;        /* город выдачи                            */
    int   town_to;          /* куда доставить (DELIVERY/ESCORT)        */
    int   loc;              /* локация (HUNT)                          */
    int   patrol_left;      /* сколько точек осталось (PATROL)         */
    int   patrol_target[4];
    int   reward_gold, reward_xp;
    int   reward_item;
    int   day_taken;
    int   days_limit;
    char  title[TEXT_LEN];
    char  desc[TEXT_LEN];
} Contract;

/* --------------------------------------------------------------- мир ---- */
typedef struct {
    int id;
    int x, y;                 /* тайловые координаты                    */
    char name[NAME_LEN];
    int type;                 /* TownType                               */
    int population;
    int prosperity;           /* 20..160, влияет на цены                */
    int buildings;            /* битовая маска BLD_*                    */
    Item stock[MAX_STOCK];
    int n_stock;
    int recruits[MAX_RECRUITS];   /* индексы во временной таблице       */
    int n_recruits;
    int recruit_day;              /* день обновления найма/рынка        */
    int contract_day;
    int visited;
} Town;

typedef struct {
    int id;
    int x, y;
    int type;                 /* LocType                                */
    char name[NAME_LEN];
    int enemy_power;          /* сила гарнизона                         */
    int cleared;              /* зачищена                               */
    int loot_gold;
    int loot_item;
} Location;

/* Путь */
typedef struct { int x, y; } TilePos;

/* ------------------------------------------------------------- события -- */
typedef struct {
    bool active;
    int  event_id;
    int  seed_offset;
} PendingEvent;

/* Отчёт об автобое */
typedef struct {
    bool active;
    bool won;
    char lines[MAX_BATTLE_LINES][128];
    int  n_lines;
    int  gold, xp;
    int  casualties;
    char title[NAME_LEN];
} BattleReport;

/* ------------------------------------------------------------- данные --- */
typedef struct {
    /* --- мир --- */
    int      world_w, world_h;
    uint8_t* terrain;      /* TerrainType на тайл                       */
    uint8_t* road;         /* 1 — дорога                                */
    uint8_t* decor;        /* вариант тайла                             */
    uint32_t seed;

    Town     towns[MAX_TOWNS];
    int      n_towns;
    Location locations[MAX_LOCATIONS];
    int      n_locations;

    /* --- отряд --- */
    Brother  brothers[MAX_BROTHERS];
    int      n_brothers;
    Item     items[MAX_ITEMS];
    int      n_items;
    int      crowns;
    int      food;         /* единицы провианта                         */

    /* --- время --- */
    int      day;
    int      hour;         /* 0..23                                     */

    /* --- положение отряда --- */
    float    party_x, party_y;      /* в тайлах                          */
    TilePos  path[MAX_PATH];
    int      path_len, path_pos;
    float    move_progress;         /* прогресс до следующего тайла      */
    float    speed_mult;            /* множитель скорости (эскорт)        */
    bool     traveling;

    /* --- контракты --- */
    Contract contracts[MAX_CONTRACTS];
    int      active_contracts[MAX_ACTIVE];
    int      n_active;

    /* --- модальные окна --- */
    BattleReport battle;
    PendingEvent pending_event;
    int      event_choice;          /* выбранный вариант, -1             */
    bool     levelup_open;

    /* --- мета --- */
    uint64_t rng;
    char     company_name[NAME_LEN];
    int      difficulty;            /* 0..2                              */
    int      renown;                /* репутация                         */
    int      battles_won, battles_lost;
    bool     game_over;
} GameState;

extern GameState g;

/* ================================================== реализуемые модули === */

/* rng.h */
uint64_t rng_next(void);
int      rng_range(int lo, int hi);        /* [lo, hi] включительно     */
float    rng_float(void);
int      rng_pick(const int* arr, int n);
void     rng_seed(uint64_t s);

/* names.c */
const char* gen_first_name(void);
const char* gen_nickname(void);
const char* gen_brother_name(char* buf, int n);
const char* gen_town_name(void);
const char* gen_loc_name(LocType t);
const char* gen_company_name(void);

/* worldgen.c */
bool  world_generate(int w, int h, uint32_t seed);
void  world_free(void);
int   terrain_at(int x, int y);
bool  road_at(int x, int y);
int   tile_move_cost(int x, int y);
bool  find_path(int x0, int y0, int x1, int y1, TilePos* out, int* out_len);

/* items.c */
extern const ItemDef g_item_defs[];
extern const int     g_item_def_count;
int  item_create(int def);
void item_remove(int idx);
int  item_total_armor(const Brother* b);
int  item_weapon_damage_min(const Brother* b);
int  item_weapon_damage_max(const Brother* b);
int  item_weapon_range(const Brother* b);
bool item_is_ranged(const Brother* b);
const char* slot_name(ItemSlot s);

/* units.c */
extern const Background g_backgrounds[N_BACKGROUNDS];
extern const TraitDef   g_traits[N_TRAITS];
extern const PerkDef    g_perks[PERK_COUNT];
extern const InjuryDef  g_injuries[N_INJURIES];
void  brother_clear(Brother* b);
void  brother_generate(Brother* b, int background, int level_hint);
void  brother_apply_traits(Brother* b);
int   brother_stat(const Brother* b, StatId s);
int   brother_power(const Brother* b);
void  brother_add_xp(Brother* b, int xp);
int   xp_needed(int level);
void  brother_levelup_apply(Brother* b, const int stat_picks[3], int perk);
void  brother_recalc_current(Brother* b);
const char* background_name(int idx);
int   brother_price(const Brother* b);
int   brother_wage(const Brother* b);

/* towns.c */
void  town_generate(Town* t, int id, int x, int y, int type);
void  towns_refresh_market(int town_idx);
void  towns_refresh_recruits(int town_idx);
void  towns_daily(void);
int   town_price_mult(const Town* t);
int   buy_price(const Town* t, int item_def);
int   sell_price(const Town* t, int item_def);

/* contracts.c */
void  contracts_refresh(void);
bool  contracts_accept(int contract_id);
bool  contracts_abandon(int contract_id);
void  contracts_check_arrival(void);
void  contracts_check_daily(void);
Contract* contract_get(int id);

/* events.c */
void  events_roll_daily(void);
void  events_resolve_choice(int choice);
bool  events_pending(void);
const char* event_title(void);
const char* event_text(void);
int   event_choice_count(void);
const char* event_choice_label(int i);
void  battle_autofight(BattleReport* br, int enemy_power, const char* enemy_name, bool scale_to_party);
void  battle_report_open(BattleReport* br);

/* towns.c — доступ к заявкам найма */
const Brother* town_recruit_brother(int town_idx, int rec_idx);
int  town_recruit_price(int town_idx, int rec_idx);
bool town_recruit_hire(int town_idx, int rec_idx, Brother* out);

/* econ.c */
void  econ_daily(void);
void  econ_start(void);

/* travel.c */
void  travel_set_dest(int x, int y);
void  travel_update(float dt);
void  travel_stop(void);
bool  travel_is_traveling(void);
float travel_hours_per_tile(int x, int y);

/* save.c */
bool  save_game(const char* path);
bool  load_game(const char* path);

/* game_logic.c */
bool  game_new(int world_w, int world_h, uint32_t seed, int difficulty);
void  game_free(void);
void  game_daily_tick(void);
int   game_add_item(int def);
void  game_remove_item(int idx);
bool  game_try_buy(int town_idx, int stock_idx);
bool  game_try_sell(int town_idx, int inv_idx);
bool  game_try_recruit(int town_idx, int rec_idx);
bool  game_try_heal(int town_idx, int brother_idx);
bool  game_try_repair(int town_idx, int inv_idx);
void  game_equip(int brother_idx, int inv_idx);
void  game_unequip(int brother_idx, int slot);
int   game_find_town_at(int tx, int ty);
int   game_find_location_at(int tx, int ty);
void  game_arrive_at(int x, int y);

/* утилиты */
static inline int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

#endif /* GAME_H */
