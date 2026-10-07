## SaveSlotsController.gd — экран слотов встроенной локальной БД сохранений (без сервера).
extends PanelContainer

signal slot_chosen(slot: String)

var mode: String = "load"
var list: VBoxContainer
var notice: Label


## Создает прокручиваемый список слотов и строку состояния.
func _ready() -> void:
	theme = ThemeFactory.create()
	var box = VBoxContainer.new()
	box.add_theme_constant_override("separation", 10)
	add_child(box)
	var scroll = ScrollContainer.new()
	scroll.custom_minimum_size = Vector2(520, 300)
	box.add_child(scroll)
	list = VBoxContainer.new()
	list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	list.add_theme_constant_override("separation", 8)
	scroll.add_child(list)
	notice = ThemeFactory.label("", 15)
	box.add_child(notice)


## Перерисовывает список; mode = "load" или "save".
func refresh(new_mode: String, can_save: bool = true) -> void:
	mode = new_mode
	for child in list.get_children():
		list.remove_child(child)
		child.queue_free()
	list.add_child(ThemeFactory.label("Локальная БД: user://saves/hexempire_saves.json", 15))
	var saves = GameManager.list_saves()
	if saves.is_empty():
		list.add_child(ThemeFactory.label("Сохранений пока нет.", 16))
	for entry: Dictionary in saves:
		_row(entry, true, can_save)
	if mode == "save":
		var free_index = GameManager.next_free_slot()
		if free_index >= 0:
			list.add_child(
				ThemeFactory.button(
					"Новый слот %d" % (free_index + 1), _pick.bind("slot_%d" % (free_index + 1))
				)
			)
		else:
			list.add_child(
				ThemeFactory.label("Все слоты заняты — выберите слот для перезаписи.", 15)
			)
	list.add_child(ThemeFactory.button("Открыть папку сохранений", _open_folder))
	notice.text = GameManager.save_db.last_message


## Строка одного слота с датой, раундом и действиями.
func _row(entry: Dictionary, exists: bool, can_save: bool) -> void:
	var slot = str(entry.slot)
	var title = (
		"Автосохранение" if slot == SaveDatabase.AUTOSAVE_SLOT else slot.replace("slot_", "Слот ")
	)
	var box = PanelContainer.new()
	list.add_child(box)
	var row = VBoxContainer.new()
	box.add_child(row)
	row.add_child(
		ThemeFactory.label(
			(
				"%s · %s\n%s · карта %d · раунд %d · игроков %d"
				% [
					title,
					str(entry.time_text),
					str(entry.label),
					int(entry.map_size),
					int(entry.round),
					int(entry.factions)
				]
			),
			16
		)
	)
	var actions = HBoxContainer.new()
	row.add_child(actions)
	if exists and mode == "load":
		actions.add_child(ThemeFactory.button("Загрузить", _pick.bind(slot)))
	elif exists and can_save:
		actions.add_child(ThemeFactory.button("Перезаписать", _pick.bind(slot)))
	if exists and slot != SaveDatabase.AUTOSAVE_SLOT:
		actions.add_child(ThemeFactory.button("Удалить", _delete.bind(slot)))


## Подтверждает выбор слота для сохранения или загрузки.
func _pick(slot: String) -> void:
	slot_chosen.emit(slot)


## Удаляет слот и обновляет список.
func _delete(slot: String) -> void:
	GameManager.delete_slot(slot)
	refresh(mode, true)


## Открывает папку с файлом БД в проводнике системы.
func _open_folder() -> void:
	var path = ProjectSettings.globalize_path("user://saves")
	DirAccess.make_dir_recursive_absolute("user://saves")
	var code = OS.shell_open(path)
	notice.text = "Папка сохранений: %s" % path if code == OK else "Папка сохранений: %s" % path
