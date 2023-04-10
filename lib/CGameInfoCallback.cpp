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

#include "CGameState.h" // PlayerState
#include "CGeneralTextHandler.h"
#include "mapObjects/CObjectHandler.h" // for CGObjectInstance
#include "StartInfo.h" // for StartInfo
#include "battle/BattleInfo.h" // for BattleInfo
#include "NetPacks.h" // for InfoWindow
#include "CModHandler.h"
#include "GameSettings.h"
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
	ERROR_RET_VAL_IF(p->resources.size() <= which || which < 0, "No such resource!", -1);
	return p->resources[which];
}

const PlayerSettings * CGameInfoCallback::getPlayerSettings(PlayerColor color) const
{
	return &gs->scenarioOps->getIthPlayersSettings(color);
}

bool CGameInfoCallback::isAllowed(int32_t type, int32_t id) const
{
	switch(type)
	{
	case 0:
		return gs->map->allowedSpell[id];
	case 1:
		return gs->map->allowedArtifact[id];
	case 2:
		return gs->map->allowedAbilities[id];
	default:
		ERROR_RET_VAL_IF(1, "Wrong type!", false);
	}
}

const Player * CGameInfoCallback::getPlayer(PlayerColor color) const
{
	return getPlayerState(color, false);
}

const PlayerState * CGameInfoCallback::getPlayerState(PlayerColor color, bool verbose) const
{
	//funtion written from scratch since it's accessed A LOT by AI

	if(!color.isValidPlayer())
	{
		return nullptr;
	}
	auto player = gs->players.find(color);
	if (player != gs->players.end())
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

const CTown * CGameInfoCallback::getNativeTown(PlayerColor color) const
{
	const PlayerSettings *ps = getPlayerSettings(color);
	ERROR_RET_VAL_IF(!ps, "There is no such player!", nullptr);
	return (*VLC->townh)[ps->castle]->town;
}

const CGObjectInstance * CGameInfoCallback::getObjByQuestIdentifier(int identifier) const
{
	if(gs->map->questIdentifierToId.empty())
	{
		//assume that it is VCMI map and quest identifier equals instance identifier
		return getObj(ObjectInstanceID(identifier), true);
	}
	else
	{
		ERROR_RET_VAL_IF(!vstd::contains(gs->map->questIdentifierToId, identifier), "There is no object with such quest identifier!", nullptr);
		return getObj(gs->map->questIdentifierToId[identifier]);
	}
}

/************************************************************************/
/*                                                                      */
/************************************************************************/

const CGObjectInstance* CGameInfoCallback::getObj(ObjectInstanceID objid, bool verbose) const
{
	si32 oid = objid.num;
	if(oid < 0  ||  oid >= gs->map->objects.size())
	{
		if(verbose)
			logGlobal->error("Cannot get object with id %d", oid);
		return nullptr;
	}

	const CGObjectInstance *ret = gs->map->objects[oid];
	if(!ret)
	{
		if(verbose)
			logGlobal->error("Cannot get object with id %d. Object was removed", oid);
		return nullptr;
	}

	if(!isVisible(ret, player) && ret->tempOwner != player)
	{
		if(verbose)
			logGlobal->error("Cannot get object with id %d. Object is not visible.", oid);
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

void CGameInfoCallback::fillUpgradeInfo(const CArmedInstance *obj, SlotID stackPos, UpgradeInfo &out) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	ERROR_RET_IF(!canGetFullInfo(obj), "Cannot get info about not owned object!");
	ERROR_RET_IF(!obj->hasStackAtSlot(stackPos), "There is no such stack!");
	gs->fillUpgradeInfo(obj, stackPos, out);
	//return gs->fillUpgradeInfo(obj->getStack(stackPos));
}

const StartInfo * CGameInfoCallback::getStartInfo(bool beforeRandomization) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	if(beforeRandomization)
		return gs->initialOpts;
	else
		return gs->scenarioOps;
}

int32_t CGameInfoCallback::getSpellCost(const spells::Spell * sp, const CGHeroInstance * caster) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	ERROR_RET_VAL_IF(!canGetFullInfo(caster), "Cannot get info about caster!", -1);
	//if there is a battle
	if(gs->curB)
		return gs->curB->battleGetSpellCost(sp, caster);

	//if there is no battle
	return caster->getSpellCost(sp);
}

