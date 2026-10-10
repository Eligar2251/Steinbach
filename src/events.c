/* events.c — случайные события в пути, автобои, отчёты о сражениях. */
#include "game.h"
#include "battle.h"
#include "rng.h"
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------- автобой ------- */

void battle_autofight(BattleReport* br, int enemy_power, const char* enemy_name, bool scale_to_party) {
    memset(br, 0, sizeof(*br));
    br->active = true;
    snprintf(br->title, sizeof(br->title), "Бой: %s", enemy_name);

    int party_power = 0;
    int n_ready = 0;
    for (int i = 0; i < g.n_brothers; i++) {
        party_power += brother_power(&g.brothers[i]);
        n_ready++;
    }
    if (n_ready == 0) {
        snprintf(br->lines[br->n_lines++], 128, "Некому сражаться — вы бежите с поля боя.");
        br->won = false;
        return;
    }
    if (scale_to_party) {
        /* масштабируем врага под отряд, чтобы бой был не всегда trivial */
        int scaled = party_power * (70 + rng_range(0, 70)) / 100;
        if (scaled < enemy_power) enemy_power = (enemy_power + scaled) / 2;
        else enemy_power = scaled * 80 / 100;
        if (enemy_power < 20) enemy_power = 20;
    }

    snprintf(br->lines[br->n_lines++], 128, "Ваши силы: %d. Сила врага: %d.", party_power, enemy_power);
    snprintf(br->lines[br->n_lines++], 128, "Отряд «%s» вступает в бой с врагом: %s.", g.company_name, enemy_name);

    /* несколько раундов обмена ударами */
    int pp = party_power, ep = enemy_power;
    int rounds = 0;
    bool won = false;
    while (rounds < 8) {
        rounds++;
        int pdmg = (int)(pp * (0.16f + rng_float() * 0.14f));
        int edmg = (int)(ep * (0.13f + rng_float() * 0.13f));
        ep -= pdmg;
        pp -= edmg;
        if (ep <= 0) { won = true; break; }
        if (pp <= 0) { won = false; break; }
        if (br->n_lines < MAX_BATTLE_LINES) {
            snprintf(br->lines[br->n_lines++], 128, "Раунд %d: вы наносите %d урона, враг — %d.",
                     rounds, pdmg, edmg);
        }
    }
    if (ep <= 0) won = true;
    if (pp <= 0) won = false;

    /* распределение урона по бойцам */
    int share = won ? 4 + rng_range(0, 6) : 10 + rng_range(0, 12);
    for (int i = 0; i < g.n_brothers; i++) {
        Brother* b = &g.brothers[i];
        b->battles++;
        int dmg = share + rng_range(0, 10);
        b->hp -= dmg;
        if (b->hp < 1) {
            /* тяжёлая рана, но не смерть (в этой фазе игры) */
            b->hp = 1;
            b->injury = rng_range(0, N_INJURIES - 1);
            b->injury_days = g_injuries[b->injury].days_min +
                             rng_range(0, g_injuries[b->injury].days_max - g_injuries[b->injury].days_min);
            br->casualties++;
            if (br->n_lines < MAX_BATTLE_LINES)
                snprintf(br->lines[br->n_lines++], 128, "%s тяжело ранен (%s).", b->name, g_injuries[b->injury].name);
        }
        b->fatigue = clampi(b->fatigue - rng_range(10, 30), 0, brother_stat(b, STAT_FATIGUE));
    }

    if (won) {
        br->won = true;
        br->xp = 40 + enemy_power / 2;
        br->gold = enemy_power * 2 + rng_range(10, 60);
        g.crowns += br->gold;
        g.battles_won++;
        if (br->n_lines < MAX_BATTLE_LINES)
            snprintf(br->lines[br->n_lines++], 128, "ПОБЕДА! Добыча: %d крон.", br->gold);
        for (int i = 0; i < g.n_brothers; i++) {
            brother_add_xp(&g.brothers[i], br->xp);
            g.brothers[i].mood = clampi(g.brothers[i].mood + 4, 0, 100);
            g.brothers[i].kills += 1;
            if (g.brothers[i].perks & (1u << 7))
                g.brothers[i].fatigue = brother_stat(&g.brothers[i], STAT_FATIGUE);
        }
    } else {
        br->won = false;
        br->xp = 15;
        br->gold = 0;
        g.battles_lost++;
        if (br->n_lines < MAX_BATTLE_LINES)
            snprintf(br->lines[br->n_lines++], 128, "ПОРАЖЕНИЕ. Отряд отступает, потеряв %d крон.",
                     50 + enemy_power);
        g.crowns -= 50 + enemy_power;
        if (g.crowns < 0) g.crowns = 0;
        for (int i = 0; i < g.n_brothers; i++) {
            brother_add_xp(&g.brothers[i], br->xp);
            g.brothers[i].mood = clampi(g.brothers[i].mood - 8, 0, 100);
        }
    }
}

