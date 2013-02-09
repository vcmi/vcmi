#include "StdInc.h"
#include "IGameCallback.h"

#include <boost/random/linear_congruential.hpp>

#include "CGameState.h"
#include "Mapping/CMap.h"
#include "CObjectHandler.h"
#include "CHeroHandler.h"
#include "StartInfo.h"
#include "CArtHandler.h"
#include "CSpellHandler.h"
#include "VCMI_Lib.h"
#include "CTownHandler.h"
#include "BattleState.h"
#include "NetPacks.h"
#include "CBuildingHandler.h"
#include "GameConstants.h"
#include "CModHandler.h"

/*
 * IGameCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

//TODO make clean
#define ERROR_SILENT_RET_VAL_IF(cond, txt, retVal) do {if(cond){return retVal;}} while(0)
#define ERROR_VERBOSE_OR_NOT_RET_VAL_IF(cond, verbose, txt, retVal) do {if(cond){if(verbose)tlog1 << BOOST_CURRENT_FUNCTION << ": " << txt << std::endl; return retVal;}} while(0)
#define ERROR_RET_IF(cond, txt) do {if(cond){tlog1 << BOOST_CURRENT_FUNCTION << ": " << txt << std::endl; return;}} while(0)
#define ERROR_RET_VAL_IF(cond, txt, retVal) do {if(cond){tlog1 << BOOST_CURRENT_FUNCTION << ": " << txt << std::endl; return retVal;}} while(0)

extern boost::rand48 ran;

CGameState * CPrivilagedInfoCallback::gameState ()
{ 
	return gs;
}

int CGameInfoCallback::getOwner(int heroID) const
{
	const CGObjectInstance *obj = getObj(heroID);
	ERROR_RET_VAL_IF(!obj, "No such object!", -1);
	return gs->map->objects[heroID]->tempOwner;
}

int CGameInfoCallback::getResource(TPlayerColor Player, Res::ERes which) const
{
	const PlayerState *p = getPlayer(Player);
	ERROR_RET_VAL_IF(!p, "No player info!", -1);
	ERROR_RET_VAL_IF(p->resources.size() <= which || which < 0, "No such resource!", -1);
	return p->resources[which];
}

const CGHeroInstance* CGameInfoCallback::getSelectedHero( TPlayerColor Player ) const
{
	const PlayerState *p = getPlayer(Player);
	ERROR_RET_VAL_IF(!p, "No player info!", NULL);
	return getHero(p->currentSelection);
}

const CGHeroInstance* CGameInfoCallback::getSelectedHero() const
{
	return getSelectedHero(gs->currentPlayer);
}

const PlayerSettings * CGameInfoCallback::getPlayerSettings(TPlayerColor color) const
{
	return &gs->scenarioOps->getIthPlayersSettings(color);
}

void CPrivilagedInfoCallback::getTilesInRange( boost::unordered_set<int3, ShashInt3> &tiles, int3 pos, int radious, int player/*=-1*/, int mode/*=0*/ ) const
{
	if(player >= GameConstants::PLAYER_LIMIT)
	{
		tlog1 << "Illegal call to getTilesInRange!\n";
		return;
	}
	if (radious == -1) //reveal entire map
		getAllTiles (tiles, player, -1, 0);
	else
	{
		const TeamState * team = gs->getPlayerTeam(player);
		for (int xd = std::max<int>(pos.x - radious , 0); xd <= std::min<int>(pos.x + radious, gs->map->width - 1); xd++)
		{
			for (int yd = std::max<int>(pos.y - radious, 0); yd <= std::min<int>(pos.y + radious, gs->map->height - 1); yd++)
			{
				double distance = pos.dist2d(int3(xd,yd,pos.z)) - 0.5;
				if(distance <= radious)
				{
					if(player < 0 
						|| (mode == 1  && team->fogOfWarMap[xd][yd][pos.z]==0)
						|| (mode == -1 && team->fogOfWarMap[xd][yd][pos.z]==1)
					)
						tiles.insert(int3(xd,yd,pos.z));
				}
			}
		}
	}
}

