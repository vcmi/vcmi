/*
 * CGameInfoCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CGameInfoCallback.h"

#include "../entities/building/CBuilding.h"
#include "../gameState/CGameState.h"
#include "../gameState/UpgradeInfo.h"
#include "../gameState/InfoAboutArmy.h"
#include "../gameState/TavernHeroesPool.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGTownInstance.h"
#include "../mapObjects/MiscObjects.h"
#include "../StartInfo.h"
#include "../battle/BattleInfo.h"
#include "../IGameSettings.h"
#include "../spells/CSpellHandler.h"
#include "../mapping/CMap.h"
#include "../CPlayerState.h"

#define ASSERT_IF_CALLED_WITH_PLAYER if(!getPlayerID()) {logGlobal->error(BOOST_CURRENT_FUNCTION); assert(0);}

VCMI_LIB_NAMESPACE_BEGIN

//TODO make clean
#define ERROR_VERBOSE_OR_NOT_RET_VAL_IF(cond, verbose, txt, retVal) do {if(cond){if(verbose)logGlobal->error("%s: %s",BOOST_CURRENT_FUNCTION, txt); return retVal;}} while(0)
#define ERROR_RET_IF(cond, txt) do {if(cond){logGlobal->error("%s: %s", BOOST_CURRENT_FUNCTION, txt); return;}} while(0)
#define ERROR_RET_VAL_IF(cond, txt, retVal) do {if(cond){logGlobal->error("%s: %s", BOOST_CURRENT_FUNCTION, txt); return retVal;}} while(0)

const IMarket * CGameInfoCallback::getMarket(ObjectInstanceID objid) const
{
	const CGObjectInstance * obj = getObj(objid, false);
	return dynamic_cast<const IMarket*>(obj);
}

int CGameInfoCallback::getResource(PlayerColor Player, GameResID which) const
{
	const PlayerState *p = getPlayerState(Player);
	ERROR_RET_VAL_IF(!p, "No player info!", -1);
	ERROR_RET_VAL_IF(p->resources.size() <= which.getNum() || which.getNum() < 0, "No such resource!", -1);
	return p->resources[which];
}

const PlayerSettings * CGameInfoCallback::getPlayerSettings(PlayerColor color) const
{
	return &gameState().getStartInfo()->getIthPlayersSettings(color);
}

std::optional<PlayerColor> CGameInfoCallback::getPlayerID() const
{
	return std::nullopt;
}

const Player * CGameInfoCallback::getPlayer(PlayerColor color) const
{
	return getPlayerState(color, false);
}

const PlayerState * CGameInfoCallback::getPlayerState(PlayerColor color, bool verbose) const
{
	//function written from scratch since it's accessed A LOT by AI

	if(!color.isValidPlayer())
	{
		return nullptr;
	}
	auto player = gameState().players.find(color);
	if (player != gameState().players.end())
	{
		if (hasAccess(color))
			return &player->second;
		else
		{
			if (verbose)
				logGlobal->error("Cannot access player %d info!", color);
			return nullptr;
		}
	}
	else
	{
		if (verbose)
			logGlobal->error("Cannot find player %d info!", color);
		return nullptr;
	}
}

TurnTimerInfo CGameInfoCallback::getPlayerTurnTime(PlayerColor color) const
{
	if(!color.isValidPlayer())
	{
		return TurnTimerInfo{};
	}
	
	auto player = gameState().players.find(color);
	if(player != gameState().players.end())
	{
		return player->second.turnTimer;
	}
	
	return TurnTimerInfo{};
}

/************************************************************************/
/*                                                                      */
/************************************************************************/

const CGObjectInstance* CGameInfoCallback::getObj(ObjectInstanceID objid, bool verbose) const
{
	const CGObjectInstance * ret = MapInfoCallback::getObj(objid, verbose);

	if(!ret)
		return nullptr;

	if(getPlayerID().has_value() && !isVisibleFor(ret, *getPlayerID()) && ret->tempOwner != getPlayerID())
	{
		if(verbose)
			logGlobal->error("Cannot get object with id %d. Object is not visible.", objid.getNum());
		return nullptr;
	}

	return ret;
}

