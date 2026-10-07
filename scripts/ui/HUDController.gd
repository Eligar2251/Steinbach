## HUDController.gd — адаптивный HUD, действия клетки, меню, сохранение и облако.
extends Control

var world_map: Node2D
var selected_unit: Unit = null
var selected_pos: Vector2i = Vector2i(-1, -1)
var human_id: int = 0
var header: Label
var status: Label
var side: VBoxContainer
var end_button: Button
var tech_button: Button
var side_panel: PanelContainer
var save_button: Button
var cloud: SupabaseClient
var cloud_status: Label
var cloud_dialog: AcceptDialog
var email_field: LineEdit
var password_field: LineEdit


## Создает панели с незахватывающим карту центральным пространством.
func _ready() -> void:
	theme = ThemeFactory.create()
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	for f in GameManager.factions:
		if f.human:
			human_id = f.id
	var top = PanelContainer.new()
	top.set_anchors_and_offsets_preset(Control.PRESET_TOP_WIDE)
	top.offset_bottom = 70
	add_child(top)
	var row = HBoxContainer.new()
	top.add_child(row)
	header = ThemeFactory.label("", 22)
	header.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	row.add_child(header)
	save_button = ThemeFactory.button("Сохранить", _save)
	row.add_child(save_button)
	row.add_child(ThemeFactory.button("Меню", _menu))
	side_panel = PanelContainer.new()
	side_panel.anchor_left = 1
	side_panel.anchor_right = 1
	side_panel.anchor_bottom = 1
	side_panel.offset_left = -320
	side_panel.offset_top = 82
	side_panel.offset_right = -12
	side_panel.offset_bottom = -90
	add_child(side_panel)
	var scroll = ScrollContainer.new()
	side_panel.add_child(scroll)
	side = VBoxContainer.new()
	side.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	side.custom_minimum_size.x = 265
	side.add_theme_constant_override("separation", 10)
	scroll.add_child(side)
	var bottom = PanelContainer.new()
	bottom.set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_WIDE)
	bottom.offset_top = -78
	add_child(bottom)
	var bottom_row = HBoxContainer.new()
	bottom.add_child(bottom_row)
	tech_button = ThemeFactory.button("Технологии", _techs)
	bottom_row.add_child(tech_button)
	status = ThemeFactory.label("Выберите юнита. Клик по клетке — ход, по врагу — атака.", 16)
	status.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	bottom_row.add_child(status)
	end_button = ThemeFactory.button("Конец хода →", GameManager.turn_manager.end_turn)
	bottom_row.add_child(end_button)
	cloud = SupabaseClient.new()
	add_child(cloud)
	EventBus.state_changed.connect(refresh)
	EventBus.message.connect(func(text: String) -> void: status.text = text)
	EventBus.unit_died.connect(
		func(u: Unit) -> void:
			if selected_unit == u:
				selected_unit = null
	)
	EventBus.game_over.connect(_game_over)
	resized.connect(_resize)
	_resize()


## Уменьшает боковую панель на узких экранах; базовый layout масштабируется canvas_items.
func _resize() -> void:
	if side_panel:
		side_panel.offset_left = -minf(320, maxf(240, size.x * 0.26))


## Связывает выбор карты и HUD.
func bind_map(map: Node2D) -> void:
	world_map = map
	world_map.tile_clicked.connect(_tile_clicked)
	refresh()
	if GameManager.finished:
		_game_over(GameManager.winner_id)


## Разрешены ли сейчас команды человека.
func can_act() -> bool:
	return (
		not GameManager.busy
		and not GameManager.finished
		and GameManager.current_faction_id() == human_id
		and not GameManager.faction_manager.get_faction(human_id).eliminated
	)


## Выбор клетки, движение или атака только видимого врага.
func _tile_clicked(p: Vector2i) -> void:
	if not GameManager.fog.knows(human_id, p):
		status.text = "Клетка не исследована. Подведите юнита ближе."
		return
	var target = GameManager.unit_at(p)
	if target != null and not GameManager.fog.sees_unit(human_id, target):
		target = null
	if can_act() and is_instance_valid(selected_unit) and selected_unit.faction_id == human_id:
		if target != null and target.faction_id != human_id:
			if GameManager.attack_unit(selected_unit, target):
				refresh()
				return
		elif target == null and GameManager.move_unit(selected_unit, p):
			selected_pos = p
			refresh()
			return
	selected_pos = p
	selected_unit = target
	refresh()


