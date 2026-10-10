/* battle.h — тактический бой на квадратной сетке (Battle Brothers-style).
 *
 * Поле: BT_W x BT_H клеток, ход по инициативе каждый раунд.
 * У бойца: AP (2 действия) + очки перемещения. Удар/выстрел — 1 AP.
 * Урон: сначала броня (с учётом пробития), переплат — в здоровье.
 */
#ifndef BATTLE_H
#define BATTLE_H

#include "game.h"
#include <stdbool.h>

#define BT_W 14
#define BT_H 9
#define BT_MAX_UNITS 24
#define BT_LOG 7

/* Тип поверхности поля боя */
enum {
    BT_T_GRASS = 0, BT_T_DIRT, BT_T_STONE, BT_T_SAND, BT_T_FOREST,
    BT_T_TYPES
};

/* Анимация текущего действия */
enum {
    BT_ANIM_NONE = 0,
    BT_ANIM_MOVE,
    BT_ANIM_LUNGE,
    BT_ANIM_SHOOT,
    BT_ANIM_DIE
};

/* Контекст, породивший бой */
enum {
    BT_CTX_EVENT = 0,
    BT_CTX_LOCATION,
    BT_CTX_CONTRACT
};

/* Фракция врага — влияет на спрайты и статы */
enum {
    BT_F_BANDITS = 0,     /* красные   */
    BT_F_RAIDERS,         /* зелёные   */
    BT_F_GUARDS,          /* серые     */
    BT_F_CULTISTS,        /* серые + фиолет */
    BT_F_DESERTERS,       /* красные + тень  */
    BT_F_MILITIA,         /* жёлтые          */
    BT_F_COUNT
};

typedef struct {
    bool used;
    bool alive;
    bool fled;
    bool enemy;
    int  x, y;
    float fx, fy;                 /* плавная позиция для анимации       */
    int  brother;                 /* индекс в g.brothers или -1         */
    char name[NAME_LEN];
    int  hp, hp_max, fat, fat_max;
    int  armor_head, armor_body;  /* остаток прочности брони            */
    int  armor_head_max, armor_body_max;
    int  matk, ratk, mdef, rdef, ini, res;
    int  w_min, w_max, w_pen, w_range, wtype;
    int  shield;
    int  move_left, ap;
    int  move_max;
    int  sprite;                  /* 0..23 → mr_unit_01..24             */
    int  team;                    /* 0 игрок, 1 враг                    */
    int  waver;                   /* мораль: потрясений подряд          */
    bool defending;               /* «оборона»: +защита до след. хода   */
    int  kills;
    int  start_hp;                /* для отчёта                         */
} BTUnit;

typedef struct {
    bool active;
    bool finished;
    bool victory;
    bool retreated;

    int   terrain;
    uint8_t cell[BT_H][BT_W];
    bool  blocked[BT_H][BT_W];

    BTUnit units[BT_MAX_UNITS];
    int  n;
    int  order[BT_MAX_UNITS];
    int  turn_i;
    int  round;
    int  sel;                     /* выбранный юнит игрока, -1          */

    int   anim;
    float anim_t;
    int   anim_a, anim_b;
    int   float_dmg;
    float float_t;
    int   float_x, float_y;
    bool  float_crit;

    char  log[BT_LOG][112];
    int   n_log;

    /* контекст */
    int   ctx, ctx_idx;
    int   loc_idx;
    char  pre[6][128];
    int   n_pre;
    char  enemy_name[NAME_LEN];
    int   enemy_power;
    bool  scale;
    int   kills_player, kills_enemy;
    int   faction;

    /* таймер ИИ */
    float ai_delay;
} BattleState;

extern BattleState bt;

/* Настройка боя: ctx — BT_CTX_*, enemy_power — базовая сила врага. */
void bt_setup(int ctx, int ctx_idx, int faction, int enemy_power,
              const char* enemy_name, int terrain, bool scale);
/* Старт: расстановка, порядок ходов, первый ход. */
void bt_begin(void);
/* Полный запуск тактического боя (setup + begin). loc_idx — локация (-1 нет). */
void bt_launch(int ctx, int ctx_idx, int loc_idx, int faction, int enemy_power,
               const char* enemy_name, int terrain, bool scale);
/* Строка-преамбула в отчёт (текст события перед боем). */
void bt_pre_add(const char* line);

int   bt_alive_enemy(void);
int   bt_alive_player(void);
int   bt_current(void);                  /* юнит, чей ход, или -1        */
bool  bt_is_player_turn(void);

void  bt_reachable(int u, bool vis[BT_H][BT_W]);
bool  bt_walkable(int x, int y);
bool  bt_attack_possible(int a, int b);  /* в радиусе + линия огня       */
int   bt_hit_chance(int a, int b);

/* Действия. Возврат: false — действие невозможно. */
bool  bt_move(GameState* gs, int u, int x, int y);
bool  bt_attack(GameState* gs, int a, int b);
void  bt_defend(GameState* gs, int u);
void  bt_wait(GameState* gs, int u);
void  bt_retreat(GameState* gs);

/* Конец хода текущего юнита → следующий по порядку. */
void  bt_end_turn(GameState* gs);
/* Шаг ИИ текущего вражеского юнита (вызывать по таймеру). */
void  bt_ai_act(GameState* gs);

/* Прогресс анимации; true — анимация завершена в этом кадре. */
bool  bt_anim_tick(float dt);

/* Проверка конца боя; если всё решено — применить результаты. */
bool  bt_check_end(GameState* gs);
/* Заполнить g.battle отчётом и применить последствия. */
void  bt_finish(GameState* gs);

void  bt_logf(const char* fmt, ...);

#endif
