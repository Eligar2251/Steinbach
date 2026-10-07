## TechTreeController.gd — дерево из JSON с предпосылками и состоянием исследования.
extends PanelContainer

var grid: GridContainer


## Создает адаптивную прокручиваемую сетку.
func _ready() -> void:
	var scroll = ScrollContainer.new()
	scroll.custom_minimum_size = Vector2(620, 400)
	add_child(scroll)
	grid = GridContainer.new()
	grid.columns = 2
	grid.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll.add_child(grid)


## Строит карточки технологий из справочника.
func refresh(faction_id: int, enabled: bool) -> void:
	for child in grid.get_children():
		grid.remove_child(child)
		child.queue_free()
	var f = GameManager.faction_manager.get_faction(faction_id)
	var allowed: Array[String] = []
	for t in GameManager.tech_manager.available(faction_id):
		allowed.append(str(t.id))
	for tech: Dictionary in GameManager.tech_definitions.values():
		var text = "%s · %d ⭐\n%s" % [tech.name, tech.cost, tech.description]
		if not tech.prerequisites.is_empty():
			text += "\n← " + ", ".join(tech.prerequisites)
		if f.techs.has(str(tech.id)):
			text += "\n✓ Исследовано"
		var b = ThemeFactory.button(
			text,
			func() -> void:
				GameManager.tech_manager.research(faction_id, str(tech.id))
				refresh(faction_id, enabled)
		)
		b.custom_minimum_size = Vector2(280, 100)
		b.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		b.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		b.disabled = not enabled or not allowed.has(str(tech.id)) or f.stars < int(tech.cost)
		grid.add_child(b)
