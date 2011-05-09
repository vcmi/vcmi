#define VCMI_DLL
#include "IGameCallback.h"
#include "../lib/CGameState.h"
#include "../lib/map.h"
#include "CObjectHandler.h"
#include "CHeroHandler.h"
#include "../StartInfo.h"
#include "CArtHandler.h"
#include "CSpellHandler.h"
#include "../lib/VCMI_Lib.h"
#include <boost/random/linear_congruential.hpp>
#include "CTownHandler.h"
#include "BattleState.h"
#include <boost/foreach.hpp>
#include "NetPacks.h"
#include <boost/bind.hpp>
#include "CBuildingHandler.h"

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

si8 CBattleInfoCallback::battleHasDistancePenalty( const CStack * stack, THex destHex )
{
	return gs->curB->hasDistancePenalty(stack, destHex);
}

si8 CBattleInfoCallback::battleHasWallPenalty( const CStack * stack, THex destHex )
{
	return gs->curB->hasWallPenalty(stack, destHex);
}

si8 CBattleInfoCallback::battleCanTeleportTo(const CStack * stack, THex destHex, int telportLevel)
{
	return gs->curB->canTeleportTo(stack, destHex, telportLevel);
}

std::vector<int> CBattleInfoCallback::battleGetDistances(const CStack * stack, THex hex /*= THex::INVALID*/, THex * predecessors /*= NULL*/)
{
	if(!hex.isValid())
		hex = stack->position;

	std::vector<int> ret;
	bool ac[BFIELD_SIZE] = {0};
	std::set<THex> occupyable;
	gs->curB->getAccessibilityMap(ac, stack->doubleWide(), stack->attackerOwned, false, occupyable, stack->hasBonusOfType(Bonus::FLYING), stack);
	THex pr[BFIELD_SIZE];
	int dist[BFIELD_SIZE];
	gs->curB->makeBFS(stack->position, ac, pr, dist, stack->doubleWide(), stack->attackerOwned, stack->hasBonusOfType(Bonus::FLYING), false);

	for(int i=0; i<BFIELD_SIZE; ++i)
	{
		if(pr[i] == -1)
			ret.push_back(-1);
		else
			ret.push_back(dist[i]);
	}

	if(predecessors)
	{
		memcpy(predecessors, pr, BFIELD_SIZE * sizeof(THex));
	}

	return ret;
}

SpellCasting::ESpellCastProblem CBattleInfoCallback::battleCanCastThisSpell( const CSpell * spell )
{
	if(!gs->curB)
	{

		tlog1 << "battleCanCastThisSpell called when there is no battle!\n";
		return SpellCasting::NO_HERO_TO_CAST_SPELL;
	}

	return gs->curB->battleCanCastThisSpell(player, spell, SpellCasting::HERO_CASTING);
}

si8 CBattleInfoCallback::battleGetTacticDist()
{
	if (!gs->curB)
	{
		tlog1 << "battleGetTacticDist called when no battle!\n";
		return 0;
	}

	if (gs->curB->sides[gs->curB->tacticsSide] == player)
	{
		return gs->curB->tacticDistance;
	}
	return 0;
}

ui8 CBattleInfoCallback::battleGetMySide()
{
	if (!gs->curB)
	{
		tlog1 << "battleGetMySide called when no battle!\n";
		return 0;
	}

	return gs->curB->sides[1] == player;
}

int CBattleInfoCallback::battleGetSurrenderCost()
{
	if (!gs->curB)
	{
		tlog1 << "battleGetSurrenderCost called when no battle!\n";
		return -1;
	}

	return gs->curB->getSurrenderingCost(player);
}

int CBattleInfoCallback::battleGetBattlefieldType()
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	//return gs->battleGetBattlefieldType();

	if(!gs->curB)
	{
		tlog2<<"battleGetBattlefieldType called when there is no battle!"<<std::endl;
		return -1;
	}
	return gs->curB->battlefieldType;
}

