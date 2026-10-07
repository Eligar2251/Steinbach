## VictoryController.gd — результат игры и итоговый рейтинг.
extends Control


## Показывает победителя или ничью и таблицу очков.
func _ready() -> void:
	theme = ThemeFactory.create()
	var center = CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(center)
	var panel = PanelContainer.new()
	panel.custom_minimum_size.x = 560
	center.add_child(panel)
	var box = VBoxContainer.new()
	box.add_theme_constant_override("separation", 16)
	panel.add_child(box)
	var winner = GameManager.faction_manager.get_faction(GameManager.winner_id)
	var title = "Ничья" if winner == null else ("Победа!" if winner.human else "Поражение")
	box.add_child(ThemeFactory.label(title, 40))
	box.add_child(
		ThemeFactory.label(
			(
				"Равные очки или нет выживших"
				if winner == null
				else "Победитель: " + winner.display_name()
			),
			24
		)
	)
	for f in GameManager.factions:
		box.add_child(
			ThemeFactory.label(
				(
					"%s — %d очков%s"
					% [
						f.display_name(),
						GameManager.score(f.id),
						" · выбыл" if f.eliminated else ""
					]
				)
			)
		)
	box.add_child(ThemeFactory.label("Города ×10 + уровни ×5 + юниты ×2 + уровень героя ×8", 16))
	box.add_child(ThemeFactory.button("Главное меню", _return_menu))


## Возвращает в меню после результата.
func _return_menu() -> void:
	GameManager.clear_game()
	get_tree().change_scene_to_file("res://scenes/MainMenu.tscn")
