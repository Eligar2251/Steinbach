## TurnManager.gd — последовательные ходы, ожидание ИИ и отмена сменой session.
class_name TurnManager
extends Node


## Заканчивает ход человека; повторный клик блокируется busy.
func end_turn() -> void:
	if GameManager.busy or GameManager.finished or not GameManager.active:
		return
	GameManager.busy = true
	var token = GameManager.session
	_advance()
	await run_ai_turns(token)


## Переходит к следующей невыбывшей фракции и начисляет ее доход.
func _advance() -> void:
	EventBus.ended(GameManager.current_faction_id())
	for _i in GameManager.factions.size():
		GameManager.turn_index += 1
		if GameManager.turn_index >= GameManager.factions.size():
			GameManager.turn_index = 0
			GameManager.round_number += 1
		GameManager.check_victory()
		if GameManager.finished:
			return
		if not GameManager.factions[GameManager.turn_index].eliminated:
			GameManager.begin_current_turn()
			return


## Возобновляет цепочку ИИ после загрузки сохранения.
func resume() -> void:
	if GameManager.finished:
		return
	if not GameManager.faction_manager.get_faction(GameManager.current_faction_id()).human:
		GameManager.busy = true
		await run_ai_turns(GameManager.session)


## Каждый ИИ работает после своей задержки; только затем очередь переходит дальше.
func run_ai_turns(token: int) -> void:
	while token == GameManager.session and not GameManager.finished:
		var f = GameManager.faction_manager.get_faction(GameManager.current_faction_id())
		if f.human and not f.eliminated:
			break
		EventBus.notify(f.display_name() + " думает…")
		EventBus.changed()
		await get_tree().create_timer(float(Constants.AI_DELAYS.get(f.difficulty, 1.0))).timeout
		if token != GameManager.session:
			return
		var ai: AIPlayer
		match f.difficulty:
			"easy":
				ai = AIEasy.new()
			"hard":
				ai = AIHard.new()
			_:
				ai = AIMedium.new()
		ai.faction_id = f.id
		ai.make_turn()
		ai.free()
		GameManager.check_victory()
		if GameManager.finished:
			break
		_advance()
	if token != GameManager.session:
		return
	GameManager.busy = false
	if not GameManager.finished:
		GameManager.save_local()
	EventBus.changed()
