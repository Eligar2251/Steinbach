## GameWorldController.gd — связывает карту, юнитов и HUD.
extends Node2D


## Подключает сцену к существующему состоянию партии.
func _ready() -> void:
	GameManager.attach_units($Units)
	$WorldMap.viewer_id = _human_id()
	$HUD/Interface.bind_map($WorldMap)
	$WorldMap.refresh()
	EventBus.unit_attacked.connect(_attack_effect)
	EventBus.city_captured.connect(_capture_effect)
	GameManager.turn_manager.resume.call_deferred()


## Находит id человека, независимо от выбранной фракции.
func _human_id() -> int:
	for f in GameManager.factions:
		if f.human:
			return f.id
	return GameManager.factions[0].id


## Перед уходом со сцены отвязывает контейнер (юниты освобождает GameManager).
func _exit_tree() -> void:
	for unit in GameManager.units:
		if is_instance_valid(unit) and unit.get_parent() == $Units:
			$Units.remove_child(unit)
	GameManager.unit_container = null


## Текстовый эффект урона на карте.
func _attack_effect(_attacker: Unit, defender: Unit, damage: int) -> void:
	var scene: PackedScene = load("res://scenes/effects/AttackEffect.tscn")
	var effect = scene.instantiate() as Node2D
	effect.position = defender.position
	effect.text = "−%d" % damage
	$Effects.add_child(effect)


## Подсветка захвата города.
func _capture_effect(city: WorldTile, faction_id: int) -> void:
	var scene: PackedScene = load("res://scenes/effects/CaptureEffect.tscn")
	var effect = scene.instantiate() as Node2D
	effect.position = (
		Vector2(city.pos * Constants.TILE_SIZE) + Vector2.ONE * Constants.TILE_SIZE / 2.0
	)
	effect.text = "Захват!"
	effect.tint = GameManager.faction_manager.get_faction(faction_id).color()
	$Effects.add_child(effect)
