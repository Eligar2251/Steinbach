## GameManager.gd — состояние партии и единые проверки всех игровых действий.
extends Node

var tiles: Dictionary = {}
var units: Array[Unit] = []
var factions: Array[Faction] = []
var unit_definitions: Dictionary = {}
var hero_definitions: Dictionary = {}
var faction_definitions: Dictionary = {}
var tech_definitions: Dictionary = {}
var presets: Dictionary = {}
var map_size: int = 10
var map_seed: int = 0
var round_number: int = 1
var turn_index: int = 0
var next_uid: int = 1
var finished: bool = false
var winner_id: int = -1
var active: bool = false
var busy: bool = false
var session: int = 0
var unit_container: Node2D = null
var fog: FogOfWar = FogOfWar.new()
var faction_manager: FactionManager = FactionManager.new()
var resource_manager: ResourceManager = ResourceManager.new()
var tech_manager: TechManager = TechManager.new()
var factory: UnitFactory = UnitFactory.new()
var save_db: SaveDatabase = SaveDatabase.new()
var turn_manager: TurnManager


## Загружает справочники и создает контроллер ходов.
func _ready() -> void:
	for entry: Dictionary in Constants.read_json("res://data/units.json"):
		unit_definitions[str(entry.id)] = entry
	for entry: Dictionary in Constants.read_json("res://data/heroes.json"):
		hero_definitions[str(entry.id)] = entry
	for entry: Dictionary in Constants.read_json("res://data/factions.json"):
		faction_definitions[int(entry.id)] = entry
	for entry: Dictionary in Constants.read_json("res://data/techs.json"):
		tech_definitions[str(entry.id)] = entry
	presets = Constants.read_json("res://data/map_presets.json")
	turn_manager = TurnManager.new()
	add_child(turn_manager)


## Удаляет старую партию, отменяя ожидающие асинхронные ходы.
func clear_game() -> void:
	session += 1
	for u in units:
		if is_instance_valid(u):
			u.free()
	units.clear()
	tiles.clear()
	factions.clear()
	fog = FogOfWar.new()
	busy = false
	active = false
	finished = false
	winner_id = -1
	unit_container = null


## Начинает партию по настройкам лобби; начальный доход начисляется сразу.
func new_game(preset: String, slots: Array, seed_value: int = -1) -> void:
	clear_game()
	map_size = int(presets[preset].size)
	map_seed = seed_value if seed_value >= 0 else randi()
	factions = faction_manager.create(slots)
	var ids: Array[int] = []
	for f in factions:
		ids.append(f.id)
	var generated = MapGenerator.new().generate(map_size, ids, map_seed)
	tiles = generated.tiles
	turn_index = 0
	round_number = 1
	next_uid = 1
	active = true
	for i in factions.size():
		spawn("warrior", factions[i].id, generated.starts[i])
	begin_current_turn()


## Присоединяет юнитов к новой игровой сцене.
func attach_units(container: Node2D) -> void:
	unit_container = container
	for u in units:
		if u.get_parent():
			u.reparent(container)
		else:
			container.add_child(u)
	refresh_visibility()


## Id активного игрока.
func current_faction_id() -> int:
	return factions[turn_index].id if not factions.is_empty() else -1


## Начисляет доход и сбрасывает действия только активной фракции.
func begin_current_turn() -> void:
	var f = faction_manager.get_faction(current_faction_id())
	if f == null:
		return
	f.turns_started += 1
	f.hero_cooldown = maxi(0, f.hero_cooldown - 1)
	f.stars += resource_manager.income(f.id)
	for u in units.duplicate():
		if u.faction_id != f.id:
			continue
		u.begin_turn()
		if not units.has(u):
			continue
		var t: WorldTile = tiles[u.grid_pos]
		if t.terrain == "city" and t.faction_id == f.id and f.techs.has("medicine"):
			u.hp = mini(u.max_hp, u.hp + Constants.PERK_HP)
		u.queue_redraw()
	refresh_visibility()


## Вычисляет видимость и уведомляет UI.
func refresh_visibility() -> void:
	for f in factions:
		fog.update(f.id)
	EventBus.changed()


## Поиск юнита на клетке.
func unit_at(p: Vector2i) -> Unit:
	for u in units:
		if u.grid_pos == p:
			return u
	return null