int CBattleInfoCallback::battleGetObstaclesAtTile(THex tile) //returns bitfield 
{
	//TODO - write
	return -1;
}

std::vector<CObstacleInstance> CBattleInfoCallback::battleGetAllObstacles()
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	if(gs->curB)
		return gs->curB->obstacles;
	else
		return std::vector<CObstacleInstance>();
}

const CStack* CBattleInfoCallback::battleGetStackByID(int ID, bool onlyAlive)
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	if(!gs->curB) return NULL;
	return gs->curB->getStack(ID, onlyAlive);
}

const CStack* CBattleInfoCallback::battleGetStackByPos(THex pos, bool onlyAlive)
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->curB->battleGetStack(pos, onlyAlive);
}

THex CBattleInfoCallback::battleGetPos(int stack)
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	if(!gs->curB)
	{
		tlog2<<"battleGetPos called when there is no battle!"<<std::endl;
		return THex::INVALID;
	}
	for(size_t g=0; g<gs->curB->stacks.size(); ++g)
	{
		if(gs->curB->stacks[g]->ID == stack)
			return gs->curB->stacks[g]->position;
	}
	return THex::INVALID;
}

TStacks CBattleInfoCallback::battleGetStacks(EStackOwnership whose /*= MINE_AND_ENEMY*/, bool onlyAlive /*= true*/)
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	TStacks ret;
	if(!gs->curB) //there is no battle
	{
		tlog2<<"battleGetStacks called when there is no battle!"<<std::endl;
		return ret;
	}

	BOOST_FOREACH(const CStack *s, gs->curB->stacks)
	{
		bool ownerMatches = whose == MINE_AND_ENEMY || whose == ONLY_MINE && s->owner == player || whose == ONLY_ENEMY && s->owner != player;
		bool alivenessMatches = s->alive()  ||  !onlyAlive;
		if(ownerMatches && alivenessMatches)
			ret.push_back(s);
	}

	return ret;
}

void CBattleInfoCallback::getStackQueue( std::vector<const CStack *> &out, int howMany )
{
	if(!gs->curB)
	{
		tlog2 << "battleGetStackQueue called when there is not battle!" << std::endl;
		return;
	}
	gs->curB->getStackQueue(out, howMany);
}

std::vector<THex> CBattleInfoCallback::battleGetAvailableHexes(const CStack * stack, bool addOccupiable, std::vector<THex> * attackable)
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	if(!gs->curB)
	{
		tlog2<<"battleGetAvailableHexes called when there is no battle!"<<std::endl;
		return std::vector<THex>();
	}
	return gs->curB->getAccessibility(stack, addOccupiable, attackable);
	//return gs->battleGetRange(ID);
}

bool CBattleInfoCallback::battleCanShoot(const CStack * stack, THex dest)
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);

	if(!gs->curB) return false;

	return gs->curB->battleCanShoot(stack, dest);
}

bool CBattleInfoCallback::battleCanCastSpell()
{
	if(!gs->curB) //there is no battle
		return false;

	return gs->curB->battleCanCastSpell(player, SpellCasting::HERO_CASTING) == SpellCasting::OK;
}

bool CBattleInfoCallback::battleCanFlee()
{
	return gs->curB->battleCanFlee(player);
}

const CGTownInstance *CBattleInfoCallback::battleGetDefendedTown()
{
	if(!gs->curB || gs->curB->town == NULL)
		return NULL;

	return gs->curB->town;
}

ui8 CBattleInfoCallback::battleGetWallState(int partOfWall)
{
	if(!gs->curB || gs->curB->siege == 0)
	{
		return 0;
	}
	return gs->curB->si.wallState[partOfWall];
}

int CBattleInfoCallback::battleGetWallUnderHex(THex hex)
{
	if(!gs->curB || gs->curB->siege == 0)
	{
		return -1;
	}
	return gs->curB->hexToWallPart(hex);
}