void CGameInfoCallback::fillUpgradeInfo(const CArmedInstance *obj, SlotID stackPos, UpgradeInfo & out) const
{
	ERROR_RET_IF(!canGetFullInfo(obj), "Cannot get info about not owned object!");
	ERROR_RET_IF(!obj->hasStackAtSlot(stackPos), "There is no such stack!");

	const auto & stack = obj->getStack(stackPos);
	const CCreature *base = stack.getCreature();
	UpgradeInfo ret(base->getId());

	if (stack.getArmy()->ID == Obj::HERO)
	{
		auto hero = dynamic_cast<const CGHeroInstance *>(stack.getArmy());
		hero->fillUpgradeInfo(ret, stack);

		if (hero->getVisitedTown())
		{
			hero->getVisitedTown()->fillUpgradeInfo(ret, stack);
		}
		else
		{
			auto object = vstd::frontOrNull(getVisitableObjs(hero->visitablePos()));
			auto upgradeSource = dynamic_cast<const ICreatureUpgrader*>(object);
			if (object != hero && upgradeSource != nullptr)
				upgradeSource->fillUpgradeInfo(ret, stack);
		}
	}

	if (stack.getArmy()->ID == Obj::TOWN)
	{
		auto town = dynamic_cast<const CGTownInstance *>(stack.getArmy());
		town->fillUpgradeInfo(ret, stack);
	}

	out = ret;
}

const StartInfo * CGameInfoCallback::getStartInfo() const
{
	return gameState().getStartInfo();
}

const StartInfo * CGameInfoCallback::getInitialStartInfo() const
{
	return gameState().getInitialStartInfo();
}

int32_t CGameInfoCallback::getSpellCost(const spells::Spell * sp, const CGHeroInstance * caster) const
{
	ERROR_RET_VAL_IF(!canGetFullInfo(caster), "Cannot get info about caster!", -1);
	//if there is a battle
	auto casterBattle = gameState().getBattle(caster->getOwner());

	if(casterBattle)
		return casterBattle->battleGetSpellCost(sp, caster);

	//if there is no battle
	return caster->getSpellCost(sp);
}

int64_t CGameInfoCallback::estimateSpellDamage(const CSpell * sp, const CGHeroInstance * hero) const
{
	ERROR_RET_VAL_IF(hero && !canGetFullInfo(hero), "Cannot get info about caster!", -1);

	if(hero) //we see hero's spellbook
		return sp->calculateDamage(hero);
	else
		return 0; //mage guild
}

void CGameInfoCallback::getThievesGuildInfo(SThievesGuildInfo & thi, const CGObjectInstance * obj)
{
	ERROR_RET_IF(!obj, "No guild object!");
	ERROR_RET_IF(obj->ID == Obj::TOWN && !canGetFullInfo(obj), "Cannot get info about town guild object!");
	//TODO: advmap object -> check if they're visited by our hero

	if(obj->ID == Obj::TOWN || obj->ID == Obj::TAVERN)
	{
		int taverns = gameState().players.at(*getPlayerID()).valOfBonuses(BonusType::THIEVES_GUILD_ACCESS);
		gameState().obtainPlayersStats(thi, taverns);
	}
	else if(obj->ID == Obj::DEN_OF_THIEVES)
	{
		gameState().obtainPlayersStats(thi, 20);
	}
}

int CGameInfoCallback::howManyTowns(PlayerColor Player) const
{
	ERROR_RET_VAL_IF(!hasAccess(Player), "Access forbidden!", -1);
	return static_cast<int>(gameState().players.at(Player).getTowns().size());
}