void CPrivilagedInfoCallback::getAllTiles (boost::unordered_set<int3, ShashInt3> &tiles, int Player/*=-1*/, int level, int surface ) const
{
	if(Player >= GameConstants::PLAYER_LIMIT)
	{
		tlog1 << "Illegal call to getAllTiles !\n";
		return;
	}
	bool water = surface == 0 || surface == 2,
		land = surface == 0 || surface == 1;

	std::vector<int> floors;
	if(level == -1)
	{
		for(int b = 0; b < (gs->map->twoLevel ? 2 : 1); ++b)
		{
			floors.push_back(b);
		}
	}
	else
		floors.push_back(level);

	for (std::vector<int>::const_iterator i = floors.begin(); i!= floors.end(); i++)
	{
		register int zd = *i;
		for (int xd = 0; xd < gs->map->width; xd++)
		{
			for (int yd = 0; yd < gs->map->height; yd++)
			{
				if ((getTile (int3 (xd,yd,zd))->terType == 8 && water)
					|| (getTile (int3 (xd,yd,zd))->terType != 8 && land))
					tiles.insert(int3(xd,yd,zd));
			}
		}
	}
}

void CPrivilagedInfoCallback::getFreeTiles (std::vector<int3> &tiles) const
{
	std::vector<int> floors;
	for (int b = 0; b < (gs->map->twoLevel ? 2 : 1); ++b)
	{
		floors.push_back(b);
	}
	const TerrainTile *tinfo;
	for (std::vector<int>::const_iterator i = floors.begin(); i!= floors.end(); i++)
	{
		register int zd = *i;
		for (int xd = 0; xd < gs->map->width; xd++)
		{
			for (int yd = 0; yd < gs->map->height; yd++)
			{
				tinfo = getTile(int3 (xd,yd,zd));
				if (tinfo->terType != 8 && !tinfo->blocked) //land and free
					tiles.push_back (int3 (xd,yd,zd));
			}
		}
	}
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

void CPrivilagedInfoCallback::pickAllowedArtsSet(std::vector<const CArtifact*> &out)
{
	for (int j = 0; j < 3 ; j++)
		out.push_back(VLC->arth->artifacts[getRandomArt(CArtifact::ART_TREASURE)]);
	for (int j = 0; j < 3 ; j++)
		out.push_back(VLC->arth->artifacts[getRandomArt(CArtifact::ART_MINOR)]);

	out.push_back(VLC->arth->artifacts[getRandomArt(CArtifact::ART_MAJOR)]);
}

ui16 CPrivilagedInfoCallback::getRandomArt (int flags)
{
	return VLC->arth->getRandomArt(flags);
}

ui16 CPrivilagedInfoCallback::getArtSync (ui32 rand, int flags)
{
	return VLC->arth->getArtSync (rand, flags);
}

void CPrivilagedInfoCallback::erasePickedArt (si32 id)
{
	VLC->arth->erasePickedArt(id);
}


void CPrivilagedInfoCallback::getAllowedSpells(std::vector<ui16> &out, ui16 level)
{

	CSpell *spell;
	for (ui32 i = 0; i < gs->map->allowedSpell.size(); i++) //spellh size appears to be greater (?)
	{
		spell = VLC->spellh->spells[i];
		if (isAllowed (0, spell->id) && spell->level == level)
		{
			out.push_back(spell->id);
		}
	}
}

inline TerrainTile * CNonConstInfoCallback::getTile( int3 pos )
{
	if(!gs->map->isInTheMap(pos))
		return NULL;
	return &gs->map->getTile(pos);
}

const PlayerState * CGameInfoCallback::getPlayer(TPlayerColor color, bool verbose) const
{
	ERROR_VERBOSE_OR_NOT_RET_VAL_IF(!hasAccess(color), verbose, "Cannot access player " << color << "info!", NULL);
	ERROR_VERBOSE_OR_NOT_RET_VAL_IF(!vstd::contains(gs->players,color), verbose, "Cannot find player " << color << "info!", NULL);
	return &gs->players[color];
}

const CTown * CGameInfoCallback::getNativeTown(int color) const
{
	const PlayerSettings *ps = getPlayerSettings(color);
	ERROR_RET_VAL_IF(!ps, "There is no such player!", NULL);
	return &VLC->townh->towns[ps->castle];
}

const CGObjectInstance * CGameInfoCallback::getObjByQuestIdentifier(int identifier) const
{
	ERROR_RET_VAL_IF(!vstd::contains(gs->map->questIdentifierToId, identifier), "There is no object with such quest identifier!", NULL);
	return getObj(gs->map->questIdentifierToId[identifier]);
}

/************************************************************************/
/*                                                                      */
/************************************************************************/

const CGObjectInstance* CGameInfoCallback::getObj(int objid, bool verbose) const
{
	if(objid < 0  ||  objid >= gs->map->objects.size())
	{
		if(verbose)
			tlog1 << "Cannot get object with id " << objid << std::endl;
		return NULL;
	}

	const CGObjectInstance *ret = gs->map->objects[objid];
	if(!ret)
	{
		if(verbose)
			tlog1 << "Cannot get object with id " << objid << ". Object was removed.\n";
		return NULL;
	}

	if(!isVisible(ret, player))
	{
		if(verbose)
			tlog1 << "Cannot get object with id " << objid << ". Object is not visible.\n";
		return NULL;
	}

	return ret;
}

const CGHeroInstance* CGameInfoCallback::getHero(int objid) const
{
	const CGObjectInstance *obj = getObj(objid, false);
	if(obj)
		return dynamic_cast<const CGHeroInstance*>(obj);
	else
		return NULL;
}
const CGTownInstance* CGameInfoCallback::getTown(int objid) const
{
	const CGObjectInstance *obj = getObj(objid, false);
	if(obj)
		return dynamic_cast<const CGTownInstance*>(gs->map->objects[objid].get());
	else
		return NULL;
}

void CGameInfoCallback::getUpgradeInfo(const CArmedInstance *obj, int stackPos, UpgradeInfo &out) const
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
				sp, hero, NULL, hero->getSpellSchoolLevel(sp), hero->getPrimSkillLevel(PrimarySkill::SPELL_POWER));
		else
			return 0; //mage guild
	}
	//gs->getHero(gs->currentPlayer)
	//const CGHeroInstance * ourHero = gs->curB->heroes[0]->tempOwner == player ? gs->curB->heroes[0] : gs->curB->heroes[1];
	const CGHeroInstance * ourHero = hero;
	return gs->curB->calculateSpellDmg(
		sp, ourHero, NULL, ourHero->getSpellSchoolLevel(sp), ourHero->getPrimSkillLevel(PrimarySkill::SPELL_POWER));
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

