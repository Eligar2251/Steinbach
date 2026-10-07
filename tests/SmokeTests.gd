## SmokeTests.gd — headless интеграционные проверки правил и сериализации.
extends Node

var failures: int = 0
var checks: int = 0


## Ожидает autoload перед выполнением проверок.
func _ready() -> void:
	_run.call_deferred()


## Проверяет условие, не останавливая остальные тесты.
func check(value: bool, title: String) -> void:
	checks += 1
	if not value:
		failures += 1
		push_error("FAIL: " + title)


## Создает детерминированную тестовую партию.
func start(preset: String = "small", players: int = 2) -> void:
	var slots: Array = []
	for i in players:
		slots.append({"id": i, "human": i == 0, "difficulty": "hard"})
	GameManager.new_game(preset, slots, 42)


## Подготавливает плоскую карту для тестирования правил, не меняя JSON-справочники.
func flat() -> void:
	for t: WorldTile in GameManager.tiles.values():
		if t.terrain != "city":
			t.terrain = "grass"
	GameManager.refresh_visibility()


## Выполняет сценарии и возвращает ненулевой exit code при ошибке.
func _run() -> void:
	check(GameManager.unit_definitions.size() == 8, "units catalog")
	check(GameManager.hero_definitions.size() == 5, "heroes catalog")
	check(GameManager.tech_definitions.size() == 12, "tech catalog")
	for preset in ["small", "medium", "large"]:
		for players in [2, 3, 4]:
			start(preset, players)
			check(
				GameManager.tiles.size() == GameManager.map_size * GameManager.map_size,
				"map dimensions"
			)
			check(GameManager.units.size() == players, "start army")
			check(GameManager.valid_snapshot(GameManager.snapshot()), "valid initial snapshot")
	start()
	flat()
	var f = GameManager.factions[0]
	check(f.stars == Constants.START_STARS + 1, "initial stars plus turn income")
	var unit = GameManager.units[0]
	GameManager.tiles[Vector2i(2, 1)].terrain = "forest"
	check(GameManager.move_unit(unit, Vector2i(2, 1)), "MOV1 can enter slow forest")
	check(unit.movement_left == 0, "forest consumes action")
	check(not GameManager.move_unit(unit, Vector2i(3, 1)), "movement budget respected")
	check(
		not GameManager.recruit("archer", GameManager.tiles[Vector2i(1, 1)]),
		"tech gate recruitment"
	)
	check(GameManager.tech_manager.research(0, "hunting"), "research hunting")
	check(not GameManager.tech_manager.research(0, "hunting"), "no duplicate research")
	f.stars = 100
	check(GameManager.recruit("archer", GameManager.tiles[Vector2i(1, 1)]), "recruit archer")
	var recruited = GameManager.unit_at(Vector2i(1, 1))
	check(recruited.movement_left == 0 and recruited.attacked, "new unit waits until next turn")
	var before = GameManager.snapshot()
	check(GameManager.load_snapshot(before), "snapshot restore")
	check(JSON.stringify(GameManager.snapshot()) == JSON.stringify(before), "lossless roundtrip")
	check(GameManager.save_local(), "local save")
	check(GameManager.load_local(), "local load")
	check(not GameManager.load_snapshot({}), "invalid snapshot rejected")
	var broken = before.duplicate(true)
	broken.units[0].hp = 0
	check(not GameManager.valid_snapshot(broken), "dead unit snapshot rejected")
	start()
	flat()
	unit = GameManager.units[0]
	var enemy = GameManager.units[1]
	enemy.grid_pos = Vector2i(2, 1)
	GameManager.refresh_visibility()
	var enemy_hp = enemy.hp
	check(GameManager.attack_unit(unit, enemy), "adjacent attack")
	check(enemy.hp < enemy_hp and unit.hp < unit.max_hp, "damage and counterattack")
	check(not GameManager.attack_unit(unit, enemy), "one attack per turn")
	start()
	flat()
	var city: WorldTile = GameManager.tiles[Vector2i(1, 1)]
	f = GameManager.factions[0]
	f.stars = 100
	city.population = 20
	check(GameManager.resource_manager.upgrade(city), "city level 2")
	check(not GameManager.resource_manager.upgrade(city), "construction gate")
	GameManager.tech_manager.research(0, "construction")
	check(GameManager.resource_manager.upgrade(city), "city level 3")
	check(not GameManager.resource_manager.build_tavern(city), "architecture gate")
	GameManager.tech_manager.research(0, "architecture")
	check(GameManager.resource_manager.build_tavern(city), "build tavern")
	GameManager.units[0].grid_pos = Vector2i(2, 1)
	check(GameManager.recruit("hero_warlord", city), "recruit hero")
	check(not GameManager.recruit("hero_sage", city), "one hero limit")
	var hero = GameManager.hero_for(0)
	check(hero.use_ability(), "warlord ability")
	check(GameManager.units[0].attack_buff == Constants.WAR_CRY_BONUS, "warlord buff")
	check(not hero.use_ability(), "ability cooldown")
	hero.gain_xp(Constants.HERO_XP[0])
	check(hero.level == 2 and hero.pending_perks.size() == 2, "hero level and choice")
	hero.choose_perk(hero.pending_perks[0])
	check(hero.pending_perks.is_empty(), "perk consumed")
	GameManager.remove_unit(hero)
	check(f.hero_cooldown == Constants.HERO_REVIVE_TURNS, "hero death timer")
	check(GameManager.recruit_cost("hero_warlord", 0) == 5, "revive half price")
	check(not GameManager.recruit("hero_warlord", city), "revive blocked")
	for _turn in Constants.HERO_REVIVE_TURNS:
		GameManager.begin_current_turn()
	check(GameManager.recruit("hero_warlord", city), "revive after 5 own turns")
	for id in ["hero_sage", "hero_assassin", "hero_paladin", "hero_beastmaster"]:
		start()
		flat()
		hero = GameManager.spawn(id, 0, Vector2i(2, 1)) as Hero
		GameManager.refresh_visibility()
		if id == "hero_sage":
			check(hero.use_ability() and GameManager.factions[0].techs.size() == 1, "sage research")
		elif id == "hero_assassin":
			check(hero.use_ability() and hero.hidden_turns == 2, "assassin invisibility")
			check(not GameManager.fog.sees_unit(1, hero), "hidden from opponent")
		elif id == "hero_paladin":
			check(hero.shield, "paladin passive shield")
			enemy = GameManager.units[1]
			enemy.grid_pos = Vector2i(3, 1)
			GameManager.turn_index = 1
			GameManager.refresh_visibility()
			var old_hp = hero.hp
			GameManager.attack_unit(enemy, hero)
			check(hero.hp == old_hp and not hero.shield, "shield absorbs one attack")
		else:
			check(hero.use_ability(), "beast summon")
			var beast: Unit = null
			for u in GameManager.units:
				if u.type_id == "beast":
					beast = u
			check(
				beast != null and beast.hp == 15 and beast.life_turns == 3,
				"beast catalog and lifetime"
			)
			for _turn in Constants.BEAST_TURNS:
				beast.begin_turn()
			check(not GameManager.units.has(beast), "beast expiration")
	for mode in ["easy", "medium", "hard"]:
		start("large", 4)
		for _round in 4:
			for i in 4:
				GameManager.turn_index = i
				GameManager.begin_current_turn()
				var ai: AIPlayer = (
					AIEasy.new()
					if mode == "easy"
					else (AIMedium.new() if mode == "medium" else AIHard.new())
				)
				ai.faction_id = i
				ai.make_turn()
				ai.free()
				check(
					GameManager.valid_snapshot(GameManager.snapshot()), "AI legal snapshot " + mode
				)
	start()
	for u in GameManager.units.duplicate():
		if u.faction_id == 1:
			GameManager.remove_unit(u)
	GameManager.check_victory()
	check(GameManager.finished and GameManager.winner_id == 0, "extermination victory")
	start()
	GameManager.round_number = Constants.MAX_ROUNDS + 1
	GameManager.check_victory()
	check(GameManager.finished and GameManager.winner_id == -1, "30-round score tie")
	for slot: Dictionary in GameManager.list_saves():
		GameManager.delete_slot(str(slot.slot))
	start()
	check(GameManager.save_local(), "autosave into local DB")
	check(GameManager.list_saves().size() == 1, "local DB holds autosave")
	check(GameManager.save_to_slot("slot_1", "тест"), "save to named slot")
	check(GameManager.list_saves().size() == 2, "slot added")
	check(GameManager.next_free_slot() == 1, "next free slot")
	check(GameManager.load_slot("slot_1"), "load named slot")
	check(not GameManager.load_slot("slot_9"), "missing slot rejected")
	check(not GameManager.save_db.save_slot("bad", {}), "DB rejects invalid state")
	check(GameManager.delete_slot("slot_1"), "delete slot")
	check(GameManager.list_saves().size() == 1, "slot removed")
	check(GameManager.load_slot(SaveDatabase.AUTOSAVE_SLOT), "reload autosave slot")
	start()
	GameManager.turn_manager.end_turn()
	check(GameManager.busy, "turn reentry guard")
	GameManager.turn_manager.end_turn()
	await get_tree().create_timer(0.8).timeout
	check(not GameManager.busy and GameManager.current_faction_id() == 0, "AI returns turn")
	check(GameManager.round_number == 2, "round increments once")
	GameManager.turn_manager.end_turn()
	GameManager.clear_game()
	start()
	await get_tree().create_timer(0.8).timeout
	check(GameManager.round_number == 1 and not GameManager.busy, "cancel stale AI session")
	var malformed = GameManager.snapshot()
	malformed.tiles[0].x = {}
	check(not GameManager.valid_snapshot(malformed), "reject primitive type confusion")
	check(not SupabaseClient.is_public_key("sb_secret_test"), "reject secret key")
	var anon = Marshalls.utf8_to_base64('{"role":"anon"}')
	var admin = Marshalls.utf8_to_base64('{"role":"service_role"}')
	check(SupabaseClient.is_public_key("a." + anon + ".c"), "accept legacy anon key")
	check(not SupabaseClient.is_public_key("a." + admin + ".c"), "reject service_role JWT")
	GameManager.clear_game()
	await get_tree().process_frame
	print("SmokeTests: %d checks, %d failures" % [checks, failures])
	get_tree().quit(1 if failures else 0)
