## AIHard.gd — ограниченный minimax: одно действие и лучший ответ врага (2 полухода).
class_name AIHard
extends AIPlayer

var evaluator: AIEvaluator = AIEvaluator.new()


## Выполняет ход с последовательным перепланированием после каждого действия.
func make_turn() -> void:
	play("hard")


## Сравнивает ограниченный набор перемещений по худшему видимому ответу врага.
func choose_move(unit: Unit) -> Vector2i:
	var candidates: Array[Dictionary] = []
	for p: Vector2i in GameManager.reachable(unit):
		candidates.append({"p": p, "heuristic": _evaluate_tile(p)})
	candidates.sort_custom(
		func(a: Dictionary, b: Dictionary) -> bool: return float(a.heuristic) > float(b.heuristic)
	)
	var base = _known_state()
	var best = unit.grid_pos
	var value = -INF
	for candidate in candidates.slice(0, Constants.SEARCH_WIDTH):
		var p: Vector2i = candidate.p
		var state = base.duplicate(true)
		for data: Dictionary in state.units:
			if int(data.uid) == unit.uid:
				data.x = p.x
				data.y = p.y
		for city: Dictionary in state.cities:
			if Vector2i(int(city.x), int(city.y)) == p:
				city.faction_id = faction_id
		var rating = _min_reply(state) + float(candidate.heuristic)
		# Память старых позиций — угроза, но не реальная цель или занятая клетка.
		for entry: Dictionary in memory.values():
			if (
				int(entry.seen_turn)
				< GameManager.faction_manager.get_faction(faction_id).turns_started
			):
				if (
					Constants.distance(p, Vector2i(int(entry.x), int(entry.y)))
					<= int(entry.attack_range)
				):
					rating -= Constants.UNIT_SCORE
		if rating > value:
			value = rating
			best = p
	return best


## Снимок включает свои юниты, видимых врагов и исследованные города.
func _known_state() -> Dictionary:
	var state = {"units": [], "cities": [], "stars": 0, "tech_count": 0}
	var f = GameManager.faction_manager.get_faction(faction_id)
	state.stars = f.stars
	state.tech_count = f.techs.size()
	for unit in GameManager.units:
		if unit.faction_id == faction_id or GameManager.fog.sees_unit(faction_id, unit):
			state.units.append(unit.to_dict())
	for tile: WorldTile in GameManager.tiles.values():
		if tile.terrain == "city" and GameManager.fog.knows(faction_id, tile.pos):
			state.cities.append(tile.to_dict())
	return state


## Второй полуход: минимальная оценка после одного удара видимого противника.
func _min_reply(state: Dictionary) -> float:
	var worst = evaluator.evaluate_known(state, faction_id)
	for enemy: Dictionary in state.units:
		if int(enemy.faction_id) == faction_id:
			continue
		for ally: Dictionary in state.units:
			if int(ally.faction_id) != faction_id:
				continue
			var from = Vector2i(int(enemy.x), int(enemy.y))
			var to = Vector2i(int(ally.x), int(ally.y))
			if Constants.distance(from, to) > int(enemy.attack_range):
				continue
			var branch = state.duplicate(true)
			var attacker: Dictionary = {}
			var defender: Dictionary = {}
			for entry: Dictionary in branch.units:
				if int(entry.uid) == int(enemy.uid):
					attacker = entry
				if int(entry.uid) == int(ally.uid):
					defender = entry
			_simulate_hit(attacker, defender, branch)
			if (
				int(defender.hp) > 0
				and int(defender.attack_range) == 1
				and Constants.distance(from, to) == 1
			):
				_simulate_hit(defender, attacker, branch)
			worst = minf(worst, evaluator.evaluate_known(branch, faction_id))
	return worst


## Детерминированная симуляция урона без изменения живых объектов и RNG.
func _simulate_hit(attacker: Dictionary, defender: Dictionary, state: Dictionary) -> void:
	var pos = Vector2i(int(defender.x), int(defender.y))
	var terrain: String = GameManager.tiles[pos].terrain
	var damage = maxi(
		Constants.MIN_DAMAGE,
		int(
			round(
				(
					(
						(int(attacker.attack) + int(attacker.attack_buff))
						* float(attacker.hp)
						/ int(attacker.max_hp)
					)
					- (
						(int(defender.defense) + int(Constants.TERRAIN_DEFENSE.get(terrain, 0)))
						* float(defender.hp)
						/ int(defender.max_hp)
					)
				)
			)
		)
	)
	if bool(defender.get("shield", false)):
		defender.shield = false
		damage = 0
	defender.hp = maxi(0, int(defender.hp) - damage)
	if int(defender.hp) == 0:
		state.units.erase(defender)
