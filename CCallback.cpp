#include "stdafx.h"
#include "CCallback.h"
#include "lib/CCreatureHandler.h"
#include "client/CGameInfo.h"
#include "lib/CGameState.h"
#include "lib/BattleState.h"
#include "client/CPlayerInterface.h"
#include "client/Client.h"
#include "lib/map.h"
#include "lib/CBuildingHandler.h"
#include "lib/CDefObjInfoHandler.h"
#include "lib/CGeneralTextHandler.h"
#include "lib/CHeroHandler.h"
#include "lib/CObjectHandler.h"
#include "lib/Connection.h"
#include "lib/NetPacks.h"
#include "client/mapHandler.h"
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "lib/CSpellHandler.h"
#include "lib/CArtHandler.h"
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

/*
 * CCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

template <ui16 N> bool isType(CPack *pack)
{
	return pack->getType() == N;
}

bool CCallback::teleportHero(const CGHeroInstance *who, const CGTownInstance *where)
{
	CastleTeleportHero pack(who->id, where->id, 1);
	sendRequest(&pack);
	return true;
}

bool CCallback::moveHero(const CGHeroInstance *h, int3 dst)
{
	MoveHero pack(dst,h->id);
	sendRequest(&pack);
	return true;
}
void CCallback::selectionMade(int selection, int asker)
{
	QueryReply pack(asker,selection);
	*cl->serv << &pack;
}
void CCallback::recruitCreatures(const CGObjectInstance *obj, ui32 ID, ui32 amount, si32 level/*=-1*/)
{
	if(player!=obj->tempOwner  &&  obj->ID != 106)
		return;

	RecruitCreatures pack(obj->id,ID,amount,level);
	sendRequest(&pack);
}


bool CCallback::dismissCreature(const CArmedInstance *obj, int stackPos)
{
	if(((player>=0)  &&  obj->tempOwner != player) || (obj->stacksCount()<2  && obj->needsLastStack()))
		return false;

	DisbandCreature pack(stackPos,obj->id);
	sendRequest(&pack);
	return true;
}
bool CCallback::upgradeCreature(const CArmedInstance *obj, int stackPos, int newID)
{
	UpgradeCreature pack(stackPos,obj->id,newID);
	sendRequest(&pack);
	return false;
}
void CCallback::endTurn()
{
	tlog5 << "Player " << (unsigned)player << " ended his turn." << std::endl;
	EndTurn pack;
	sendRequest(&pack); //report that we ended turn
}
UpgradeInfo CCallback::getUpgradeInfo(const CArmedInstance *obj, int stackPos) const
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->getUpgradeInfo(obj->getStack(stackPos));
}

const StartInfo * CCallback::getStartInfo() const
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->scenarioOps;
}

int CCallback::getSpellCost(const CSpell * sp, const CGHeroInstance * caster) const
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);

	//if there is a battle
	if(gs->curB)
		return gs->curB->getSpellCost(sp, caster);

	//if there is no battle
	return caster->getSpellCost(sp);
}

int CCallback::estimateSpellDamage(const CSpell * sp, const CGHeroInstance * hero) const
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);

	if(!gs->curB) //no battle
	{
		if (hero) //but we see hero's spellbook
			return gs->curB->calculateSpellDmg(sp, hero, NULL, hero->getSpellSchoolLevel(sp), hero->getPrimSkillLevel(2));
		else
			return 0; //mage guild
	}
	//gs->getHero(gs->currentPlayer)
	const CGHeroInstance * ourHero = gs->curB->heroes[0]->tempOwner == player ? gs->curB->heroes[0] : gs->curB->heroes[1];
	return gs->curB->calculateSpellDmg(sp, ourHero, NULL, ourHero->getSpellSchoolLevel(sp), ourHero->getPrimSkillLevel(2));
}

void CCallback::getThievesGuildInfo(SThievesGuildInfo & thi, const CGObjectInstance * obj)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);

	if(obj == NULL)
		return;

	if(obj->ID == TOWNI_TYPE  ||  obj->ID == 95) //it is a town or adv map tavern
	{
		gs->obtainPlayersStats(thi, gs->players[player].towns.size());
	}
	else if(obj->ID == 97) //Den of Thieves
	{
		gs->obtainPlayersStats(thi, 20);
	}
}

