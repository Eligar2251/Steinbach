## TechManager.gd — технологии, требования и исследования.
class_name TechManager
extends RefCounted


## Возвращает неисследованные технологии с выполненными предпосылками.
func available(faction_id: int) -> Array[Dictionary]:
	var result: Array[Dictionary] = []
	var f = GameManager.faction_manager.get_faction(faction_id)
	for tech: Dictionary in GameManager.tech_definitions.values():
		if f.techs.has(str(tech.id)):
			continue
		var valid = true
		for requirement: String in tech.prerequisites:
			if not f.techs.has(requirement):
				valid = false
		if valid:
			result.append(tech)
	return result


## Исследует доступную технологию; бесплатный путь используется мудрецом.
func research(faction_id: int, tech_id: String, free: bool = false) -> bool:
	if GameManager.finished or faction_id != GameManager.current_faction_id():
		return false
	for tech in available(faction_id):
		if str(tech.id) != tech_id:
			continue
		if not free and not GameManager.resource_manager.spend(faction_id, int(tech.cost)):
			return false
		GameManager.faction_manager.get_faction(faction_id).techs.append(tech_id)
		EventBus.changed()
		return true
	return false
