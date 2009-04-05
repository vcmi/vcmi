#include "stdafx.h"
#include "CAdvmapInterface.h"
#include "CCallback.h"
#include "CGameInfo.h"
#include "CGameState.h"
#include "CPlayerInterface.h"
#include "CPlayerInterface.h"
#include "client/Client.h"
#include "hch/CAmbarCendamo.h"
#include "hch/CBuildingHandler.h"
#include "hch/CDefObjInfoHandler.h"
#include "hch/CGeneralTextHandler.h"
#include "hch/CHeroHandler.h"
#include "hch/CObjectHandler.h"
#include "hch/CTownHandler.h"
#include "lib/Connection.h"
#include "lib/NetPacks.h"
#include "mapHandler.h"
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
extern CSharedCond<std::set<CPack*> > mess;

int gcd(int x, int y)
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

bool CCallback::moveHero(const CGHeroInstance *h, int3 dst) const
{
	MoveHero pack(dst,h->id);
	*cl->serv << &pack;

	{//wait till there is server answer
		boost::unique_lock<boost::mutex> lock(*mess.mx);
		while(std::find_if(mess.res->begin(),mess.res->end(),&isType<501>) == mess.res->end())
			mess.cv->wait(lock);
		std::set<CPack*>::iterator itr = std::find_if(mess.res->begin(),mess.res->end(),&isType<501>);
		TryMoveHero *tmh = dynamic_cast<TryMoveHero*>(*itr);
		mess.res->erase(itr);
		if(!tmh->result)
		{
			delete tmh;
			return false;
		}
		delete tmh;
	}
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
	*cl->serv << &pack;
}


bool CCallback::dismissCreature(const CArmedInstance *obj, int stackPos)
{
	if(((player>=0)  &&  obj->tempOwner != player) || obj->army.slots.size()<2)
		return false;

	DisbandCreature pack(stackPos,obj->id);
	*cl->serv << &pack;
	return true;
}
bool CCallback::upgradeCreature(const CArmedInstance *obj, int stackPos, int newID)
{
	UpgradeCreature pack(stackPos,obj->id,newID);
	*cl->serv << &pack;
	return false;
}
void CCallback::endTurn()
{
	tlog5 << "Player " << (unsigned)player << " end his turn." << std::endl;
	EndTurn pack;
	*cl->serv << &pack; //report that we ended turn
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

int CCallback::howManyTowns() const
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->players[player].towns.size();
}
const CGTownInstance * CCallback::getTownInfo(int val, bool mode) const //mode = 0 -> val = serial; mode = 1 -> val = ID
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	if (!mode)
		return gs->players[gs->currentPlayer].towns[val];
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
int CCallback::howManyHeroes() const
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->players[player].heroes.size();
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
		return static_cast<CGHeroInstance*>(gs->map->objects[val]);
	}
	return NULL;
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
					((CGI->mh->ttiles[path->nodes[i].coord.x][path->nodes[i].coord.y][path->nodes[i].coord.z].tileInfo->tertype==water)
					&&
					(CGI->mh->ttiles[path->nodes[i-1].coord.x][path->nodes[i-1].coord.y][path->nodes[i-1].coord.z].tileInfo->tertype!=water))
				  ||
					((CGI->mh->ttiles[path->nodes[i].coord.x][path->nodes[i].coord.y][path->nodes[i].coord.z].tileInfo->tertype!=water)
					&&
					(CGI->mh->ttiles[path->nodes[i-1].coord.x][path->nodes[i-1].coord.y][path->nodes[i-1].coord.z].tileInfo->tertype==water))
				  ||
				  (CGI->mh->ttiles[path->nodes[i-1].coord.x][path->nodes[i-1].coord.y][path->nodes[i-1].coord.z].tileInfo->tertype==rock)
					
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
	return gs->players[Player].fogOfWarMap[pos.x][pos.y][pos.z];
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

bool CCallback::isVisible( CGObjectInstance *obj, int Player ) const
{
	//object is visible when at least one blocked tile is visible
	for(int fx=0; fx<8; ++fx)
	{
		for(int fy=0; fy<6; ++fy)
		{
			int3 pos = obj->pos + int3(fx-7,fy-5,0);
			if(gs->map->isInTheMap(pos) 
				&& !((obj->defInfo->blockMap[fy] >> (7 - fx)) & 1) 
				&& isVisible(pos,Player)  )
				return true;
		}
	}
	return false;
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
	if(!obj)
		return NULL;
	if(obj->ID == HEROI_TYPE)
		return &(dynamic_cast<const CGHeroInstance*>(obj))->army;
	else if(obj->ID == TOWNI_TYPE)
		return &(dynamic_cast<const CGTownInstance*>(obj)->army);
	else return NULL;
}