int CCallback::howManyTowns() const
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->players[player].towns.size();
}

const CGTownInstance * CCallback::getTownInfo(int val, bool mode) const //mode = 0 -> val = serial; mode = 1 -> val = ID
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	if (!mode)
	{
		const std::vector<ConstTransitivePtr<CGTownInstance> > &towns = gs->players[gs->currentPlayer].towns;
		if(val < towns.size())
			return towns[val];
		else 
			return NULL;
	}
	else if(mode == 1)
	{
		const CGObjectInstance *obj = getObjectInfo(val);
		if(!obj)
			return NULL;
		if(obj->ID != TOWNI_TYPE)
			return NULL;
		else
			return static_cast<const CGTownInstance *>(obj);
	}
	return NULL;
}

bool CCallback::getTownInfo( const CGObjectInstance *town, InfoAboutTown &dest ) const
{
	if(!isVisible(town, player)) //it's not a town or it's not visible for layer
		return false;

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

int3 CCallback::guardingCreaturePosition (int3 pos) const
{
	return gs->guardingCreaturePosition(pos);
}

int CCallback::howManyHeroes(bool includeGarrisoned) const
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return cl->getHeroCount(player,includeGarrisoned);
}
const CGHeroInstance * CCallback::getHeroInfo(int val, int mode) const //mode = 0 -> val = serial; mode = 1 -> val = ID
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx); //TODO use me?
	//if (gs->currentPlayer!=player) //TODO: checking if we are allowed to give that info
	//	return NULL;
	if (!mode) //esrial id
	{
		if(val<gs->players[player].heroes.size())
		{
			return gs->players[player].heroes[val];
		}
		else
		{
			return NULL;
		}
	}
	else if(mode==1) //it's hero type id
	{
		for (size_t i=0; i < gs->players[player].heroes.size(); ++i)
		{
			if (gs->players[player].heroes[i]->type->ID==val)
			{
				return gs->players[player].heroes[i];
			}
		}
	}
	else //object id
	{
		return static_cast<const CGHeroInstance*>(gs->map->objects[val].get());
	}
	return NULL;
}

const CGObjectInstance * CCallback::getObjectInfo(int ID) const
{
	//TODO: check for visibility
	return gs->map->objects[ID];
}

bool CCallback::getHeroInfo( const CGObjectInstance *hero, InfoAboutHero &dest ) const
{
	const CGHeroInstance *h = dynamic_cast<const CGHeroInstance *>(hero);
	if(!h || !isVisible(h->getPosition(false))) //it's not a hero or it's not visible for layer
		return false;
	
	//TODO vision support
	dest.initFromHero(h, hasAccess(h->tempOwner));
	return true;
}

int CCallback::getResourceAmount(int type) const
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->players[player].resources[type];
}
std::vector<si32> CCallback::getResourceAmount() const
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->players[player].resources;
}
int CCallback::getDate(int mode) const
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->getDate(mode);
}
std::vector < std::string > CCallback::getObjDescriptions(int3 pos) const
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	std::vector<std::string> ret;
	if(!isVisible(pos,player))
		return ret;
	BOOST_FOREACH(const CGObjectInstance * obj, gs->map->terrain[pos.x][pos.y][pos.z].blockingObjects)
		ret.push_back(obj->getHoverText());
	return ret;
}
bool CCallback::verifyPath(CPath * path, bool blockSea) const
{
	for (size_t i=0; i < path->nodes.size(); ++i)
	{
		if ( CGI->mh->map->terrain[path->nodes[i].coord.x][path->nodes[i].coord.y][path->nodes[i].coord.z].blocked 
			&& (! (CGI->mh->map->terrain[path->nodes[i].coord.x][path->nodes[i].coord.y][path->nodes[i].coord.z].visitable)))
			return false; //path is wrong - one of the tiles is blocked

		if (blockSea)
		{
			if (i==0)
				continue;

			if (
					((CGI->mh->map->terrain[path->nodes[i].coord.x][path->nodes[i].coord.y][path->nodes[i].coord.z].tertype==TerrainTile::water)
					&&
					(CGI->mh->map->terrain[path->nodes[i-1].coord.x][path->nodes[i-1].coord.y][path->nodes[i-1].coord.z].tertype!=TerrainTile::water))
				  ||
					((CGI->mh->map->terrain[path->nodes[i].coord.x][path->nodes[i].coord.y][path->nodes[i].coord.z].tertype!=TerrainTile::water)
					&&
					(CGI->mh->map->terrain[path->nodes[i-1].coord.x][path->nodes[i-1].coord.y][path->nodes[i-1].coord.z].tertype==TerrainTile::water))
				  ||
				  (CGI->mh->map->terrain[path->nodes[i-1].coord.x][path->nodes[i-1].coord.y][path->nodes[i-1].coord.z].tertype==TerrainTile::rock)
					
				)
				return false;
		}


	}
	return true;
}

