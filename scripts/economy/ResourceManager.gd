## ResourceManager.gd — доход, население, улучшение города и таверны.
class_name ResourceManager
extends RefCounted


## Доход зависит от уровней городов, технологий и бонуса фракции.
func income(faction_id: int) -> int:
	var f = GameManager.faction_manager.get_faction(faction_id)
	var total = 0
	for t: WorldTile in GameManager.tiles.values():
		if t.faction_id != faction_id:
			continue
		if t.terrain == "city":
			total += t.level + int(f.techs.has("trade"))
		if t.terrain == "village":
			total += Constants.VILLAGE_STARS + int(f.techs.has("farming"))
		if t.terrain == "forest" and f.techs.has("hunting"):
			total += Constants.VILLAGE_STARS
	if (
		str(GameManager.faction_definitions[faction_id].bonus) == "stars"
		and f.turns_started % Constants.DESERT_BONUS_PERIOD == 0
	):
		total += Constants.VILLAGE_STARS
	return total


## Тратит ресурс только при достаточном балансе.
func spend(faction_id: int, amount: int) -> bool:
	var f = GameManager.faction_manager.get_faction(faction_id)
	if amount < 0 or f.stars < amount:
		return false
	f.stars -= amount
	return true


## Посещение дает население ближайшему своему городу один раз на клетку и фракцию.
func visit(unit: Unit) -> void:
	var t: WorldTile = GameManager.tiles[unit.grid_pos]
	var f = GameManager.faction_manager.get_faction(unit.faction_id)
	if t.terrain == "ruins" and not t.ruins_used:
		t.ruins_used = true
		f.stars += Constants.RUINS_STARS
	if t.terrain == "village":
		t.faction_id = unit.faction_id
	if t.visited.has(unit.faction_id):
		return
	t.visited.append(unit.faction_id)
	var nearest: WorldTile = null
	for city: WorldTile in GameManager.tiles.values():
		if city.terrain != "city" or city.faction_id != unit.faction_id:
			continue
		if Constants.distance(city.pos, t.pos) > Constants.CITY_RADIUS:
			continue
		if (
			nearest == null
			or Constants.distance(city.pos, t.pos) < Constants.distance(nearest.pos, t.pos)
		):
			nearest = city
	if nearest != null:
		nearest.population += 1
		if t.faction_id == -1:
			t.faction_id = unit.faction_id


## Улучшение требует населения, технологии и звезд; население расходуется.
func upgrade(city: WorldTile) -> bool:
	var faction_id = GameManager.current_faction_id()
	var f = GameManager.faction_manager.get_faction(faction_id)
	if GameManager.finished or city.terrain != "city" or city.faction_id != faction_id:
		return false
	var cap = Constants.BASE_CITY_CAP
	if f.techs.has("construction"):
		cap = Constants.CONSTRUCTION_CITY_CAP
	if f.techs.has("architecture"):
		cap = Constants.MAX_CITY_LEVEL
	if city.level >= cap or city.population < city.level * Constants.POPULATION_PER_LEVEL:
		return false
	if not spend(faction_id, city.level * Constants.CITY_UPGRADE_COST):
		return false
	city.population -= city.level * Constants.POPULATION_PER_LEVEL
	city.level += 1
	EventBus.changed()
	return true


## Таверна доступна в городе уровня 3+ после архитектуры.
func build_tavern(city: WorldTile) -> bool:
	var faction_id = GameManager.current_faction_id()
	var f = GameManager.faction_manager.get_faction(faction_id)
	if (
		GameManager.finished
		or city.faction_id != faction_id
		or city.terrain != "city"
		or city.level < Constants.TAVERN_MIN_LEVEL
		or city.tavern
		or not f.techs.has("architecture")
	):
		return false
	if not spend(faction_id, Constants.TAVERN_COST):
		return false
	city.tavern = true
	EventBus.changed()
	return true