## Находит живого героя фракции.
func hero_for(faction_id: int) -> Hero:
	for u in units:
		if u.faction_id == faction_id and u is Hero:
			return u
	return null


## Стоимость входа на клетку; -1 означает непроходимость.
func terrain_cost(type_id: String, faction_id: int, p: Vector2i) -> int:
	if not tiles.has(p):
		return -1
	var t: WorldTile = tiles[p]
	if type_id == "ship":
		return 1 if t.terrain == "water" else -1
	if t.terrain == "water":
		return -1
	var f = faction_manager.get_faction(faction_id)
	if t.terrain == "mountain":
		return Constants.SLOW_TERRAIN_COST if f.techs.has("climbing") else -1
	if t.terrain == "forest" and str(faction_definitions[faction_id].bonus) != "forest":
		return Constants.SLOW_TERRAIN_COST
	if t.terrain == "snow" and str(faction_definitions[faction_id].bonus) != "snow":
		return Constants.SLOW_TERRAIN_COST
	if t.terrain == "desert":
		return Constants.SLOW_TERRAIN_COST
	return 1


## Поиск достижимых клеток Дейкстрой; дорогой сосед доступен при полном запасе хода.
func reachable(unit: Unit, known_only: bool = true) -> Dictionary:
	var costs: Dictionary = {unit.grid_pos: 0}
	var frontier: Array[Vector2i] = [unit.grid_pos]
	if unit.movement_left <= 0 or unit.attacked:
		return costs
	while not frontier.is_empty():
		var p: Vector2i = frontier.pop_front()
		for delta in Constants.NEIGHBORS:
			var q = p + delta
			if known_only and not fog.knows(unit.faction_id, q):
				continue
			if unit_at(q) != null:
				continue
			var step = terrain_cost(unit.type_id, unit.faction_id, q)
			if step < 0:
				continue
			# Даже пехота MOV 1 может войти в лес, потратив весь свой ход.
			if p == unit.grid_pos and unit.movement_left == unit.move_range:
				step = mini(step, unit.move_range)
			var total = int(costs[p]) + step
			if total > unit.movement_left:
				continue
			if not costs.has(q) or total < int(costs[q]):
				costs[q] = total
				frontier.append(q)
	return costs


## Перемещает юнита по легальному маршруту и автоматически захватывает город.
func move_unit(unit: Unit, destination: Vector2i) -> bool:
	if finished or unit.faction_id != current_faction_id() or not units.has(unit):
		return false
	var paths = reachable(unit)
	if destination == unit.grid_pos or not paths.has(destination):
		return false
	var old = unit.grid_pos
	unit.movement_left -= int(paths[destination])
	unit.grid_pos = destination
	unit.position = (
		Vector2(destination * Constants.TILE_SIZE) + Vector2.ONE * Constants.TILE_SIZE / 2.0
	)
	var t: WorldTile = tiles[destination]
	if t.terrain == "city" and t.faction_id != unit.faction_id:
		t.faction_id = unit.faction_id
		EventBus.captured(t, unit.faction_id)
		if unit is Hero:
			unit.gain_xp(Constants.CAPTURE_XP)
	resource_manager.visit(unit)
	EventBus.moved(unit, old, destination)
	unit.queue_redraw()
	refresh_visibility()
	check_victory()
	return true


## Проверка дальности, видимости и оставшегося действия атаки.
func can_attack(attacker: Unit, defender: Unit) -> bool:
	return (
		not finished
		and units.has(attacker)
		and units.has(defender)
		and attacker.faction_id == current_faction_id()
		and attacker.faction_id != defender.faction_id
		and not attacker.attacked
		and fog.sees_unit(attacker.faction_id, defender)
		and Constants.distance(attacker.grid_pos, defender.grid_pos) <= attacker.attack_range
	)


## Наносит удар и ближнюю контратаку только выжившей цели.
func attack_unit(attacker: Unit, defender: Unit) -> bool:
	if not can_attack(attacker, defender):
		return false
	attacker.attacked = true
	attacker.movement_left = 0
	_hit(attacker, defender)
	if (
		units.has(defender)
		and units.has(attacker)
		and defender.attack_range == 1
		and Constants.distance(attacker.grid_pos, defender.grid_pos) == 1
	):
		_hit(defender, attacker)
	if units.has(attacker):
		attacker.queue_redraw()
	refresh_visibility()
	check_victory()
	return true