bool CGameInfoCallback::getTownInfo(const CGObjectInstance * town, InfoAboutTown & dest, const CGObjectInstance * selectedObject) const
{
	ERROR_RET_VAL_IF(getPlayerID().has_value() && !isVisibleFor(town, *getPlayerID()), "Town is not visible!", false);  //it's not a town or it's not visible for layer
	bool detailed = hasAccess(town->tempOwner);

	if(town->ID == Obj::TOWN)
	{
		if(!detailed && nullptr != selectedObject)
		{
			const auto * selectedHero = dynamic_cast<const CGHeroInstance *>(selectedObject);
			if(nullptr != selectedHero)
				detailed = selectedHero->hasVisions(town, BonusCustomSubtype::visionsTowns);
		}

		dest.initFromTown(dynamic_cast<const CGTownInstance *>(town), detailed);
	}
	else if(town->ID == Obj::GARRISON || town->ID == Obj::GARRISON2)
		dest.initFromArmy(dynamic_cast<const CArmedInstance *>(town), detailed);
	else
		return false;
	return true;
}

int3 CGameInfoCallback::guardingCreaturePosition (int3 pos) const
{
	ERROR_RET_VAL_IF(!isVisible(pos), "Tile is not visible!", int3(-1,-1,-1));
	return gameState().getMap().guardingCreaturePositions[pos.z][pos.x][pos.y];
}

std::vector<const CGObjectInstance*> CGameInfoCallback::getGuardingCreatures (int3 pos) const
{
	ERROR_RET_VAL_IF(!isVisible(pos), "Tile is not visible!", std::vector<const CGObjectInstance*>());
	std::vector<const CGObjectInstance*> ret;
	for(auto * cr : gameState().guardingCreatures(pos))
	{
		ret.push_back(cr);
	}
	return ret;
}

bool CGameInfoCallback::isTileGuardedUnchecked(int3 tile) const
{
	return !gameState().guardingCreatures(tile).empty();
}

bool CGameInfoCallback::getHeroInfo(const CGObjectInstance * hero, InfoAboutHero & dest, const CGObjectInstance * selectedObject) const
{
	const auto * h = dynamic_cast<const CGHeroInstance *>(hero);

	ERROR_RET_VAL_IF(!h, "That's not a hero!", false);

	InfoAboutHero::EInfoLevel infoLevel = InfoAboutHero::EInfoLevel::BASIC;

	if(hasAccess(h->tempOwner))
		infoLevel = InfoAboutHero::EInfoLevel::DETAILED;

	if (infoLevel == InfoAboutHero::EInfoLevel::BASIC)
	{
		auto ourBattle = gameState().getBattle(*getPlayerID());

		if(ourBattle && ourBattle->playerHasAccessToHeroInfo(*getPlayerID(), h)) //if it's battle we can get enemy hero full data
			infoLevel = InfoAboutHero::EInfoLevel::INBATTLE;
		else
			ERROR_RET_VAL_IF(!isVisible(h->visitablePos()), "That hero is not visible!", false);
	}

	if( (infoLevel == InfoAboutHero::EInfoLevel::BASIC) && nullptr != selectedObject)
	{
		const auto * selectedHero = dynamic_cast<const CGHeroInstance *>(selectedObject);
		if(nullptr != selectedHero)
			if(selectedHero->hasVisions(hero, BonusCustomSubtype::visionsHeroes))
				infoLevel = InfoAboutHero::EInfoLevel::DETAILED;
	}

	dest.initFromHero(h, infoLevel);

	//DISGUISED bonus implementation
	if(getPlayerRelations(*getPlayerID(), hero->tempOwner) == PlayerRelations::ENEMIES)
	{
		//todo: bonus cashing
		int disguiseLevel = h->valOfBonuses(BonusType::DISGUISED);

		auto doBasicDisguise = [](InfoAboutHero & info)
		{
			int maxAIValue = 0;
			const CCreature * mostStrong = nullptr;

			for(auto & elem : info.army)
			{
				if(static_cast<int>(elem.second.getCreature()->getAIValue()) > maxAIValue)
				{
					maxAIValue = elem.second.getCreature()->getAIValue();
					mostStrong = elem.second.getCreature();
				}
			}

			if(nullptr == mostStrong)//just in case
				logGlobal->error("CGameInfoCallback::getHeroInfo: Unable to select most strong stack");
			else
				for(auto & elem : info.army)
				{
					elem.second.setType(mostStrong);
				}
		};

		auto doAdvancedDisguise = [&doBasicDisguise](InfoAboutHero & info)
		{
			doBasicDisguise(info);

			for(auto & elem : info.army)
				elem.second.setCount(0);
		};

		auto doExpertDisguise = [this,h](InfoAboutHero & info)
		{
			for(auto & elem : info.army)
				elem.second.setCount(0);

			const auto factionIndex = getStartInfo()->playerInfos.at(h->tempOwner).castle;

			int maxAIValue = 0;
			const CCreature * mostStrong = nullptr;

			for(const auto & creature : LIBRARY->creh->objects)
			{
				if(creature->getFactionID() == factionIndex && static_cast<int>(creature->getAIValue()) > maxAIValue)
				{
					maxAIValue = creature->getAIValue();
					mostStrong = creature.get();
				}
			}

			if(nullptr != mostStrong) //possible, faction may have no creatures at all
				for(auto & elem : info.army)
					elem.second.setType(mostStrong);
		};


		switch (disguiseLevel)
		{
		case 0:
			//no bonus at all - do nothing
			break;
		case 1:
			doBasicDisguise(dest);
			break;
		case 2:
			doAdvancedDisguise(dest);
			break;
		case 3:
			doExpertDisguise(dest);
			break;
		default:
			//invalid value
			logGlobal->error("CGameInfoCallback::getHeroInfo: Invalid DISGUISED bonus value %d", disguiseLevel);
			break;
		}
	}
	return true;
}