std::vector< std::vector< std::vector<unsigned char> > > & CCallback::getVisibilityMap() const
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->getPlayerTeam(player)->fogOfWarMap;
}


bool CCallback::isVisible(int3 pos, int Player) const
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->map->isInTheMap(pos) && gs->isVisible(pos, Player);
}

std::vector < const CGTownInstance *> CCallback::getTownsInfo(bool onlyOur) const
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
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
std::vector < const CGHeroInstance *> CCallback::getHeroesInfo(bool onlyOur) const
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
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

bool CCallback::isVisible(int3 pos) const
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return isVisible(pos,player);
}

bool CCallback::isVisible( const CGObjectInstance *obj, int Player ) const
{
	return gs->isVisible(obj, Player);
}

int CCallback::getMyColor() const
{
	return player;
}

int CCallback::getHeroSerial(const CGHeroInstance * hero) const
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	for (size_t i=0; i<gs->players[player].heroes.size();i++)
	{
		if (gs->players[player].heroes[i]==hero)
			return i;
	}
	return -1;
}

const CCreatureSet* CCallback::getGarrison(const CGObjectInstance *obj) const
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	const CArmedInstance *armi = dynamic_cast<const CArmedInstance*>(obj);
	if(!armi)
		return NULL;
	else 
		return armi;
}

int CCallback::swapCreatures(const CArmedInstance *s1, const CArmedInstance *s2, int p1, int p2)
{
	ArrangeStacks pack(1,p1,p2,s1->id,s2->id,0);
	sendRequest(&pack);
	return 0;
}

int CCallback::mergeStacks(const CArmedInstance *s1, const CArmedInstance *s2, int p1, int p2)
{
	ArrangeStacks pack(2,p1,p2,s1->id,s2->id,0);
	sendRequest(&pack);
	return 0;
}
int CCallback::splitStack(const CArmedInstance *s1, const CArmedInstance *s2, int p1, int p2, int val)
{
	ArrangeStacks pack(3,p1,p2,s1->id,s2->id,val);
	sendRequest(&pack);
	return 0;
}

bool CCallback::dismissHero(const CGHeroInstance *hero)
{
	if(player!=hero->tempOwner) return false;

	DismissHero pack(hero->id);
	sendRequest(&pack);
	return true;
}

// int CCallback::getMySerial() const
// {	
// 	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
// 	return gs->players[player].serial;
// }

bool CCallback::swapArtifacts(const CGHeroInstance * hero1, ui16 pos1, const CGHeroInstance * hero2, ui16 pos2)
{
	if(player!=hero1->tempOwner && player!=hero2->tempOwner)
		return false;

	ExchangeArtifacts ea(hero1->id, hero2->id, pos1, pos2);
	sendRequest(&ea);
	return true;
}

/**
 * Assembles or disassembles a combination artifact.
 * @param hero Hero holding the artifact(s).
 * @param artifactSlot The worn slot ID of the combination- or constituent artifact.
 * @param assemble True for assembly operation, false for disassembly.
 * @param assembleTo If assemble is true, this represents the artifact ID of the combination
 * artifact to assemble to. Otherwise it's not used.
 */
bool CCallback::assembleArtifacts (const CGHeroInstance * hero, ui16 artifactSlot, bool assemble, ui32 assembleTo)
{
	if (player != hero->tempOwner)
		return false;

	AssembleArtifacts aa(hero->id, artifactSlot, assemble, assembleTo);
	sendRequest(&aa);
	return true;
}