void battle_report_open(BattleReport* br) {
    g.battle = *br;
    g.battle.active = true;
}

/* ------------------------------------------------------- события ------- */

typedef struct {
    const char* title;
    const char* text;
    const char* choices[MAX_EVENT_CHOICES];
    /* 0 - без проверки; иначе StatId + 1 */
    int  test_stat[MAX_EVENT_CHOICES];
    int  difficulty[MAX_EVENT_CHOICES];
    const char* success_text[MAX_EVENT_CHOICES];
    const char* fail_text[MAX_EVENT_CHOICES];
    /* эффекты успеха/провала: золото, настроение, еда */
    int  gold_s[MAX_EVENT_CHOICES], gold_f[MAX_EVENT_CHOICES];
    int  mood_s[MAX_EVENT_CHOICES], mood_f[MAX_EVENT_CHOICES];
    int  food_s[MAX_EVENT_CHOICES], food_f[MAX_EVENT_CHOICES];
    int  battle_s[MAX_EVENT_CHOICES];   /* 1 — бой при успехе, -1 при провале */
} EventDef;

static const EventDef EVENTS[N_EVENTS] = {
    {
        "Волчья стая",
        "Ночью отряд слышит рычание. Стая волков окружила лагерь, глаза светятся в темноте.",
        { "Сражаться", "Откупиться мясом", "Стоять тихо" },
        { 0, 0, STAT_RESOLVE + 1 }, { 0, 0, 45 },
        { "", "", "Вы не двигались до рассвета — волки ушли." },
        { "", "", "Волки напали среди ночи!" },
        { 40, -30, 0 }, { -20, 0, -10 },
        { 0, -4, 0 }, { 0, 0, 0 },
        { 1, 0, -1 },
    },
    {
        "Разбитая повозка",
        "У дороги стоит опрокинутая повозка. Кто-то разбился или бежал.",
        { "Обыскать", "Пройти мимо", "Искать владельцев" },
        { 0, 0, STAT_INITIATIVE + 1 }, { 0, 0, 40 },
        { "Вы нашли немного монет и припасы.", "", "Вы нашли раненого торговца и помогли ему." },
        { "В повозке только мусор.", "", "Ничего не нашли." },
        { 60, 0, 30 }, { 0, 0, 0 },
        { 0, 0, 2 }, { 0, 0, 0 },
        { 2, 0, 0 }, { 0, 0, 0 },
        { 0, 0, 0 },
    },
    {
        "Бандиты на дороге",
        "Четверо вооружённых перекрыли дорогу и требуют «пошлину».",
        { "Платить", "Сражаться", "Угрожать" },
        { 0, 0, STAT_RESOLVE + 1 }, { 0, 0, 50 },
        { "", "Вы разогнали бандитов!", "Бандиты струсили и убежали." },
        { "", "", "Они не испугались и напали." },
        { -60, 80, 30 }, { -60, -40, -20 },
        { 0, 0, 0 }, { 0, 0, 0 },
        { 0, 1, 0 }, { 0, 0, -1 },
    },
    {
        "Странник у костра",
        "Одинокий путник греется у костра. Он предлагает сухари за разговор.",
        { "Присесть", "Купить сухари", "Пройти мимо" },
        { 0, 0, 0 }, { 0, 0, 0 },
        { "Путник рассказал о землях вокруг.", "", "" },
        { "", "", "" },
        { 0, -20, 0 }, { 0, 0, 0 },
        { 3, 0, 0 }, { 0, 0, 0 },
        { 0, 0, 0 }, { 0, 0, 0 },
        { 0, 0, 0 },
    },
    {
        "Ливень",
        "Дождь хлещет с неба. Дорога превратилась в грязь.",
        { "Идти дальше", "Ставить лагерь", "Искать укрытие" },
        { STAT_FATIGUE + 1, 0, STAT_INITIATIVE + 1 }, { 45, 0, 40 },
        { "Отряд продавился сквозь грязь.", "", "Вы нашли сарай." },
        { "Вы вымотались в грязи.", "", "Промокли до нитки." },
        { 0, 0, 0 }, { 0, 0, -15 },
        { -3, 1, 0 }, { -6, 0, -3 },
        { 0, 0, 0 },
    },
    {
        "Торговец в беде",
        "Торговца грабят разбойники. Он зовёт на помощь.",
        { "Помочь", "Требовать награду", "Пройти мимо" },
        { 0, STAT_MATK + 1, 0 }, { 0, 45, 0 },
        { "Торговец благодарен и отдал товары.", "Торговец заплатил щедро.", "" },
        { "", "Разбойники не оценили наглость — драка!", "" },
        { 20, 120, 0 }, { -20, -30, -5 },
        { 2, 0, 0 }, { 0, 0, 0 },
        { 0, 0, 0 }, { 0, -1, 0 },
    },
    {
        "Дезертир",
        "Из кустов вышел уставший солдат и просит еды и работы.",
        { "Нанять (50 крон)", "Дать еды", "Прогнать" },
        { 0, 0, 0 }, { 0, 0, 0 },
        { "К вам присоединился новый боец!", "Солдат ушёл с благодарностью.", "" },
        { "Не хватило крон.", "", "" },
        { -50, 0, 0 }, { 0, 0, 0 },
        { 2, 1, -1 }, { 0, 0, 0 },
        { 0, 0, 0 }, { 0, 0, 0 },
    },
    {
        "Дурное знамение",
        "Ночью погасли все звёзды. Отряд встревожен.",
        { "Молиться", "Смеяться над суевериями", "Гнать отряд дальше" },
        { STAT_RESOLVE + 1, STAT_RESOLVE + 1, STAT_FATIGUE + 1 }, { 40, 55, 45 },
        { "Настроение укрепилось.", "Бойцы оценили вашу смелость.", "Отряд не заметил ничего дурного." },
        { "Тревога не отпускает.", "Кто-то поверил в дурное.", "Отряд вымотался." },
        { 0, 0, 0 }, { 0, -10, -5 },
        { 3, 2, -3 }, { -3, -2, -5 },
        { 0, 0, 0 },
    },
    {
        "Заброшенный хутор",
        "Посреди полей стоит брошенный дом. Дверь приоткрыта.",
        { "Обыскать", "Осторожно войти", "Идти дальше" },
        { 0, STAT_RDEF + 1, 0 }, { 0, 40, 0 },
        { "В сундуке нашлись монеты.", "Нашли немного еды.", "" },
        { "Ловушка! Вы ранены.", "", "" },
        { 70, 0, 0 }, { -10, 0, 0 },
        { 0, 2, 0 }, { -8, 0, 0 },
        { 3, 0, 0 }, { 0, 0, 0 },
    },
    {
        "Пьяный трактирщик",
        "На привале объявился трактирщик с бочкой. Угощает за истории.",
        { "Пить и петь", "Купить бочку", "Отказаться" },
        { 0, 0, 0 }, { 0, 0, 0 },
        { "Отряд доволен.", "Хорошее вино в дорогу.", "" },
        { "", "", "" },
        { 0, -40, 0 }, { 0, 0, 0 },
        { 6, 2, 0 }, { 0, 0, 0 },
        { 0, 0, 0 }, { 0, 0, 0 },
    },
    {
        "Засада в тумане",
        "Из тумана свистят стрелы. Кто-то напал на отряд!",
        { "В бой!", "Отступать", "Искать врага" },
        { STAT_MATK + 1, STAT_INITIATIVE + 1, STAT_RATK + 1 }, { 45, 45, 45 },
        { "Вы отбили нападение.", "Вы ушли от боя.", "Вы вычислили врага и ударили первыми." },
        { "Стрелы ранили бойцов.", "Отступление далось дорого.", "Засада застала врасплох." },
        { 50, 0, 40 }, { -30, -20, -30 },
        { 0, -2, 0 }, { -8, -4, -8 },
        { 1, 0, 1 }, { -1, -1, -1 },
    },
    {
        "Находка у дороги",
        "В траве блестит что-то металлическое.",
        { "Поднять", "Проверить ловушки", "Пройти мимо" },
        { 0, STAT_MDEF + 1, 0 }, { 0, 45, 0 },
        { "Вы нашли старый кошель с монетами.", "Под кошельком была западня, но вы целы.", "" },
        { "", "Ловушка! Кто-то ранен.", "" },
        { 45, 25, 0 }, { -15, 0, 0 },
        { 0, 1, 0 }, { -6, 0, 0 },
        { 0, 0, 0 }, { 0, 0, 0 },
    },
};

