## UnitFactory.gd — единая фабрика экземпляров из JSON и сцен.
class_name UnitFactory
extends RefCounted


## Создает обычного юнита или героя без хардкода боевых характеристик.
func create(type_id: String, faction_id: int, p: Vector2i, uid: int) -> Unit:
	var hero = type_id.begins_with("hero_")
	var catalog: Dictionary = GameManager.hero_definitions if hero else GameManager.unit_definitions
	if not catalog.has(type_id):
		return null
	var scene: PackedScene = load(
		"res://scenes/units/Hero.tscn" if hero else "res://scenes/units/Unit.tscn"
	)
	var unit = scene.instantiate() as Unit
	unit.configure(catalog[type_id], faction_id, p, uid)
	if unit is Hero and type_id == "hero_paladin":
		unit.shield = true
		unit.cooldown = int(unit.definition.cooldown)
	return unit
