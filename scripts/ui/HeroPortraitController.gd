## HeroPortraitController.gd — портрет 128×128, лор 256×256 и вспышка способности.
extends PanelContainer

var hero: Hero
var portrait: TextureRect
var text: Label
var ability_button: Button
var layout: VBoxContainer


## Создает содержимое панели и слушает применение способности.
func _ready() -> void:
	layout = VBoxContainer.new()
	add_child(layout)
	portrait = TextureRect.new()
	portrait.custom_minimum_size = Vector2(128, 128)
	portrait.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	portrait.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	portrait.gui_input.connect(_portrait_input)
	layout.add_child(portrait)
	text = ThemeFactory.label("")
	layout.add_child(text)
	ability_button = ThemeFactory.button("", _ability)
	layout.add_child(ability_button)
	EventBus.hero_ability_used.connect(_flash)


## Отображает здоровье, XP, характеристики и доступные перки.
func show_hero(value: Hero, enabled: bool) -> void:
	hero = value
	portrait.texture = load(str(hero.definition.portrait))
	add_theme_stylebox_override(
		"panel",
		ThemeFactory.box(
			Color("#1a2c35"), GameManager.faction_manager.get_faction(hero.faction_id).color()
		)
	)
	var goal = "MAX" if hero.level >= 5 else str(Constants.HERO_XP[hero.level - 1])
	text.text = (
		"%s\nУр. %d · XP %d/%s\nHP %d/%d\nATK %d (+%d) · DEF %d · MOV %d\n%s"
		% [
			hero.definition.name,
			hero.level,
			hero.xp,
			goal,
			hero.hp,
			hero.max_hp,
			hero.attack,
			hero.attack_buff,
			hero.defense,
			hero.move_range,
			hero.definition.ability_desc
		]
	)
	ability_button.text = "%s · КД %d" % [hero.definition.ability_name, hero.cooldown]
	ability_button.disabled = not enabled or hero.cooldown > 0 or hero.type_id == "hero_paladin"
	for child in layout.get_children():
		if child.has_meta("perk"):
			layout.remove_child(child)
			child.queue_free()
	for perk in hero.pending_perks:
		var names = {
			"hp": "+2 HP",
			"attack": "+1 атака",
			"defense": "+1 защита",
			"move": "+1 движение",
			"ability": "Кулдаун способности −1"
		}
		var b = ThemeFactory.button("Перк: " + str(names[perk]), hero.choose_perk.bind(perk))
		b.disabled = not enabled
		b.set_meta("perk", true)
		layout.add_child(b)


## Использование способности с проверками в модели.
func _ability() -> void:
	if is_instance_valid(hero):
		hero.use_ability()


## Открывает увеличенный портрет и лор.
func _portrait_input(event: InputEvent) -> void:
	if (
		event is InputEventMouseButton
		and event.pressed
		and event.button_index == MOUSE_BUTTON_LEFT
		and is_instance_valid(hero)
	):
		var dialog = AcceptDialog.new()
		dialog.title = str(hero.definition.name)
		var box = VBoxContainer.new()
		var image = TextureRect.new()
		image.texture = portrait.texture
		image.custom_minimum_size = Vector2(256, 256)
		image.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		image.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		box.add_child(image)
		box.add_child(ThemeFactory.label(str(hero.definition.lore)))
		dialog.add_child(box)
		add_child(dialog)
		dialog.popup_centered(Vector2i(400, 400))
		dialog.confirmed.connect(dialog.queue_free)


## Подсветка портрета после способности.
func _flash(value: Hero, _name: String) -> void:
	if value != hero:
		return
	var tween = create_tween()
	tween.tween_property(portrait, "modulate", Color(1.8, 1.6, 1.1), 0.12)
	tween.tween_property(portrait, "modulate", Color.WHITE, 0.35)
