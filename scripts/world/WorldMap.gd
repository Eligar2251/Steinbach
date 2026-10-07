## WorldMap.gd — TileMapLayer, туман, выбор клетки и камера с перетаскиванием.
extends Node2D

signal tile_clicked(pos: Vector2i)

var selected: Vector2i = Vector2i(-1, -1)
var highlights: Dictionary = {}
var viewer_id: int = 0
var dragging: bool = false
var drag_origin: Vector2 = Vector2.ZERO
var dragged: bool = false
var camera: Camera2D
var terrain_layer: TileMapLayer
var fog_layer: TileMapLayer


## Создает TileSet из текстовых SVG-заглушек и подключает обновление.
func _ready() -> void:
	camera = $Camera2D
	terrain_layer = $Terrain
	fog_layer = $Fog
	terrain_layer.tile_set = _make_tileset(false)
	fog_layer.tile_set = _make_tileset(true)
	# Сдвиг центра в свободную область слева от боковой панели.
	camera.offset.x = 150
	camera.position = Vector2.ONE * GameManager.map_size * Constants.TILE_SIZE / 2.0
	camera.zoom = Vector2.ONE * minf(0.9, 620.0 / (GameManager.map_size * Constants.TILE_SIZE))
	EventBus.state_changed.connect(refresh)
	refresh()


## Атлас для каждого типа клетки; отдельный слой тумана.
func _make_tileset(is_fog: bool) -> TileSet:
	var tile_set_resource = TileSet.new()
	tile_set_resource.tile_size = Vector2i.ONE * Constants.TILE_SIZE
	var names: Array[String] = []
	names.assign(["fog", "shroud"] if is_fog else Constants.TERRAIN)
	for i in names.size():
		var atlas = TileSetAtlasSource.new()
		atlas.texture = load("res://assets/textures/tiles/" + names[i] + ".svg")
		atlas.texture_region_size = Vector2i.ONE * Constants.TILE_SIZE
		atlas.create_tile(Vector2i.ZERO)
		tile_set_resource.add_source(atlas, i)
	return tile_set_resource


## Рисует карту, затем скрывает юнитов вне текущей видимости человека.
func refresh() -> void:
	terrain_layer.clear()
	fog_layer.clear()
	for t: WorldTile in GameManager.tiles.values():
		terrain_layer.set_cell(t.pos, Constants.TERRAIN.find(t.terrain), Vector2i.ZERO)
		if not GameManager.fog.knows(viewer_id, t.pos):
			fog_layer.set_cell(t.pos, 0, Vector2i.ZERO)
		elif not GameManager.fog.can_see(viewer_id, t.pos):
			fog_layer.set_cell(t.pos, 1, Vector2i.ZERO)
	for u in GameManager.units:
		u.visible = GameManager.fog.sees_unit(viewer_id, u)
	queue_redraw()


## Рамки городов, выбор клетки и доступные перемещения.
func _draw() -> void:
	for t: WorldTile in GameManager.tiles.values():
		if not GameManager.fog.knows(viewer_id, t.pos):
			continue
		var rect = Rect2(
			Vector2(t.pos * Constants.TILE_SIZE) + Vector2.ONE * 3,
			Vector2.ONE * (Constants.TILE_SIZE - 6)
		)
		if t.terrain == "city" and t.faction_id >= 0:
			draw_rect(rect, GameManager.faction_manager.get_faction(t.faction_id).color(), false, 3)
		if highlights.has(t.pos) and t.pos != selected:
			draw_circle(rect.get_center(), 6, Color(0.8, 0.9, 0.7, 0.7))
	if GameManager.tiles.has(selected):
		draw_rect(
			Rect2(
				Vector2(selected * Constants.TILE_SIZE) + Vector2.ONE * 2,
				Vector2.ONE * (Constants.TILE_SIZE - 4)
			),
			Color("#f4d58a"),
			false,
			3
		)


## Мышь и touch: клик выбирает, перетаскивание сдвигает карту, колесо масштабирует.
func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_LEFT:
			if event.pressed:
				dragging = true
				dragged = false
				drag_origin = event.position
			else:
				if dragging and not dragged:
					var p = terrain_layer.local_to_map(terrain_layer.get_local_mouse_position())
					if GameManager.tiles.has(p):
						tile_clicked.emit(p)
				dragging = false
		if event.pressed and event.button_index in [MOUSE_BUTTON_WHEEL_UP, MOUSE_BUTTON_WHEEL_DOWN]:
			var factor = 1.1 if event.button_index == MOUSE_BUTTON_WHEEL_UP else 0.9
			camera.zoom = Vector2.ONE * clampf(camera.zoom.x * factor, 0.25, 1.75)
	if event is InputEventMouseMotion and dragging:
		if event.position.distance_to(drag_origin) > 6:
			dragged = true
		if dragged:
			camera.position -= event.relative / camera.zoom
	if event is InputEventMagnifyGesture:
		camera.zoom = Vector2.ONE * clampf(camera.zoom.x * event.factor, 0.25, 1.75)
