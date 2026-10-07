## MainMenuController.gd — меню, лобби, слоты локальной БД и опциональное облако.
extends Control

var content: VBoxContainer
var slot_modes: Array[OptionButton] = []
var faction_picks: Array[OptionButton] = []
var size_pick: OptionButton
var notice: Label
var backdrop: Node2D
var elapsed: float = 0.0


## Строит меню поверх анимированной фоновой карты-заглушки.
func _ready() -> void:
	theme = ThemeFactory.create()
	backdrop = Node2D.new()
	add_child(backdrop)
	for y in 14:
		for x in 24:
			var sprite = Sprite2D.new()
			sprite.texture = load(
				(
					"res://assets/textures/tiles/"
					+ ["grass", "forest", "water", "mountain"][(x * 7 + y * 3) % 4]
					+ ".svg"
				)
			)
			sprite.position = Vector2(x * 64, y * 64)
			sprite.modulate = Color(0.45, 0.55, 0.55, 0.4)
			backdrop.add_child(sprite)
	var center = CenterContainer.new()
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(center)
	var panel = PanelContainer.new()
	panel.custom_minimum_size = Vector2(680, 0)
	center.add_child(panel)
	content = VBoxContainer.new()
	content.add_theme_constant_override("separation", 12)
	panel.add_child(content)
	show_menu()


## Небольшой параллакс фонового поля.
func _process(delta: float) -> void:
	elapsed += delta
	backdrop.position = Vector2(-32 + sin(elapsed * 0.15) * 20, -40 + cos(elapsed * 0.12) * 16)


## Очищает текущую форму перед переключением.
func clear_content() -> void:
	for child in content.get_children():
		content.remove_child(child)
		child.queue_free()


## Главный экран.
func show_menu() -> void:
	clear_content()
	content.add_child(ThemeFactory.label("H E X E M P I R E", 42))
	content.add_child(ThemeFactory.label("Квадратная карта. Большие решения.", 20))
	content.add_child(ThemeFactory.label("Пошаговая 4X • Godot 4 • прототип", 16))
	content.add_child(ThemeFactory.button("Новая игра", show_lobby))
	content.add_child(ThemeFactory.button("Продолжить • локальная БД", show_saves))
	content.add_child(
		ThemeFactory.button("Настройки / облако Supabase (опционально)", show_settings)
	)
	content.add_child(ThemeFactory.button("Выход", func() -> void: get_tree().quit()))
	notice = ThemeFactory.label(
		"SVG-заглушки • без звуков • сохранения в локальной БД на вашем устройстве", 14
	)
	content.add_child(notice)


## Форма лобби: один человек, три независимых слота ИИ.
func show_lobby() -> void:
	clear_content()
	slot_modes.clear()
	faction_picks.clear()
	content.add_child(ThemeFactory.label("Новая экспедиция", 30))
	size_pick = OptionButton.new()
	for key: String in ["small", "medium", "large"]:
		size_pick.add_item(str(GameManager.presets[key].name))
	content.add_child(size_pick)
	for i in 4:
		var row = HBoxContainer.new()
		var mode = OptionButton.new()
		if i == 0:
			mode.add_item("Вы • человек")
		else:
			for text in ["Нет", "ИИ • Easy", "ИИ • Medium", "ИИ • Hard"]:
				mode.add_item(text)
			mode.select(2 if i == 1 else 0)
		mode.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		row.add_child(mode)
		slot_modes.append(mode)
		var pick = OptionButton.new()
		for id in 4:
			pick.add_item(str(GameManager.faction_definitions[id].name))
		pick.add_item("Случайная")
		pick.select(i)
		pick.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		row.add_child(pick)
		faction_picks.append(pick)
		content.add_child(row)
	notice = ThemeFactory.label(
		"Фракции должны быть уникальными. Победа: города, армия или 30 раундов.", 16
	)
	content.add_child(notice)
	content.add_child(ThemeFactory.button("Начать →", start_game))
	content.add_child(ThemeFactory.button("← Назад", show_menu))


