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
#include "BattleState.h" // for BattleInfo
#include "NetPacks.h" // for InfoWindow
#include "CModHandler.h"
#include "spells/CSpellHandler.h"
#include "mapping/CMap.h"
#include "CPlayerState.h"

//TODO make clean
#define ERROR_VERBOSE_OR_NOT_RET_VAL_IF(cond, verbose, txt, retVal) do {if(cond){if(verbose)logGlobal->errorStream() << BOOST_CURRENT_FUNCTION << ": " << txt; return retVal;}} while(0)
#define ERROR_RET_IF(cond, txt) do {if(cond){logGlobal->errorStream() << BOOST_CURRENT_FUNCTION << ": " << txt; return;}} while(0)
#define ERROR_RET_VAL_IF(cond, txt, retVal) do {if(cond){logGlobal->errorStream() << BOOST_CURRENT_FUNCTION << ": " << txt; return retVal;}} while(0)

PlayerColor CGameInfoCallback::getOwner(ObjectInstanceID heroID) const
{
	const CGObjectInstance *obj = getObj(heroID);
	ERROR_RET_VAL_IF(!obj, "No such object!", PlayerColor::CANNOT_DETERMINE);
	return obj->tempOwner;
}

int CGameInfoCallback::getResource(PlayerColor Player, Res::ERes which) const
{
	const PlayerState *p = getPlayer(Player);
	ERROR_RET_VAL_IF(!p, "No player info!", -1);
	ERROR_RET_VAL_IF(p->resources.size() <= which || which < 0, "No such resource!", -1);
	return p->resources[which];
}

const PlayerSettings * CGameInfoCallback::getPlayerSettings(PlayerColor color) const
{
	return &gs->scenarioOps->getIthPlayersSettings(color);
}

bool CGameInfoCallback::isAllowed( int type, int id )
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

const PlayerState * CGameInfoCallback::getPlayer(PlayerColor color, bool verbose) const
{
	//funtion written from scratch since it's accessed A LOT by AI

	auto player = gs->players.find(color);
	if (player != gs->players.end())
	{
		if (hasAccess(color))
			return &player->second;
		else
		{
			if (verbose)
				logGlobal->errorStream() << boost::format("Cannot access player %d info!") % color;
			return nullptr;
		}
	}
	else
	{
		if (verbose)
			logGlobal->errorStream() << boost::format("Cannot find player %d info!") % color;
		return nullptr;
	}
}

const CTown * CGameInfoCallback::getNativeTown(PlayerColor color) const
{
	const PlayerSettings *ps = getPlayerSettings(color);
	ERROR_RET_VAL_IF(!ps, "There is no such player!", nullptr);
	return VLC->townh->factions[ps->castle]->town;
}

const CGObjectInstance * CGameInfoCallback::getObjByQuestIdentifier(int identifier) const
{
	ERROR_RET_VAL_IF(!vstd::contains(gs->map->questIdentifierToId, identifier), "There is no object with such quest identifier!", nullptr);
	return getObj(gs->map->questIdentifierToId[identifier]);
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
            logGlobal->errorStream() << "Cannot get object with id " << oid;
		return nullptr;
	}

	const CGObjectInstance *ret = gs->map->objects[oid];
	if(!ret)
	{
		if(verbose)
            logGlobal->errorStream() << "Cannot get object with id " << oid << ". Object was removed.";
		return nullptr;
	}

	if(!isVisible(ret, player) && ret->tempOwner != player)
	{
		if(verbose)
            logGlobal->errorStream() << "Cannot get object with id " << oid << ". Object is not visible.";
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

void CGameInfoCallback::getUpgradeInfo(const CArmedInstance *obj, SlotID stackPos, UpgradeInfo &out) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	ERROR_RET_IF(!canGetFullInfo(obj), "Cannot get info about not owned object!");
	ERROR_RET_IF(!obj->hasStackAtSlot(stackPos), "There is no such stack!");
	out = gs->getUpgradeInfo(obj->getStack(stackPos));
	//return gs->getUpgradeInfo(obj->getStack(stackPos));
}

const StartInfo * CGameInfoCallback::getStartInfo(bool beforeRandomization /*= false*/) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	if(beforeRandomization)
		return gs->initialOpts;
	else
		return gs->scenarioOps;
}

int CGameInfoCallback::getSpellCost(const CSpell * sp, const CGHeroInstance * caster) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	ERROR_RET_VAL_IF(!canGetFullInfo(caster), "Cannot get info about caster!", -1);
	//if there is a battle
	if(gs->curB)
		return gs->curB->battleGetSpellCost(sp, caster);

	//if there is no battle
	return caster->getSpellCost(sp);
}

