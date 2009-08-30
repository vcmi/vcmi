#include "stdafx.h"
#include "CCallback.h"
#include "client/CGameInfo.h"
#include "lib/CGameState.h"
#include "client/CPlayerInterface.h"
#include "client/Client.h"
#include "lib/map.h"
#include "hch/CBuildingHandler.h"
#include "hch/CDefObjInfoHandler.h"
#include "hch/CHeroHandler.h"
#include "hch/CObjectHandler.h"
#include "lib/Connection.h"
#include "lib/NetPacks.h"
#include "mapHandler.h"
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "hch/CSpellHandler.h"
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

static int gcd(int x, int y)
{
	int temp;
	if (y > x)
		std::swap(x,y);
	while (y != 0)
	{
		temp = y;
		y = x-y;
		x = temp;
		if (y > x)
			std::swap(x,y);
	}
	return x;
}
HeroMoveDetails::HeroMoveDetails(int3 Src, int3 Dst, CGHeroInstance*Ho)
	:src(Src),dst(Dst),ho(Ho)
{
	owner = ho->getOwner();
};

template <ui16 N> bool isType(CPack *pack)
{
	return pack->getType() == N;
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
void CCallback::recruitCreatures(const CGObjectInstance *obj, ui32 ID, ui32 amount)
{
	if(player!=obj->tempOwner) return;

	RecruitCreatures pack(obj->id,ID,amount);
	sendRequest(&pack);
}


bool CCallback::dismissCreature(const CArmedInstance *obj, int stackPos)
{
	if(((player>=0)  &&  obj->tempOwner != player) || (obj->army.slots.size()<2  && obj->needsLastStack()))
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
	tlog5 << "Player " << (unsigned)player << " end his turn." << std::endl;
	EndTurn pack;
	sendRequest(&pack); //report that we ended turn
}
UpgradeInfo CCallback::getUpgradeInfo(const CArmedInstance *obj, int stackPos) const
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->getUpgradeInfo(const_cast<CArmedInstance*>(obj),stackPos);
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
	return sp->costs[caster->getSpellSchoolLevel(sp)];
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
		const std::vector<CGTownInstance *> &towns = gs->players[gs->currentPlayer].towns;
		if(val < towns.size())
			return towns[val];
		else 
			return NULL;
	}
	else 
	{
		//TODO: add some smart ID to the CTownInstance


		//for (int i=0; i<gs->players[gs->currentPlayer].towns.size();i++)
		//{
		//	if (gs->players[gs->currentPlayer].towns[i]->someID==val)
		//		return gs->players[gs->currentPlayer].towns[i];
		//}
		return NULL;
	}
	return NULL;
}

bool CCallback::getTownInfo( const CGObjectInstance *town, InfoAboutTown &dest ) const
{
	const CGTownInstance *t = dynamic_cast<const CGTownInstance *>(town);
	if(!t || !isVisible(t, player)) //it's not a town or it's not visible for layer
		return false;

	//TODO vision support, info about allies
	dest.initFromTown(t, false);
	return true;
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
		return static_cast<const CGHeroInstance*>(gs->map->objects[val]);
	}
	return NULL;
}

const CGObjectInstance * CCallback::getObjectInfo(int ID) const
{
	return gs->map->objects[ID];
}

bool CCallback::getHeroInfo( const CGObjectInstance *hero, InfoAboutHero &dest ) const
{
	const CGHeroInstance *h = dynamic_cast<const CGHeroInstance *>(hero);
	if(!h || !isVisible(h->getPosition(false))) //it's not a hero or it's not visible for layer
		return false;
	
	//TODO vision support, info about allies
	dest.initFromHero(h, false);
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
		if ( CGI->mh->ttiles[path->nodes[i].coord.x][path->nodes[i].coord.y][path->nodes[i].coord.z].tileInfo->blocked 
			&& (! (CGI->mh->ttiles[path->nodes[i].coord.x][path->nodes[i].coord.y][path->nodes[i].coord.z].tileInfo->visitable)))
			return false; //path is wrong - one of the tiles is blocked

		if (blockSea)
		{
			if (i==0)
				continue;

			if (
					((CGI->mh->ttiles[path->nodes[i].coord.x][path->nodes[i].coord.y][path->nodes[i].coord.z].tileInfo->tertype==TerrainTile::water)
					&&
					(CGI->mh->ttiles[path->nodes[i-1].coord.x][path->nodes[i-1].coord.y][path->nodes[i-1].coord.z].tileInfo->tertype!=TerrainTile::water))
				  ||
					((CGI->mh->ttiles[path->nodes[i].coord.x][path->nodes[i].coord.y][path->nodes[i].coord.z].tileInfo->tertype!=TerrainTile::water)
					&&
					(CGI->mh->ttiles[path->nodes[i-1].coord.x][path->nodes[i-1].coord.y][path->nodes[i-1].coord.z].tileInfo->tertype==TerrainTile::water))
				  ||
				  (CGI->mh->ttiles[path->nodes[i-1].coord.x][path->nodes[i-1].coord.y][path->nodes[i-1].coord.z].tileInfo->tertype==TerrainTile::rock)
					
				)
				return false;
		}


	}
	return true;
}