## Валидирует слоты, разрешает случайные фракции без дубликатов.
func start_game() -> void:
	var slots: Array = []
	var used: Array[int] = []
	var enabled: Array[int] = [0]
	for i in range(1, 4):
		if slot_modes[i].selected > 0:
			enabled.append(i)
	if enabled.size() < 2:
		notice.text = "Нужен хотя бы один ИИ."
		return
	for i in enabled:
		var id = faction_picks[i].selected
		if id == 4:
			continue
		if used.has(id):
			notice.text = "Выберите разные фракции."
			return
		used.append(id)
	for i in enabled:
		var id = faction_picks[i].selected
		if id == 4:
			var pool: Array[int] = []
			for possible in 4:
				if not used.has(possible):
					pool.append(possible)
			id = pool.pick_random()
			used.append(id)
		slots.append(
			{
				"id": id,
				"human": i == 0,
				"difficulty":
				"medium" if i == 0 else ["easy", "medium", "hard"][slot_modes[i].selected - 1]
			}
		)
	GameManager.new_game(["small", "medium", "large"][size_pick.selected], slots)
	get_tree().change_scene_to_file("res://scenes/GameWorld.tscn")


## Экран слотов локальной БД: загрузка и удаление сохранений.
func show_saves() -> void:
	clear_content()
	content.add_child(ThemeFactory.label("Локальная БД сохранений", 30))
	(
		content
		. add_child(
			(
				ThemeFactory
				. label(
					(
						"Сохранения хранятся в файле игры на этом устройстве, интернет и Supabase не нужны.\n"
						+ "Файл: user://saves/hexempire_saves.json"
					),
					16
				)
			)
		)
	)
	var scene: PackedScene = load("res://scenes/ui/SaveSlots.tscn")
	var slots_panel = scene.instantiate()
	slots_panel.slot_chosen.connect(_load_slot)
	content.add_child(slots_panel)
	slots_panel.refresh("load")
	notice = ThemeFactory.label("Выберите сохранение или вернитесь в меню.", 15)
	content.add_child(notice)
	content.add_child(ThemeFactory.button("← Назад", show_menu))


## Загружает выбранный слот и открывает поле только после успешной проверки.
func _load_slot(slot: String) -> void:
	if GameManager.load_slot(slot):
		get_tree().change_scene_to_file("res://scenes/GameWorld.tscn")
	else:
		notice.text = "Сохранение повреждено или несовместимо."


## Загружает автосохранение из локальной БД.
func load_game() -> void:
	if GameManager.load_local():
		get_tree().change_scene_to_file("res://scenes/GameWorld.tscn")
	else:
		notice.text = "Сохранение повреждено или несовместимо."


## Настройки облака сохраняют только URL и публичный ключ; пароль и токены не пишутся на диск.
func show_settings() -> void:
	clear_content()
	content.add_child(ThemeFactory.label("Облачные сохранения", 30))
	content.add_child(
		ThemeFactory.label(
			(
				"Опционально. Укажите Project URL и anon / publishable key. "
				+ "Никогда не вводите service_role или secret key."
			),
			16
		)
	)
	var config = SupabaseClient.read_config()
	var url = LineEdit.new()
	url.placeholder_text = "https://PROJECT.supabase.co"
	url.text = str(config.get("url", ""))
	content.add_child(url)
	var key = LineEdit.new()
	key.placeholder_text = "Публичный anon / publishable key"
	key.text = str(config.get("public_key", ""))
	content.add_child(key)
	notice = ThemeFactory.label("Вход и загрузка облачного слота доступны в меню партии.", 16)
	content.add_child(notice)
	content.add_child(ThemeFactory.button("Сохранить настройки", _save_config.bind(url, key)))
	content.add_child(ThemeFactory.button("← Назад", show_menu))


## Записывает только проверенные публичные настройки подключения.
func _save_config(url: LineEdit, key: LineEdit) -> void:
	var clean = url.text.strip_edges().trim_suffix("/")
	if (
		not clean.begins_with("https://")
		or not SupabaseClient.is_public_key(key.text.strip_edges())
	):
		notice.text = "Требуется HTTPS URL и только публичный ключ."
		return
	var file = FileAccess.open("user://supabase.json", FileAccess.WRITE)
	if file:
		file.store_string(JSON.stringify({"url": clean, "public_key": key.text.strip_edges()}))
		notice.text = "Настройки сохранены."