int CGameInfoCallback::estimateSpellDamage(const CSpell * sp, const CGHeroInstance * hero) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);

	ERROR_RET_VAL_IF(hero && !canGetFullInfo(hero), "Cannot get info about caster!", -1);

	if (hero) //we see hero's spellbook
		return sp->calculateDamage(hero, nullptr, hero->getEffectLevel(sp), hero->getEffectPower(sp));
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
	return gs->players[Player].towns.size();
}

bool CGameInfoCallback::getTownInfo(const CGObjectInstance * town, InfoAboutTown & dest, const CGObjectInstance * selectedObject/* = nullptr*/) const
{
	ERROR_RET_VAL_IF(!isVisible(town, player), "Town is not visible!", false);  //it's not a town or it's not visible for layer
	bool detailed = hasAccess(town->tempOwner);

	if(town->ID == Obj::TOWN)
	{
		if(!detailed && nullptr != selectedObject)
		{
			const CGHeroInstance * selectedHero = dynamic_cast<const CGHeroInstance *>(selectedObject);		
			if(nullptr != selectedHero)
				detailed = selectedHero->hasVisions(town, 1);			
		}
		
		dest.initFromTown(static_cast<const CGTownInstance *>(town), detailed);
	}		
	else if(town->ID == Obj::GARRISON || town->ID == Obj::GARRISON2)
		dest.initFromArmy(static_cast<const CArmedInstance *>(town), detailed);
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
	for(auto cr : gs->guardingCreatures(pos))
	{
		ret.push_back(cr);
	}
	return ret;
}