std::vector< std::vector< std::vector<unsigned char> > > & CCallback::getVisibilityMap() const
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->players[player].fogOfWarMap;
}


bool CCallback::isVisible(int3 pos, int Player) const
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->isVisible(pos, Player);
}

std::vector < const CGTownInstance *> CCallback::getTownsInfo(bool onlyOur) const
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	std::vector < const CGTownInstance *> ret = std::vector < const CGTownInstance *>();
	for ( std::map<ui8, PlayerState>::iterator i=gs->players.begin() ; i!=gs->players.end();i++)
	{
		for (size_t j=0; j < (*i).second.towns.size(); ++j)
		{
			if ( ( isVisible((*i).second.towns[j],player) ) || (*i).first==player)
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
		return &armi->army;
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

int CCallback::getMySerial() const
{	
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->players[player].serial;
}

bool CCallback::swapArtifacts(const CGHeroInstance * hero1, ui16 pos1, const CGHeroInstance * hero2, ui16 pos2)
{
	if(player!=hero1->tempOwner || player!=hero2->tempOwner)
		return false;

	ExchangeArtifacts ea(hero1->id, hero2->id, pos1, pos2);
	sendRequest(&ea);
	return true;
}

bool CCallback::buildBuilding(const CGTownInstance *town, si32 buildingID)
{
	CGTownInstance * t = const_cast<CGTownInstance *>(town);

	if(town->tempOwner!=player)
		return false;
	CBuilding *b = CGI->buildh->buildings[t->subID][buildingID];
	for(int i=0;i<b->resources.size();i++)
		if(b->resources[i] > gs->players[player].resources[i])
			return false; //lack of resources

	BuildStructure pack(town->id,buildingID);
	sendRequest(&pack);
	return true;
}

int CCallback::battleGetBattlefieldType()
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->battleGetBattlefieldType();
}

int CCallback::battleGetObstaclesAtTile(int tile) //returns bitfield 
{
	//TODO - write
	return -1;
}

std::vector<CObstacleInstance> CCallback::battleGetAllObstacles()
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	if(gs->curB)
		return gs->curB->obstacles;
	else
		return std::vector<CObstacleInstance>();
}

int CCallback::battleGetStack(int pos, bool onlyAlive)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->battleGetStack(pos, onlyAlive);
}

const CStack* CCallback::battleGetStackByID(int ID, bool onlyAlive)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	if(!gs->curB) return NULL;
	return gs->curB->getStack(ID, onlyAlive);
}

int CCallback::battleMakeAction(BattleAction* action)
{
	MakeCustomAction mca(*action);
	sendRequest(&mca);
	return 0;
}

const CStack* CCallback::battleGetStackByPos(int pos, bool onlyAlive)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return battleGetStackByID(battleGetStack(pos, onlyAlive), onlyAlive);
}

int CCallback::battleGetPos(int stack)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	if(!gs->curB)
	{
		tlog2<<"battleGetPos called when there is no battle!"<<std::endl;
		return -1;
	}
	for(size_t g=0; g<gs->curB->stacks.size(); ++g)
	{
		if(gs->curB->stacks[g]->ID == stack)
			return gs->curB->stacks[g]->position;
	}
	return -1;
}

std::map<int, CStack> CCallback::battleGetStacks()
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	std::map<int, CStack> ret;
	if(!gs->curB) //there is no battle
	{
		return ret;
	}

	for(size_t g=0; g<gs->curB->stacks.size(); ++g)
	{
		ret[gs->curB->stacks[g]->ID] = *(gs->curB->stacks[g]);
	}
	return ret;
}

