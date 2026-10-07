## MapGenerator.gd — воспроизводимая генерация с сухопутными путями между столицами.
class_name MapGenerator
extends RefCounted


## Создание карты по seed; столицы и дороги гарантируют старт без изоляции водой.
func generate(size: int, faction_ids: Array[int], seed_value: int) -> Dictionary:
	var rng = RandomNumberGenerator.new()
	rng.seed = seed_value
	var tiles: Dictionary = {}
	for y in size:
		for x in size:
			var t = WorldTile.new()
			t.pos = Vector2i(x, y)
			var roll = rng.randf()
			t.terrain = "grass"
			for terrain: String in Constants.MAP_TERRAIN_THRESHOLDS:
				if roll < float(Constants.MAP_TERRAIN_THRESHOLDS[terrain]):
					t.terrain = terrain
					break
			tiles[t.pos] = t
	var starts: Array[Vector2i] = [
		Vector2i(1, 1), Vector2i(size - 2, size - 2), Vector2i(size - 2, 1), Vector2i(1, size - 2)
	]
	var center = Vector2i(size / 2, size / 2)
	for i in faction_ids.size():
		var p = starts[i]
		_carve(tiles, p, center)
		for delta in Constants.NEIGHBORS:
			var neighbor: WorldTile = tiles[p + delta]
			neighbor.terrain = str(GameManager.faction_definitions[faction_ids[i]].biome)
		var city: WorldTile = tiles[p]
		city.terrain = "city"
		city.faction_id = faction_ids[i]
	# Нейтральные города не должны перекрывать стартовые столицы.
	for p in [center, Vector2i(size / 2, 2), Vector2i(2, size / 2)]:
		var t: WorldTile = tiles[p]
		if t.faction_id == -1:
			t.terrain = "city"
	return {"tiles": tiles, "starts": starts.slice(0, faction_ids.size())}


## Прокладка проходимой дороги через непроходимые биомы.
func _carve(tiles: Dictionary, start: Vector2i, end: Vector2i) -> void:
	var p = start
	while p != end:
		if p.x != end.x:
			p.x += signi(end.x - p.x)
		else:
			p.y += signi(end.y - p.y)
		var t: WorldTile = tiles[p]
		t.terrain = "grass"
