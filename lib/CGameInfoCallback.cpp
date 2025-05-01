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

#include "entities/building/CBuilding.h"
#include "gameState/CGameState.h"
#include "gameState/InfoAboutArmy.h"
#include "gameState/SThievesGuildInfo.h"
#include "gameState/TavernHeroesPool.h"
#include "gameState/QuestInfo.h"
#include "mapObjects/CGHeroInstance.h"
#include "mapObjects/CGTownInstance.h"
#include "mapObjects/MiscObjects.h"
#include "networkPacks/ArtifactLocation.h"
#include "texts/CGeneralTextHandler.h"
#include "StartInfo.h" // for StartInfo
#include "battle/BattleInfo.h" // for BattleInfo
#include "IGameSettings.h"
#include "TerrainHandler.h"
#include "spells/CSpellHandler.h"
#include "mapping/CMap.h"
#include "CPlayerState.h"

VCMI_LIB_NAMESPACE_BEGIN

//TODO make clean
#define ERROR_VERBOSE_OR_NOT_RET_VAL_IF(cond, verbose, txt, retVal) do {if(cond){if(verbose)logGlobal->error("%s: %s",BOOST_CURRENT_FUNCTION, txt); return retVal;}} while(0)
#define ERROR_RET_IF(cond, txt) do {if(cond){logGlobal->error("%s: %s", BOOST_CURRENT_FUNCTION, txt); return;}} while(0)
#define ERROR_RET_VAL_IF(cond, txt, retVal) do {if(cond){logGlobal->error("%s: %s", BOOST_CURRENT_FUNCTION, txt); return retVal;}} while(0)