bool CCallback::buildBuilding(const CGTownInstance *town, si32 buildingID)
{
	CGTownInstance * t = const_cast<CGTownInstance *>(town);

	if(town->tempOwner!=player)
		return false;
	const CBuilding *b = CGI->buildh->buildings[t->subID][buildingID];
	for(int i=0;i<b->resources.size();i++)
		if(b->resources[i] > gs->players[player].resources[i])
			return false; //lack of resources

	BuildStructure pack(town->id,buildingID);
	sendRequest(&pack);
	return true;
}

int CBattleCallback::battleGetBattlefieldType()
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	//return gs->battleGetBattlefieldType();

	if(!gs->curB)
	{
		tlog2<<"battleGetBattlefieldType called when there is no battle!"<<std::endl;
		return -1;
	}
	return gs->curB->battlefieldType;
}

int CBattleCallback::battleGetObstaclesAtTile(THex tile) //returns bitfield 
{
	//TODO - write
	return -1;
}

std::vector<CObstacleInstance> CBattleCallback::battleGetAllObstacles()
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	if(gs->curB)
		return gs->curB->obstacles;
	else
		return std::vector<CObstacleInstance>();
}

const CStack* CBattleCallback::battleGetStackByID(int ID, bool onlyAlive)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	if(!gs->curB) return NULL;
	return gs->curB->getStack(ID, onlyAlive);
}

int CBattleCallback::battleMakeAction(BattleAction* action)
{
	assert(action->actionType == BattleAction::HERO_SPELL);
	MakeCustomAction mca(*action);
	sendRequest(&mca);
	return 0;
}

const CStack* CBattleCallback::battleGetStackByPos(THex pos, bool onlyAlive)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->curB->battleGetStack(pos, onlyAlive);
}

THex CBattleCallback::battleGetPos(int stack)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
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

TStacks CBattleCallback::battleGetStacks(EStackOwnership whose /*= MINE_AND_ENEMY*/, bool onlyAlive /*= true*/)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
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

void CBattleCallback::getStackQueue( std::vector<const CStack *> &out, int howMany )
{
	if(!gs->curB)
	{
		tlog2 << "battleGetStackQueue called when there is not battle!" << std::endl;
		return;
	}
	gs->curB->getStackQueue(out, howMany);
}

std::vector<THex> CBattleCallback::battleGetAvailableHexes(const CStack * stack, bool addOccupiable, std::vector<THex> * attackable)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	if(!gs->curB)
	{
		tlog2<<"battleGetAvailableHexes called when there is no battle!"<<std::endl;
		return std::vector<THex>();
	}
	return gs->curB->getAccessibility(stack, addOccupiable, attackable);
	//return gs->battleGetRange(ID);
}

bool CBattleCallback::battleCanShoot(const CStack * stack, THex dest)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);

	if(!gs->curB) return false;

	return gs->curB->battleCanShoot(stack, dest);
}

bool CBattleCallback::battleCanCastSpell()
{
	if(!gs->curB) //there is no battle
		return false;

	return gs->curB->battleCanCastSpell(player, SpellCasting::HERO_CASTING) == SpellCasting::OK;
}

bool CBattleCallback::battleCanFlee()
{
	return gs->curB->battleCanFlee(player);
}

const CGTownInstance *CBattleCallback::battleGetDefendedTown()
{
	if(!gs->curB || gs->curB->town == NULL)
		return NULL;

	return gs->curB->town;
}

ui8 CBattleCallback::battleGetWallState(int partOfWall)
{
	if(!gs->curB || gs->curB->siege == 0)
	{
		return 0;
	}
	return gs->curB->si.wallState[partOfWall];
}

int CBattleCallback::battleGetWallUnderHex(THex hex)
{
	if(!gs->curB || gs->curB->siege == 0)
	{
		return -1;
	}
	return gs->curB->hexToWallPart(hex);
}

