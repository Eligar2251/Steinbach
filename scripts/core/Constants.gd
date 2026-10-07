## Constants.gd — общие правила и параметры прототипа.
extends Node

const TILE_SIZE: int = 64
const START_STARS: int = 5
const VISION: int = 2
const MAX_ROUNDS: int = 30
const MAX_CITY_LEVEL: int = 4
const TAVERN_COST: int = 8
const HERO_REVIVE_TURNS: int = 5
const HERO_XP: Array[int] = [3, 6, 12, 20]
const KILL_XP: int = 2
const CAPTURE_XP: int = 3
const RUINS_STARS: int = 3
const VILLAGE_STARS: int = 1
const CITY_UPGRADE_COST: int = 3
const POPULATION_PER_LEVEL: int = 2
const CITY_RADIUS: int = 2
const WAR_CRY_BONUS: int = 2
const SHADOW_TURNS: int = 2
const BEAST_TURNS: int = 3
const PERK_HP: int = 2
const PERK_STAT: int = 1
const MIN_DAMAGE: int = 1
const SEARCH_WIDTH: int = 6
const AI_DELAYS: Dictionary = {"easy": 1.5, "medium": 1.0, "hard": 0.5}
const TERRAIN: Array[String] = [
	"grass", "forest", "mountain", "water", "desert", "snow", "city", "village", "ruins"
]
const TERRAIN_NAMES: Dictionary = {
	"grass": "Трава",
	"forest": "Лес",
	"mountain": "Горы",
	"water": "Вода",
	"desert": "Пустыня",
	"snow": "Снег",
	"city": "Город",
	"village": "Деревня",
	"ruins": "Руины"
}
const TERRAIN_DEFENSE: Dictionary = {"forest": 1, "mountain": 2}
const NEIGHBORS: Array[Vector2i] = [Vector2i.LEFT, Vector2i.RIGHT, Vector2i.UP, Vector2i.DOWN]
const CITY_SCORE: int = 10
const LEVEL_SCORE: int = 5
const UNIT_SCORE: int = 2
const HERO_SCORE: int = 8
const SAVE_VERSION: int = 1
const SAVE_SLOTS: int = 8
const SAVE_DB_PATH: String = "user://saves/hexempire_saves.json"
const SLOW_TERRAIN_COST: int = 2
const BASE_CITY_CAP: int = 2
const CONSTRUCTION_CITY_CAP: int = 3
const TAVERN_MIN_LEVEL: int = 3
const DESERT_BONUS_PERIOD: int = 2
const MAX_HERO_LEVEL: int = 5
const HERO_REVIVE_DIVISOR: int = 2
const EVALUATOR_TECH_WEIGHT: int = 3
const AI_MEMORY_TURNS: int = 3
const MEDIUM_ABILITY_FREQUENCY: int = 2
const MAX_SAVE_BYTES: int = 1048576
const MAX_SAVE_STARS: int = 1000000
const MAX_SAVE_HP: int = 1000
const MAX_SAVE_STAT: int = 100
const MAX_SAVE_RANGE: int = 24
const MAX_MEMORY_ENTRIES: int = 576
const MAP_TERRAIN_THRESHOLDS: Dictionary = {
	"water": 0.12,
	"mountain": 0.22,
	"forest": 0.42,
	"snow": 0.52,
	"desert": 0.62,
	"village": 0.65,
	"ruins": 0.68
}
const START_INSET: int = 1


## Расстояние на квадратной сетке (без движения по диагонали).
func distance(a: Vector2i, b: Vector2i) -> int:
	return absi(a.x - b.x) + absi(a.y - b.y)


## Загрузка справочников исключительно через FileAccess.
func read_json(path: String) -> Variant:
	if not FileAccess.file_exists(path):
		push_error("Не найден JSON: " + path)
		return null
	return JSON.parse_string(FileAccess.get_file_as_string(path))
