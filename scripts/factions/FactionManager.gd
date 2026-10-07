## FactionManager.gd — создание и поиск фракций.
class_name FactionManager
extends RefCounted


## Создает игроков из слотов лобби; id фракций уникальны.
func create(slots: Array) -> Array[Faction]:
	var result: Array[Faction] = []
	for slot: Dictionary in slots:
		var f = Faction.new()
		f.id = int(slot.id)
		f.human = bool(slot.human)
		f.difficulty = str(slot.difficulty)
		result.append(f)
	return result


## Возвращает фракцию по стабильному id, не индексу очереди.
func get_faction(id: int) -> Faction:
	for f in GameManager.factions:
		if f.id == id:
			return f
	return null