int64_t CGameInfoCallback::estimateSpellDamage(const CSpell * sp, const CGHeroInstance * hero) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);

	ERROR_RET_VAL_IF(hero && !canGetFullInfo(hero), "Cannot get info about caster!", -1);

	if(hero) //we see hero's spellbook
		return sp->calculateDamage(hero);
	else
		return 0; //mage guild
}

void CGameInfoCallback::getThievesGuildInfo(SThievesGuildInfo & thi, const CGObjectInstance * obj)
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	ERROR_RET_IF(!obj, "No guild object!");
	ERROR_RET_IF(obj->ID == Obj::TOWN && !canGetFullInfo(obj), "Cannot get info about town guild object!");
	//TODO: advmap object -> check if they're visited by our hero

	if(obj->ID == Obj::TOWN  ||  obj->ID == Obj::TAVERN)
	{
		int taverns = 0;
		for(auto town : gs->players[*player].towns)
		{
			if(town->hasBuilt(BuildingID::TAVERN))
				taverns++;
		}

		gs->obtainPlayersStats(thi, taverns);
	}
	else if(obj->ID == Obj::DEN_OF_THIEVES)
	{
		gs->obtainPlayersStats(thi, 20);
	}
}

int CGameInfoCallback::howManyTowns(PlayerColor Player) const
{
	ERROR_RET_VAL_IF(!hasAccess(Player), "Access forbidden!", -1);
	return static_cast<int>(gs->players[Player].towns.size());
}

bool CGameInfoCallback::getTownInfo(const CGObjectInstance * town, InfoAboutTown & dest, const CGObjectInstance * selectedObject) const
{
	ERROR_RET_VAL_IF(!isVisible(town, player), "Town is not visible!", false);  //it's not a town or it's not visible for layer
	bool detailed = hasAccess(town->tempOwner);

	if(town->ID == Obj::TOWN)
	{
		if(!detailed && nullptr != selectedObject)
		{
			const auto * selectedHero = dynamic_cast<const CGHeroInstance *>(selectedObject);
			if(nullptr != selectedHero)
				detailed = selectedHero->hasVisions(town, 1);
		}

		dest.initFromTown(dynamic_cast<const CGTownInstance *>(town), detailed);
	}
	else if(town->ID == Obj::GARRISON || town->ID == Obj::GARRISON2)
		dest.initFromArmy(dynamic_cast<const CArmedInstance *>(town), detailed);
	else
		return false;
	return true;
}

int3 CGameInfoCallback::guardingCreaturePosition (int3 pos) const //FIXME: redundant?
{
	ERROR_RET_VAL_IF(!isVisible(pos), "Tile is not visible!", int3(-1,-1,-1));
	return gs->guardingCreaturePosition(pos);
}