std::vector<CStack> CCallback::battleGetStackQueue()
{
	if(!gs->curB)
	{
		tlog2<<"battleGetStackQueue called when there is not battle!"<<std::endl;
		return std::vector<CStack>();
	}
	return gs->curB->getStackQueue();
}

CCreature CCallback::battleGetCreature(int number)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx); //TODO use me?
	if(!gs->curB)
	{
		tlog2<<"battleGetCreature called when there is no battle!"<<std::endl;
	}
	for(size_t h=0; h<gs->curB->stacks.size(); ++h)
	{
		if(gs->curB->stacks[h]->ID == number) //creature found
			return *(gs->curB->stacks[h]->creature);
	}
#ifndef __GNUC__
	throw new std::exception("Cannot find the creature");
#else
	throw new std::exception();
#endif
}

std::vector<int> CCallback::battleGetAvailableHexes(int ID, bool addOccupiable)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	if(!gs->curB)
	{
		tlog2<<"battleGetAvailableHexes called when there is no battle!"<<std::endl;
		return std::vector<int>();
	}
	return gs->curB->getAccessibility(ID, addOccupiable);
	//return gs->battleGetRange(ID);
}

bool CCallback::battleIsStackMine(int ID)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	if(!gs->curB)
	{
		tlog2<<"battleIsStackMine called when there is no battle!"<<std::endl;
		return false;
	}
	for(size_t h=0; h<gs->curB->stacks.size(); ++h)
	{
		if(gs->curB->stacks[h]->ID == ID) //creature found
			return gs->curB->stacks[h]->owner == player;
	}
	return false;
}	
bool CCallback::battleCanShoot(int ID, int dest)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	const CStack *our = battleGetStackByID(ID), *dst = battleGetStackByPos(dest);

	if(!our || !dst || !gs->curB) return false;
	
	int ourHero = our->attackerOwned ? gs->curB->hero1 : gs->curB->hero2;

	//for(size_t g=0; g<our->effects.size(); ++g)
	//{
	//	if(61 == our->effects[g].id) //forgetfulness
	//		return false;
	//}
	if(our->hasFeatureOfType(StackFeature::FORGETFULL)) //forgetfulness
		return false;


	if(our->hasFeatureOfType(StackFeature::SHOOTER)//it's shooter
		&& our->owner != dst->owner
		&& dst->alive()
		&& (!gs->curB->isStackBlocked(ID) || 
			( gs->getHero(ourHero) && gs->getHero(ourHero)->hasBonusOfType(HeroBonus::FREE_SHOOTING) ) )
		&& our->shots
		)
		return true;
	return false;
}

bool CCallback::battleCanCastSpell()
{
	if(!gs->curB) //there is no battle
		return false;

	if(gs->curB->side1 == player)
		return gs->curB->castSpells[0] == 0 && gs->getHero(gs->curB->hero1)->getArt(17);
	else
		return gs->curB->castSpells[1] == 0 && gs->getHero(gs->curB->hero2)->getArt(17);
}

bool CCallback::battleCanFlee()
{
	return gs->battleCanFlee(player);
}

const CGTownInstance *CCallback::battleGetDefendedTown()
{
	if(!gs->curB || gs->curB->tid == -1)
		return NULL;

	return static_cast<const CGTownInstance *>(gs->map->objects[gs->curB->tid]);
}

ui8 CCallback::battleGetWallState(int partOfWall)
{
	if(!gs->curB || gs->curB->siege == 0)
	{
		return 0;
	}
	return gs->curB->si.wallState[partOfWall];
}

