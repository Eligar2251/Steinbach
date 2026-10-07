## AIEvaluator.gd — оценка полного состояния и доступного ИИ снимка.
class_name AIEvaluator
extends RefCounted


## Полная оценка для диагностики, не применяется для чтения скрытой экономики ИИ.
func evaluate_position(faction_id: int) -> float:
	var score = 0.0
	for tile: WorldTile in GameManager.tiles.values():
		if tile.faction_id == faction_id and tile.terrain == "city":
			score += Constants.CITY_SCORE
	for unit in GameManager.units:
		if unit.faction_id == faction_id:
			score += unit.hp * Constants.UNIT_SCORE
			if unit is Hero:
				score += unit.level * Constants.LEVEL_SCORE
	var f = GameManager.faction_manager.get_faction(faction_id)
	score += f.stars + f.techs.size() * Constants.EVALUATOR_TECH_WEIGHT
	return score


## Дифференциальная оценка известного состояния; ресурсы врага не читаются.
func evaluate_known(state: Dictionary, faction_id: int) -> float:
	var score = float(state.stars) + int(state.tech_count) * Constants.EVALUATOR_TECH_WEIGHT
	for city: Dictionary in state.cities:
		if int(city.faction_id) == faction_id:
			score += Constants.CITY_SCORE
		elif int(city.faction_id) != -1:
			score -= Constants.CITY_SCORE
	for unit: Dictionary in state.units:
		var sign_value = 1.0 if int(unit.faction_id) == faction_id else -1.0
		score += (
			sign_value
			* (
				int(unit.hp) * Constants.UNIT_SCORE
				+ int(unit.get("level", 0)) * Constants.LEVEL_SCORE
			)
		)
	return score