std::vector<const CGObjectInstance*> CGameInfoCallback::getGuardingCreatures (int3 pos) const
{
	ERROR_RET_VAL_IF(!isVisible(pos), "Tile is not visible!", std::vector<const CGObjectInstance*>());
	std::vector<const CGObjectInstance*> ret;
	for(auto * cr : gs->guardingCreatures(pos))
	{
		ret.push_back(cr);
	}
	return ret;
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
		if(gs->curB && gs->curB->playerHasAccessToHeroInfo(*player, h)) //if it's battle we can get enemy hero full data
			infoLevel = InfoAboutHero::EInfoLevel::INBATTLE;
		else
			ERROR_RET_VAL_IF(!isVisible(h->visitablePos()), "That hero is not visible!", false);
	}

	if( (infoLevel == InfoAboutHero::EInfoLevel::BASIC) && nullptr != selectedObject)
	{
		const auto * selectedHero = dynamic_cast<const CGHeroInstance *>(selectedObject);
		if(nullptr != selectedHero)
			if(selectedHero->hasVisions(hero, 1))
				infoLevel = InfoAboutHero::EInfoLevel::DETAILED;
	}

	dest.initFromHero(h, infoLevel);

	//DISGUISED bonus implementation

	if(getPlayerRelations(getLocalPlayer(), hero->tempOwner) == PlayerRelations::ENEMIES)
	{
		//todo: bonus cashing
		int disguiseLevel = h->valOfBonuses(Selector::typeSubtype(Bonus::DISGUISED, 0));

		auto doBasicDisguise = [](InfoAboutHero & info)
		{
			int maxAIValue = 0;
			const CCreature * mostStrong = nullptr;

			for(auto & elem : info.army)
			{
				if(static_cast<int>(elem.second.type->getAIValue()) > maxAIValue)
				{
					maxAIValue = elem.second.type->getAIValue();
					mostStrong = elem.second.type;
				}
			}

			if(nullptr == mostStrong)//just in case
				logGlobal->error("CGameInfoCallback::getHeroInfo: Unable to select most strong stack");
			else
				for(auto & elem : info.army)
				{
					elem.second.type = mostStrong;
				}
		};

		auto doAdvancedDisguise = [&doBasicDisguise](InfoAboutHero & info)
		{
			doBasicDisguise(info);

			for(auto & elem : info.army)
				elem.second.count = 0;
		};

		auto doExpertDisguise = [this,h](InfoAboutHero & info)
		{
			for(auto & elem : info.army)
				elem.second.count = 0;

			const auto factionIndex = getStartInfo(false)->playerInfos.at(h->tempOwner).castle;

			int maxAIValue = 0;
			const CCreature * mostStrong = nullptr;

			for(auto creature : VLC->creh->objects)
			{
				if(static_cast<si16>(creature->getFactionIndex()) == factionIndex && static_cast<int>(creature->getAIValue()) > maxAIValue)
				{
					maxAIValue = creature->getAIValue();
					mostStrong = creature;
				}
			}

			if(nullptr != mostStrong) //possible, faction may have no creatures at all
				for(auto & elem : info.army)
					elem.second.type = mostStrong;
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

int CGameInfoCallback::getDate(Date::EDateType mode) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->getDate(mode);
}

bool CGameInfoCallback::isVisible(int3 pos, const boost::optional<PlayerColor> & Player) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->isVisible(pos, Player);
}

bool CGameInfoCallback::isVisible(int3 pos) const
{
	return isVisible(pos, player);
}

bool CGameInfoCallback::isVisible( const CGObjectInstance *obj,const boost::optional<PlayerColor> & Player) const
{
	return gs->isVisible(obj, Player);
}

bool CGameInfoCallback::isVisible(const CGObjectInstance *obj) const
{
	return isVisible(obj, player);
}
// const CCreatureSet* CInfoCallback::getGarrison(const CGObjectInstance *obj) const
// {
// 	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
// 	if()
// 	const CArmedInstance *armi = dynamic_cast<const CArmedInstance*>(obj);
// 	if(!armi)
// 		return nullptr;
// 	else
// 		return armi;
// }

std::vector < const CGObjectInstance * > CGameInfoCallback::getBlockingObjs( int3 pos ) const
{
	std::vector<const CGObjectInstance *> ret;
	const TerrainTile *t = getTile(pos);
	ERROR_RET_VAL_IF(!t, "Not a valid tile requested!", ret);

	for(const CGObjectInstance * obj : t->blockingObjects)
		ret.push_back(obj);
	return ret;
}

std::vector <const CGObjectInstance * > CGameInfoCallback::getVisitableObjs(int3 pos, bool verbose) const
{
	std::vector<const CGObjectInstance *> ret;
	const TerrainTile *t = getTile(pos, verbose);
	ERROR_VERBOSE_OR_NOT_RET_VAL_IF(!t, verbose, pos.toString() + " is not visible!", ret);

	for(const CGObjectInstance * obj : t->visitableObjects)
	{
		if(player || obj->ID != Obj::EVENT) //hide events from players
			ret.push_back(obj);
	}

	return ret;
}
const CGObjectInstance * CGameInfoCallback::getTopObj (int3 pos) const
{
	return vstd::backOrNull(getVisitableObjs(pos));
}

