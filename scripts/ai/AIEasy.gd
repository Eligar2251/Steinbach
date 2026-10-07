## AIEasy.gd — случайное движение, соседние атаки, без способностей героя.
class_name AIEasy
extends AIPlayer


## Выполняет легкий ход.
func make_turn() -> void:
	play("easy")
