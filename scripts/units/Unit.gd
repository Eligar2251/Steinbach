## Unit.gd — базовый юнит; характеристики и графика только из JSON.
class_name Unit
extends Node2D

var uid: int = 0
var type_id: String = "warrior"
var faction_id: int = 0
var grid_pos: Vector2i = Vector2i.ZERO
var definition: Dictionary = {}
var hp: int = 0
var max_hp: int = 0
var attack: int = 0
var defense: int = 0
var move_range: int = 0
var attack_range: int = 0
var movement_left: int = 0
var attacked: bool = false
var attack_buff: int = 0
var life_turns: int = -1


## Настройка экземпляра по загруженному справочнику.
func configure(data: Dictionary, new_faction_id: int, p: Vector2i, unique_id: int) -> void:
	definition = data
	type_id = str(data.id)
	faction_id = new_faction_id
	grid_pos = p
	uid = unique_id
	max_hp = int(data.hp)
	hp = max_hp
	attack = int(data.attack)
	defense = int(data.defense)
	move_range = int(data.move)
	attack_range = int(data.range)
	movement_left = move_range
	if str(GameManager.faction_definitions[faction_id].bonus) == "attack":
		attack += Constants.PERK_STAT
	position = Vector2(grid_pos * Constants.TILE_SIZE) + Vector2.ONE * Constants.TILE_SIZE / 2.0
	var sprite = get_node_or_null("Sprite2D") as Sprite2D
	if sprite:
		sprite.texture = load(str(data.get("texture", data.get("sprite", ""))))
		sprite.modulate = GameManager.faction_manager.get_faction(faction_id).color()
	queue_redraw()


## Обновляет очки действий и временные эффекты в начале своего хода.
func begin_turn() -> void:
	movement_left = move_range
	attacked = false
	attack_buff = 0
	if life_turns > 0:
		life_turns -= 1
		if life_turns == 0:
			GameManager.remove_unit(self)


## Сила удара с учетом здоровья и временного бонуса.
func damage_to(target: Unit, terrain_defense: int = 0) -> int:
	var damage = (
		(attack + attack_buff) * float(hp) / max_hp
		- (target.defense + terrain_defense) * float(target.hp) / target.max_hp
	)
	return maxi(Constants.MIN_DAMAGE, int(round(damage)))


## Полоса здоровья и маркер доступных действий.
func _draw() -> void:
	draw_rect(Rect2(-21, 25, 42, 4), Color("#172830"))
	draw_rect(Rect2(-21, 25, 42 * float(hp) / maxi(1, max_hp), 4), Color("#9bd7a6"))
	if not attacked:
		draw_circle(Vector2(19, -19), 3, Color("#f5d581"))


## Запись состояния юнита, включая прокачанные характеристики.
func to_dict() -> Dictionary:
	return {
		"uid": uid,
		"type_id": type_id,
		"faction_id": faction_id,
		"x": grid_pos.x,
		"y": grid_pos.y,
		"hp": hp,
		"max_hp": max_hp,
		"attack": attack,
		"defense": defense,
		"move_range": move_range,
		"attack_range": attack_range,
		"movement_left": movement_left,
		"attacked": attacked,
		"attack_buff": attack_buff,
		"life_turns": life_turns
	}


## Восстановление изменяемых характеристик из сохранения.
func restore(d: Dictionary) -> void:
	hp = int(d.hp)
	max_hp = int(d.max_hp)
	attack = int(d.attack)
	defense = int(d.defense)
	move_range = int(d.move_range)
	attack_range = int(d.attack_range)
	movement_left = int(d.movement_left)
	attacked = bool(d.attacked)
	attack_buff = int(d.attack_buff)
	life_turns = int(d.life_turns)
	queue_redraw()