TDmgRange CBattleCallback::battleEstimateDamage(const CStack * attacker, const CStack * defender, TDmgRange * retaliationDmg)
{
	if(!gs->curB)
		return std::make_pair(0, 0);

	const CGHeroInstance * attackerHero, * defenderHero;
	bool shooting = battleCanShoot(attacker, defender->position);

	if(gs->curB->sides[0] == player)
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

ui8 CBattleCallback::battleGetSiegeLevel()
{
	if(!gs->curB)
		return 0;

	return gs->curB->siege;
}

const CGHeroInstance * CBattleCallback::battleGetFightingHero(ui8 side) const
{
	if(!gs->curB)
		return 0;

	return gs->curB->heroes[side];
}

template <typename T>
void CBattleCallback::sendRequest(const T* request)
{
	//TODO? should be part of CClient but it would have to be very tricky cause template/serialization issues
	if(waitTillRealize)
		cl->waitingRequest.set(true);

	*cl->serv << request;

	if(waitTillRealize)
		cl->waitingRequest.waitWhileTrue();
}

void CCallback::swapGarrisonHero( const CGTownInstance *town )
{
	if(town->tempOwner != player) return;

	GarrisonHeroSwap pack(town->id);
	sendRequest(&pack);
}

void CCallback::buyArtifact(const CGHeroInstance *hero, int aid)
{
	if(hero->tempOwner != player) return;

	BuyArtifact pack(hero->id,aid);
	sendRequest(&pack);
}

std::vector < const CGObjectInstance * > CCallback::getBlockingObjs( int3 pos ) const
{
	std::vector<const CGObjectInstance *> ret;
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	if(!gs->map->isInTheMap(pos) || !isVisible(pos))
		return ret;
	BOOST_FOREACH(const CGObjectInstance * obj, gs->map->terrain[pos.x][pos.y][pos.z].blockingObjects)
		ret.push_back(obj);
	return ret;
}

std::vector < const CGObjectInstance * > CCallback::getVisitableObjs( int3 pos ) const
{
	std::vector<const CGObjectInstance *> ret;
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	if(!gs->map->isInTheMap(pos) || !isVisible(pos))
		return ret;
	BOOST_FOREACH(const CGObjectInstance * obj, gs->map->terrain[pos.x][pos.y][pos.z].visitableObjects)
		ret.push_back(obj);
	return ret;
}

std::vector < const CGObjectInstance * > CCallback::getFlaggableObjects(int3 pos) const
{
	if(!isVisible(pos))
		return std::vector < const CGObjectInstance * >();

	std::vector < const CGObjectInstance * > ret;

	const std::vector < std::pair<const CGObjectInstance*,SDL_Rect> > & objs = CGI->mh->ttiles[pos.x][pos.y][pos.z].objects;
	for(size_t b=0; b<objs.size(); ++b)
	{
		if(objs[b].first->tempOwner!=254 && !((objs[b].first->defInfo->blockMap[pos.y - objs[b].first->pos.y + 5] >> (objs[b].first->pos.x - pos.x)) & 1))
			ret.push_back(CGI->mh->ttiles[pos.x][pos.y][pos.z].objects[b].first);
	}
	return ret;
}

int3 CCallback::getMapSize() const
{
	return CGI->mh->sizes;
}

void CCallback::trade(const CGObjectInstance *market, int mode, int id1, int id2, int val1, const CGHeroInstance *hero/* = NULL*/)
{
	TradeOnMarketplace pack;
	pack.market = market;
	pack.hero = hero;
	pack.mode = mode;
	pack.r1 = id1;
	pack.r2 = id2;
	pack.val = val1;
	sendRequest(&pack);
}

void CCallback::setFormation(const CGHeroInstance * hero, bool tight)
{
	const_cast<CGHeroInstance*>(hero)-> formation = tight;
	SetFormation pack(hero->id,tight);
	sendRequest(&pack);
}

void CCallback::setSelection(const CArmedInstance * obj)
{
	SetSelection ss;
	ss.player = player;
	ss.id = obj->id;
	sendRequest(&ss);

	if(obj->ID == HEROI_TYPE)
	{
		cl->gs->calculatePaths(static_cast<const CGHeroInstance *>(obj), *cl->pathInfo);
		//nasty workaround. TODO: nice workaround
		cl->gs->getPlayer(player)->currentSelection = obj->id;
	}
}

void CCallback::recruitHero(const CGObjectInstance *townOrTavern, const CGHeroInstance *hero)
{
	ui8 i=0;
	for(; i<gs->players[player].availableHeroes.size(); i++)
	{
		if(gs->players[player].availableHeroes[i] == hero)
		{
			HireHero pack(i,townOrTavern->id);
			pack.player = player;
			sendRequest(&pack);
			return;
		}
	}
}

std::vector<const CGHeroInstance *> CCallback::getAvailableHeroes(const CGObjectInstance * townOrTavern) const
{
	std::vector<const CGHeroInstance *> ret(gs->players[player].availableHeroes.size());
	std::copy(gs->players[player].availableHeroes.begin(),gs->players[player].availableHeroes.end(),ret.begin());
	return ret;
}	

const TerrainTile * CCallback::getTileInfo( int3 tile ) const
{
	if(!gs->map->isInTheMap(tile)) 
	{
		tlog1 << tile << "is outside the map! (call to getTileInfo)\n";
		return NULL;
	}
	if(!isVisible(tile, player)) return NULL;
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return &gs->map->getTile(tile);
}

int CCallback::canBuildStructure( const CGTownInstance *t, int ID )
{
	return gs->canBuildStructure(t,ID);
}

std::set<int> CCallback::getBuildingRequiments( const CGTownInstance *t, int ID )
{
	return gs->getBuildingRequiments(t,ID);
}

bool CCallback::getPath(int3 src, int3 dest, const CGHeroInstance * hero, CPath &ret)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->getPath(src,dest,hero, ret);
}