int CGameInfoCallback::howManyTowns(int Player) const
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

int3 CGameInfoCallback::guardingCreaturePosition (int3 pos) const
{
	ERROR_RET_VAL_IF(!isVisible(pos), "Tile is not visible!", int3(-1,-1,-1));
	return gs->guardingCreaturePosition(pos);
}

std::vector<const CGObjectInstance*> CGameInfoCallback::getGuardingCreatures (int3 pos) const
{
	ERROR_RET_VAL_IF(!isVisible(pos), "Tile is not visible!", std::vector<const CGObjectInstance*>());
	std::vector<const CGObjectInstance*> ret;
	BOOST_FOREACH(auto cr, gs->guardingCreatures(pos))
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
std::vector < std::string > CGameInfoCallback::getObjDescriptions(int3 pos) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	std::vector<std::string> ret;
	const TerrainTile *t = getTile(pos);
	ERROR_RET_VAL_IF(!t, "Not a valid tile given!", ret);


	BOOST_FOREACH(const CGObjectInstance * obj, t->blockingObjects)
		ret.push_back(obj->getHoverText());
	return ret;
}

bool CGameInfoCallback::isVisible(int3 pos, int Player) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->map->isInTheMap(pos) && (Player == -1 || gs->isVisible(pos, Player));
}

