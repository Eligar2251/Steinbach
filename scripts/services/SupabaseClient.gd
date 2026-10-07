## SupabaseClient.gd — Supabase Auth + REST; только публичный ключ, RLS и токен в памяти.
class_name SupabaseClient
extends Node

var access_token: String = ""
var refresh_token: String = ""
var user_id: String = ""
var expires_at: int = 0
var request_busy: bool = false


## Читает локальные публичные настройки без токенов и паролей.
static func read_config() -> Dictionary:
	if not FileAccess.file_exists("user://supabase.json"):
		return {}
	var value: Variant = JSON.parse_string(FileAccess.get_file_as_string("user://supabase.json"))
	return value if value is Dictionary else {}


## Отклоняет администраторские ключи, включая legacy service_role JWT.
static func is_public_key(key: String) -> bool:
	if key.begins_with("sb_publishable_"):
		return true
	var parts = key.split(".")
	if parts.size() != 3:
		return false
	var payload = parts[1].replace("-", "+").replace("_", "/")
	while payload.length() % 4 != 0:
		payload += "="
	var claims: Variant = JSON.parse_string(Marshalls.base64_to_utf8(payload))
	return claims is Dictionary and str(claims.get("role", "")) == "anon"


## Выполняет один запрос; ошибки сети и не-JSON ответы не считаются успехом.
func _request(
	path: String, method: HTTPClient.Method, body: Variant = null, authenticated: bool = false
) -> Dictionary:
	if request_busy:
		return {"ok": false, "message": "Другой облачный запрос еще выполняется."}
	var config = read_config()
	var url = str(config.get("url", "")).trim_suffix("/")
	var key = str(config.get("public_key", ""))
	if not url.begins_with("https://") or not is_public_key(key):
		return {"ok": false, "message": "Настройте HTTPS URL и публичный ключ Supabase."}
	if authenticated and access_token.is_empty():
		return {"ok": false, "message": "Сначала войдите в аккаунт."}
	request_busy = true
	var http = HTTPRequest.new()
	http.timeout = 20.0
	add_child(http)
	var headers = PackedStringArray(
		[
			"apikey: " + key,
			"Content-Type: application/json",
			"Prefer: resolution=merge-duplicates,return=representation"
		]
	)
	if authenticated:
		headers.append("Authorization: Bearer " + access_token)
	var error = http.request(
		url + path, headers, method, "" if body == null else JSON.stringify(body)
	)
	if error != OK:
		http.queue_free()
		request_busy = false
		return {"ok": false, "message": "Не удалось отправить запрос: %d" % error}
	var response: Array = await http.request_completed
	http.queue_free()
	request_busy = false
	var status_code = int(response[1])
	var raw = (response[3] as PackedByteArray).get_string_from_utf8()
	var data: Variant = JSON.parse_string(raw)
	if int(response[0]) != HTTPRequest.RESULT_SUCCESS:
		return {"ok": false, "message": "Ошибка сети / TLS / таймаут."}
	if status_code < 200 or status_code >= 300:
		var message = "HTTP %d" % status_code
		if data is Dictionary:
			message = str(
				data.get("msg", data.get("message", data.get("error_description", message)))
			)
		return {"ok": false, "message": message, "status": status_code}
	return {"ok": true, "data": data, "message": "Готово."}


## Сохраняет успешную авторизацию только в оперативной памяти.
func _accept_session(result: Dictionary) -> bool:
	var data: Variant = result.get("data")
	if not result.get("ok", false) or not data is Dictionary or not data.has("access_token"):
		return false
	if not data.get("user") is Dictionary or not data.user.get("id") is String:
		return false
	access_token = str(data.access_token)
	refresh_token = str(data.get("refresh_token", ""))
	user_id = str(data.user.id)
	expires_at = int(Time.get_unix_time_from_system()) + int(data.get("expires_in", 3600))
	return true


## Регистрация пользователя; service_role никогда не используется.
func register(email: String, password: String) -> Dictionary:
	var result = await _request(
		"/auth/v1/signup",
		HTTPClient.METHOD_POST,
		{"email": email.strip_edges(), "password": password}
	)
	if _accept_session(result):
		result.message = "Зарегистрирован и авторизован."
	elif result.get("ok", false):
		result.message = "Регистрация отправлена. Подтвердите email, затем войдите."
	return result


## Парольный вход через Supabase Auth.
func login(email: String, password: String) -> Dictionary:
	var result = await _request(
		"/auth/v1/token?grant_type=password",
		HTTPClient.METHOD_POST,
		{"email": email.strip_edges(), "password": password}
	)
	if _accept_session(result):
		result.message = "Вход выполнен."
	elif result.get("ok", false):
		return {"ok": false, "message": "В ответе нет сессии."}
	return result


## Обновляет истекший access token перед REST операцией.
func _ensure_session() -> bool:
	if access_token.is_empty():
		return false
	if int(Time.get_unix_time_from_system()) < expires_at - 60:
		return true
	var result = await _request(
		"/auth/v1/token?grant_type=refresh_token",
		HTTPClient.METHOD_POST,
		{"refresh_token": refresh_token}
	)
	if _accept_session(result):
		return true
	sign_out()
	return false


## Создает или обновляет только личный слот autosave; faction_id проверяется RLS.
func save_game(state: Dictionary) -> Dictionary:
	if not await _ensure_session():
		return {"ok": false, "message": "Войдите в аккаунт (сессия истекла)."}
	var result = await _request(
		"/rest/v1/game_saves?on_conflict=owner_id,slot",
		HTTPClient.METHOD_POST,
		{
			"owner_id": user_id,
			"slot": "autosave",
			"state": state,
			"schema_version": Constants.SAVE_VERSION
		},
		true
	)
	if result.get("ok", false):
		result.message = "Личный слот сохранен в Supabase."
	return result


## Читает личный слот, остальные пользователи недоступны благодаря RLS.
func load_game() -> Dictionary:
	if not await _ensure_session():
		return {"ok": false, "message": "Войдите в аккаунт (сессия истекла)."}
	var result = await _request(
		(
			"/rest/v1/game_saves?select=state&owner_id=eq."
			+ user_id.uri_encode()
			+ "&slot=eq.autosave&limit=1"
		),
		HTTPClient.METHOD_GET,
		null,
		true
	)
	var data: Variant = result.get("data")
	if (
		result.get("ok", false)
		and data is Array
		and not data.is_empty()
		and data[0] is Dictionary
		and data[0].has("state")
	):
		return {"ok": true, "state": data[0].state, "message": "Сохранение загружено."}
	if result.get("ok", false):
		result.message = "Облачный слот пока пуст."
	return result


## Очищает локальную сессию; не хранит refresh token на устройстве.
func sign_out() -> void:
	access_token = ""
	refresh_token = ""
	user_id = ""
	expires_at = 0