static int s_event_id = -1;
static int s_event_choice_result = -2;  /* -2 не выбрано, -1 без проверки */

bool events_pending(void) {
    return g.pending_event.active && s_event_id >= 0 && !g.battle.active;
}

const char* event_title(void) {
    return s_event_id >= 0 ? EVENTS[s_event_id].title : "";
}

const char* event_text(void) {
    return s_event_id >= 0 ? EVENTS[s_event_id].text : "";
}

int event_choice_count(void) {
    return MAX_EVENT_CHOICES;
}

const char* event_choice_label(int i) {
    if (s_event_id < 0 || i < 0 || i >= MAX_EVENT_CHOICES) return "";
    return EVENTS[s_event_id].choices[i];
}

void events_roll_daily(void) {
    if (g.pending_event.active || g.battle.active) return;
    if (g.traveling && rng_float() < 0.55f) {
        s_event_id = rng_range(0, N_EVENTS - 1);
        s_event_choice_result = -2;
        g.pending_event.active = true;
        g.pending_event.event_id = s_event_id;
    }
}

static void apply_mood_all(int v) {
    for (int i = 0; i < g.n_brothers; i++)
        g.brothers[i].mood = clampi(g.brothers[i].mood + v, 0, 100);
}

void events_resolve_choice(int choice) {
    if (s_event_id < 0 || choice < 0 || choice >= MAX_EVENT_CHOICES) return;
    const EventDef* ev = &EVENTS[s_event_id];
    const char* result_text = NULL;
    bool success = true;

    int test = ev->test_stat[choice];
    if (test > 0) {
        StatId st = (StatId)(test - 1);
        /* проверка: средняя характеристика отряда против сложности */
        int avg = 0;
        int n = g.n_brothers > 0 ? g.n_brothers : 1;
        for (int i = 0; i < g.n_brothers; i++) avg += brother_stat(&g.brothers[i], st);
        avg = g.n_brothers ? avg / n : 40;
        int roll = rng_range(0, 99);
        int target = ev->difficulty[choice] - (avg - 45);
        success = roll >= target;
    }
    result_text = success ? ev->success_text[choice] : ev->fail_text[choice];

    int gold = success ? ev->gold_s[choice] : ev->gold_f[choice];
    int mood = success ? ev->mood_s[choice] : ev->mood_f[choice];
    int food = success ? ev->food_s[choice] : ev->food_f[choice];
    int battle = success ? ev->battle_s[choice] : -ev->battle_s[choice];
    if (battle < -1) battle = -1;

    g.crowns = clampi(g.crowns + gold, 0, 999999);
    g.food = clampi(g.food + food, 0, 9999);
    apply_mood_all(mood);

    /* спец-случай: наём дезертира (событие 6, вариант 0) */
    if (s_event_id == 6 && choice == 0 && success && g.n_brothers < MAX_BROTHERS) {
        Brother* b = &g.brothers[g.n_brothers++];
        brother_generate(b, 5, 1);
        b->mood = 60;
    }

    /* вывод результата в окно */
    BattleReport* br = &g.battle;
    memset(br, 0, sizeof(*br));
    br->active = true;
    br->won = success;
    snprintf(br->title, sizeof(br->title), "%s", ev->title);
    snprintf(br->lines[br->n_lines++], 128, "%s", result_text && result_text[0] ? result_text :
             (success ? "Всё прошло удачно." : "Всё пошло не так."));
    if (gold != 0) snprintf(br->lines[br->n_lines++], 128, "Кроны: %+d.", gold);
    if (food != 0) snprintf(br->lines[br->n_lines++], 128, "Провиант: %+d.", food);
    if (mood != 0) snprintf(br->lines[br->n_lines++], 128, "Настроение отряда: %+d.", mood);

    g.pending_event.active = false;
    s_event_id = -1;

    if (battle != 0) {
        /* тактический бой: запускаем поле, преамбула события — в отчёт */
        BattleReport tmp = *br;
        static const char* BATTLE_ENEMIES[] = {
            "Разбойники", "Бандиты", "Дезертиры", "Ополчение"
        };
        int fi = rng_range(0, 3);
        const char* ename = BATTLE_ENEMIES[fi];
        int terrain = BT_T_GRASS;
        if (g.terrain[(int)(g.party_y) * g.world_w + (int)(g.party_x)] == TERR_FOREST)
            terrain = BT_T_FOREST;
        bt_launch(BT_CTX_EVENT, -1, -1,
                  fi == 0 ? BT_F_RAIDERS : (fi == 1 ? BT_F_BANDITS :
                   (fi == 2 ? BT_F_DESERTERS : BT_F_MILITIA)),
                  30 + rng_range(0, 60), ename, terrain, true);
        for (int i = 0; i < tmp.n_lines; i++) bt_pre_add(tmp.lines[i]);
    }
}
