## AIPlayer.gd — общие действия ИИ; память основана только на наблюдаемых клетках.
class_name AIPlayer
extends Node

var faction_id: int = 0
var difficulty: String = "easy"
var memory: Dictionary = {}
var opportunities: int = 0


## Базовая реализация выполняет безопасный случайный ход.
func make_turn() -> void:
	play("easy")


## Общий цикл экономики и последовательных действий армии.
func play(mode: String) -> void:
	difficulty = mode
	var faction = GameManager.faction_manager.get_faction(faction_id)
	memory = faction.ai_memory
	opportunities = faction.ai_opportunities
	update_memory()
	manage_economy()
	for unit in GameManager.units.duplicate():
		if GameManager.finished:
			return
		if (
			not is_instance_valid(unit)
			or not GameManager.units.has(unit)
			or unit.faction_id != faction_id
		):
			continue
		if unit is Hero and mode != "easy":
			if ability_useful(unit):
				opportunities += 1
				if mode == "hard" or opportunities % Constants.MEDIUM_ABILITY_FREQUENCY == 0:
					unit.use_ability()
		var target = choose_target(unit)
		if target != null:
			GameManager.attack_unit(unit, target)
			continue
		var destination = choose_move(unit)
		if destination != unit.grid_pos:
			GameManager.move_unit(unit, destination)
		if not GameManager.units.has(unit):
			continue
		target = choose_target(unit)
		if target != null:
			GameManager.attack_unit(unit, target)
	update_memory()
	faction.ai_memory = memory
	faction.ai_opportunities = opportunities


## Наблюдение не получает скрытых позиций, HP, ресурсов или технологий противника.
func update_memory() -> void:
	var turn = GameManager.faction_manager.get_faction(faction_id).turns_started
	for key: int in memory.keys():
		var entry: Dictionary = memory[key]
		var p = Vector2i(int(entry.x), int(entry.y))
		if GameManager.fog.can_see(faction_id, p):
			var present = GameManager.unit_at(p)
			if (
				present == null
				or present.uid != key
				or not GameManager.fog.sees_unit(faction_id, present)
			):
				memory.erase(key)
		elif difficulty == "medium" and turn - int(entry.seen_turn) > Constants.AI_MEMORY_TURNS:
			memory.erase(key)
	for unit in GameManager.units:
		if unit.faction_id != faction_id and GameManager.fog.sees_unit(faction_id, unit):
			var entry = unit.to_dict()
			entry.seen_turn = turn
			memory[unit.uid] = entry


## Доступные противники; память не превращается в разрешение атаковать невидимую цель.
func choose_target(unit: Unit) -> Unit:
	var best: Unit = null
	var value = -INF
	for enemy in GameManager.units:
		if not GameManager.can_attack(unit, enemy):
			continue
		if difficulty == "easy" and Constants.distance(unit.grid_pos, enemy.grid_pos) > 1:
			continue
		if difficulty == "medium" and unit.attack + unit.attack_buff <= enemy.defense:
			continue
		var damage = unit.damage_to(
			enemy, int(Constants.TERRAIN_DEFENSE.get(GameManager.tiles[enemy.grid_pos].terrain, 0))
		)
		var rating = float(damage) + (Constants.CITY_SCORE if enemy.hp <= damage else 0)
		if rating > value:
			value = rating
			best = enemy
	return best


## Выбор движения к известным городам и границе исследования.
func choose_move(unit: Unit) -> Vector2i:
	var best = unit.grid_pos
	var value = -INF
	for p: Vector2i in GameManager.reachable(unit):
		if p == unit.grid_pos:
			continue
		var rating = _evaluate_tile(p)
		if difficulty == "easy":
			rating = randf() * Constants.CITY_SCORE + rating * 0.2
		if rating > value:
			value = rating
			best = p
	return best


## Оценивает только исследованные города, туман и известные угрозы.
func _evaluate_tile(p: Vector2i) -> float:
	var score = 0.0
	var nearest = GameManager.map_size * 2
	for tile: WorldTile in GameManager.tiles.values():
		if not GameManager.fog.knows(faction_id, tile.pos):
			continue
		if tile.terrain == "city" and tile.faction_id != faction_id:
			nearest = mini(nearest, Constants.distance(p, tile.pos))
			if tile.pos == p:
				score += Constants.CITY_SCORE * 2.0
	for delta in Constants.NEIGHBORS:
		if GameManager.tiles.has(p + delta) and not GameManager.fog.knows(faction_id, p + delta):
			score += Constants.LEVEL_SCORE
	score -= nearest
	# Не оставлять союзников на пути к еще занятой своей столице.
	if GameManager.tiles[p].terrain == "city" and GameManager.tiles[p].faction_id == faction_id:
		score -= Constants.LEVEL_SCORE
	return score


## Управление ресурсами: сложный ИИ копит на архитектуру, таверну и героя.
func manage_economy() -> void:
	var f = GameManager.faction_manager.get_faction(faction_id)
	var priorities: Array[String] = [
		"hunting",
		"farming",
		"riding",
		"construction",
		"architecture",
		"smithing",
		"mathematics",
		"sailing",
		"giants",
		"climbing",
		"trade",
		"medicine"
	]
	if difficulty == "hard":
		priorities = [
			"construction",
			"architecture",
			"hunting",
			"farming",
			"sailing",
			"riding",
			"smithing",
			"climbing",
			"mathematics",
			"giants",
			"trade",
			"medicine"
		]
	if difficulty == "easy":
		priorities.shuffle()
	for id in priorities:
		if f.techs.has(id):
			continue
		GameManager.tech_manager.research(faction_id, id)
		break
	for city: WorldTile in GameManager.tiles.values():
		if city.faction_id != faction_id or city.terrain != "city":
			continue
		if difficulty != "easy":
			GameManager.resource_manager.upgrade(city)
			GameManager.resource_manager.build_tavern(city)
			if city.tavern and GameManager.hero_for(faction_id) == null:
				var hero_id = (
					f.fallen_hero
					if not f.fallen_hero.is_empty()
					else str(GameManager.hero_definitions.keys().pick_random())
				)
				GameManager.recruit(hero_id, city)
		var reserve = (
			Constants.TAVERN_COST
			if (
				difficulty == "hard"
				and f.techs.has("architecture")
				and GameManager.hero_for(faction_id) == null
			)
			else 0
		)
		if f.stars < reserve + int(GameManager.unit_definitions.warrior.cost):
			continue
		if difficulty == "hard" and f.techs.has("sailing"):
			GameManager.recruit("ship", city)
		for type_id in ["giant", "knight", "rider", "archer", "warrior"]:
			if GameManager.recruit(type_id, city):
				break


## Проверяет подходящий момент способности, не расходуя ее без эффекта.
func ability_useful(hero: Hero) -> bool:
	if hero.cooldown > 0:
		return false
	match hero.type_id:
		"hero_sage":
			return not GameManager.tech_manager.available(faction_id).is_empty()
		"hero_beastmaster":
			return true
		"hero_assassin":
			return not memory.is_empty()
		"hero_warlord":
			for ally in GameManager.units:
				if (
					ally.faction_id == faction_id
					and Constants.distance(hero.grid_pos, ally.grid_pos) <= Constants.VISION
				):
					if choose_target(ally) != null:
						return true
	return false