int CGameInfoCallback::getDate(Date mode) const
{
	return gameState().getDate(mode);
}

bool CGameInfoCallback::isVisibleFor(int3 pos, PlayerColor player) const
{
	return gameState().isVisibleFor(pos, player);
}

bool CGameInfoCallback::isVisible(int3 pos) const
{
	if (!getPlayerID().has_value())
		return true; // weird, but we do have such calls
	return gameState().isVisibleFor(pos, *getPlayerID());
}

bool CGameInfoCallback::isVisibleFor(const CGObjectInstance * obj, PlayerColor player) const
{
	return gameState().isVisibleFor(obj, player);
}

bool CGameInfoCallback::isVisible(const CGObjectInstance *obj) const
{
	if (!getPlayerID().has_value())
		return true; // weird, but we do have such calls
	return gameState().isVisibleFor(obj, *getPlayerID());
}

std::vector <const CGObjectInstance *> CGameInfoCallback::getBlockingObjs( int3 pos ) const
{
	std::vector<const CGObjectInstance *> ret;
	const TerrainTile *t = getTile(pos);
	ERROR_RET_VAL_IF(!t, "Not a valid tile requested!", ret);

	for(const auto & objID : t->blockingObjects)
		ret.push_back(getObj(objID));
	return ret;
}

std::vector <const CGObjectInstance *> CGameInfoCallback::getVisitableObjs(int3 pos, bool verbose) const
{
	std::vector<const CGObjectInstance *> ret;
	const TerrainTile *t = getTile(pos, verbose);
	ERROR_VERBOSE_OR_NOT_RET_VAL_IF(!t, verbose, pos.toString() + " is not visible!", ret);

	for(const auto & objID : t->visitableObjects)
	{
		const auto & object = getObj(objID, false);
		if (!object)
			continue; // event - visitable, but not visible

		if(!getPlayerID().has_value() || object->ID != Obj::EVENT) //hide events from players
			ret.push_back(object);
	}
	return ret;
}

std::vector<const CGObjectInstance *> CGameInfoCallback::getAllVisitableObjs() const
{
	std::vector<const CGObjectInstance *> ret;
	for(auto & obj : gameState().getMap().getObjects())
		if(obj->isVisitable() && obj->ID != Obj::EVENT && isVisible(obj))
			ret.push_back(obj);

	return ret;
}

const CGObjectInstance * CGameInfoCallback::getTopObj(int3 pos) const
{
	return vstd::backOrNull(getVisitableObjs(pos));
}