## Применяет урон, щит, опыт за убийство и удаление погибшего.
func _hit(attacker: Unit, defender: Unit) -> void:
	var t: WorldTile = tiles[defender.grid_pos]
	var damage = attacker.damage_to(defender, int(Constants.TERRAIN_DEFENSE.get(t.terrain, 0)))
	if defender is Hero and defender.shield:
		defender.shield = false
		damage = 0
	defender.hp = maxi(0, defender.hp - damage)
	EventBus.attacked(attacker, defender, damage)
	defender.queue_redraw()
	if defender.hp == 0:
		if attacker is Hero:
			attacker.gain_xp(Constants.KILL_XP)
		remove_unit(defender)


## Удаляет юнита и регистрирует пять собственных ходов до повторного найма героя.
func remove_unit(unit: Unit) -> void:
	if not units.has(unit):
		return
	if unit is Hero:
		var f = faction_manager.get_faction(unit.faction_id)
		f.hero_cooldown = Constants.HERO_REVIVE_TURNS
		f.fallen_hero = unit.type_id
	units.erase(unit)
	EventBus.died(unit)
	unit.queue_free()


## Может ли экземпляр находиться на свободной клетке.
func can_spawn(type_id: String, faction_id: int, p: Vector2i) -> bool:
	return unit_at(p) == null and terrain_cost(type_id, faction_id, p) > 0


## Создает экземпляр без оплаты; применяется при старте, загрузке и призыве.
func spawn(type_id: String, faction_id: int, p: Vector2i) -> Unit:
	var u = factory.create(type_id, faction_id, p, next_uid)
	if u == null:
		return null
	next_uid += 1
	units.append(u)
	if unit_container != null:
		unit_container.add_child(u)
	return u


## Цена героя после смерти: половина цены только погибшего типа.
func recruit_cost(type_id: String, faction_id: int) -> int:
	var d: Dictionary = (
		hero_definitions[type_id] if type_id.begins_with("hero_") else unit_definitions[type_id]
	)
	var f = faction_manager.get_faction(faction_id)
	return int(d.cost) / Constants.HERO_REVIVE_DIVISOR if f.fallen_hero == type_id else int(d.cost)


## Найм в своем городе; корабли появляются на соседней воде, герой требует таверну.
func recruit(type_id: String, city: WorldTile) -> bool:
	var faction_id = current_faction_id()
	var f = faction_manager.get_faction(faction_id)
	if finished or city.terrain != "city" or city.faction_id != faction_id:
		return false
	var hero = type_id.begins_with("hero_")
	var catalog: Dictionary = hero_definitions if hero else unit_definitions
	if not catalog.has(type_id):
		return false
	if hero:
		if not city.tavern or hero_for(faction_id) != null or f.hero_cooldown > 0:
			return false
	else:
		var requirement = str(catalog[type_id].required_tech)
		if requirement != "none" and not f.techs.has(requirement):
			return false
	var p = city.pos
	if type_id == "ship":
		var found = false
		for delta in Constants.NEIGHBORS:
			if can_spawn(type_id, faction_id, p + delta):
				p += delta
				found = true
				break
		if not found:
			return false
	if not can_spawn(type_id, faction_id, p):
		return false
	if not resource_manager.spend(faction_id, recruit_cost(type_id, faction_id)):
		return false
	var u = spawn(type_id, faction_id, p)
	u.movement_left = 0
	u.attacked = true
	if hero:
		f.fallen_hero = ""
	refresh_visibility()
	return true


## Очки для финального рейтинга.
func score(faction_id: int) -> int:
	var value = 0
	for t: WorldTile in tiles.values():
		if t.faction_id == faction_id and t.terrain == "city":
			value += Constants.CITY_SCORE + t.level * Constants.LEVEL_SCORE
	for u in units:
		if u.faction_id == faction_id:
			value += Constants.UNIT_SCORE
			if u is Hero:
				value += u.level * Constants.HERO_SCORE
	return value


