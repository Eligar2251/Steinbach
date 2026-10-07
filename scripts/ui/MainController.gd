## MainController.gd — точка входа и переключение экранов.
extends Node


## Открывает главное меню.
func _ready() -> void:
	get_tree().change_scene_to_file.call_deferred("res://scenes/MainMenu.tscn")