std::vector <const CGObjectInstance *> CGameInfoCallback::getFlaggableObjects(int3 pos) const
{
	std::vector<const CGObjectInstance *> ret;
	const TerrainTile *t = getTile(pos);
	ERROR_RET_VAL_IF(!t, "Not a valid tile requested!", ret);
	for(const auto & objectID : t->blockingObjects)
	{
		const auto * obj = getObj(objectID);
		if(obj->tempOwner != PlayerColor::UNFLAGGABLE)
			ret.push_back(obj);
	}
	return ret;
}

std::vector<const CGHeroInstance *> CGameInfoCallback::getAvailableHeroes(const CGObjectInstance * townOrTavern) const
{
	ASSERT_IF_CALLED_WITH_PLAYER
	std::vector<const CGHeroInstance *> ret;
	//TODO: town needs to be owned, advmap tavern needs to be visited; to be reimplemented when visit tracking is done
	const CGTownInstance * town = getTown(townOrTavern->id);

	if(townOrTavern->ID == Obj::TAVERN || (town && town->hasBuilt(BuildingID::TAVERN)))
		return gameState().heroesPool->getHeroesFor(*getPlayerID());

	return ret;
}

const TerrainTile * CGameInfoCallback::getTile(int3 tile, bool verbose) const
{
	if(isVisible(tile))
		return &gameState().getMap().getTile(tile);

	if(verbose)
		logGlobal->error("\r\n%s: %s\r\n", BOOST_CURRENT_FUNCTION, tile.toString() + " is not visible!");
	return nullptr;
}

const TerrainTile * CGameInfoCallback::getTileUnchecked(int3 tile) const
{
	if (isInTheMap(tile))
		return &gameState().getMap().getTile(tile);

	return nullptr;
}

EDiggingStatus CGameInfoCallback::getTileDigStatus(int3 tile, bool verbose) const
{
	if(!isVisible(tile))
		return EDiggingStatus::UNKNOWN;

	for(const auto & object : gameState().getMap().getObjects())
	{
		if(object->ID == Obj::HOLE && object->anchorPos() == tile)
			return EDiggingStatus::TILE_OCCUPIED;
	}
	return getTile(tile)->getDiggingStatus();
}

EBuildingState CGameInfoCallback::canBuildStructure( const CGTownInstance *t, BuildingID ID ) const
{
	ERROR_RET_VAL_IF(!canGetFullInfo(t), "Town is not owned!", EBuildingState::TOWN_NOT_OWNED);

	if(!t->getTown()->buildings.count(ID))
		return EBuildingState::BUILDING_ERROR;

	const auto & building = t->getTown()->buildings.at(ID);

	if(t->hasBuilt(ID))	//already built
		return EBuildingState::ALREADY_PRESENT;

	//can we build it?
	if(vstd::contains(t->forbiddenBuildings, ID))
		return EBuildingState::FORBIDDEN; //forbidden

	auto possiblyNotBuiltTest = [&](const BuildingID & id) -> bool
	{
		return ((id == BuildingID::CAPITOL) ? true : !t->hasBuilt(id));
	};

	std::function<bool(BuildingID id)> allowedTest = [&](const BuildingID & id) -> bool
	{
		return !vstd::contains(t->forbiddenBuildings, id);
	};

	if (!t->genBuildingRequirements(ID, true).satisfiable(allowedTest, possiblyNotBuiltTest))
		return EBuildingState::FORBIDDEN;

	if(ID == BuildingID::CAPITOL)
	{
		const PlayerState *ps = getPlayerState(t->tempOwner, false);
		if(ps)
		{
			for(const CGTownInstance *town : ps->getTowns())
			{
				if(town->hasBuilt(BuildingID::CAPITOL))
				{
					return EBuildingState::HAVE_CAPITAL; //no more than one capitol
				}
			}
		}
	}
	else if(ID == BuildingID::SHIPYARD)
	{
		const TerrainTile *tile = getTile(t->bestLocation(), false);

		if(!tile || !tile->isWater())
			return EBuildingState::NO_WATER; //lack of water
	}

	auto buildTest = [&](const BuildingID & id) -> bool
	{
		return t->hasBuilt(id);
	};

	if (!t->genBuildingRequirements(ID).test(buildTest))
		return EBuildingState::PREREQUIRES;

	if(t->built >= getSettings().getInteger(EGameSettings::TOWNS_BUILDINGS_PER_TURN_CAP))
		return EBuildingState::CANT_BUILD_TODAY; //building limit

	//checking resources
	if(!building->resources.canBeAfforded(getPlayerState(t->tempOwner)->resources))
		return EBuildingState::NO_RESOURCES; //lack of res

	return EBuildingState::ALLOWED;
}