std::pair<ui32, ui32> CCallback::battleEstimateDamage(int attackerID, int defenderID)
{
	if(!gs->curB)
		return std::make_pair(0, 0);

	const CGHeroInstance * attackerHero, * defenderHero;

	if(gs->curB->side1 == player)
	{
		attackerHero = gs->getHero(gs->curB->hero1);
		defenderHero = gs->getHero(gs->curB->hero2);
	}
	else
	{
		attackerHero = gs->getHero(gs->curB->hero2);
		defenderHero = gs->getHero(gs->curB->hero1);
	}

	const CStack * attacker = gs->curB->stacks[attackerID], * defender = gs->curB->stacks[defenderID];

	return BattleInfo::calculateDmgRange(attacker, defender, attackerHero, defenderHero, battleCanShoot(attacker->ID, defender->position), 0);
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

void CCallback::getMarketOffer( int t1, int t2, int &give, int &rec, int mode/*=0*/ ) const
{
	if(mode) return; //TODO - support
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	//if(gs->resVals[t1] >= gs->resVals[t2])
	float r = gs->resVals[t1], //price of given resource
		g = gs->resVals[t2] / gs->getMarketEfficiency(player,mode); //price of wanted resource
	if(r>g) //if given resource is more expensive than wanted
	{
		rec = ceil(r / g);
		give = 1;
	}
	else //if wanted resource is more expensive
	{
		give = ceil(g / r);
		rec = 1;
	}
}

std::vector < const CGObjectInstance * > CCallback::getFlaggableObjects(int3 pos) const
{
	if(!isVisible(pos))
		return std::vector < const CGObjectInstance * >();

	std::vector < const CGObjectInstance * > ret;

	std::vector < std::pair<const CGObjectInstance*,SDL_Rect> > & objs = CGI->mh->ttiles[pos.x][pos.y][pos.z].objects;
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

void CCallback::trade( int mode, int id1, int id2, int val1 )
{
	int p1, p2;
	getMarketOffer(id1,id2,p1,p2,mode);
	TradeOnMarketplace pack(player,mode,id1,id2,val1);
	sendRequest(&pack);
}

void CCallback::setFormation(const CGHeroInstance * hero, bool tight)
{
	const_cast<CGHeroInstance*>(hero)->army.formation = tight;
	SetFormation pack(hero->id,tight);
	sendRequest(&pack);
}

void CCallback::setSelection(const CArmedInstance * obj)
{
	SetSelection ss;
	ss.player = player;
	ss.id = obj->id;
	sendRequest(&ss);
}

void CCallback::recruitHero(const CGTownInstance *town, const CGHeroInstance *hero)
{
	ui8 i=0;
	for(; i<gs->players[player].availableHeroes.size(); i++)
	{
		if(gs->players[player].availableHeroes[i] == hero)
		{
			HireHero pack(i,town->id);
			sendRequest(&pack);
			return;
		}
	}
}

std::vector<const CGHeroInstance *> CCallback::getAvailableHeroes(const CGTownInstance * town) const
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

template <typename T>
void CCallback::sendRequest(const T* request)
{
	//TODO? should be part of CClient but it would have to be very tricky cause template/serialization issues
	if(waitTillRealize)
		cl->waitingRequest.set(true);

	*cl->serv << request;

	if(waitTillRealize)
		cl->waitingRequest.waitWhileTrue();
}

CCallback::CCallback( CGameState * GS, int Player, CClient *C ) 
	:gs(GS), cl(C), player(Player)
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
	return cl->pathInfo->getPath(dest, ret);
}

InfoAboutHero::InfoAboutHero()
{
	details = NULL;
	hclass = NULL;
	portrait = -1;
}

InfoAboutHero::~InfoAboutHero()
{
	delete details;
}

void InfoAboutHero::initFromHero( const CGHeroInstance *h, bool detailed )
{
	owner = h->tempOwner;
	hclass = h->type->heroClass;
	name = h->name;
	portrait = h->portrait;
	army = h->army; 

	if(detailed) 
	{
		//include details about hero
		details = new Details;
		details->luck = h->getCurrentLuck();
		details->morale = h->getCurrentMorale();
		details->mana = h->mana;
		details->primskills.resize(PRIMARY_SKILLS);

		for (int i = 0; i < PRIMARY_SKILLS ; i++)
		{
			details->primskills[i] = h->getPrimSkillLevel(i);
		}
	}
	else
	{
		//hide info about hero stacks counts using descriptives names ids
		for(std::map<si32,std::pair<ui32,si32> >::iterator i = army.slots.begin(); i != army.slots.end(); ++i)
		{
			i->second.second = CCreature::getQuantityID(i->second.second);
		}
	}
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
	army = t->army;
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
	else
	{
		//hide info about hero stacks counts
		for(std::map<si32,std::pair<ui32,si32> >::iterator i = army.slots.begin(); i != army.slots.end(); ++i)
		{
			i->second.second = 0;
		}
	}
}