PlayerColor CGameInfoCallback::getOwner(ObjectInstanceID heroID) const
{
	const CGObjectInstance *obj = getObj(heroID);
	ERROR_RET_VAL_IF(!obj, "No such object!", PlayerColor::CANNOT_DETERMINE);
	return obj->tempOwner;
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

bool CGameInfoCallback::isAllowed(SpellID id) const
{
	return gameState().getMap().allowedSpells.count(id) != 0;
}

bool CGameInfoCallback::isAllowed(ArtifactID id) const
{
	return gameState().getMap().allowedArtifact.count(id) != 0;
}

bool CGameInfoCallback::isAllowed(SecondarySkill id) const
{
	return gameState().getMap().allowedAbilities.count(id) != 0;
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
	if (!objid.hasValue())
	{
		if(verbose)
			logGlobal->error("Cannot get object with id %d. No such object", objid.getNum());
		return nullptr;
	}

	const CGObjectInstance *ret = gameState().getMap().getObject(objid);
	if(!ret)
	{
		if(verbose)
			logGlobal->error("Cannot get object with id %d. Object was removed", objid.getNum());
		return nullptr;
	}

	if(!isVisible(ret, getPlayerID()) && ret->tempOwner != getPlayerID())
	{
		if(verbose)
			logGlobal->error("Cannot get object with id %d. Object is not visible.", objid.getNum());
		return nullptr;
	}

	return ret;
}

const CGHeroInstance* CGameInfoCallback::getHero(ObjectInstanceID objid) const
{
	const CGObjectInstance *obj = getObj(objid, false);
	if(obj)
		return dynamic_cast<const CGHeroInstance*>(obj);
	else
		return nullptr;
}
const CGTownInstance* CGameInfoCallback::getTown(ObjectInstanceID objid) const
{
	const CGObjectInstance *obj = getObj(objid, false);
	if(obj)
		return dynamic_cast<const CGTownInstance*>(obj);
	else
		return nullptr;
}

const IMarket * CGameInfoCallback::getMarket(ObjectInstanceID objid) const
{
	const CGObjectInstance * obj = getObj(objid, false);
	if(obj)
		return dynamic_cast<const IMarket*>(obj);
	else
		return nullptr;
}

void CGameInfoCallback::fillUpgradeInfo(const CArmedInstance *obj, SlotID stackPos, UpgradeInfo & out) const
{
	ERROR_RET_IF(!canGetFullInfo(obj), "Cannot get info about not owned object!");
	ERROR_RET_IF(!obj->hasStackAtSlot(stackPos), "There is no such stack!");
	gameState().fillUpgradeInfo(obj, stackPos, out);
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
	ERROR_RET_VAL_IF(!isVisible(town, getPlayerID()), "Town is not visible!", false);  //it's not a town or it's not visible for layer
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

const IGameSettings & CGameInfoCallback::getSettings() const
{
	return gameState().getSettings();
}

int3 CGameInfoCallback::guardingCreaturePosition (int3 pos) const //FIXME: redundant?
{
	ERROR_RET_VAL_IF(!isVisible(pos), "Tile is not visible!", int3(-1,-1,-1));
	return gameState().guardingCreaturePosition(pos);
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

bool CGameInfoCallback::isVisible(int3 pos, const std::optional<PlayerColor> & Player) const
{
	return gameState().isVisible(pos, Player);
}

bool CGameInfoCallback::isVisible(int3 pos) const
{
	return isVisible(pos, getPlayerID());
}

bool CGameInfoCallback::isVisible(const CGObjectInstance * obj, const std::optional<PlayerColor> & Player) const
{
	return gameState().isVisible(obj, Player);
}

bool CGameInfoCallback::isVisible(const CGObjectInstance *obj) const
{
	return isVisible(obj, getPlayerID());
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

const CGObjectInstance * CGameInfoCallback::getTopObj (int3 pos) const
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

int3 CGameInfoCallback::getMapSize() const
{
	return int3(gameState().getMap().width, gameState().getMap().height, gameState().getMap().twoLevel ? 2 : 1);
}

std::vector<const CGHeroInstance *> CGameInfoCallback::getAvailableHeroes(const CGObjectInstance * townOrTavern) const
{
	ASSERT_IF_CALLED_WITH_PLAYER
	std::vector<const CGHeroInstance *> ret;
	//ERROR_RET_VAL_IF(!isOwnedOrVisited(townOrTavern), "Town or tavern must be owned or visited!", ret);
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

EBuildingState CGameInfoCallback::canBuildStructure( const CGTownInstance *t, BuildingID ID )
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

const CMapHeader * CGameInfoCallback::getMapHeader() const
{
	return &gameState().getMap();
}

bool CGameInfoCallback::hasAccess(std::optional<PlayerColor> playerId) const
{
	return !getPlayerID() || getPlayerID()->isSpectator() || gameState().getPlayerRelations(*playerId, *getPlayerID()) != PlayerRelations::ENEMIES;
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

bool CGameInfoCallback::isOwnedOrVisited(const CGObjectInstance *obj) const
{
	if(canGetFullInfo(obj))
		return true;

	const TerrainTile *t = getTile(obj->visitablePos()); //get entrance tile
	const ObjectInstanceID visitorID = t->visitableObjects.back(); //visitong hero if present or the object itself at last
	const CGObjectInstance * visitor = getObj(visitorID);
	return visitor->ID == Obj::HERO && canGetFullInfo(visitor); //owned or allied hero is a visitor
}

bool CGameInfoCallback::isPlayerMakingTurn(PlayerColor player) const
{
	return gameState().actingPlayers.count(player);
}

int CPlayerSpecificInfoCallback::howManyTowns() const
{
	ERROR_RET_VAL_IF(!getPlayerID(), "Applicable only for player callbacks", -1);
	return CGameInfoCallback::howManyTowns(*getPlayerID());
}

std::vector < const CGTownInstance *> CPlayerSpecificInfoCallback::getTownsInfo(bool onlyOur) const
{
	auto ret = std::vector < const CGTownInstance *>();
	for(const auto & i : gameState().players)
	{
		for(const auto & town : i.second.getTowns())
		{
			if(i.first == getPlayerID() || (!onlyOur && isVisible(town, getPlayerID())))
			{
				ret.push_back(town);
			}
		}
	} //	for ( std::map<int, PlayerState>::iterator i=gameState().players.begin() ; i!=gameState().players.end();i++)
	return ret;
}
std::vector < const CGHeroInstance *> CPlayerSpecificInfoCallback::getHeroesInfo() const
{
	const auto * playerState = gameState().getPlayerState(*getPlayerID());
	return playerState->getHeroes();
}

int CPlayerSpecificInfoCallback::getHeroSerial(const CGHeroInstance * hero, bool includeGarrisoned) const
{
	if (hero->isGarrisoned() && !includeGarrisoned)
		return -1;

	size_t index = 0;
	const auto & heroes = gameState().players.at(*getPlayerID()).getHeroes();

	for (auto & possibleHero : heroes)
	{
		if (includeGarrisoned || !possibleHero->isGarrisoned())
			index++;

		if (possibleHero == hero)
			return static_cast<int>(index);
	}
	return -1;
}

int3 CPlayerSpecificInfoCallback::getGrailPos( double *outKnownRatio )
{
	if (!getPlayerID() || gameState().getMap().obeliskCount == 0)
	{
		*outKnownRatio = 0.0;
	}
	else
	{
		TeamID t = gameState().getPlayerTeam(*getPlayerID())->id;
		double visited = 0.0;
		if(gameState().getMap().obelisksVisited.count(t))
			visited = static_cast<double>(gameState().getMap().obelisksVisited[t]);

		*outKnownRatio = visited / gameState().getMap().obeliskCount;
	}
	return gameState().getMap().grailPos;
}

std::vector < const CGObjectInstance * > CPlayerSpecificInfoCallback::getMyObjects() const
{
	return gameState().getPlayerState(*getPlayerID())->getOwnedObjects();
}

std::vector <QuestInfo> CPlayerSpecificInfoCallback::getMyQuests() const
{
	return gameState().getPlayerState(*getPlayerID())->quests;
}

int CPlayerSpecificInfoCallback::howManyHeroes(bool includeGarrisoned) const
{
	ERROR_RET_VAL_IF(!getPlayerID(), "Applicable only for player callbacks", -1);
	return getHeroCount(*getPlayerID(), includeGarrisoned);
}

const CGHeroInstance* CPlayerSpecificInfoCallback::getHeroBySerial(int serialId, bool includeGarrisoned) const
{
	ASSERT_IF_CALLED_WITH_PLAYER
	const PlayerState *p = getPlayerState(*getPlayerID());
	ERROR_RET_VAL_IF(!p, "No player info", nullptr);

	if (!includeGarrisoned)
	{
		for(ui32 i = 0; i < p->getHeroes().size() && static_cast<int>(i) <= serialId; i++)
			if(p->getHeroes()[i]->isGarrisoned())
				serialId++;
	}
	ERROR_RET_VAL_IF(serialId < 0 || serialId >= p->getHeroes().size(), "No player info", nullptr);
	return p->getHeroes()[serialId];
}

const CGTownInstance* CPlayerSpecificInfoCallback::getTownBySerial(int serialId) const
{
	ASSERT_IF_CALLED_WITH_PLAYER
	const PlayerState *p = getPlayerState(*getPlayerID());
	ERROR_RET_VAL_IF(!p, "No player info", nullptr);
	ERROR_RET_VAL_IF(serialId < 0 || serialId >= p->getTowns().size(), "No player info", nullptr);
	return p->getTowns()[serialId];
}

int CPlayerSpecificInfoCallback::getResourceAmount(GameResID type) const
{
	ERROR_RET_VAL_IF(!getPlayerID(), "Applicable only for player callbacks", -1);
	return getResource(*getPlayerID(), type);
}

TResources CPlayerSpecificInfoCallback::getResourceAmount() const
{
	ERROR_RET_VAL_IF(!getPlayerID(), "Applicable only for player callbacks", TResources());
	return gameState().players.at(*getPlayerID()).resources;
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

bool CGameInfoCallback::isInTheMap(const int3 &pos) const
{
	return gameState().getMap().isInTheMap(pos);
}

void CGameInfoCallback::getVisibleTilesInRange(std::unordered_set<int3> &tiles, int3 pos, int radious, int3::EDistanceFormula distanceFormula) const
{
	gameState().getTilesInRange(tiles, pos, radious, ETileVisibility::REVEALED, *getPlayerID(),  distanceFormula);
}

void CGameInfoCallback::calculatePaths(const std::shared_ptr<PathfinderConfig> & config) const
{
	gameState().calculatePaths(config);
}

const CArtifactInstance * CGameInfoCallback::getArtInstance( ArtifactInstanceID aid ) const
{
	return gameState().getMap().getArtifactInstance(aid);
}

const CGObjectInstance * CGameInfoCallback::getObjInstance( ObjectInstanceID oid ) const
{
	return gameState().getMap().getObject((oid));
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
		return player != PlayerColor::UNFLAGGABLE && (!obj || !isVisible(obj->visitablePos(), player));
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

VCMI_LIB_NAMESPACE_END
