## EffectController.gd — краткий текстовый эффект атаки или захвата.
extends Node2D

var text: String = ""
var tint: Color = Color("#f4d58a")


## Эффект поднимается и исчезает.
func _ready() -> void:
	var tween = create_tween().set_parallel(true)
	tween.tween_property(self, "position:y", position.y - 32, 0.6)
	tween.tween_property(self, "modulate:a", 0.0, 0.6)
	tween.finished.connect(queue_free)


## Рисует текст стандартным шрифтом Godot.
func _draw() -> void:
	draw_string(
		ThemeDB.fallback_font, Vector2(-12, -20), text, HORIZONTAL_ALIGNMENT_CENTER, -1, 22, tint
	)
