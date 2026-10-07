## TileData.gd — сериализуемые данные клетки (не встроенный Godot TileData).
class_name WorldTile
extends RefCounted

var pos: Vector2i = Vector2i.ZERO
var terrain: String = "grass"
var faction_id: int = -1
var level: int = 1
var population: int = 0
var tavern: bool = false
var visited: Array[int] = []
var ruins_used: bool = false


## Запись состояния клетки.
func to_dict() -> Dictionary:
	return {
		"x": pos.x,
		"y": pos.y,
		"terrain": terrain,
		"faction_id": faction_id,
		"level": level,
		"population": population,
		"tavern": tavern,
		"visited": visited,
		"ruins_used": ruins_used
	}


## Восстановление клетки из JSON.
static func from_dict(d: Dictionary) -> WorldTile:
	var tile = WorldTile.new()
	tile.pos = Vector2i(int(d.x), int(d.y))
	tile.terrain = str(d.terrain)
	tile.faction_id = int(d.faction_id)
	tile.level = int(d.level)
	tile.population = int(d.population)
	tile.tavern = bool(d.tavern)
	tile.visited.assign(d.visited)
	tile.ruins_used = bool(d.ruins_used)
	return tile