TDmgRange CBattleInfoCallback::battleEstimateDamage(const CStack * attacker, const CStack * defender, TDmgRange * retaliationDmg)
{
	if(!gs->curB)
		return std::make_pair(0, 0);

	const CGHeroInstance * attackerHero, * defenderHero;
	bool shooting = battleCanShoot(attacker, defender->position);

	if(gs->curB->sides[0] == attacker->owner)
	{
		attackerHero = gs->curB->heroes[0];
		defenderHero = gs->curB->heroes[1];
	}
	else
	{
		attackerHero = gs->curB->heroes[1];
		defenderHero = gs->curB->heroes[0];
	}

	TDmgRange ret = gs->curB->calculateDmgRange(attacker, defender, attackerHero, defenderHero, shooting, 0, false, false);

	if(retaliationDmg)
	{
		if(shooting)
		{
			retaliationDmg->first = retaliationDmg->second = 0;
		}
		else
		{
			ui32 TDmgRange::* pairElems[] = {&TDmgRange::first, &TDmgRange::second};
			for (int i=0; i<2; ++i)
			{
				BattleStackAttacked bsa;
				bsa.damageAmount = ret.*pairElems[i];
				retaliationDmg->*pairElems[!i] = gs->curB->calculateDmgRange(defender, attacker, bsa.newAmount, attacker->count, attackerHero, defenderHero, false, false, false, false).*pairElems[!i];
			}
		}
	}

	return ret;
}

ui8 CBattleInfoCallback::battleGetSiegeLevel()
{
	if(!gs->curB)
		return 0;

	return gs->curB->siege;
}

const CGHeroInstance * CBattleInfoCallback::battleGetFightingHero(ui8 side) const
{
	if(!gs->curB)
		return 0;

	return gs->curB->heroes[side];
}



CGameState *const CPrivilagedInfoCallback::gameState ()
{ 
	return gs;
}

int CGameInfoCallback::getOwner(int heroID) const
{
	const CGObjectInstance *obj = getObj(heroID);
	ERROR_RET_VAL_IF(!obj, "No such object!", -1);
	return gs->map->objects[heroID]->tempOwner;
}

int CGameInfoCallback::getResource(int Player, int which) const
{
	const PlayerState *p = getPlayer(Player);
	ERROR_RET_VAL_IF(!p, "No player info!", -1);
	ERROR_RET_VAL_IF(p->resources.size() <= which || which < 0, "No such resource!", -1);
	return p->resources[which];
}

const CGHeroInstance* CGameInfoCallback::getSelectedHero( int Player ) const
{
	const PlayerState *p = getPlayer(Player);
	ERROR_RET_VAL_IF(!p, "No player info!", NULL);
	return getHero(p->currentSelection);
}

int CGameInfoCallback::getSelectedHero() const
{
	const CGHeroInstance *h = getSelectedHero(gs->currentPlayer);
	return h ? h->id : -1; // TODO synchronous turns?
}

const PlayerSettings * CGameInfoCallback::getPlayerSettings(int color) const
{
	return &gs->scenarioOps->getIthPlayersSettings(color);
}

void CPrivilagedInfoCallback::getTilesInRange( boost::unordered_set<int3, ShashInt3> &tiles, int3 pos, int radious, int player/*=-1*/, int mode/*=0*/ ) const
{
	if(player >= PLAYER_LIMIT)
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
	if(Player >= PLAYER_LIMIT)
	{
		tlog1 << "Illegal call to getAllTiles !\n";
		return;
	}
	bool water = surface == 0 || surface == 2,
		land = surface == 0 || surface == 1;

	std::vector<int> floors;
	if(level == -1)
	{
		
		for (int xd = 0; xd <= gs->map->width - 1; xd++)
		for(int b=0; b<gs->map->twoLevel + 1; ++b) //if gs->map->twoLevel is false then false (0) + 1 is 1, if it's true (1) then we have 2
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
				if ((getTile (int3 (xd,yd,zd))->tertype == 8 && water)
					|| (getTile (int3 (xd,yd,zd))->tertype != 8 && land))
					tiles.insert(int3(xd,yd,zd));
			}
		}
	}
}