bool CGameInfoCallback::getHeroInfo(const CGObjectInstance * hero, InfoAboutHero & dest, const CGObjectInstance * selectedObject/* = nullptr*/) const
{
	const CGHeroInstance *h = dynamic_cast<const CGHeroInstance *>(hero);

	ERROR_RET_VAL_IF(!h, "That's not a hero!", false);
	ERROR_RET_VAL_IF(!isVisible(h->getPosition(false)), "That hero is not visible!", false);

	bool accessFlag = hasAccess(h->tempOwner);
	
	if(!accessFlag && nullptr != selectedObject)
	{
		const CGHeroInstance * selectedHero = dynamic_cast<const CGHeroInstance *>(selectedObject);		
		if(nullptr != selectedHero)
			accessFlag = selectedHero->hasVisions(hero, 1);			
	}
	
	dest.initFromHero(h, accessFlag);
	
	//DISGUISED bonus implementation
	
	if(getPlayerRelations(getLocalPlayer(), hero->tempOwner) == PlayerRelations::ENEMIES)
	{
		//todo: bonus cashing	
		int disguiseLevel = h->valOfBonuses(Selector::typeSubtype(Bonus::DISGUISED, 0));
		
		auto doBasicDisguise = [disguiseLevel](InfoAboutHero & info)		
		{
			int maxAIValue = 0;
			const CCreature * mostStrong = nullptr;
			
			for(auto & elem : info.army)
			{
				if(elem.second.type->AIValue > maxAIValue)
				{
					maxAIValue = elem.second.type->AIValue;
					mostStrong = elem.second.type;
				}
			}
			
			if(nullptr == mostStrong)//just in case
				logGlobal->errorStream() << "CGameInfoCallback::getHeroInfo: Unable to select most strong stack" << disguiseLevel;
			else
				for(auto & elem : info.army)
				{
					elem.second.type = mostStrong;
				}
		};
		
		auto doAdvancedDisguise = [accessFlag, &doBasicDisguise](InfoAboutHero & info)		
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
			
			for(auto creature : VLC->creh->creatures)
			{
				if(creature->faction == factionIndex && creature->AIValue > maxAIValue)
				{
					maxAIValue = creature->AIValue;
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
			logGlobal->errorStream() << "CGameInfoCallback::getHeroInfo: Invalid DISGUISED bonus value " << disguiseLevel;
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

bool CGameInfoCallback::isVisible(int3 pos, boost::optional<PlayerColor> Player) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->map->isInTheMap(pos) && (!Player || gs->isVisible(pos, *Player));
}

bool CGameInfoCallback::isVisible(int3 pos) const
{
	return isVisible(pos, player);
}

bool CGameInfoCallback::isVisible( const CGObjectInstance *obj, boost::optional<PlayerColor> Player ) const
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

std::vector <const CGObjectInstance * > CGameInfoCallback::getVisitableObjs(int3 pos, bool verbose /*= true*/) const
{
	std::vector<const CGObjectInstance *> ret;
	const TerrainTile *t = getTile(pos, verbose);
	ERROR_VERBOSE_OR_NOT_RET_VAL_IF(!t, verbose, pos << " is not visible!", ret);

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
// 	const std::vector < std::pair<const CGObjectInstance*,SDL_Rect> > & objs = CGI->mh->ttiles[pos.x][pos.y][pos.z].objects;
// 	for(size_t b=0; b<objs.size(); ++b)
// 	{
// 		if(objs[b].first->tempOwner!=254 && !((objs[b].first->defInfo->blockMap[pos.y - objs[b].first->pos.y + 5] >> (objs[b].first->pos.x - pos.x)) & 1))
// 			ret.push_back(CGI->mh->ttiles[pos.x][pos.y][pos.z].objects[b].first);
// 	}
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
	range::copy(gs->players[*player].availableHeroes, std::back_inserter(ret));
	vstd::erase_if(ret, [](const CGHeroInstance *h) { return h == nullptr; });
	return ret;
}

const TerrainTile * CGameInfoCallback::getTile( int3 tile, bool verbose) const
{
	ERROR_VERBOSE_OR_NOT_RET_VAL_IF(!isVisible(tile), verbose, tile << " is not visible!", nullptr);

	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return &gs->map->getTile(tile);
}

//TODO: typedef?
std::shared_ptr<boost::multi_array<TerrainTile*, 3>> CGameInfoCallback::getAllVisibleTiles() const
{
	assert(player.is_initialized());
	auto team = getPlayerTeam(player.get());

	size_t width = gs->map->width;
	size_t height = gs->map->height;
	size_t levels = (gs->map->twoLevel ? 2 : 1);


	boost::multi_array<TerrainTile*, 3> tileArray(boost::extents[width][height][levels]);
	
	for (size_t x = 0; x < width; x++)
		for (size_t y = 0; y < height; y++)
			for (size_t z = 0; z < levels; z++)
			{
				if (team->fogOfWarMap[x][y][z])
					tileArray[x][y][z] = &gs->map->getTile(int3(x, y, z));
				else
					tileArray[x][y][z] = nullptr;
			}
	return std::make_shared<boost::multi_array<TerrainTile*, 3>>(tileArray);
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

	if(ID == BuildingID::CAPITOL)
	{
		const PlayerState *ps = getPlayer(t->tempOwner, false);
		if(ps)
		{
			for(const CGTownInstance *t : ps->towns)
			{
				if(t->hasBuilt(BuildingID::CAPITOL))
				{
					return EBuildingState::HAVE_CAPITAL; //no more than one capitol
				}
			}
		}
	}
	else if(ID == BuildingID::SHIPYARD)
	{
		const TerrainTile *tile = getTile(t->bestLocation(), false);

		if(!tile || tile->terType != ETerrainType::WATER)
			return EBuildingState::NO_WATER; //lack of water
	}

	auto buildTest = [&](BuildingID id) -> bool
	{
		return t->hasBuilt(id);
	};

	if (!t->genBuildingRequirements(ID).test(buildTest))
		return EBuildingState::PREREQUIRES;

	if(t->builded >= VLC->modh->settings.MAX_BUILDING_PER_TURN)
		return EBuildingState::CANT_BUILD_TODAY; //building limit

	//checking resources
	if(!building->resources.canBeAfforded(getPlayer(t->tempOwner)->resources))
		return EBuildingState::NO_RESOURCES; //lack of res

	return EBuildingState::ALLOWED;
}

const CMapHeader * CGameInfoCallback::getMapHeader() const
{
	return gs->map;
}

bool CGameInfoCallback::hasAccess(boost::optional<PlayerColor> playerId) const
{
	return !player || gs->getPlayerRelations( *playerId, *player ) != PlayerRelations::ENEMIES;
}

EPlayerStatus::EStatus CGameInfoCallback::getPlayerStatus(PlayerColor player, bool verbose) const
{
	const PlayerState *ps = gs->getPlayer(player, verbose);
	ERROR_VERBOSE_OR_NOT_RET_VAL_IF(!ps, verbose, "No such player!", EPlayerStatus::WRONG);

	return ps->status;
}

std::string CGameInfoCallback::getTavernRumor(const CGObjectInstance * townOrTavern) const
{
	std::string text = "", extraText = "";
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
	const PlayerState *p = gs->getPlayer(player);
	ERROR_RET_VAL_IF(!p, "No such player!", -1);

	if(includeGarrisoned)
		return p->heroes.size();
	else
		for(auto & elem : p->heroes)
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

CGameInfoCallback::CGameInfoCallback()
{
}

CGameInfoCallback::CGameInfoCallback(CGameState *GS, boost::optional<PlayerColor> Player)
{
	gs = GS;
	player = Player;
}

const std::vector< std::vector< std::vector<ui8> > > & CPlayerSpecificInfoCallback::getVisibilityMap() const
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
			if (i.first==player || (isVisible(town, player) && !onlyOur))
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
			(isVisible(hero->getPosition(false), player) && !onlyOur)	)
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
			return index;
	}
	return -1;
}

int3 CPlayerSpecificInfoCallback::getGrailPos( double &outKnownRatio )
{
	if (!player || CGObelisk::obeliskCount == 0)
	{
		outKnownRatio = 0.0;
	}
	else
	{
		outKnownRatio = static_cast<double>(CGObelisk::visited[gs->getPlayerTeam(*player)->id]) / CGObelisk::obeliskCount;
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
	for(CGDwelling * dw : gs->getPlayer(*player)->dwellings)
	{
		ret.push_back(dw);
	}
	return ret;
}

std::vector <QuestInfo> CPlayerSpecificInfoCallback::getMyQuests() const
{
	std::vector <QuestInfo> ret;
	for (auto quest : gs->getPlayer(*player)->quests)
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
	const PlayerState *p = getPlayer(*player);
	ERROR_RET_VAL_IF(!p, "No player info", nullptr);

	if (!includeGarrisoned)
	{
		for(ui32 i = 0; i < p->heroes.size() && i<=serialId; i++)
			if(p->heroes[i]->inTownGarrison)
				serialId++;
	}
	ERROR_RET_VAL_IF(serialId < 0 || serialId >= p->heroes.size(), "No player info", nullptr);
	return p->heroes[serialId];
}

const CGTownInstance* CPlayerSpecificInfoCallback::getTownBySerial(int serialId) const
{
	ASSERT_IF_CALLED_WITH_PLAYER
	const PlayerState *p = getPlayer(*player);
	ERROR_RET_VAL_IF(!p, "No player info", nullptr);
	ERROR_RET_VAL_IF(serialId < 0 || serialId >= p->towns.size(), "No player info", nullptr);
	return p->towns[serialId];
}

int CPlayerSpecificInfoCallback::getResourceAmount(Res::ERes type) const
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
				logGlobal->errorStream() << boost::format("Illegal attempt to access team data!");
				return nullptr;
			}
		}
	}
	else
	{
		logGlobal->errorStream() << boost::format("Cannot find info for team %d") % teamID;
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

const CGHeroInstance* CGameInfoCallback::getHeroWithSubid( int subid ) const
{
	for(const CGHeroInstance *h : gs->map->heroesOnMap)
		if(h->subID == subid)
			return h;

	return nullptr;
}

PlayerColor CGameInfoCallback::getLocalPlayer() const
{
	return getCurrentPlayer();
}

bool CGameInfoCallback::isInTheMap(const int3 &pos) const
{
	return gs->map->isInTheMap(pos);
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
	vstd::erase_if(ids, [&](ObjectInstanceID id) -> bool
	{
		auto obj = getObj(id, false);
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
	if((!entrances.size() || !exits.size()) // impassable if exits or entrances list are empty
		|| (entrances.size() == 1 && entrances == exits)) // impassable if only entrance and only exit is same object. e.g bidirectional monolith
	{
		return ETeleportChannelType::IMPASSABLE;
	}

	auto intersection = vstd::intersection(entrances, exits);
	if(intersection.size() == entrances.size() && intersection.size() == exits.size())
		return ETeleportChannelType::BIDIRECTIONAL;
	else if(!intersection.size())
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

void IGameEventRealizer::showInfoDialog( InfoWindow *iw )
{
	commitPackage(iw);
}

void IGameEventRealizer::showInfoDialog(const std::string &msg, PlayerColor player)
{
	InfoWindow iw;
	iw.player = player;
	iw.text << msg;
	showInfoDialog(&iw);
}

void IGameEventRealizer::setObjProperty(ObjectInstanceID objid, int prop, si64 val)
{
	SetObjectProperty sob;
	sob.id = objid;
	sob.what = prop;
	sob.val = static_cast<ui32>(val);
	commitPackage(&sob);
}

