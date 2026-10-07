## SaveDatabase.gd — встроенная локальная БД сохранений (JSON в user://), без сервера и интернета.
class_name SaveDatabase
extends RefCounted

const DB_PATH: String = "user://saves/hexempire_saves.json"
const TMP_PATH: String = "user://saves/hexempire_saves.json.tmp"
const BACKUP_PATH: String = "user://saves/hexempire_saves.json.bak"
const LEGACY_PATH: String = "user://saves/autosave.json"
const AUTOSAVE_SLOT: String = "autosave"

var slots: Dictionary = {}
var last_message: String = ""


## Читает файл БД при первом обращении; поврежденный файл не удаляет данные.
func load_db() -> Dictionary:
	if not slots.is_empty():
		return slots
	var raw: Variant = null
	if FileAccess.file_exists(DB_PATH):
		raw = Constants.read_json(DB_PATH)
		if not raw is Dictionary:
			raw = Constants.read_json(BACKUP_PATH)
	if not raw is Dictionary:
		raw = {}
	slots = _clean_db(raw)
	return slots


## Отбрасывает записи с некорректным снимком, сохраняя остальные слоты.
func _clean_db(raw: Dictionary) -> Dictionary:
	var result: Dictionary = {}
	var entries: Variant = raw.get("slots", {})
	if entries is Dictionary:
		for key: Variant in entries:
			if not key is String:
				continue
			var entry: Variant = entries[key]
			if not entry is Dictionary:
				continue
			if not GameManager.valid_snapshot(entry.get("state")):
				continue
			result[str(key)] = _normalize_entry(str(key), entry)
	if result.is_empty():
		last_message = "Локальная БД пуста или повреждена; старые слоты пропущены."
		result = _import_legacy()
	return result


## Переносит старый autosave.json прошлых версий в слот автосохранения.
func _import_legacy() -> Dictionary:
	if not FileAccess.file_exists(LEGACY_PATH):
		return {}
	var state: Variant = Constants.read_json(LEGACY_PATH)
	if not GameManager.valid_snapshot(state):
		return {}
	var entry := _normalize_entry(
		AUTOSAVE_SLOT,
		{
			"label": "Перенесено из старого сохранения",
			"saved_at": int(Time.get_unix_time_from_system()),
			"time_text": Time.get_datetime_string_from_system(true),
			"round": int(state.get("round", 1)),
			"map_size": int(state.get("map_size", 0)),
			"factions": int((state.get("factions", []) as Array).size()),
			"schema_version": Constants.SAVE_VERSION,
			"state": state
		}
	)
	last_message = "Старое autosave.json перенесено в локальную БД."
	slots[AUTOSAVE_SLOT] = entry
	_write()
	return {AUTOSAVE_SLOT: entry}


## Приводит метаданные слота к допустимым строковым и числовым полям.
func _normalize_entry(slot: String, entry: Dictionary) -> Dictionary:
	return {
		"slot": slot,
		"label": str(entry.get("label", slot)).substr(0, 60),
		"saved_at": int(entry.get("saved_at", 0)),
		"time_text": str(entry.get("time_text", "")).substr(0, 40),
		"round": int(entry.get("round", 1)),
		"map_size": int(entry.get("map_size", 0)),
		"factions": int(entry.get("factions", 0)),
		"schema_version": int(entry.get("schema_version", Constants.SAVE_VERSION)),
		"state": entry.state
	}


## Список слотов для UI: автосейв первым, затем по времени обновления.
func list_slots() -> Array[Dictionary]:
	load_db()
	var result: Array[Dictionary] = []
	for key: Variant in slots:
		result.append(slots[key])
	result.sort_custom(
		func(a: Dictionary, b: Dictionary) -> bool:
			if str(a.slot) == AUTOSAVE_SLOT:
				return true
			if str(b.slot) == AUTOSAVE_SLOT:
				return false
			return int(a.saved_at) > int(b.saved_at)
	)
	return result


## Номер следующего свободного слота или -1 при переполнении.
func next_free_slot() -> int:
	load_db()
	for i in Constants.SAVE_SLOTS:
		if not slots.has("slot_%d" % (i + 1)):
			return i
	return -1


## Записывает партию в слот; снимок проверяется до обращения к диску.
func save_slot(slot: String, state: Dictionary, label: String = "") -> bool:
	if not GameManager.valid_snapshot(state):
		last_message = "Снимок не прошел проверку, запись отменена."
		return false
	load_db()
	var stamp := int(Time.get_unix_time_from_system())
	slots[slot] = _normalize_entry(
		slot,
		{
			"label": label if not label.is_empty() else _default_label(state),
			"saved_at": stamp,
			"time_text": Time.get_datetime_string_from_system(true),
			"round": int(state.get("round", 1)),
			"map_size": int(state.get("map_size", 0)),
			"factions": int((state.get("factions", []) as Array).size()),
			"schema_version": Constants.SAVE_VERSION,
			"state": state
		}
	)
	if not _write():
		slots.erase(slot)
		last_message = "Не удалось записать локальную БД (нет места или доступ запрещен)."
		return false
	last_message = "Сохранено в локальную БД, слот «%s»." % slot
	return true


## Подпись слота по умолчанию: размер карты, раунд и число игроков.
func _default_label(state: Dictionary) -> String:
	return (
		"Карта %s • раунд %d • %d игрока"
		% [
			int(state.get("map_size", 0)),
			int(state.get("round", 1)),
			int((state.get("factions", []) as Array).size())
		]
	)


## Читает снимок слота; пустой или поврежденный слот возвращает null.
func load_slot(slot: String) -> Variant:
	load_db()
	if not slots.has(slot):
		last_message = "Слот «%s» пуст." % slot
		return null
	var state: Variant = slots[slot].state
	if not GameManager.valid_snapshot(state):
		last_message = "Слот «%s» поврежден, загрузка отменена." % slot
		return null
	last_message = "Загружен слот «%s» из локальной БД." % slot
	return state


## Удаляет слот и перезаписывает БД.
func delete_slot(slot: String) -> bool:
	load_db()
	if not slots.has(slot):
		return false
	slots.erase(slot)
	if not _write():
		last_message = "Не удалось обновить локальную БД."
		return false
	last_message = "Слот «%s» удален." % slot
	return true


## Автосохранение на границе хода человека.
func autosave(state: Dictionary) -> bool:
	return save_slot(AUTOSAVE_SLOT, state, "Автосохранение")


## Атомарная запись: tmp → резервная копия → переименование.
func _write() -> bool:
	DirAccess.make_dir_recursive_absolute("user://saves")
	var data := {"version": Constants.SAVE_VERSION, "slots": slots}
	var file = FileAccess.open(TMP_PATH, FileAccess.WRITE)
	if file == null:
		return false
	file.store_string(JSON.stringify(data))
	file.close()
	if FileAccess.file_exists(BACKUP_PATH):
		DirAccess.remove_absolute(BACKUP_PATH)
	if FileAccess.file_exists(DB_PATH):
		DirAccess.rename_absolute(DB_PATH, BACKUP_PATH)
	return DirAccess.rename_absolute(TMP_PATH, DB_PATH) == OK


## Есть ли хотя бы одна сохраненная партия.
func has_any() -> bool:
	return not list_slots().is_empty()