std::vector < const CGObjectInstance * > CGameInfoCallback::getFlaggableObjects(int3 pos) const
{
	std::vector<const CGObjectInstance *> ret;
	const TerrainTile *t = getTile(pos);
	ERROR_RET_VAL_IF(!t, "Not a valid tile requested!", ret);
	for(const CGObjectInstance *obj : t->blockingObjects)
		if(obj->tempOwner != PlayerColor::UNFLAGGABLE)
			ret.push_back(obj);
	return ret;
}

int3 CGameInfoCallback::getMapSize() const
{
	return int3(gs->map->width, gs->map->height, gs->map->twoLevel ? 2 : 1);
}

std::vector<const CGHeroInstance *> CGameInfoCallback::getAvailableHeroes(const CGObjectInstance * townOrTavern) const
{
	ASSERT_IF_CALLED_WITH_PLAYER
	std::vector<const CGHeroInstance *> ret;
	//ERROR_RET_VAL_IF(!isOwnedOrVisited(townOrTavern), "Town or tavern must be owned or visited!", ret);
	//TODO: town needs to be owned, advmap tavern needs to be visited; to be reimplemented when visit tracking is done
	const CGTownInstance * town = getTown(townOrTavern->id);
	if(townOrTavern->ID == Obj::TAVERN || (town && town->hasBuilt(BuildingID::TAVERN)))
	{
		range::copy(gs->players[*player].availableHeroes, std::back_inserter(ret));
		vstd::erase_if(ret, [](const CGHeroInstance * h) {
			return h == nullptr;
		});
	}
	return ret;
}

const TerrainTile * CGameInfoCallback::getTile( int3 tile, bool verbose) const
{
	if(isVisible(tile))
		return &gs->map->getTile(tile);

	if(verbose)
		logGlobal->error("\r\n%s: %s\r\n", BOOST_CURRENT_FUNCTION, tile.toString() + " is not visible!");
	return nullptr;
}

EDiggingStatus CGameInfoCallback::getTileDigStatus(int3 tile, bool verbose) const
{
	if(!isVisible(tile))
		return EDiggingStatus::UNKNOWN;

	for(const auto & object : gs->map->objects)
	{
		if(object && object->ID == Obj::HOLE && object->pos == tile)
			return EDiggingStatus::TILE_OCCUPIED;
	}
	return getTile(tile)->getDiggingStatus();
}

//TODO: typedef?
std::shared_ptr<const boost::multi_array<TerrainTile*, 3>> CGameInfoCallback::getAllVisibleTiles() const
{
	assert(player.is_initialized());
	const auto * team = getPlayerTeam(player.get());

	size_t width = gs->map->width;
	size_t height = gs->map->height;
	size_t levels = gs->map->levels();

	auto * ptr = new boost::multi_array<TerrainTile *, 3>(boost::extents[levels][width][height]);

	int3 tile;
	for(tile.z = 0; tile.z < levels; tile.z++)
		for(tile.x = 0; tile.x < width; tile.x++)
			for(tile.y = 0; tile.y < height; tile.y++)
			{
				if ((*team->fogOfWarMap)[tile.z][tile.x][tile.y])
					(*ptr)[tile.z][tile.x][tile.y] = &gs->map->getTile(tile);
				else
					(*ptr)[tile.z][tile.x][tile.y] = nullptr;
			}

	return std::shared_ptr<const boost::multi_array<TerrainTile*, 3>>(ptr);
}

EBuildingState::EBuildingState CGameInfoCallback::canBuildStructure( const CGTownInstance *t, BuildingID ID )
{
	ERROR_RET_VAL_IF(!canGetFullInfo(t), "Town is not owned!", EBuildingState::TOWN_NOT_OWNED);

	if(!t->town->buildings.count(ID))
		return EBuildingState::BUILDING_ERROR;

	const CBuilding * building = t->town->buildings.at(ID);


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
			for(const CGTownInstance *town : ps->towns)
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

		if(!tile || tile->terType->isLand())
			return EBuildingState::NO_WATER; //lack of water
	}

	auto buildTest = [&](const BuildingID & id) -> bool
	{
		return t->hasBuilt(id);
	};

	if (!t->genBuildingRequirements(ID).test(buildTest))
		return EBuildingState::PREREQUIRES;

	if(t->builded >= VLC->settings()->getInteger(EGameSettings::TOWNS_BUILDINGS_PER_TURN_CAP))
		return EBuildingState::CANT_BUILD_TODAY; //building limit

	//checking resources
	if(!building->resources.canBeAfforded(getPlayerState(t->tempOwner)->resources))
		return EBuildingState::NO_RESOURCES; //lack of res

	return EBuildingState::ALLOWED;
}