## Выбывание по потере городов ИЛИ армии; нейтральные города учитываются в доминировании.
func check_victory() -> void:
	if finished or not active:
		return
	var alive: Array[Faction] = []
	var city_counts: Dictionary = {}
	var total_cities = 0
	for t: WorldTile in tiles.values():
		if t.terrain == "city":
			total_cities += 1
			city_counts[t.faction_id] = int(city_counts.get(t.faction_id, 0)) + 1
	for f in factions:
		var army = false
		for u in units:
			if u.faction_id == f.id:
				army = true
		if int(city_counts.get(f.id, 0)) == 0 or not army:
			f.eliminated = true
		if not f.eliminated:
			alive.append(f)
		if int(city_counts.get(f.id, 0)) == total_cities:
			finish(f.id)
			return
	if alive.size() == 1:
		finish(alive[0].id)
	elif alive.is_empty():
		finish(-1)
	elif round_number > Constants.MAX_ROUNDS:
		var best = -1
		var winner = -1
		for f in alive:
			var points = score(f.id)
			if points > best:
				best = points
				winner = f.id
			elif points == best:
				winner = -1
		finish(winner)
	else:
		# Не завершать остальных ИИ искусственно: игрок может наблюдать до конца.
		for f in factions:
			if f.human and f.eliminated:
				EventBus.notify("Ваша фракция выбыла. Можно наблюдать оставшиеся ходы.")


## Завершение партии.
func finish(winner: int) -> void:
	finished = true
	winner_id = winner
	busy = false
	EventBus.finished(winner)
	EventBus.changed()


## Версионированный снимок без объектов Godot.
func snapshot() -> Dictionary:
	var tile_list: Array = []
	var unit_list: Array = []
	var faction_list: Array = []
	for t: WorldTile in tiles.values():
		tile_list.append(t.to_dict())
	for u in units:
		unit_list.append(u.to_dict())
	for f in factions:
		faction_list.append(f.to_dict())
	return {
		"version": Constants.SAVE_VERSION,
		"map_size": map_size,
		"map_seed": map_seed,
		"round": round_number,
		"turn_index": turn_index,
		"next_uid": next_uid,
		"finished": finished,
		"winner_id": winner_id,
		"tiles": tile_list,
		"units": unit_list,
		"factions": faction_list,
		"fog": fog.to_dict()
	}


## Сохраняет партию во встроенную локальную БД (слот autosave).
func save_local() -> bool:
	if not active or busy:
		return false
	return save_db.autosave(snapshot())


## Сохраняет текущую партию в выбранный слот локальной БД.
func save_to_slot(slot: String, label: String = "") -> bool:
	if not active or busy:
		return false
	return save_db.save_slot(slot, snapshot(), label)


## Список сохранений локальной БД для меню.
func list_saves() -> Array[Dictionary]:
	return save_db.list_slots()


## Загружает слот локальной БД, не меняя игру при ошибке валидации.
func load_slot(slot: String) -> bool:
	var d: Variant = save_db.load_slot(slot)
	return load_snapshot(d) if d != null else false


## Удаляет слот локальной БД.
func delete_slot(slot: String) -> bool:
	return save_db.delete_slot(slot)


## Индекс следующего свободного слота или -1.
func next_free_slot() -> int:
	return save_db.next_free_slot()


## Загружает автосохранение из локальной БД.
func load_local() -> bool:
	return load_slot(SaveDatabase.AUTOSAVE_SLOT)