## Обновляет ресурсы и панели; все действия повторно проверяются моделью.
func refresh() -> void:
	if not GameManager.active or header == null:
		return
	var f = GameManager.faction_manager.get_faction(human_id)
	var current = GameManager.faction_manager.get_faction(GameManager.current_faction_id())
	if f == null or current == null:
		return
	header.text = (
		"%s   ⭐ %d (+%d)   Раунд %d/%d\nХод: %s"
		% [
			f.display_name(),
			f.stars,
			GameManager.resource_manager.income(human_id),
			mini(GameManager.round_number, Constants.MAX_ROUNDS),
			Constants.MAX_ROUNDS,
			current.display_name()
		]
	)
	header.modulate = f.color()
	end_button.disabled = (
		GameManager.busy
		or GameManager.finished
		or (GameManager.current_faction_id() != human_id and not f.eliminated)
	)
	end_button.text = (
		"ИИ думает…"
		if GameManager.busy
		else ("Продолжить наблюдение →" if f.eliminated else "Конец хода →")
	)
	tech_button.disabled = not can_act()
	save_button.disabled = GameManager.busy
	for child in side.get_children():
		side.remove_child(child)
		child.queue_free()
	if not is_instance_valid(selected_unit) or not GameManager.units.has(selected_unit):
		selected_unit = null
	if selected_unit != null and not GameManager.fog.sees_unit(human_id, selected_unit):
		selected_unit = null
	if selected_unit is Hero:
		var portrait_scene: PackedScene = load("res://scenes/ui/HeroPortraitPanel.tscn")
		var panel = portrait_scene.instantiate()
		side.add_child(panel)
		panel.show_hero(selected_unit, can_act() and selected_unit.faction_id == human_id)
	elif selected_unit != null:
		side.add_child(ThemeFactory.label(str(selected_unit.definition.name), 24))
		side.add_child(
			ThemeFactory.label(
				(
					"HP %d/%d\nATK %d (+%d) · DEF %d\nMOV %d/%d · RANGE %d\nАтака: %s"
					% [
						selected_unit.hp,
						selected_unit.max_hp,
						selected_unit.attack,
						selected_unit.attack_buff,
						selected_unit.defense,
						selected_unit.movement_left,
						selected_unit.move_range,
						selected_unit.attack_range,
						"использована" if selected_unit.attacked else "готова"
					]
				)
			)
		)
	if GameManager.tiles.has(selected_pos) and GameManager.fog.knows(human_id, selected_pos):
		var t: WorldTile = GameManager.tiles[selected_pos]
		side.add_child(
			ThemeFactory.label(
				"%s · (%d, %d)" % [Constants.TERRAIN_NAMES[t.terrain], t.pos.x, t.pos.y], 22
			)
		)
		side.add_child(
			ThemeFactory.label("Защита +%d" % int(Constants.TERRAIN_DEFENSE.get(t.terrain, 0)))
		)
		if not GameManager.fog.can_see(human_id, selected_pos):
			side.add_child(ThemeFactory.label("Нет текущей видимости: враги скрыты.", 14))
		if t.terrain == "city":
			_city_panel(t)
	else:
		side.add_child(ThemeFactory.label("Ваша империя", 26))
		side.add_child(
			ThemeFactory.label(
				(
					"Исследуйте землю, занимайте деревни и захватывайте города.\n\n"
					+ "1. Выберите своего юнита\n2. Кликните по отмеченной клетке\n"
					+ "3. Исследуйте технологии\n4. Завершите ход\n\n"
					+ "Перетаскивание — камера\nКолесо / жест — масштаб"
				),
				18
			)
		)
	if world_map:
		world_map.selected = selected_pos
		world_map.highlights = (
			GameManager.reachable(selected_unit)
			if selected_unit != null and selected_unit.faction_id == human_id and can_act()
			else {}
		)
		world_map.queue_redraw()