const CMap * CGameInfoCallback::getMapConstPtr() const
{
	return &gameState().getMap();
}

bool CGameInfoCallback::hasAccess(std::optional<PlayerColor> playerId) const
{
	return !getPlayerID() || getPlayerID()->isSpectator() || getPlayerRelations(*playerId, *getPlayerID()) != PlayerRelations::ENEMIES;
}

EPlayerStatus CGameInfoCallback::getPlayerStatus(PlayerColor player, bool verbose) const
{
	const PlayerState *ps = gameState().getPlayerState(player, verbose);
	ERROR_VERBOSE_OR_NOT_RET_VAL_IF(!ps, verbose, "No such player!", EPlayerStatus::WRONG);

	return ps->status;
}

std::string CGameInfoCallback::getTavernRumor(const CGObjectInstance * townOrTavern) const
{
	MetaString text;
	text.appendLocalString(EMetaText::GENERAL_TXT, 216);
	
	std::string extraText;
	if(gameState().currentRumor.type == RumorState::TYPE_NONE)
		return text.toString();

	auto rumor = gameState().currentRumor.last.at(gameState().currentRumor.type);
	switch(gameState().currentRumor.type)
	{
	case RumorState::TYPE_SPECIAL:
		text.replaceLocalString(EMetaText::GENERAL_TXT, rumor.first);
		if(rumor.first == RumorState::RUMOR_GRAIL)
			text.replaceTextID(TextIdentifier("core", "arraytxt", 158 + rumor.second).get());
		else
			text.replaceTextID(TextIdentifier("core", "plcolors", rumor.second).get());

		break;
	case RumorState::TYPE_MAP:
		text.replaceRawString(gameState().getMap().rumors[rumor.first].text.toString());
		break;

	case RumorState::TYPE_RAND:
		text.replaceTextID(TextIdentifier("core", "randtvrn", rumor.first).get());
		break;
	}

	return text.toString();
}

PlayerRelations CGameInfoCallback::getPlayerRelations( PlayerColor color1, PlayerColor color2 ) const
{
	return gameState().getPlayerRelations(color1, color2);
}

bool CGameInfoCallback::canGetFullInfo(const CGObjectInstance *obj) const
{
	return !obj || hasAccess(obj->tempOwner);
}

int CGameInfoCallback::getHeroCount( PlayerColor player, bool includeGarrisoned ) const
{
	int ret = 0;
	const PlayerState *p = gameState().getPlayerState(player);
	ERROR_RET_VAL_IF(!p, "No such player!", -1);

	if(includeGarrisoned)
		return static_cast<int>(p->getHeroes().size());
	else
		for(const auto & elem : p->getHeroes())
			if(!elem->isGarrisoned())
				ret++;
	return ret;
}

std::vector<const CGHeroInstance*> CGameInfoCallback::getHeroes(PlayerColor player) const
{
	std::vector<const CGHeroInstance*> ret;
	const PlayerState *p = gameState().getPlayerState(player);
	ERROR_RET_VAL_IF(!p, "No such player!", ret);

	for(const auto & hero : p->getHeroes())
	{
		if(!getPlayerID().has_value() || isVisibleFor(hero, *getPlayerID()) || hero->getOwner() == getPlayerID())
			ret.push_back(hero);
	}
	return ret;
}

bool CGameInfoCallback::isPlayerMakingTurn(PlayerColor player) const
{
	return gameState().actingPlayers.count(player);
}