void CCallback::save( const std::string &fname )
{
	cl->save(fname);
}


void CCallback::sendMessage(const std::string &mess)
{
	PlayerMessage pm(player, mess);
	sendRequest(&pm);
}

void CCallback::buildBoat( const IShipyard *obj )
{
	BuildBoat bb;
	bb.objid = obj->o->id;
	sendRequest(&bb);
}

CCallback::CCallback( CGameState * GS, int Player, CClient *C ) 
	:CBattleCallback(GS, Player, C)
{
	waitTillRealize = false;
}

const CMapHeader * CCallback::getMapHeader() const
{
	return gs->map;
}

const CGPathNode * CCallback::getPathInfo( int3 tile )
{
	return &cl->pathInfo->nodes[tile.x][tile.y][tile.z];
}

bool CCallback::getPath2( int3 dest, CGPath &ret )
{
	if (!gs->map->isInTheMap(dest))
		return false;

	const CGHeroInstance *h = cl->IGameCallback::getSelectedHero(player);
	assert(cl->pathInfo->hero == h);
	if(cl->pathInfo->hpos != h->getPosition(false)) //hero position changed, must update paths
	{ 
		recalculatePaths();
	}
	return cl->pathInfo->getPath(dest, ret);
}

void CCallback::recalculatePaths()
{
	gs->calculatePaths(cl->IGameCallback::getSelectedHero(player), *cl->pathInfo);
}

void CCallback::calculatePaths( const CGHeroInstance *hero, CPathsInfo &out, int3 src /*= int3(-1,-1,-1)*/, int movement /*= -1*/ )
{
	gs->calculatePaths(hero, out, src, movement);
}

int3 CCallback::getGrailPos( float &outKnownRatio )
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

void CCallback::dig( const CGObjectInstance *hero )
{
	DigWithHero dwh;
	dwh.id = hero->id;
	sendRequest(&dwh);
}

void CCallback::castSpell(const CGHeroInstance *hero, int spellID, const int3 &pos)
{
	CastAdvSpell cas;
	cas.hid = hero->id;
	cas.sid = spellID;
	cas.pos = pos;
	sendRequest(&cas);
}

bool CCallback::hasAccess(int playerId) const
{
	return gs->getPlayerRelations( playerId, player ) ||  player < 0;
}

si8 CBattleCallback::battleHasDistancePenalty( const CStack * stack, THex destHex )
{
	return gs->curB->hasDistancePenalty(stack, destHex);
}

si8 CBattleCallback::battleHasWallPenalty( const CStack * stack, THex destHex )
{
	return gs->curB->hasWallPenalty(stack, destHex);
}

si8 CBattleCallback::battleCanTeleportTo(const CStack * stack, THex destHex, int telportLevel)
{
	return gs->curB->canTeleportTo(stack, destHex, telportLevel);
}