## Действия города, таверны и найма из JSON.
func _city_panel(city: WorldTile) -> void:
	side.add_child(
		ThemeFactory.label(
			(
				"Уровень %d · +%d ⭐/ход\nНаселение %d/%d%s"
				% [
					city.level,
					city.level,
					city.population,
					city.level * Constants.POPULATION_PER_LEVEL,
					"\nТаверна построена" if city.tavern else ""
				]
			)
		)
	)
	if city.faction_id != human_id:
		return
	var enabled = can_act()
	var upgrade = ThemeFactory.button(
		"Улучшить · %d ⭐" % (city.level * Constants.CITY_UPGRADE_COST),
		func() -> void:
			if not GameManager.resource_manager.upgrade(city):
				status.text = "Нужны население, звезды и технология для следующего уровня."
	)
	upgrade.disabled = not enabled or city.level >= Constants.MAX_CITY_LEVEL
	side.add_child(upgrade)
	if not city.tavern:
		var tavern = ThemeFactory.button(
			"Таверна · %d ⭐" % Constants.TAVERN_COST,
			func() -> void:
				if not GameManager.resource_manager.build_tavern(city):
					status.text = "Требуются город уровня 3+, Архитектура и 8 звезд."
		)
		tavern.disabled = not enabled
		side.add_child(tavern)
	for d: Dictionary in GameManager.unit_definitions.values():
		if str(d.required_tech) == "summon_only":
			continue
		var b = ThemeFactory.button("%s · %d ⭐" % [d.name, d.cost], _recruit.bind(str(d.id), city))
		var f = GameManager.faction_manager.get_faction(human_id)
		b.disabled = (
			not enabled
			or (str(d.required_tech) != "none" and not f.techs.has(str(d.required_tech)))
			or f.stars < int(d.cost)
		)
		side.add_child(b)
	if city.tavern:
		var f = GameManager.faction_manager.get_faction(human_id)
		side.add_child(
			ThemeFactory.label("Герои · возрождение через %d ходов" % f.hero_cooldown, 16)
		)
		for d: Dictionary in GameManager.hero_definitions.values():
			var b = ThemeFactory.button(
				"%s · %d ⭐" % [d.name, GameManager.recruit_cost(str(d.id), human_id)],
				_recruit.bind(str(d.id), city)
			)
			b.disabled = (
				not enabled
				or GameManager.hero_for(human_id) != null
				or f.hero_cooldown > 0
				or f.stars < GameManager.recruit_cost(str(d.id), human_id)
			)
			side.add_child(b)


## Найм с подсказкой о занятой клетке и технологии.
func _recruit(type_id: String, city: WorldTile) -> void:
	if not GameManager.recruit(type_id, city):
		status.text = "Найм недоступен: проверьте место, звезды, технологию и таверну."


## Открывает дерево технологий в модальном окне.
func _techs() -> void:
	var dialog = AcceptDialog.new()
	dialog.title = "Дерево технологий"
	var scene: PackedScene = load("res://scenes/ui/TechTree.tscn")
	var tree = scene.instantiate()
	dialog.add_child(tree)
	add_child(dialog)
	tree.refresh(human_id, can_act())
	dialog.popup_centered(Vector2i(660, 460))
	dialog.confirmed.connect(dialog.queue_free)


## Ручное локальное сохранение.
func _save() -> void:
	status.text = (
		"Сохранено в локальную БД (слот autosave)."
		if GameManager.save_local()
		else "Сохранение недоступно во время хода ИИ."
	)


## Окно слотов локальной БД для записи текущей партии.
func _save_slots() -> void:
	var dialog = AcceptDialog.new()
	dialog.title = "Сохранить в локальную БД"
	var scene: PackedScene = load("res://scenes/ui/SaveSlots.tscn")
	var slots_panel = scene.instantiate()
	slots_panel.slot_chosen.connect(_write_slot.bind(dialog))
	dialog.add_child(slots_panel)
	add_child(dialog)
	slots_panel.refresh("save", not GameManager.busy)
	dialog.popup_centered(Vector2i(640, 460))
	dialog.confirmed.connect(dialog.queue_free)


## Записывает партию в выбранный слот локальной БД.
func _write_slot(slot: String, dialog: AcceptDialog) -> void:
	if GameManager.busy:
		status.text = "Сохранение недоступно во время хода ИИ."
		return
	status.text = (
		"Сохранено в локальную БД: %s" % slot
		if GameManager.save_to_slot(slot)
		else "Сохранение не удалось."
	)
	dialog.hide()