const CMapHeader * CGameInfoCallback::getMapHeader() const
{
	return gs->map;
}

bool CGameInfoCallback::hasAccess(boost::optional<PlayerColor> playerId) const
{
	return !player || player.get().isSpectator() || gs->getPlayerRelations( *playerId, *player ) != PlayerRelations::ENEMIES;
}

EPlayerStatus::EStatus CGameInfoCallback::getPlayerStatus(PlayerColor player, bool verbose) const
{
	const PlayerState *ps = gs->getPlayerState(player, verbose);
	ERROR_VERBOSE_OR_NOT_RET_VAL_IF(!ps, verbose, "No such player!", EPlayerStatus::WRONG);

	return ps->status;
}

std::string CGameInfoCallback::getTavernRumor(const CGObjectInstance * townOrTavern) const
{
	std::string text;
	std::string extraText;
	if(gs->rumor.type == RumorState::TYPE_NONE)
		return text;

	auto rumor = gs->rumor.last[gs->rumor.type];
	switch(gs->rumor.type)
	{
	case RumorState::TYPE_SPECIAL:
		if(rumor.first == RumorState::RUMOR_GRAIL)
			extraText = VLC->generaltexth->arraytxt[158 + rumor.second];
		else
			extraText = VLC->generaltexth->capColors[rumor.second];

		text = boost::str(boost::format(VLC->generaltexth->allTexts[rumor.first]) % extraText);

		break;
	case RumorState::TYPE_MAP:
		text = gs->map->rumors[rumor.first].text;
		break;

	case RumorState::TYPE_RAND:
		text = VLC->generaltexth->tavernRumors[rumor.first];
		break;
	}

	return text;
}

PlayerRelations::PlayerRelations CGameInfoCallback::getPlayerRelations( PlayerColor color1, PlayerColor color2 ) const
{
	return gs->getPlayerRelations(color1, color2);
}

bool CGameInfoCallback::canGetFullInfo(const CGObjectInstance *obj) const
{
	return !obj || hasAccess(obj->tempOwner);
}

int CGameInfoCallback::getHeroCount( PlayerColor player, bool includeGarrisoned ) const
{
	int ret = 0;
	const PlayerState *p = gs->getPlayerState(player);
	ERROR_RET_VAL_IF(!p, "No such player!", -1);

	if(includeGarrisoned)
		return static_cast<int>(p->heroes.size());
	else
		for(const auto & elem : p->heroes)
			if(!elem->inTownGarrison)
				ret++;
	return ret;
}

bool CGameInfoCallback::isOwnedOrVisited(const CGObjectInstance *obj) const
{
	if(canGetFullInfo(obj))
		return true;

	const TerrainTile *t = getTile(obj->visitablePos()); //get entrance tile
	const CGObjectInstance *visitor = t->visitableObjects.back(); //visitong hero if present or the obejct itself at last
	return visitor->ID == Obj::HERO && canGetFullInfo(visitor); //owned or allied hero is a visitor
}

PlayerColor CGameInfoCallback::getCurrentPlayer() const
{
	return gs->currentPlayer;
}

CGameInfoCallback::CGameInfoCallback(CGameState * GS, boost::optional<PlayerColor> Player):
	gs(GS)
{
	player = std::move(Player);
}

std::shared_ptr<const boost::multi_array<ui8, 3>> CPlayerSpecificInfoCallback::getVisibilityMap() const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->getPlayerTeam(*player)->fogOfWarMap;
}

int CPlayerSpecificInfoCallback::howManyTowns() const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	ERROR_RET_VAL_IF(!player, "Applicable only for player callbacks", -1);
	return CGameInfoCallback::howManyTowns(*player);
}

