## Hero.gd — опыт, перки и пять способностей особого персонажа.
class_name Hero
extends Unit

var level: int = 1
var xp: int = 0
var cooldown: int = 0
var hidden_turns: int = 0
var shield: bool = false
var ability_enhanced: bool = false
var pending_perks: Array[String] = []


## Перезарядка способностей и пассивный щит паладина.
func begin_turn() -> void:
	super.begin_turn()
	cooldown = maxi(0, cooldown - 1)
	hidden_turns = maxi(0, hidden_turns - 1)
	if type_id == "hero_paladin" and cooldown == 0:
		shield = true
		cooldown = maxi(1, int(definition.cooldown) - int(ability_enhanced))


## Добавление XP; превышение порога сохраняется, выбор перка блокирует дальнейшую прокачку.
func gain_xp(amount: int) -> void:
	xp += amount
	_check_level()


## Проверяет возможность очередного повышения уровня.
func _check_level() -> void:
	if level >= Constants.HERO_XP.size() + 1 or not pending_perks.is_empty():
		return
	if xp < Constants.HERO_XP[level - 1]:
		return
	xp -= Constants.HERO_XP[level - 1]
	level += 1
	var pool: Array[String] = ["hp", "attack", "defense", "move"]
	if not ability_enhanced:
		pool.append("ability")
	pool.shuffle()
	pending_perks.assign(pool.slice(0, 2))
	EventBus.leveled(self)
	if not GameManager.faction_manager.get_faction(faction_id).human:
		choose_perk(pending_perks[0])


## Применяет выбранный перк; улучшение способности сокращает кулдаун на один ход.
func choose_perk(perk: String) -> void:
	if not pending_perks.has(perk):
		return
	match perk:
		"hp":
			max_hp += Constants.PERK_HP
			hp += Constants.PERK_HP
		"attack":
			attack += Constants.PERK_STAT
		"defense":
			defense += Constants.PERK_STAT
		"move":
			move_range += Constants.PERK_STAT
		"ability":
			ability_enhanced = true
	pending_perks.clear()
	_check_level()
	EventBus.changed()


## Активирует способность только в собственный ход; паладин действует пассивно.
func use_ability() -> bool:
	if GameManager.current_faction_id() != faction_id or GameManager.finished or cooldown > 0:
		return false
	match type_id:
		"hero_warlord":
			for u in GameManager.units:
				if (
					u.faction_id == faction_id
					and Constants.distance(grid_pos, u.grid_pos) <= Constants.VISION
				):
					u.attack_buff = Constants.WAR_CRY_BONUS
		"hero_sage":
			var choices = GameManager.tech_manager.available(faction_id)
			if choices.is_empty():
				return false
			GameManager.tech_manager.research(faction_id, str(choices.pick_random().id), true)
		"hero_assassin":
			hidden_turns = Constants.SHADOW_TURNS
		"hero_paladin":
			return false
		"hero_beastmaster":
			var summoned = false
			for delta in Constants.NEIGHBORS:
				var p = grid_pos + delta
				if GameManager.can_spawn("beast", faction_id, p):
					var beast = GameManager.spawn("beast", faction_id, p)
					beast.life_turns = Constants.BEAST_TURNS
					summoned = true
					break
			if not summoned:
				return false
	cooldown = maxi(1, int(definition.cooldown) - int(ability_enhanced))
	EventBus.ability(self)
	GameManager.refresh_visibility()
	return true


## Дополняет сериализацию полями героя.
func to_dict() -> Dictionary:
	var d = super.to_dict()
	d.merge(
		{
			"level": level,
			"xp": xp,
			"cooldown": cooldown,
			"hidden_turns": hidden_turns,
			"shield": shield,
			"ability_enhanced": ability_enhanced,
			"pending_perks": pending_perks
		}
	)
	return d


## Восстанавливает опыт, перки и временные эффекты героя.
func restore(d: Dictionary) -> void:
	super.restore(d)
	level = int(d.level)
	xp = int(d.xp)
	cooldown = int(d.cooldown)
	hidden_turns = int(d.hidden_turns)
	shield = bool(d.shield)
	ability_enhanced = bool(d.ability_enhanced)
	pending_perks.assign(d.pending_perks)
