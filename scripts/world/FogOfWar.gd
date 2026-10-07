## FogOfWar.gd — раздельные исследованные и видимые клетки каждой фракции.
class_name FogOfWar
extends RefCounted

var explored: Dictionary = {}
var visible: Dictionary = {}


## Обновление видимости после каждого действия; исследование сохраняется навсегда.
func update(faction_id: int) -> void:
	visible[faction_id] = {}
	if not explored.has(faction_id):
		explored[faction_id] = {}
	var origins: Array[Vector2i] = []
	for u in GameManager.units:
		if u.faction_id == faction_id:
			origins.append(u.grid_pos)
	for t: WorldTile in GameManager.tiles.values():
		if t.terrain == "city" and t.faction_id == faction_id:
			origins.append(t.pos)
	for origin in origins:
		for dy in range(-Constants.VISION, Constants.VISION + 1):
			for dx in range(-Constants.VISION, Constants.VISION + 1):
				var p = origin + Vector2i(dx, dy)
				if GameManager.tiles.has(p) and Constants.distance(p, origin) <= Constants.VISION:
					visible[faction_id][p] = true
					explored[faction_id][p] = true


## Видна ли клетка прямо сейчас.
func can_see(faction_id: int, p: Vector2i) -> bool:
	return visible.get(faction_id, {}).has(p)


## Исследована ли клетка раньше.
func knows(faction_id: int, p: Vector2i) -> bool:
	return explored.get(faction_id, {}).has(p)


## Виден ли юнит с учетом невидимости ассасина.
func sees_unit(faction_id: int, unit: Unit) -> bool:
	if unit.faction_id == faction_id:
		return true
	if unit is Hero and unit.hidden_turns > 0:
		return false
	return can_see(faction_id, unit.grid_pos)


## Экспорт исследованных клеток для сохранения.
func to_dict() -> Dictionary:
	var result: Dictionary = {}
	for id: int in explored:
		var positions: Array = []
		for p: Vector2i in explored[id]:
			positions.append([p.x, p.y])
		result[str(id)] = positions
	return result


## Импорт карты исследования; текущая видимость вычисляется заново.
func restore(d: Dictionary) -> void:
	explored.clear()
	visible.clear()
	for key: String in d:
		var id = int(key)
		explored[id] = {}
		for p: Array in d[key]:
			explored[id][Vector2i(int(p[0]), int(p[1]))] = true
