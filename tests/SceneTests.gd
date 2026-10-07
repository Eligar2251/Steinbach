## SceneTests.gd — проверка создания UI-сцен и перепривязки юнитов без дисплея.
extends Node

var failures: int = 0


## Отложенный старт после загрузки singleton.
func _ready() -> void:
	_run.call_deferred()


## Проверка UI жизненного цикла.
func _run() -> void:
	var slots: Array = [
		{"id": 2, "human": true, "difficulty": "medium"},
		{"id": 1, "human": false, "difficulty": "hard"}
	]
	GameManager.new_game("medium", slots, 7)
	var game_scene: PackedScene = load("res://scenes/GameWorld.tscn")
	var game: Node = game_scene.instantiate()
	get_tree().root.add_child(game)
	await get_tree().process_frame
	game.get_node("HUD/Interface")._tile_clicked(Vector2i(1, 1))
	game.get_node("HUD/Interface")._techs()
	await get_tree().process_frame
	var hero = GameManager.spawn("hero_sage", 2, Vector2i(2, 1))
	GameManager.refresh_visibility()
	game.get_node("HUD/Interface")._tile_clicked(hero.grid_pos)
	await get_tree().process_frame
	get_tree().root.remove_child(game)
	game.free()
	for unit in GameManager.units:
		if not is_instance_valid(unit) or unit.get_parent() != null:
			failures += 1
	game = game_scene.instantiate()
	get_tree().root.add_child(game)
	await get_tree().process_frame
	get_tree().root.remove_child(game)
	game.free()
	GameManager.clear_game()
	var menu: Node = load("res://scenes/MainMenu.tscn").instantiate()
	get_tree().root.add_child(menu)
	await get_tree().process_frame
	menu.show_lobby()
	await get_tree().process_frame
	menu.show_settings()
	await get_tree().process_frame
	menu.show_saves()
	await get_tree().process_frame
	menu.show_menu()
	await get_tree().process_frame
	menu.free()
	GameManager.new_game("small", slots, 10)
	GameManager.finish(2)
	var victory: Node = load("res://scenes/VictoryScreen.tscn").instantiate()
	get_tree().root.add_child(victory)
	await get_tree().process_frame
	victory.free()
	GameManager.clear_game()
	print("SceneTests: %d failures" % failures)
	get_tree().quit(1 if failures else 0)