## Меню не приостанавливает ИИ; опасные действия блокируются busy.
func _menu() -> void:
	var dialog = AcceptDialog.new()
	dialog.title = "Меню партии"
	var box = VBoxContainer.new()
	box.add_child(ThemeFactory.button("Сохранить (автослот)", _save))
	box.add_child(ThemeFactory.button("Сохранить в слот…", _save_slots))
	box.add_child(ThemeFactory.button("Облако Supabase", _show_cloud_from_menu.bind(dialog)))
	box.add_child(ThemeFactory.button("Главное меню", _return_menu))
	dialog.add_child(box)
	add_child(dialog)
	dialog.popup_centered(Vector2i(420, 240))
	dialog.confirmed.connect(dialog.queue_free)


## Экран результата с очками каждой фракции.
func _game_over(_winner: int) -> void:
	get_tree().change_scene_to_file.call_deferred("res://scenes/VictoryScreen.tscn")


## Авторизация облака и операции одного пользовательского слота.
func _open_cloud() -> void:
	cloud_dialog = AcceptDialog.new()
	cloud_dialog.title = "Supabase • личное сохранение"
	var box = VBoxContainer.new()
	email_field = LineEdit.new()
	email_field.placeholder_text = "Email"
	box.add_child(email_field)
	password_field = LineEdit.new()
	password_field.placeholder_text = "Пароль"
	password_field.secret = true
	box.add_child(password_field)
	box.add_child(ThemeFactory.button("Регистрация", _cloud_register))
	box.add_child(ThemeFactory.button("Войти", _cloud_login))
	box.add_child(ThemeFactory.button("Сохранить в облако", _cloud_save))
	box.add_child(ThemeFactory.button("Загрузить из облака", _cloud_load))
	box.add_child(ThemeFactory.button("Выйти из аккаунта", _cloud_logout))
	cloud_status = ThemeFactory.label(
		"URL и публичный ключ задаются в настройках главного меню.", 16
	)
	box.add_child(cloud_status)
	cloud_dialog.add_child(box)
	add_child(cloud_dialog)
	cloud_dialog.popup_centered(Vector2i(500, 480))
	cloud_dialog.confirmed.connect(cloud_dialog.queue_free)


## Регистрация через Supabase Auth; подтверждение email зависит от проекта.
func _cloud_register() -> void:
	var result: Dictionary = await cloud.register(email_field.text, password_field.text)
	if is_instance_valid(cloud_status):
		cloud_status.text = str(result.get("message", "Регистрация отправлена; проверьте почту."))


## Парольный вход, токены остаются только в памяти.
func _cloud_login() -> void:
	var result: Dictionary = await cloud.login(email_field.text, password_field.text)
	if is_instance_valid(cloud_status):
		cloud_status.text = str(result.message)
	if is_instance_valid(password_field):
		password_field.clear()


## Отправляет стабильный JSON-снимок под RLS текущего пользователя.
func _cloud_save() -> void:
	if GameManager.busy:
		cloud_status.text = "Дождитесь своего хода."
		return
	var result: Dictionary = await cloud.save_game(GameManager.snapshot())
	if is_instance_valid(cloud_status):
		cloud_status.text = str(result.message)


## Загрузка проверяет снимок перед заменой текущей партии.
func _cloud_load() -> void:
	if GameManager.busy:
		cloud_status.text = "Дождитесь своего хода."
		return
	var result: Dictionary = await cloud.load_game()
	if not is_instance_valid(cloud_status):
		return
	if bool(result.get("ok", false)) and GameManager.load_snapshot(result.get("state")):
		get_tree().change_scene_to_file("res://scenes/GameWorld.tscn")
	else:
		cloud_status.text = str(result.get("message", "Некорректное облачное сохранение."))


## Скрывает меню перед открытием облачного окна.
func _show_cloud_from_menu(dialog: AcceptDialog) -> void:
	dialog.hide()
	_open_cloud()


## Отменяет текущую партию и возвращается в меню.
func _return_menu() -> void:
	if not GameManager.busy:
		GameManager.save_local()
	GameManager.clear_game()
	get_tree().change_scene_to_file("res://scenes/MainMenu.tscn")


## Завершает локальную облачную сессию.
func _cloud_logout() -> void:
	cloud.sign_out()
	cloud_status.text = "Локальная сессия завершена."