void CPrivilagedInfoCallback::getFreeTiles (std::vector<int3> &tiles) const
{
	std::vector<int> floors;
	for (int b=0; b<gs->map->twoLevel + 1; ++b) //if gs->map->twoLevel is false then false (0) + 1 is 1, if it's true (1) then we have 2
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
				if (tinfo->tertype != 8 && !tinfo->blocked) //land and free
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
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 3 ; j++)
		{
			out.push_back(VLC->arth->artifacts[getRandomArt(CArtifact::ART_TREASURE << i)]);
		}
	}
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
	for (int i = 0; i < gs->map->allowedSpell.size(); i++) //spellh size appears to be greater (?)
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

const PlayerState * CGameInfoCallback::getPlayer(int color, bool verbose) const
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

const StartInfo * CGameInfoCallback::getStartInfo() const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->scenarioOps;
}

int CGameInfoCallback::getSpellCost(const CSpell * sp, const CGHeroInstance * caster) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	ERROR_RET_VAL_IF(!canGetFullInfo(caster), "Cannot get info about caster!", -1);
	//if there is a battle
	if(gs->curB)
		return gs->curB->getSpellCost(sp, caster);

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
			return gs->curB->calculateSpellDmg(sp, hero, NULL, hero->getSpellSchoolLevel(sp), hero->getPrimSkillLevel(2));
		else
			return 0; //mage guild
	}
	//gs->getHero(gs->currentPlayer)
	//const CGHeroInstance * ourHero = gs->curB->heroes[0]->tempOwner == player ? gs->curB->heroes[0] : gs->curB->heroes[1];
	const CGHeroInstance * ourHero = hero;
	return gs->curB->calculateSpellDmg(sp, ourHero, NULL, ourHero->getSpellSchoolLevel(sp), ourHero->getPrimSkillLevel(2));
}

void CGameInfoCallback::getThievesGuildInfo(SThievesGuildInfo & thi, const CGObjectInstance * obj)
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	ERROR_RET_IF(!obj, "No guild object!");
	ERROR_RET_IF(obj->ID == TOWNI_TYPE && !canGetFullInfo(obj), "Cannot get info about town guild object!");
	//TODO: advmap object -> check if they're visited by our hero

	if(obj->ID == TOWNI_TYPE  ||  obj->ID == 95) //it is a town or adv map tavern
	{
		gs->obtainPlayersStats(thi, gs->players[obj->tempOwner].towns.size());
	}
	else if(obj->ID == 97) //Den of Thieves
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
	if(town->ID == TOWNI_TYPE)
		dest.initFromTown(static_cast<const CGTownInstance *>(town), detailed);
	else if(town->ID == 33 || town->ID == 219)
		dest.initFromGarrison(static_cast<const CGGarrison *>(town), detailed);
	else
		return false;
	return true;
}