int CCallback::getPlayerStatus(int player) const
{
	const PlayerState *ps = gs->getPlayer(player, false);
	if(!ps)
		return -1;
	return ps->status;
}

std::string CCallback::getTavernGossip(const CGObjectInstance * townOrTavern) const
{
	return "GOSSIP TEST";
}

std::vector < const CGObjectInstance * > CCallback::getMyObjects() const
{
	std::vector < const CGObjectInstance * > ret;
	for (int g=0; g<gs->map->objects.size(); ++g)
	{
		if (gs->map->objects[g] && gs->map->objects[g]->tempOwner == LOCPLINT->playerID)
		{
			ret.push_back(gs->map->objects[g]);
		}
	}
	return ret;
}

std::vector < const CGDwelling * > CCallback::getMyDwellings() const
{
	std::vector < const CGDwelling * > ret;
	BOOST_FOREACH(CGDwelling * dw, gs->getPlayer(player)->dwellings)
	{
		ret.push_back(dw);
	}
	return ret;
}

int CCallback::getPlayerRelations( ui8 color1, ui8 color2 ) const
{
	return gs->getPlayerRelations(color1, color2);
}

InfoAboutTown::InfoAboutTown()
{
	tType = NULL;
	details = NULL;
	fortLevel = 0;
	owner = -1;
}

InfoAboutTown::~InfoAboutTown()
{
	delete details;
}

void InfoAboutTown::initFromTown( const CGTownInstance *t, bool detailed )
{
	obj = t;
	army = ArmyDescriptor(t->getUpperArmy(), detailed);
	built = t->builded;
	fortLevel = t->fortLevel();
	name = t->name;
	tType = t->town;
	owner = t->tempOwner;

	if(detailed) 
	{
		//include details about hero
		details = new Details;
		details->goldIncome = t->dailyIncome();
		details->customRes = vstd::contains(t->builtBuildings, 15);
		details->hallLevel = t->hallLevel();
		details->garrisonedHero = t->garrisonHero;
	}
	//TODO: adjust undetailed info about army to our count of thieves guilds
}

void InfoAboutTown::initFromGarrison(const CGGarrison *garr, bool detailed)
{
	obj = garr;
	fortLevel = 0;
	army = ArmyDescriptor(garr, detailed);
	name = CGI->generaltexth->names[33]; // "Garrison"
	owner = garr->tempOwner;
	built = false;
	tType = NULL;

	// Show detailed info only to owning player.
	if(detailed)
	{
		details = new InfoAboutTown::Details;
		details->customRes = false;
		details->garrisonedHero = false;
		details->goldIncome = -1;
		details->hallLevel = -1;
	}
}

bool CBattleCallback::hasAccess( int playerId ) const
{
	return playerId == player || player < 0;
}

CBattleCallback::CBattleCallback(CGameState *GS, int Player, CClient *C )
{
	gs = GS;
	player = Player;
	cl = C;
}

std::vector<int> CBattleCallback::battleGetDistances(const CStack * stack, THex hex /*= THex::INVALID*/, THex * predecessors /*= NULL*/)
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

SpellCasting::ESpellCastProblem CBattleCallback::battleCanCastThisSpell( const CSpell * spell )
{
	if(!gs->curB)
	{

		tlog1 << "battleCanCastThisSpell called when there is no battle!\n";
		return SpellCasting::NO_HERO_TO_CAST_SPELL;
	}

	return gs->curB->battleCanCastThisSpell(player, spell, SpellCasting::HERO_CASTING);
}

si8 CBattleCallback::battleGetTacticDist()
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

ui8 CBattleCallback::battleGetMySide()
{
	if (!gs->curB)
	{
		tlog1 << "battleGetMySide called when no battle!\n";
		return 0;
	}

	return gs->curB->sides[1] == player;
}

bool CBattleCallback::battleMakeTacticAction( BattleAction * action )
{
	MakeAction ma;
	ma.ba = *action;
	sendRequest(&ma);
	return true;
}

int CBattleCallback::battleGetSurrenderCost()
{
	if (!gs->curB)
	{
		tlog1 << "battleGetSurrenderCost called when no battle!\n";
		return -1;
	}

	return gs->curB->getSurrenderingCost(player);
}