std::vector < const CGTownInstance *> CPlayerSpecificInfoCallback::getTownsInfo(bool onlyOur) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	std::vector < const CGTownInstance *> ret = std::vector < const CGTownInstance *>();
	for(const auto & i : gs->players)
	{
		for(const auto & town : i.second.towns)
		{
			if(i.first == player || (!onlyOur && isVisible(town, player)))
			{
				ret.push_back(town);
			}
		}
	} //	for ( std::map<int, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
	return ret;
}
std::vector < const CGHeroInstance *> CPlayerSpecificInfoCallback::getHeroesInfo(bool onlyOur) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	std::vector < const CGHeroInstance *> ret;
	for(auto hero : gs->map->heroesOnMap)
	{
		// !player || // - why would we even get access to hero not owned by any player?
		if((hero->tempOwner == *player) ||
			(isVisible(hero->visitablePos(), player) && !onlyOur)	)
		{
			ret.push_back(hero);
		}
	}
	return ret;
}

boost::optional<PlayerColor> CPlayerSpecificInfoCallback::getMyColor() const
{
	return player;
}

int CPlayerSpecificInfoCallback::getHeroSerial(const CGHeroInstance * hero, bool includeGarrisoned) const
{
	if (hero->inTownGarrison && !includeGarrisoned)
		return -1;

	size_t index = 0;
	auto & heroes = gs->players[*player].heroes;

	for (auto & heroe : heroes)
	{
		if (includeGarrisoned || !(heroe)->inTownGarrison)
			index++;

		if (heroe == hero)
			return static_cast<int>(index);
	}
	return -1;
}

int3 CPlayerSpecificInfoCallback::getGrailPos( double *outKnownRatio )
{
	if (!player || CGObelisk::obeliskCount == 0)
	{
		*outKnownRatio = 0.0;
	}
	else
	{
		TeamID t = gs->getPlayerTeam(*player)->id;
		double visited = 0.0;
		if(CGObelisk::visited.count(t))
			visited = static_cast<double>(CGObelisk::visited[t]);

		*outKnownRatio = visited / CGObelisk::obeliskCount;
	}
	return gs->map->grailPos;
}

std::vector < const CGObjectInstance * > CPlayerSpecificInfoCallback::getMyObjects() const
{
	std::vector < const CGObjectInstance * > ret;
	for(const CGObjectInstance * obj : gs->map->objects)
	{
		if(obj && obj->tempOwner == player)
			ret.push_back(obj);
	}
	return ret;
}

std::vector < const CGDwelling * > CPlayerSpecificInfoCallback::getMyDwellings() const
{
	ASSERT_IF_CALLED_WITH_PLAYER
	std::vector < const CGDwelling * > ret;
	for(CGDwelling * dw : gs->getPlayerState(*player)->dwellings)
	{
		ret.push_back(dw);
	}
	return ret;
}

std::vector <QuestInfo> CPlayerSpecificInfoCallback::getMyQuests() const
{
	std::vector <QuestInfo> ret;
	for(const auto & quest : gs->getPlayerState(*player)->quests)
	{
		ret.push_back (quest);
	}
	return ret;
}

int CPlayerSpecificInfoCallback::howManyHeroes(bool includeGarrisoned) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	ERROR_RET_VAL_IF(!player, "Applicable only for player callbacks", -1);
	return getHeroCount(*player,includeGarrisoned);
}

const CGHeroInstance* CPlayerSpecificInfoCallback::getHeroBySerial(int serialId, bool includeGarrisoned) const
{
	ASSERT_IF_CALLED_WITH_PLAYER
	const PlayerState *p = getPlayerState(*player);
	ERROR_RET_VAL_IF(!p, "No player info", nullptr);

	if (!includeGarrisoned)
	{
		for(ui32 i = 0; i < p->heroes.size() && static_cast<int>(i) <= serialId; i++)
			if(p->heroes[i]->inTownGarrison)
				serialId++;
	}
	ERROR_RET_VAL_IF(serialId < 0 || serialId >= p->heroes.size(), "No player info", nullptr);
	return p->heroes[serialId];
}