## Проверяет недоверенные облачные и локальные JSON перед созданием объектов.
func valid_snapshot(d: Variant) -> bool:
	if not d is Dictionary:
		return false
	if JSON.stringify(d).to_utf8_buffer().size() > Constants.MAX_SAVE_BYTES:
		return false
	if not _typed_fields(
		d,
		["version", "map_size", "map_seed", "round", "turn_index", "next_uid", "winner_id"],
		[],
		["finished"]
	):
		return false
	for key in [
		"version",
		"map_size",
		"map_seed",
		"round",
		"turn_index",
		"next_uid",
		"finished",
		"winner_id",
		"tiles",
		"units",
		"factions",
		"fog"
	]:
		if not d.has(key):
			return false
	if int(d.version) != Constants.SAVE_VERSION or int(d.map_size) not in [10, 16, 24]:
		return false
	if (
		not d.tiles is Array
		or not d.units is Array
		or not d.factions is Array
		or not d.fog is Dictionary
	):
		return false
	if (
		d.factions.size() < 2
		or d.factions.size() > 4
		or int(d.turn_index) < 0
		or int(d.turn_index) >= d.factions.size()
	):
		return false
	if int(d.round) < 1 or int(d.round) > Constants.MAX_ROUNDS + 1 or int(d.next_uid) < 1:
		return false
	if d.tiles.size() != int(d.map_size) * int(d.map_size) or d.units.size() > d.tiles.size():
		return false
	var ids: Array[int] = []
	for f: Variant in d.factions:
		if not f is Dictionary:
			return false
		if not _typed_fields(
			f,
			["id", "stars", "hero_cooldown", "turns_started"],
			["difficulty", "fallen_hero"],
			["human", "eliminated"]
		):
			return false
		if (
			str(f.difficulty) not in ["easy", "medium", "hard"]
			or int(f.stars) < 0
			or int(f.stars) > Constants.MAX_SAVE_STARS
		):
			return false
		if not str(f.fallen_hero).is_empty() and not hero_definitions.has(str(f.fallen_hero)):
			return false
		if not _valid_ai_memory(f.get("ai_memory", {})):
			return false
		for k in [
			"id",
			"human",
			"difficulty",
			"stars",
			"techs",
			"hero_cooldown",
			"fallen_hero",
			"turns_started",
			"eliminated"
		]:
			if not f.has(k):
				return false
		if not faction_definitions.has(int(f.id)) or ids.has(int(f.id)) or not f.techs is Array:
			return false
		ids.append(int(f.id))
		for tech: Variant in f.techs:
			if not tech is String or not tech_definitions.has(tech):
				return false
	var humans = 0
	for entry: Dictionary in d.factions:
		if bool(entry.human):
			humans += 1
	if humans != 1:
		return false
	var positions: Dictionary = {}
	for t: Variant in d.tiles:
		if not t is Dictionary:
			return false
		if not _typed_fields(
			t,
			["x", "y", "faction_id", "level", "population"],
			["terrain"],
			["tavern", "ruins_used"]
		):
			return false
		for k in [
			"x",
			"y",
			"terrain",
			"faction_id",
			"level",
			"population",
			"tavern",
			"visited",
			"ruins_used"
		]:
			if not t.has(k):
				return false
		var p = Vector2i(int(t.x), int(t.y))
		if (
			p.x < 0
			or p.y < 0
			or p.x >= int(d.map_size)
			or p.y >= int(d.map_size)
			or positions.has(p)
		):
			return false
		if (
			not Constants.TERRAIN.has(str(t.terrain))
			or not t.visited is Array
			or int(t.level) < 1
			or int(t.level) > Constants.MAX_CITY_LEVEL
		):
			return false
		if int(t.faction_id) != -1 and not ids.has(int(t.faction_id)):
			return false
		for visitor: Variant in t.visited:
			if not _integer(visitor) or not ids.has(int(visitor)):
				return false
		positions[p] = true
	var occupied: Dictionary = {}
	var uids: Dictionary = {}
	var heroes: Dictionary = {}
	for u: Variant in d.units:
		if not u is Dictionary:
			return false
		if not _typed_fields(
			u,
			[
				"uid",
				"faction_id",
				"x",
				"y",
				"hp",
				"max_hp",
				"attack",
				"defense",
				"move_range",
				"attack_range",
				"movement_left",
				"attack_buff",
				"life_turns"
			],
			["type_id"],
			["attacked"]
		):
			return false
		if (
			int(u.uid) < 1
			or int(u.uid) >= int(d.next_uid)
			or int(u.attack) < 0
			or int(u.attack) > Constants.MAX_SAVE_STAT
			or int(u.defense) < 0
			or int(u.defense) > Constants.MAX_SAVE_STAT
			or int(u.attack_range) < 1
			or int(u.attack_range) > Constants.MAX_SAVE_RANGE
		):
			return false
		if int(u.movement_left) < 0 or int(u.movement_left) > int(u.move_range):
			return false
		for k in [
			"uid",
			"type_id",
			"faction_id",
			"x",
			"y",
			"hp",
			"max_hp",
			"attack",
			"defense",
			"move_range",
			"attack_range",
			"movement_left",
			"attacked",
			"attack_buff",
			"life_turns"
		]:
			if not u.has(k):
				return false
		var hero = hero_definitions.has(str(u.type_id))
		if not hero and not unit_definitions.has(str(u.type_id)):
			return false
		var p = Vector2i(int(u.x), int(u.y))
		if (
			not positions.has(p)
			or occupied.has(p)
			or uids.has(int(u.uid))
			or not ids.has(int(u.faction_id))
		):
			return false
		if (
			int(u.hp) <= 0
			or int(u.max_hp) < int(u.hp)
			or int(u.max_hp) > Constants.MAX_SAVE_HP
			or int(u.move_range) < 1
			or int(u.move_range) > Constants.MAX_SAVE_RANGE
		):
			return false
		occupied[p] = true
		uids[int(u.uid)] = true
		if hero:
			if not _typed_fields(
				u, ["level", "xp", "cooldown", "hidden_turns"], [], ["shield", "ability_enhanced"]
			):
				return false
			for k in [
				"level",
				"xp",
				"cooldown",
				"hidden_turns",
				"shield",
				"ability_enhanced",
				"pending_perks"
			]:
				if not u.has(k):
					return false
			if (
				heroes.has(int(u.faction_id))
				or int(u.level) < 1
				or int(u.level) > Constants.MAX_HERO_LEVEL
				or not u.pending_perks is Array
			):
				return false
			for perk: Variant in u.pending_perks:
				if not perk is String or perk not in ["hp", "attack", "defense", "move", "ability"]:
					return false
			heroes[int(u.faction_id)] = true
	for key: Variant in d.fog:
		if (
			not key is String
			or not key.is_valid_int()
			or not ids.has(int(key))
			or not d.fog[key] is Array
		):
			return false
		for p: Variant in d.fog[key]:
			if not p is Array or p.size() != 2 or not _integer(p[0]) or not _integer(p[1]):
				return false
			if not positions.has(Vector2i(int(p[0]), int(p[1]))):
				return false
	return true