const TeamState * CGameInfoCallback::getTeam( TeamID teamID ) const
{
	//rewritten by hand, AI calls this function a lot

	auto team = gameState().teams.find(teamID);
	if (team != gameState().teams.end())
	{
		const TeamState *ret = &team->second;
		if(!getPlayerID().has_value()) //neutral (or invalid) player
			return ret;
		else
		{
			if (vstd::contains(ret->players, *getPlayerID())) //specific player
				return ret;
			else
			{
				logGlobal->error("Illegal attempt to access team data!");
				return nullptr;
			}
		}
	}
	else
	{
		logGlobal->error("Cannot find info for team %d", teamID);
		return nullptr;
	}
}

const TeamState * CGameInfoCallback::getPlayerTeam( PlayerColor color ) const
{
	auto player = gameState().players.find(color);
	if (player != gameState().players.end())
	{
		return getTeam (player->second.team);
	}
	else
	{
		return nullptr;
	}
}

void CGameInfoCallback::getVisibleTilesInRange(std::unordered_set<int3> &tiles, int3 pos, int radious, int3::EDistanceFormula distanceFormula) const
{
	gameState().getTilesInRange(tiles, pos, radious, ETileVisibility::REVEALED, *getPlayerID(),  distanceFormula);
}

void CGameInfoCallback::calculatePaths(const std::shared_ptr<PathfinderConfig> & config) const
{
	gameState().calculatePaths(config);
}

const CArtifactSet * CGameInfoCallback::getArtSet(const ArtifactLocation & loc) const
{
	auto & gs = const_cast<CGameState&>(gameState());
	return gs.getArtSet(loc);
}

std::vector<ObjectInstanceID> CGameInfoCallback::getVisibleTeleportObjects(std::vector<ObjectInstanceID> ids, PlayerColor player) const
{
	vstd::erase_if(ids, [&](const ObjectInstanceID & id) -> bool
	{
		const auto * obj = getObj(id, false);
		return player != PlayerColor::UNFLAGGABLE && (!obj || !isVisibleFor(obj->visitablePos(), player));
	});
	return ids;
}

std::vector<ObjectInstanceID> CGameInfoCallback::getTeleportChannelEntrances(TeleportChannelID id, PlayerColor player) const
{
	return getVisibleTeleportObjects(gameState().getMap().teleportChannels.at(id)->entrances, player);
}

std::vector<ObjectInstanceID> CGameInfoCallback::getTeleportChannelExits(TeleportChannelID id, PlayerColor player) const
{
	return getVisibleTeleportObjects(gameState().getMap().teleportChannels.at(id)->exits, player);
}

ETeleportChannelType CGameInfoCallback::getTeleportChannelType(TeleportChannelID id, PlayerColor player) const
{
	std::vector<ObjectInstanceID> entrances = getTeleportChannelEntrances(id, player);
	std::vector<ObjectInstanceID> exits = getTeleportChannelExits(id, player);
	if((entrances.empty() || exits.empty()) // impassable if exits or entrances list are empty
	   || (entrances.size() == 1 && entrances == exits)) // impassable if only entrance and only exit is same object. e.g bidirectional monolith
	{
		return ETeleportChannelType::IMPASSABLE;
	}

	auto intersection = vstd::intersection(entrances, exits);
	if(intersection.size() == entrances.size() && intersection.size() == exits.size())
		return ETeleportChannelType::BIDIRECTIONAL;
	else if(intersection.empty())
		return ETeleportChannelType::UNIDIRECTIONAL;
	else
		return ETeleportChannelType::MIXED;
}

bool CGameInfoCallback::isTeleportChannelImpassable(TeleportChannelID id, PlayerColor player) const
{
	return ETeleportChannelType::IMPASSABLE == getTeleportChannelType(id, player);
}

bool CGameInfoCallback::isTeleportChannelBidirectional(TeleportChannelID id, PlayerColor player) const
{
	return ETeleportChannelType::BIDIRECTIONAL == getTeleportChannelType(id, player);
}

bool CGameInfoCallback::isTeleportChannelUnidirectional(TeleportChannelID id, PlayerColor player) const
{
	return ETeleportChannelType::UNIDIRECTIONAL == getTeleportChannelType(id, player);
}

bool CGameInfoCallback::isTeleportEntrancePassable(const CGTeleport * obj, PlayerColor player) const
{
	return obj && obj->isEntrance() && !isTeleportChannelImpassable(obj->channel, player);
}