int CCallback::swapCreatures(const CGObjectInstance *s1, const CGObjectInstance *s2, int p1, int p2)
{
	if(s1->tempOwner != player   ||   s2->tempOwner != player)
		return -1;

	ArrangeStacks pack(1,p1,p2,s1->id,s2->id,0);
	*cl->serv << &pack;
	return 0;
}

int CCallback::mergeStacks(const CGObjectInstance *s1, const CGObjectInstance *s2, int p1, int p2)
{	
	if ((s1->tempOwner!= player  ||  s2->tempOwner!=player))
	{
		return -1;
	}
	ArrangeStacks pack(2,p1,p2,s1->id,s2->id,0);
	*cl->serv << &pack;
	return 0;
}
int CCallback::splitStack(const CGObjectInstance *s1, const CGObjectInstance *s2, int p1, int p2, int val)
{
	if (s1->tempOwner!= player  ||  s2->tempOwner!=player || (!val))
	{
		return -1;
	}
	ArrangeStacks pack(3,p1,p2,s1->id,s2->id,val);
	*cl->serv << &pack;
	return 0;
}

bool CCallback::dismissHero(const CGHeroInstance *hero)
{
	if(player!=hero->tempOwner) return false;

	DismissHero pack(hero->id);
	*cl->serv << &pack;
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
	*cl->serv << &ea;
	return true;
}

bool CCallback::buildBuilding(const CGTownInstance *town, si32 buildingID)
{
	CGTownInstance * t = const_cast<CGTownInstance *>(town);

	if(town->tempOwner!=player)
		return false;
	CBuilding *b = CGI->buildh->buildings[t->subID][buildingID];
	for(int i=0;i<7;i++)
		if(b->resources[i] > gs->players[player].resources[i])
			return false; //lack of resources

	BuildStructure pack(town->id,buildingID);
	*cl->serv << &pack;
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

int CCallback::battleGetStack(int pos)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->battleGetStack(pos);
}

CStack* CCallback::battleGetStackByID(int ID)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	if(!gs->curB) return NULL;
	return gs->curB->getStack(ID);
}

int CCallback::battleMakeAction(BattleAction* action)
{
	MakeCustomAction mca(*action);
	*cl->serv << &mca;
	return 0;
}

CStack* CCallback::battleGetStackByPos(int pos)
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return battleGetStackByID(battleGetStack(pos));
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
	CStack *our = battleGetStackByID(ID), *dst = battleGetStackByPos(dest);
	if(!our || !dst || !gs->curB) return false; 

	for(size_t g=0; g<our->effects.size(); ++g)
	{
		if(61 == our->effects[g].id) //forgetfulness
			return false;
	}

	if(vstd::contains(our->abilities,SHOOTER)//it's shooter
		&& our->owner != dst->owner
		&& dst->alive()
		&& !gs->curB->isStackBlocked(ID)
		&& our->shots
		)
		return true;
	return false;
}

void CCallback::swapGarrisonHero( const CGTownInstance *town )
{
	if(town->tempOwner != player) return;

	GarrisonHeroSwap pack(town->id);
	*cl->serv << &pack;
}

void CCallback::buyArtifact(const CGHeroInstance *hero, int aid)
{
	if(hero->tempOwner != player) return;

	BuyArtifact pack(hero->id,aid);
	*cl->serv << &pack;
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
	float r = gs->resVals[t1],
		g = gs->resVals[t2] / gs->getMarketEfficiency(player,mode);
	if(r>g)
	{
		rec = ceil(r / g);
		give = 1;
	}
	else
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
	*cl->serv << &pack;
}

void CCallback::setFormation(const CGHeroInstance * hero, bool tight)
{
	const_cast<CGHeroInstance*>(hero)->army.formation = tight;
	SetFormation pack(hero->id,tight);
	*cl->serv << &pack;
}

void CCallback::setSelection(const CArmedInstance * obj)
{
	SetSelection ss;
	ss.player = player;
	ss.id = obj->id;
	*cl->serv << &ss;
}

void CCallback::recruitHero(const CGTownInstance *town, const CGHeroInstance *hero)
{
	ui8 i=0;
	for(; i<gs->players[player].availableHeroes.size(); i++)
	{
		if(gs->players[player].availableHeroes[i] == hero)
		{
			HireHero pack(i,town->id);
			*cl->serv << &pack;
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
	if(!isVisible(tile, player)) return NULL;
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return &gs->map->getTile(tile);
}

int CCallback::canBuildStructure( const CGTownInstance *t, int ID )
{
	return gs->canBuildStructure(t,ID);
}

CPath * CCallback::getPath( int3 src, int3 dest, const CGHeroInstance * hero )
{
	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
	return gs->getPath(src,dest,hero);
}

void CCallback::save( const std::string &fname )
{
	cl->save(fname);
}


void CCallback::sendMessage(const std::string &mess)
{
	PlayerMessage pm(player, mess);
	*cl->serv << &pm;
}