bool CGameInfoCallback::isVisible(int3 pos) const
{
	return isVisible(pos,player);
}

bool CGameInfoCallback::isVisible( const CGObjectInstance *obj, int Player ) const
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
// 		return NULL;
// 	else 
// 		return armi;
// }

std::vector < const CGObjectInstance * > CGameInfoCallback::getBlockingObjs( int3 pos ) const
{
	std::vector<const CGObjectInstance *> ret;
	const TerrainTile *t = getTile(pos);
	ERROR_RET_VAL_IF(!t, "Not a valid tile requested!", ret);

	BOOST_FOREACH(const CGObjectInstance * obj, t->blockingObjects)
		ret.push_back(obj);
	return ret;
}

std::vector <const CGObjectInstance * > CGameInfoCallback::getVisitableObjs(int3 pos, bool verbose /*= true*/) const
{
	std::vector<const CGObjectInstance *> ret;
	const TerrainTile *t = getTile(pos, verbose);
	ERROR_VERBOSE_OR_NOT_RET_VAL_IF(!t, verbose, pos << " is not visible!", ret);

	BOOST_FOREACH(const CGObjectInstance * obj, t->visitableObjects)
	{
		if(player < 0 || obj->ID != Obj::EVENT) //hide events from players
			ret.push_back(obj);
	}

	return ret;
}

std::vector < const CGObjectInstance * > CGameInfoCallback::getFlaggableObjects(int3 pos) const
{
	std::vector<const CGObjectInstance *> ret;
	const TerrainTile *t = getTile(pos);
	ERROR_RET_VAL_IF(!t, "Not a valid tile requested!", ret);
	BOOST_FOREACH(const CGObjectInstance *obj, t->blockingObjects)
		if(obj->tempOwner != 254)
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
	std::vector<const CGHeroInstance *> ret;
	//ERROR_RET_VAL_IF(!isOwnedOrVisited(townOrTavern), "Town or tavern must be owned or visited!", ret);
	//TODO: town needs to be owned, advmap tavern needs to be visited; to be reimplemented when visit tracking is done
	ret.resize(gs->players[player].availableHeroes.size());
	std::copy(gs->players[player].availableHeroes.begin(),gs->players[player].availableHeroes.end(),ret.begin());
	return ret;
}	

const TerrainTile * CGameInfoCallback::getTile( int3 tile, bool verbose) const
{
	ERROR_VERBOSE_OR_NOT_RET_VAL_IF(!isVisible(tile), verbose, tile << " is not visible!", NULL);

	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return &gs->map->getTile(tile);
}

EBuildingState::EBuildingState CGameInfoCallback::canBuildStructure( const CGTownInstance *t, int ID )
{
	ERROR_RET_VAL_IF(!canGetFullInfo(t), "Town is not owned!", EBuildingState::TOWN_NOT_OWNED);

	CBuilding * pom = t->town->buildings[ID];

	if(!pom)
		return EBuildingState::BUILDING_ERROR;

	if(t->hasBuilt(ID))	//already built
		return EBuildingState::ALREADY_PRESENT;

	//can we build it?
	if(t->forbiddenBuildings.find(ID)!=t->forbiddenBuildings.end())
		return EBuildingState::FORBIDDEN; //forbidden

	//checking for requirements
	std::set<int> reqs = getBuildingRequiments(t, ID);//getting all requirements

	bool notAllBuilt = false;
	for( std::set<int>::iterator ri  =  reqs.begin(); ri != reqs.end(); ri++ )
	{
		if(!t->hasBuilt(*ri)) //lack of requirements - cannot build
		{
			if(vstd::contains(t->forbiddenBuildings, *ri)) // not built requirement forbidden - same goes to this build
				return EBuildingState::FORBIDDEN;
			else
				notAllBuilt = true; // no return here - we need to check if any required builds are forbidden
		}
	}

	if(t->builded >= VLC->modh->settings.MAX_BUILDING_PER_TURN)
		return EBuildingState::CANT_BUILD_TODAY; //building limit

	if (notAllBuilt)
		return EBuildingState::PREREQUIRES;

	if(ID == EBuilding::CAPITOL)
	{
		const PlayerState *ps = getPlayer(t->tempOwner);
		if(ps)
		{
			BOOST_FOREACH(const CGTownInstance *t, ps->towns)
			{
				if(t->hasBuilt(EBuilding::CAPITOL))
				{
					return EBuildingState::HAVE_CAPITAL; //no more than one capitol
				}
			}
		}
	}
	else if(ID == EBuilding::SHIPYARD)
	{
		const TerrainTile *tile = getTile(t->bestLocation(), false);
		
        if(!tile || tile->terType != ETerrainType::WATER)
			return EBuildingState::NO_WATER; //lack of water
	}

	//checking resources
	if(!pom->resources.canBeAfforded(getPlayer(t->tempOwner)->resources))
		return EBuildingState::NO_RESOURCES; //lack of res

	return EBuildingState::ALLOWED;
}