void CGameInfoCallback::getFreeTiles(std::vector<int3> & tiles) const
{
	std::vector<int> floors;
	floors.reserve(gameState().getMap().levels());
	for(int b = 0; b < gameState().getMap().levels(); ++b)
	{
		floors.push_back(b);
	}
	const TerrainTile * tinfo = nullptr;
	for (auto zd : floors)
	{
		for (int xd = 0; xd < gameState().getMap().width; xd++)
		{
			for (int yd = 0; yd < gameState().getMap().height; yd++)
			{
				tinfo = getTile(int3 (xd,yd,zd));
				if (tinfo->isLand() && tinfo->getTerrain()->isPassable() && !tinfo->blocked()) //land and free
					tiles.emplace_back(xd, yd, zd);
			}
		}
	}
}

void CGameInfoCallback::getTilesInRange(std::unordered_set<int3> & tiles,
											  const int3 & pos,
											  int radious,
											  ETileVisibility mode,
											  std::optional<PlayerColor> player,
											  int3::EDistanceFormula distanceFormula) const
{
	if(player.has_value() && !player->isValidPlayer())
	{
		logGlobal->error("Illegal call to getTilesInRange!");
		return;
	}
	if(radious == GameConstants::FULL_MAP_RANGE)
		getAllTiles (tiles, player, -1, [](auto * tile){return true;});
	else
	{
		const TeamState * team = !player ? nullptr : gameState().getPlayerTeam(*player);
		for (int xd = std::max<int>(pos.x - radious , 0); xd <= std::min<int>(pos.x + radious, gameState().getMap().width - 1); xd++)
		{
			for (int yd = std::max<int>(pos.y - radious, 0); yd <= std::min<int>(pos.y + radious, gameState().getMap().height - 1); yd++)
			{
				int3 tilePos(xd,yd,pos.z);
				int distance = pos.dist(tilePos, distanceFormula);

				if(distance <= radious)
				{
					if(!player
					   || (mode == ETileVisibility::HIDDEN  && team->fogOfWarMap[pos.z][xd][yd] == 0)
					   || (mode == ETileVisibility::REVEALED && team->fogOfWarMap[pos.z][xd][yd] == 1)
					   )
						tiles.insert(int3(xd,yd,pos.z));
				}
			}
		}
	}
}

void CGameInfoCallback::getAllTiles(std::unordered_set<int3> & tiles, std::optional<PlayerColor> Player, int level, std::function<bool(const TerrainTile *)> filter) const
{
	if(Player.has_value() && !Player->isValidPlayer())
	{
		logGlobal->error("Illegal call to getAllTiles !");
		return;
	}

	std::vector<int> floors;
	if(level == -1)
	{
		for(int b = 0; b < gameState().getMap().levels(); ++b)
		{
			floors.push_back(b);
		}
	}
	else
		floors.push_back(level);

	for(auto zd: floors)
	{
		for(int xd = 0; xd < gameState().getMap().width; xd++)
		{
			for(int yd = 0; yd < gameState().getMap().height; yd++)
			{
				int3 coordinates(xd, yd, zd);
				if (filter(getTile(coordinates)))
					tiles.insert(coordinates);
			}
		}
	}
}

void CGameInfoCallback::getAllowedSpells(std::vector<SpellID> & out, std::optional<ui16> level) const
{
	for (auto const & spellID : gameState().getMap().allowedSpells)
	{
		const auto * spell = spellID.toEntity(LIBRARY);

		if (!isAllowed(spellID))
			continue;

		if (level.has_value() && spell->getLevel() != level)
			continue;

		out.push_back(spellID);
	}
}

bool CGameInfoCallback::checkForVisitableDir(const int3 & src, const int3 & dst) const
{
	const CMap & map = gameState().getMap();
	const TerrainTile * pom = &map.getTile(dst);
	return map.checkForVisitableDir(src, pom, dst);
}

#if SCRIPTING_ENABLED
scripting::Pool * CGameInfoCallback::getGlobalContextPool() const
{
	return nullptr; // TODO
}
#endif

VCMI_LIB_NAMESPACE_END
