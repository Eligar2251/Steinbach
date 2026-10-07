## Faction.gd — состояние игрока и таймер возрождения героя.
class_name Faction
extends RefCounted

var id: int = 0
var human: bool = false
var difficulty: String = "medium"
var stars: int = Constants.START_STARS
var techs: Array[String] = []
var hero_cooldown: int = 0
var fallen_hero: String = ""
var turns_started: int = 0
var eliminated: bool = false
var ai_memory: Dictionary = {}
var ai_opportunities: int = 0


## Цвет фракции из справочника.
func color() -> Color:
	return Color(str(GameManager.faction_definitions[id].color))


## Название фракции из справочника.
func display_name() -> String:
	return str(GameManager.faction_definitions[id].name)


## Запись состояния игрока.
func to_dict() -> Dictionary:
	var serialized_memory: Dictionary = {}
	for key: int in ai_memory:
		serialized_memory[str(key)] = ai_memory[key]
	return {
		"id": id,
		"human": human,
		"difficulty": difficulty,
		"stars": stars,
		"techs": techs,
		"hero_cooldown": hero_cooldown,
		"fallen_hero": fallen_hero,
		"turns_started": turns_started,
		"eliminated": eliminated,
		"ai_memory": serialized_memory,
		"ai_opportunities": ai_opportunities
	}


## Чтение состояния игрока.
static func from_dict(d: Dictionary) -> Faction:
	var f = Faction.new()
	f.id = int(d.id)
	f.human = bool(d.human)
	f.difficulty = str(d.difficulty)
	f.stars = int(d.stars)
	f.techs.assign(d.techs)
	f.hero_cooldown = int(d.hero_cooldown)
	f.fallen_hero = str(d.fallen_hero)
	f.turns_started = int(d.turns_started)
	f.eliminated = bool(d.eliminated)
	for key: String in d.get("ai_memory", {}):
		f.ai_memory[int(key)] = d.ai_memory[key]
	f.ai_opportunities = int(d.get("ai_opportunities", 0))
	return f