## Принимает только целые числа JSON без неявного приведения строк и объектов.
func _integer(value: Variant) -> bool:
	return value is int or (value is float and is_finite(value) and value == floor(value))


## Проверка примитивных полей до любого обращения к типизированным конструкторам.
func _typed_fields(d: Dictionary, numbers: Array, strings: Array, booleans: Array) -> bool:
	for key: String in numbers:
		if not d.has(key) or not _integer(d[key]):
			return false
	for key: String in strings:
		if not d.has(key) or not d[key] is String:
			return false
	for key: String in booleans:
		if not d.has(key) or not d[key] is bool:
			return false
	return true


## Проверяет сериализованную память ИИ; она никогда не исполняется как код.
func _valid_ai_memory(value: Variant) -> bool:
	if not value is Dictionary or value.size() > Constants.MAX_MEMORY_ENTRIES:
		return false
	for key: Variant in value:
		if not key is String or not key.is_valid_int():
			return false
		var entry: Variant = value[key]
		if not entry is Dictionary:
			return false
		if not _typed_fields(entry, ["x", "y", "attack_range", "seen_turn"], [], []):
			return false
	return true


## Восстанавливает проверенный снимок; доход повторно не начисляется.
func load_snapshot(d: Variant) -> bool:
	if busy or not valid_snapshot(d):
		return false
	clear_game()
	map_size = int(d.map_size)
	map_seed = int(d.map_seed)
	round_number = int(d.round)
	turn_index = int(d.turn_index)
	finished = bool(d.finished)
	winner_id = int(d.winner_id)
	for f: Dictionary in d.factions:
		factions.append(Faction.from_dict(f))
	for t: Dictionary in d.tiles:
		var tile = WorldTile.from_dict(t)
		tiles[tile.pos] = tile
	for entry: Dictionary in d.units:
		var u = spawn(
			str(entry.type_id), int(entry.faction_id), Vector2i(int(entry.x), int(entry.y))
		)
		u.uid = int(entry.uid)
		u.restore(entry)
	next_uid = int(d.next_uid)
	fog.restore(d.fog)
	active = true
	refresh_visibility()
	return true