int3 CGameInfoCallback::guardingCreaturePosition (int3 pos) const
{
	ERROR_RET_VAL_IF(!isVisible(pos), "Tile is not visible!", int3(-1,-1,-1));
	return gs->guardingCreaturePosition(pos);
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

int CGameInfoCallback::getDate(int mode) const
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
bool CGameInfoCallback::verifyPath(CPath * path, bool blockSea) const
{
	for (size_t i=0; i < path->nodes.size(); ++i)
	{
		const TerrainTile *t = getTile(path->nodes[i].coord); //current tile
		ERROR_RET_VAL_IF(!t, "Path contains not visible tile: " << path->nodes[i].coord << "!", false);
		if (t->blocked && !t->visitable)
			return false; //path is wrong - one of the tiles is blocked

		if (blockSea)
		{
			if (i==0)
				continue;

			const TerrainTile *prev = getTile(path->nodes[i-1].coord); //tile of previous node on the path
			if ((   t->tertype == TerrainTile::water  &&  prev->tertype != TerrainTile::water)
				|| (t->tertype != TerrainTile::water  &&  prev->tertype == TerrainTile::water)
				||  prev->tertype == TerrainTile::rock
				)
				return false;
		}


	}
	return true;
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

std::vector < const CGObjectInstance * > CGameInfoCallback::getVisitableObjs( int3 pos ) const
{
	std::vector<const CGObjectInstance *> ret;
	const TerrainTile *t = getTile(pos);
	ERROR_RET_VAL_IF(!t, "Not a valid tile requested!", ret);
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);

	BOOST_FOREACH(const CGObjectInstance * obj, t->visitableObjects)
		ret.push_back(obj);
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
	return int3(gs->map->width, gs->map->height, gs->map->twoLevel+1);
}

std::vector<const CGHeroInstance *> CGameInfoCallback::getAvailableHeroes(const CGObjectInstance * townOrTavern) const
{
	std::vector<const CGHeroInstance *> ret;
	ERROR_RET_VAL_IF(!isOwnedOrVisited(townOrTavern), "Town or tavern must be owned or visited!", ret);
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

int CGameInfoCallback::canBuildStructure( const CGTownInstance *t, int ID )
{
	ERROR_RET_VAL_IF(!canGetFullInfo(t), "Town is not owned!", -1);

	int ret = Buildings::ALLOWED;
	if(t->builded >= MAX_BUILDING_PER_TURN)
		ret = Buildings::CANT_BUILD_TODAY; //building limit

	CBuilding * pom = VLC->buildh->buildings[t->subID][ID];

	if(!pom)
		return Buildings::ERROR;

	//checking resources
	for(int res=0; res<RESOURCE_QUANTITY; res++)
	{
		if(pom->resources[res] > getResource(t->tempOwner, res))
			ret = Buildings::NO_RESOURCES; //lack of res
	}

	//checking for requirements
	std::set<int> reqs = getBuildingRequiments(t, ID);//getting all requirements

	for( std::set<int>::iterator ri  =  reqs.begin(); ri != reqs.end(); ri++ )
	{
		if(t->builtBuildings.find(*ri)==t->builtBuildings.end())
			ret = Buildings::PREREQUIRES; //lack of requirements - cannot build
	}

	//can we build it?
	if(t->forbiddenBuildings.find(ID)!=t->forbiddenBuildings.end())
		ret = Buildings::FORBIDDEN; //forbidden

	if(ID == 13) //capitol
	{
		const PlayerState *ps = getPlayer(t->tempOwner);
		if(ps)
		{
			BOOST_FOREACH(const CGTownInstance *t, ps->towns)
			{
				if(vstd::contains(t->builtBuildings, 13))
				{
					ret = Buildings::HAVE_CAPITAL; //no more than one capitol
					break;
				}
			}
		}
	}
	else if(ID == 6) //shipyard
	{
		const TerrainTile *tile = getTile(t->bestLocation());
		
		if(!tile || tile->tertype != TerrainTile::water )
			ret = Buildings::NO_WATER; //lack of water
	}

	if(t->builtBuildings.find(ID)!=t->builtBuildings.end())	//already built
		ret = Buildings::ALREADY_PRESENT;
	return ret;
}

std::set<int> CGameInfoCallback::getBuildingRequiments( const CGTownInstance *t, int ID )
{
	ERROR_RET_VAL_IF(!canGetFullInfo(t), "Town is not owned!", std::set<int>());

	std::set<int> used;
	used.insert(ID);
	std::set<int> reqs = VLC->townh->requirements[t->subID][ID];

	while(true)
	{
		size_t noloop=0;
		for(std::set<int>::iterator i=reqs.begin();i!=reqs.end();i++)
		{
			if(used.find(*i)==used.end()) //we haven't added requirements for this building
			{
				used.insert(*i);
				for(
					std::set<int>::iterator j=VLC->townh->requirements[t->subID][*i].begin();
					j!=VLC->townh->requirements[t->subID][*i].end();
				j++)
				{
					reqs.insert(*j);//creating full list of requirements
				}
			}
			else
			{
				noloop++;
			}
		}
		if(noloop==reqs.size())
			break;
	}
	return reqs;
}

const CMapHeader * CGameInfoCallback::getMapHeader() const
{
	return gs->map;
}

bool CGameInfoCallback::hasAccess(int playerId) const
{
	return player < 0 || gs->getPlayerRelations( playerId, player );
}

int CGameInfoCallback::getPlayerStatus(int player) const
{
	const PlayerState *ps = gs->getPlayer(player, false);
	if(!ps)
		return -1;
	return ps->status;
}

std::string CGameInfoCallback::getTavernGossip(const CGObjectInstance * townOrTavern) const
{
	return "GOSSIP TEST";
}

int CGameInfoCallback::getPlayerRelations( ui8 color1, ui8 color2 ) const
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
		for(int i=0; i < p->heroes.size(); i++)
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
	return visitor->ID == HEROI_TYPE && canGetFullInfo(visitor); //owned or allied hero is a visitor
}

int CGameInfoCallback::getCurrentPlayer() const
{
	return gs->currentPlayer;
}

CGameInfoCallback::CGameInfoCallback()
{
}

const std::vector< std::vector< std::vector<unsigned char> > > & CPlayerSpecificInfoCallback::getVisibilityMap() const
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
	for ( std::map<ui8, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
	{
		for (size_t j=0; j < (*i).second.towns.size(); ++j)
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

int CPlayerSpecificInfoCallback::getHeroSerial(const CGHeroInstance * hero) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	for (size_t i=0; i<gs->players[player].heroes.size();i++)
	{
		if (gs->players[player].heroes[i]==hero)
			return i;
	}
	return -1;
}

int3 CPlayerSpecificInfoCallback::getGrailPos( float &outKnownRatio )
{
	if (CGObelisk::obeliskCount == 0)
	{
		outKnownRatio = 0.0f;
	}
	else
	{
		outKnownRatio = (float)CGObelisk::visited[gs->getPlayerTeam(player)->id] / CGObelisk::obeliskCount;
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

int CPlayerSpecificInfoCallback::howManyHeroes(bool includeGarrisoned) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	ERROR_RET_VAL_IF(player == -1, "Applicable only for player callbacks", -1);
	return getHeroCount(player,includeGarrisoned);
}

const CGHeroInstance* CPlayerSpecificInfoCallback::getHeroBySerial(int serialId) const
{
	const PlayerState *p = getPlayer(serialId);
	ERROR_RET_VAL_IF(!p, "No player info", NULL);
	ERROR_RET_VAL_IF(serialId < 0 || serialId >= p->heroes.size(), "No player info", NULL);
	return p->heroes[serialId];
}

const CGTownInstance* CPlayerSpecificInfoCallback::getTownBySerial(int serialId) const
{
	const PlayerState *p = getPlayer(serialId);
	ERROR_RET_VAL_IF(!p, "No player info", NULL);
	ERROR_RET_VAL_IF(serialId < 0 || serialId >= p->towns.size(), "No player info", NULL);
	return p->towns[serialId];
}

int CPlayerSpecificInfoCallback::getResourceAmount(int type) const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	ERROR_RET_VAL_IF(player == -1, "Applicable only for player callbacks", -1);
	return getResource(player, type);
}

std::vector<si32> CPlayerSpecificInfoCallback::getResourceAmount() const
{
	//boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	ERROR_RET_VAL_IF(player == -1, "Applicable only for player callbacks", std::vector<si32>());
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

TeamState *CNonConstInfoCallback::getPlayerTeam(ui8 color)
{
	return const_cast<TeamState*>(CGameInfoCallback::getPlayerTeam(color));
}

PlayerState * CNonConstInfoCallback::getPlayer( ui8 color, bool verbose )
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