std::set<int> CGameInfoCallback::getBuildingRequiments( const CGTownInstance *t, int ID )
{
	ERROR_RET_VAL_IF(!canGetFullInfo(t), "Town is not owned!", std::set<int>());

	std::set<int> used;
	used.insert(ID);
	auto reqs = t->town->buildings[ID]->requirements;

	bool found;
	do
	{
		found = false;
		for(auto i=reqs.begin();i!=reqs.end();i++)
		{
			if(used.find(*i)==used.end()) //we haven't added requirements for this building
			{
				found = true;
				auto & requires = t->town->buildings[*i]->requirements;

				used.insert(*i);
				for(auto j = requires.begin(); j!= requires.end(); j++)
					reqs.insert(*j);//creating full list of requirements
			}
		}
	}
	while (found);

	return reqs;
}

const CMapHeader * CGameInfoCallback::getMapHeader() const
{
	return gs->map;
}

bool CGameInfoCallback::hasAccess(int playerId) const
{
	return player < 0 || gs->getPlayerRelations( playerId, player ) != PlayerRelations::ENEMIES;
}

EPlayerStatus::EStatus CGameInfoCallback::getPlayerStatus(TPlayerColor player) const
{
	const PlayerState *ps = gs->getPlayer(player, false);
	ERROR_RET_VAL_IF(!ps, "No such player!", EPlayerStatus::WRONG);

	return ps->status;
}

std::string CGameInfoCallback::getTavernGossip(const CGObjectInstance * townOrTavern) const
{
	return "GOSSIP TEST";
}

PlayerRelations::PlayerRelations CGameInfoCallback::getPlayerRelations( TPlayerColor color1, TPlayerColor color2 ) const
{
	return gs->getPlayerRelations(color1, color2);
}

bool CGameInfoCallback::canGetFullInfo(const CGObjectInstance *obj) const
{
	return !obj || hasAccess(obj->tempOwner);
}

int CGameInfoCallback::getHeroCount( int player, bool includeGarrisoned ) const
{
	int ret = 0;
	const PlayerState *p = gs->getPlayer(player);
	ERROR_RET_VAL_IF(!p, "No such player!", -1);
	
	if(includeGarrisoned)
		return p->heroes.size();
	else
		for(ui32 i = 0; i < p->heroes.size(); i++)
			if(!p->heroes[i]->inTownGarrison)
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

int CGameInfoCallback::getCurrentPlayer() const
{
	return gs->currentPlayer;
}

CGameInfoCallback::CGameInfoCallback()
{
}

CGameInfoCallback::CGameInfoCallback(CGameState *GS, int Player)
{
	gs = GS;
	player = Player;
}

const std::vector< std::vector< std::vector<ui8> > > & CPlayerSpecificInfoCallback::getVisibilityMap() const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->getPlayerTeam(player)->fogOfWarMap;
}

int CPlayerSpecificInfoCallback::howManyTowns() const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	ERROR_RET_VAL_IF(player == -1, "Applicable only for player callbacks", -1);
	return CGameInfoCallback::howManyTowns(player);
}

