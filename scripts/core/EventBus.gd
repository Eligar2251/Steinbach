## EventBus.gd — глобальные события; emit выполняется методами этого файла.
extends Node

signal unit_moved(unit: Unit, from_pos: Vector2i, to_pos: Vector2i)
signal unit_attacked(attacker: Unit, defender: Unit, damage: int)
signal unit_died(unit: Unit)
signal city_captured(city: WorldTile, new_owner: int)
signal hero_ability_used(hero: Hero, ability_name: String)
signal hero_level_up(hero: Hero, new_level: int)
signal turn_ended(faction_id: int)
signal game_over(winner_faction_id: int)
signal state_changed
signal message(text: String)


## Сообщение о перемещении.
func moved(unit: Unit, from_pos: Vector2i, to_pos: Vector2i) -> void:
	unit_moved.emit(unit, from_pos, to_pos)


## Сообщение об атаке.
func attacked(attacker: Unit, defender: Unit, damage: int) -> void:
	unit_attacked.emit(attacker, defender, damage)


## Сообщение о смерти.
func died(unit: Unit) -> void:
	unit_died.emit(unit)


## Сообщение о захвате.
func captured(city: WorldTile, faction_id: int) -> void:
	city_captured.emit(city, faction_id)


## Сообщение о способности.
func ability(hero: Hero) -> void:
	hero_ability_used.emit(hero, str(hero.definition.ability_name))


## Сообщение о повышении уровня.
func leveled(hero: Hero) -> void:
	hero_level_up.emit(hero, hero.level)


## Сообщение о конце хода.
func ended(faction_id: int) -> void:
	turn_ended.emit(faction_id)


## Сообщение о завершении игры; -1 означает ничью.
func finished(winner: int) -> void:
	game_over.emit(winner)


## Обновление интерфейса.
func changed() -> void:
	state_changed.emit()


## Краткое уведомление игроку.
func notify(text: String) -> void:
	message.emit(text)
