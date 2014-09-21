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
#include "mapObjects/CObjectHandler.h" // for CGObjectInstance
#include "StartInfo.h" // for StartInfo
#include "BattleState.h" // for BattleInfo
#include "NetPacks.h" // for InfoWindow
#include "CModHandler.h"

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
	ERROR_VERBOSE_OR_NOT_RET_VAL_IF(!hasAccess(color), verbose, "Cannot access player " << color << "info!", nullptr);
	//if (!vstd::contains(gs->players, color))
	//{
	//	logGlobal->errorStream() << "Cannot access player " << color << "info!";
	//	return nullptr; //macros are not really useful when debugging :?
	//}
	//else
	//{
	ERROR_VERBOSE_OR_NOT_RET_VAL_IF(!vstd::contains(gs->players,color), verbose, "Cannot find player " << color << "info!", nullptr);
	return &gs->players[color];
	//}
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

	if(!isVisible(ret, player))
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
	if(!gs->curB) //no battle
	{
		if (hero) //but we see hero's spellbook
			return gs->curB->calculateSpellDmg(
				sp, hero, nullptr, hero->getSpellSchoolLevel(sp), hero->getPrimSkillLevel(PrimarySkill::SPELL_POWER));
		else
			return 0; //mage guild
	}
	//gs->getHero(gs->currentPlayer)
	//const CGHeroInstance * ourHero = gs->curB->heroes[0]->tempOwner == player ? gs->curB->heroes[0] : gs->curB->heroes[1];
	const CGHeroInstance * ourHero = hero;
	return gs->curB->calculateSpellDmg(
		sp, ourHero, nullptr, ourHero->getSpellSchoolLevel(sp), ourHero->getPrimSkillLevel(PrimarySkill::SPELL_POWER));
}

void CGameInfoCallback::getThievesGuildInfo(SThievesGuildInfo & thi, const CGObjectInstance * obj)
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	ERROR_RET_IF(!obj, "No guild object!");
	ERROR_RET_IF(obj->ID == Obj::TOWN && !canGetFullInfo(obj), "Cannot get info about town guild object!");
	//TODO: advmap object -> check if they're visited by our hero

	if(obj->ID == Obj::TOWN  ||  obj->ID == Obj::TAVERN)
	{
		gs->obtainPlayersStats(thi, gs->players[obj->tempOwner].towns.size());
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

bool CGameInfoCallback::getTownInfo( const CGObjectInstance *town, InfoAboutTown &dest ) const
{
	ERROR_RET_VAL_IF(!isVisible(town, player), "Town is not visible!", false);  //it's not a town or it's not visible for layer
	bool detailed = hasAccess(town->tempOwner);

	//TODO vision support
	if(town->ID == Obj::TOWN)
		dest.initFromTown(static_cast<const CGTownInstance *>(town), detailed);
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

bool CGameInfoCallback::getHeroInfo( const CGObjectInstance *hero, InfoAboutHero &dest ) const
{
	const CGHeroInstance *h = dynamic_cast<const CGHeroInstance *>(hero);

	ERROR_RET_VAL_IF(!h, "That's not a hero!", false);
	ERROR_RET_VAL_IF(!isVisible(h->getPosition(false)), "That hero is not visible!", false);

	//TODO vision support
	dest.initFromHero(h, hasAccess(h->tempOwner));
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
		if(player < nullptr || obj->ID != Obj::EVENT) //hide events from players
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

std::string CGameInfoCallback::getTavernGossip(const CGObjectInstance * townOrTavern) const
{
	return "GOSSIP TEST";
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
		if( !player || (hero->tempOwner == *player) ||
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
	ERROR_RET_VAL_IF(!vstd::contains(gs->teams, teamID), "Cannot find info for team " << teamID, nullptr);
	const TeamState *ret = &gs->teams[teamID];
	ERROR_RET_VAL_IF(!!player && !vstd::contains(ret->players, *player), "Illegal attempt to access team data!", nullptr);
	return ret;
}

const TeamState * CGameInfoCallback::getPlayerTeam( PlayerColor color ) const
{
	const PlayerState * ps = getPlayer(color);
	if (ps)
		return getTeam(ps->team);
	return nullptr;
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