std::vector < const CGTownInstance *> CPlayerSpecificInfoCallback::getTownsInfo(bool onlyOur) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	std::vector < const CGTownInstance *> ret = std::vector < const CGTownInstance *>();
	for ( auto i=gs->players.begin() ; i!=gs->players.end();i++)
	{
		for (size_t j = 0; j < (*i).second.towns.size(); ++j)
		{
			if ((*i).first==player  
				|| (isVisible((*i).second.towns[j],player) && !onlyOur))
			{
				ret.push_back((*i).second.towns[j]);
			}
		}
	} //	for ( std::map<int, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
	return ret;
}
std::vector < const CGHeroInstance *> CPlayerSpecificInfoCallback::getHeroesInfo(bool onlyOur) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	std::vector < const CGHeroInstance *> ret;
	for(size_t i=0;i<gs->map->heroes.size();i++)
	{
		if(	 (gs->map->heroes[i]->tempOwner==player) ||
			(isVisible(gs->map->heroes[i]->getPosition(false),player) && !onlyOur)	)
		{
			ret.push_back(gs->map->heroes[i]);
		}
	}
	return ret;
}

int CPlayerSpecificInfoCallback::getMyColor() const
{
	return player;
}

int CPlayerSpecificInfoCallback::getHeroSerial(const CGHeroInstance * hero, bool includeGarrisoned) const
{
	if (hero->inTownGarrison && !includeGarrisoned)
		return -1;

	size_t index = 0;
	auto & heroes = gs->players[player].heroes;

	for (auto curHero= heroes.begin(); curHero!=heroes.end(); curHero++)
	{
		if (includeGarrisoned || !(*curHero)->inTownGarrison)
			index++;

		if (*curHero == hero)
			return index;
	}
	return -1;
}

int3 CPlayerSpecificInfoCallback::getGrailPos( double &outKnownRatio )
{
	if (CGObelisk::obeliskCount == 0)
	{
		outKnownRatio = 0.0;
	}
	else
	{
		outKnownRatio = static_cast<double>(CGObelisk::visited[gs->getPlayerTeam(player)->id]) / CGObelisk::obeliskCount;
	}
	return gs->map->grailPos;
}

std::vector < const CGObjectInstance * > CPlayerSpecificInfoCallback::getMyObjects() const
{
	std::vector < const CGObjectInstance * > ret;
	BOOST_FOREACH(const CGObjectInstance * obj, gs->map->objects)
	{
		if(obj && obj->tempOwner == player)
			ret.push_back(obj);
	}
	return ret;
}

std::vector < const CGDwelling * > CPlayerSpecificInfoCallback::getMyDwellings() const
{
	std::vector < const CGDwelling * > ret;
	BOOST_FOREACH(CGDwelling * dw, gs->getPlayer(player)->dwellings)
	{
		ret.push_back(dw);
	}
	return ret;
}

std::vector <QuestInfo> CPlayerSpecificInfoCallback::getMyQuests() const
{
	std::vector <QuestInfo> ret;
	BOOST_FOREACH (auto quest, gs->getPlayer(player)->quests)
	{
		ret.push_back (quest);
	}
	return ret;
}

int CPlayerSpecificInfoCallback::howManyHeroes(bool includeGarrisoned) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	ERROR_RET_VAL_IF(player == -1, "Applicable only for player callbacks", -1);
	return getHeroCount(player,includeGarrisoned);
}

const CGHeroInstance* CPlayerSpecificInfoCallback::getHeroBySerial(int serialId, bool includeGarrisoned) const
{
	const PlayerState *p = getPlayer(player);
	ERROR_RET_VAL_IF(!p, "No player info", NULL);

	if (!includeGarrisoned)
	{
		for(ui32 i = 0; i < p->heroes.size() && i<=serialId; i++)
			if(p->heroes[i]->inTownGarrison)
				serialId++;
	}
	ERROR_RET_VAL_IF(serialId < 0 || serialId >= p->heroes.size(), "No player info", NULL);
	return p->heroes[serialId];
}