const CGTownInstance* CPlayerSpecificInfoCallback::getTownBySerial(int serialId) const
{
	ASSERT_IF_CALLED_WITH_PLAYER
	const PlayerState *p = getPlayerState(*player);
	ERROR_RET_VAL_IF(!p, "No player info", nullptr);
	ERROR_RET_VAL_IF(serialId < 0 || serialId >= p->towns.size(), "No player info", nullptr);
	return p->towns[serialId];
}

int CPlayerSpecificInfoCallback::getResourceAmount(GameResID type) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	ERROR_RET_VAL_IF(!player, "Applicable only for player callbacks", -1);
	return getResource(*player, type);
}

TResources CPlayerSpecificInfoCallback::getResourceAmount() const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	ERROR_RET_VAL_IF(!player, "Applicable only for player callbacks", TResources());
	return gs->players[*player].resources;
}

const TeamState * CGameInfoCallback::getTeam( TeamID teamID ) const
{
	//rewritten by hand, AI calls this function a lot

	auto team = gs->teams.find(teamID);
	if (team != gs->teams.end())
	{
		const TeamState *ret = &team->second;
		if (!player.is_initialized()) //neutral (or invalid) player
			return ret;
		else
		{
			if (vstd::contains(ret->players, *player)) //specific player
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
	auto player = gs->players.find(color);
	if (player != gs->players.end())
	{
		return getTeam (player->second.team);
	}
	else
	{
		return nullptr;
	}
}

const CGHeroInstance * CGameInfoCallback::getHeroWithSubid( int subid ) const
{
	if(subid<0)
		return nullptr;
	if(subid>= gs->map->allHeroes.size())
		return nullptr;

	return gs->map->allHeroes.at(subid).get();
}

PlayerColor CGameInfoCallback::getLocalPlayer() const
{
	return getCurrentPlayer();
}

bool CGameInfoCallback::isInTheMap(const int3 &pos) const
{
	return gs->map->isInTheMap(pos);
}

void CGameInfoCallback::getVisibleTilesInRange(std::unordered_set<int3, ShashInt3> &tiles, int3 pos, int radious, int3::EDistanceFormula distanceFormula) const
{
	gs->getTilesInRange(tiles, pos, radious, getLocalPlayer(), -1, distanceFormula);
}

void CGameInfoCallback::calculatePaths(const std::shared_ptr<PathfinderConfig> & config)
{
	gs->calculatePaths(config);
}

void CGameInfoCallback::calculatePaths( const CGHeroInstance *hero, CPathsInfo &out)
{
	gs->calculatePaths(hero, out);
}


const CArtifactInstance * CGameInfoCallback::getArtInstance( ArtifactInstanceID aid ) const
{
	return gs->map->artInstances[aid.num];
}

const CGObjectInstance * CGameInfoCallback::getObjInstance( ObjectInstanceID oid ) const
{
	return gs->map->objects[oid.num];
}

std::vector<ObjectInstanceID> CGameInfoCallback::getVisibleTeleportObjects(std::vector<ObjectInstanceID> ids, PlayerColor player) const
{
	vstd::erase_if(ids, [&](const ObjectInstanceID & id) -> bool
	{
		const auto * obj = getObj(id, false);
		return player != PlayerColor::UNFLAGGABLE && (!obj || !isVisible(obj->pos, player));
	});
	return ids;
}

std::vector<ObjectInstanceID> CGameInfoCallback::getTeleportChannelEntraces(TeleportChannelID id, PlayerColor player) const
{
	return getVisibleTeleportObjects(gs->map->teleportChannels[id]->entrances, player);
}

std::vector<ObjectInstanceID> CGameInfoCallback::getTeleportChannelExits(TeleportChannelID id, PlayerColor player) const
{
	return getVisibleTeleportObjects(gs->map->teleportChannels[id]->exits, player);
}

ETeleportChannelType CGameInfoCallback::getTeleportChannelType(TeleportChannelID id, PlayerColor player) const
{
	std::vector<ObjectInstanceID> entrances = getTeleportChannelEntraces(id, player);
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