const CGTownInstance* CPlayerSpecificInfoCallback::getTownBySerial(int serialId) const
{
	const PlayerState *p = getPlayer(player);
	ERROR_RET_VAL_IF(!p, "No player info", NULL);
	ERROR_RET_VAL_IF(serialId < 0 || serialId >= p->towns.size(), "No player info", NULL);
	return p->towns[serialId];
}

int CPlayerSpecificInfoCallback::getResourceAmount(Res::ERes type) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	ERROR_RET_VAL_IF(player == -1, "Applicable only for player callbacks", -1);
	return getResource(player, type);
}

TResources CPlayerSpecificInfoCallback::getResourceAmount() const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	ERROR_RET_VAL_IF(player == -1, "Applicable only for player callbacks", TResources());
	return gs->players[player].resources;
}

CGHeroInstance *CNonConstInfoCallback::getHero(int objid)
{
	return const_cast<CGHeroInstance*>(CGameInfoCallback::getHero(objid));
}

CGTownInstance *CNonConstInfoCallback::getTown(int objid)
{

	return const_cast<CGTownInstance*>(CGameInfoCallback::getTown(objid));
}

TeamState *CNonConstInfoCallback::getTeam(ui8 teamID)
{
	return const_cast<TeamState*>(CGameInfoCallback::getTeam(teamID));
}

TeamState *CNonConstInfoCallback::getPlayerTeam(TPlayerColor color)
{
	return const_cast<TeamState*>(CGameInfoCallback::getPlayerTeam(color));
}

PlayerState * CNonConstInfoCallback::getPlayer( TPlayerColor color, bool verbose )
{
	return const_cast<PlayerState*>(CGameInfoCallback::getPlayer(color, verbose));
}

const TeamState * CGameInfoCallback::getTeam( ui8 teamID ) const
{
	ERROR_RET_VAL_IF(!vstd::contains(gs->teams, teamID), "Cannot find info for team " << int(teamID), NULL);
	const TeamState *ret = &gs->teams[teamID];
	ERROR_RET_VAL_IF(player != -1 && !vstd::contains(ret->players, player), "Illegal attempt to access team data!", NULL);
	return ret;
}

const TeamState * CGameInfoCallback::getPlayerTeam( ui8 teamID ) const
{
	const PlayerState * ps = getPlayer(teamID);
	if (ps)
		return getTeam(ps->team);
	return NULL;
}

const CGHeroInstance* CGameInfoCallback::getHeroWithSubid( int subid ) const
{
	BOOST_FOREACH(const CGHeroInstance *h, gs->map->heroes)
		if(h->subID == subid)
			return h;

	return NULL;
}

int CGameInfoCallback::getLocalPlayer() const
{
	return getCurrentPlayer();
}

bool CGameInfoCallback::isInTheMap(const int3 &pos) const
{
	return gs->map->isInTheMap(pos);
}

void IGameEventRealizer::showInfoDialog( InfoWindow *iw )
{
	commitPackage(iw);
}

void IGameEventRealizer::showInfoDialog(const std::string &msg, int player)
{
	InfoWindow iw;
	iw.player = player;
	iw.text << msg;
	showInfoDialog(&iw);
}

void IGameEventRealizer::setObjProperty(int objid, int prop, si64 val)
{
	SetObjectProperty sob;
	sob.id = objid;
	sob.what = prop;
	sob.val = static_cast<ui32>(val);
	commitPackage(&sob);
}

const CGObjectInstance * IGameCallback::putNewObject(Obj::Obj ID, int subID, int3 pos)
{
	NewObject no;
	no.ID = ID; //creature
	no.subID= subID;
	no.pos = pos;
	commitPackage(&no);
	return getObj(no.id); //id field will be filled during applying on gs
}

const CGCreature * IGameCallback::putNewMonster(int creID, int count, int3 pos)
{
	const CGObjectInstance *m = putNewObject(Obj::MONSTER, creID, pos);
	setObjProperty(m->id, ObjProperty::MONSTER_COUNT, count);
	setObjProperty(m->id, ObjProperty::MONSTER_POWER, (si64)1000*count);
	return dynamic_cast<const CGCreature*>(m);
}
