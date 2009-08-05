#include "CGeniusAI.h"
#include <iostream>
#include "../../hch/CBuildingHandler.h"
using namespace std;
using namespace GeniusAI;

#if defined (_MSC_VER) && (_MSC_VER >= 1020) || (__MINGW32__)
#include <windows.h>
#endif

void DbgBox(const char *msg, bool messageBox)
{
#if defined PRINT_DEBUG
#if defined _DEBUG
//#if 0
#	if defined (_MSC_VER) && (_MSC_VER >= 1020)
	if (messageBox)
	{
		MessageBoxA(NULL, msg, "Debug message", MB_OK | MB_ICONASTERISK);
	}
#	endif
	std::cout << msg << std::endl;
#endif
#endif
}

CGeniusAI::CGeniusAI()
	: m_generalAI(), turn(0), m_state(NO_BATTLE), firstTurn(true)
{
}

CGeniusAI::~CGeniusAI()
{
}

void CGeniusAI::init(ICallback *CB)
{
	m_cb = CB;
	m_generalAI.init(CB);

	human = false;
	playerID = m_cb->getMyColor();
	serialID = m_cb->getMySerial();
	std::string info = std::string("GeniusAI initialized for player ") + boost::lexical_cast<std::string>(playerID);
	m_battleLogic = NULL;
	DbgBox(info.c_str());
}

unsigned long randomFromInt(unsigned long in)
{
	return (in*214013+2531011);
}


void CGeniusAI::reportResources()
{
	std::cout << "AI Player " <<m_cb->getMySerial()<< " with " <<  m_cb->howManyHeroes(true) << " heroes. " << std::endl;
	
	std::cout << m_cb->getResourceAmount(0) << " wood. ";
	std::cout << m_cb->getResourceAmount(1) << " mercury. ";
	std::cout << m_cb->getResourceAmount(2) << " ore. ";
	std::cout << m_cb->getResourceAmount(3) << " sulfer. ";
	std::cout << m_cb->getResourceAmount(4) << " cristal. ";
	std::cout << m_cb->getResourceAmount(5) << " gems. ";
	std::cout << m_cb->getResourceAmount(6) << " gold.";
	std::cout << std::endl;
}

void CGeniusAI::addHeroObjectives(CGeniusAI::HypotheticalGameState::HeroModel &h, CGeniusAI::HypotheticalGameState &hgs)
{
	int3 hpos, destination;
	CPath path;
	hpos = h.pos;
	int movement = h.remainingMovement;
	for(std::set<AIObjectContainer>::const_iterator i = hgs.knownVisitableObjects.begin(); i != hgs.knownVisitableObjects.end();i++)
	{
	//	if(i->o->ID==54||i->o->ID==34)	//creatures, another hero
	//		continue;

		if(i->o->ID==53)	//mine
		if(i->o->getOwner()==m_cb->getMyColor())//don't visit if you own, there's almost no point(maybe to leave guards or because the hero's trapped).
			continue;
		
		destination = i->o->getSightCenter();
		if(hpos.z==destination.z)
			if(m_cb->getPath(hpos,destination,h.h,path))
		{
			path.convert(0);
			if(path.nodes[0].dist<movement)
			{
				AIObjective::Type tp = AIObjective::visit;
				
				HeroObjective ho(tp,i->o,&h);
				std::set<HeroObjective>::iterator found = currentHeroObjectives.find(ho);
				if(found==currentHeroObjectives.end())
					currentHeroObjectives.insert(ho);
				else
					found->whoCanAchieve.push_back(&h);
			}
		}
	}
}
void CGeniusAI::addTownObjectives(HypotheticalGameState::TownModel &t, HypotheticalGameState & hgs)
{
	//recruitHero
	//recruitCreatures
	//upgradeCreatures
	//buildBuilding
	if(hgs.heroModels.size()<3&&hgs.resourceAmounts[6]>=2500) //recruitHero
	{
		if(!t.visitingHero  &&  vstd::contains(t.t->builtBuildings, 5)) //no visiting hero and built tavern
		{
			for(int i =0; i < hgs.AvailableHeroesToBuy.size();i++)
				if(hgs.AvailableHeroesToBuy[i]!=NULL&&hgs.AvailableHeroesToBuy[i]->army.slots.size()>1)//only buy heros with units
				{
					TownObjective to(AIObjective::recruitHero,&t,0);
					currentTownObjectives.insert(to);
				}

		}
	}
	
/*	for(int i = 0; i < t.t->creatures.size() ;i++)
	{
		if(t.t->creatures[i].second.empty()) continue;

		int ID = t.t->creatures[i].second.back();
		
		const CCreature *creature = m_cb->getCCreatureByID(ID);
		cout << "town has " << t.t->creatures[i].first  << " "<< creature->namePl << " (AI Strength " << creature->AIValue << ")." << endl;
	}

	//buildBuilding
	std::map<int, CBuilding *> thisTownsBuildings = m_cb->getCBuildingsByID(t.t);
	for(std::map<int, CBuilding *>::iterator i = thisTownsBuildings.begin(); i != thisTownsBuildings.end();i++)
		cout << "structure "<< i->first << ", " << i->second->Name() << ", build?= " << m_cb->canBuildStructure(t.t,i->first) << endl;
*/
}

void CGeniusAI::TownObjective::fulfill(CGeniusAI & cg,HypotheticalGameState &hgs)
{
	cg.m_cb->waitTillRealize = true;
	switch(type)
	{
	case recruitHero:
		cg.m_cb->recruitHero(whichTown->t,hgs.AvailableHeroesToBuy[which]);
		hgs.heroModels.push_back(hgs.AvailableHeroesToBuy[which]);
		whichTown->visitingHero = true;
		
		//TODO: sub 2500 gold from hgs here
	}
	
	cg.m_cb->waitTillRealize = false;
}


void CGeniusAI::HeroObjective::fulfill(CGeniusAI & cg,HypotheticalGameState & hgs)
{
	cg.m_cb->waitTillRealize = true;
	switch(type)
	{
	case visit:
		HypotheticalGameState::HeroModel * h = whoCanAchieve[rand()%whoCanAchieve.size()];
		int3 hpos, destination;
		CPath path;
		hpos = h->pos;
		//std::cout << "trying to visit " << object->hoverName << std::endl;
		
		destination = object->getSightCenter();
		if(cg.m_cb->getPath(hpos,destination,h->h,path))
		{
			path.convert(0);
													//wait over, battle over too. hero might be killed. check.
			for(int i = path.nodes.size()-2;i>=0&&(cg.m_cb->getHeroSerial(h->h) >= 0);i--)
			{
				cg.m_cb->moveHero(h->h,path.nodes[i].coord);

				if(cg.m_state.get() != NO_BATTLE)
					cg.m_state.waitUntil(NO_BATTLE);//wait for battle end
			}


			h->remainingMovement-=path.nodes[0].dist;
			if(object->blockVisit)
				h->pos = path.nodes[1].coord;
			else
				h->pos=destination;
			std::set<AIObjectContainer>::iterator i = hgs.knownVisitableObjects.find(AIObjectContainer(object));
			if(i!=hgs.knownVisitableObjects.end())
				hgs.knownVisitableObjects.erase(i);
		}


	}
	cg.m_cb->waitTillRealize = false;
}


void CGeniusAI::fillObjectiveQueue(HypotheticalGameState & hgs)
{
	objectiveQueue.clear();
	currentHeroObjectives.clear();
	currentTownObjectives.clear();
	
	for(std::vector <CGeniusAI::HypotheticalGameState::HeroModel>::iterator i = hgs.heroModels.begin(); i != hgs.heroModels.end(); i++)
		addHeroObjectives(*i,hgs);

	for(std::vector <CGeniusAI::HypotheticalGameState::TownModel>::iterator i = hgs.townModels.begin(); i != hgs.townModels.end(); i++)
		addTownObjectives(*i,hgs);
	for(std::set<CGeniusAI::HeroObjective>::iterator i = currentHeroObjectives.begin(); i != currentHeroObjectives.end(); i++)
		objectiveQueue.push_back(AIObjectivePtrCont(&(*i)));
	for(std::set<CGeniusAI::TownObjective>::iterator i = currentTownObjectives.begin(); i != currentTownObjectives.end(); i++)
		objectiveQueue.push_back(AIObjectivePtrCont(&(*i)));
}
CGeniusAI::AIObjective * CGeniusAI::getBestObjective()
{
	trueGameState = HypotheticalGameState(*this);
	
	fillObjectiveQueue(trueGameState);
	
	if(!objectiveQueue.empty())
		return max_element(objectiveQueue.begin(),objectiveQueue.end())->obj;
	return NULL;

}
void CGeniusAI::yourTurn()
{

	static int seed = rand();
	srand(seed);
	if(firstTurn)
	{
		//m_cb->sendMessage("vcmieagles");
		//m_cb->sendMessage("vcmiformenos");
		//m_cb->sendMessage("vcmiformenos");
		firstTurn = false;
	}
	//////////////TODO: replace with updates. Also add suspected objects list./////////
	knownVisitableObjects.clear();
	int3 pos = m_cb->getMapSize();
	for(int x = 0;x<pos.x;x++)
		for(int y = 0;y<pos.y;y++)
			for(int z = 0;z<pos.z;z++)
				tileRevealed(int3(x,y,z));
	///////////////////////////////////////////////////////////////////////////////////

	reportResources();
	turn++;

	AIObjective * objective;
	while((objective = getBestObjective())!=NULL)
		objective->fulfill(*this,trueGameState);

	seed = rand();
	m_cb->endTurn();
}

void CGeniusAI::heroKilled(const CGHeroInstance * hero)
{

}

void CGeniusAI::heroCreated(const CGHeroInstance *hero)
{

}

void CGeniusAI::tileRevealed(int3 pos)
{
	std::vector < const CGObjectInstance * > objects = 	m_cb->getVisitableObjs(pos);
	for(std::vector < const CGObjectInstance * >::iterator o = objects.begin();o!=objects.end();o++)
		knownVisitableObjects.insert(*o);
	objects = m_cb->getFlaggableObjects(pos);
	for(std::vector < const CGObjectInstance * >::iterator o = objects.begin();o!=objects.end();o++)
		knownVisitableObjects.insert(*o);
}

void CGeniusAI::newObject(const CGObjectInstance * obj) //eg. ship built in shipyard
{
	knownVisitableObjects.insert(obj);
}

void CGeniusAI::objectRemoved(const CGObjectInstance *obj) //eg. collected resource, picked artifact, beaten hero
{
	std::set <AIObjectContainer>::iterator o = knownVisitableObjects.find(obj);
	if(o!=knownVisitableObjects.end())
		knownVisitableObjects.erase(o);
}

void CGeniusAI::tileHidden(int3 pos)
{
	
}

void CGeniusAI::heroMoved(const TryMoveHero &TMH)
{
	//DbgBox("** CGeniusAI::heroMoved **");


}

void CGeniusAI::heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback)
{
	callback(rand() % skills.size());
}

void GeniusAI::CGeniusAI::showGarrisonDialog( const CArmedInstance *up, const CGHeroInstance *down, boost::function<void()> &onEnd )
{
	onEnd();
}

void GeniusAI::CGeniusAI::playerBlocked( int reason )
{
	if(reason == 0) //battle is coming...
	{
		m_state.setn(UPCOMING_BATTLE);
	}
}

void GeniusAI::CGeniusAI::battleResultsApplied()
{
	assert(m_state.get() == ENDING_BATTLE);
	m_state.setn(NO_BATTLE);
}

void CGeniusAI::showBlockingDialog(const std::string &text, const std::vector<Component> &components, ui32 askID, const int soundID, bool selection, bool cancel)
{
	m_cb->selectionMade(cancel ? 0 : 1, askID);
}

/**
 * occurs AFTER every action taken by any stack or by the hero
 */
void CGeniusAI::actionFinished(const BattleAction *action)
{
	std::string message("\t\tCGeniusAI::actionFinished - type(");
	message += boost::lexical_cast<std::string>((unsigned)action->actionType);
	message += "), side(";
	message += boost::lexical_cast<std::string>((unsigned)action->side);
	message += ")";
	DbgBox(message.c_str());
}
/**
 * occurs BEFORE every action taken by any stack or by the hero
 */
void CGeniusAI::actionStarted(const BattleAction *action)
{
	std::string message("\t\tCGeniusAI::actionStarted - type(");
	message += boost::lexical_cast<std::string>((unsigned)action->actionType);
	message += "), side(";
	message += boost::lexical_cast<std::string>((unsigned)action->side);
	message += ")";
	DbgBox(message.c_str());
}
/**
 * called when stack is performing attack
 */
void CGeniusAI::battleAttack(BattleAttack *ba)
{
	DbgBox("\t\t\tCGeniusAI::battleAttack");
}
/**
 * called when stack receives damage (after battleAttack())
 */
void CGeniusAI::battleStacksAttacked(std::set<BattleStackAttacked> & bsa)
{
	DbgBox("\t\t\tCGeniusAI::battleStacksAttacked");
}
/**
 * called by engine when battle starts; side=0 - left, side=1 - right
 */

void CGeniusAI::battleStart(CCreatureSet *army1, CCreatureSet *army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, bool side)
{
	assert(!m_battleLogic);
	assert(playerID > PLAYER_LIMIT  ||  m_state.get() == UPCOMING_BATTLE); //we have been informed that battle will start (or we are neutral AI)

	m_state.setn(ONGOING_BATTLE);
	m_battleLogic = new BattleAI::CBattleLogic(m_cb, army1, army2, tile, hero1, hero2, side);

	DbgBox("** CGeniusAI::battleStart **");
}
/**
 *
 */
void CGeniusAI::battleEnd(BattleResult *br)
{
	/*switch(br->winner)
	{
		case 0:	std::cout << "The winner is the attacker." << std::endl;break;
		case 1:	std::cout << "The winner is the defender." << std::endl;break;
		case 2:	std::cout << "It's a draw." << std::endl;break;
	};*/
	
	delete m_battleLogic;
	m_battleLogic = NULL;

	assert(m_state.get() == ONGOING_BATTLE);
	m_state.setn(ENDING_BATTLE);

	DbgBox("** CGeniusAI::battleEnd **");
}
/**
 * called at the beggining of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
 */
void CGeniusAI::battleNewRound(int round)
{
	std::string message("\tCGeniusAI::battleNewRound - ");
	message += boost::lexical_cast<std::string>(round);
	DbgBox(message.c_str());

	m_battleLogic->SetCurrentTurn(round);
}
/**
 *
 */
void CGeniusAI::battleStackMoved(int ID, int dest, int distance, bool end)
{
	std::string message("\t\t\tCGeniusAI::battleStackMoved ID(");
	message += boost::lexical_cast<std::string>(ID);
	message += "), dest(";
	message += boost::lexical_cast<std::string>(dest);
	message += ")";
	DbgBox(message.c_str());
}
/**
 *
 */
void CGeniusAI::battleSpellCast(SpellCast *sc)
{
	DbgBox("\t\t\tCGeniusAI::battleSpellCast");
}
/**
 * called when battlefield is prepared, prior the battle beginning
 */
void CGeniusAI::battlefieldPrepared(int battlefieldType, std::vector<CObstacle*> obstacles)
{
	DbgBox("CGeniusAI::battlefieldPrepared");
}
/**
 *
 */
void CGeniusAI::battleStackMoved(int ID, int dest, bool startMoving, bool endMoving)
{
	DbgBox("\t\t\tCGeniusAI::battleStackMoved");
}
/**
 *
 */
void CGeniusAI::battleStackAttacking(int ID, int dest)
{
	DbgBox("\t\t\tCGeniusAI::battleStackAttacking");
}
/**
 *
 */
void CGeniusAI::battleStackIsAttacked(int ID, int dmg, int killed, int IDby, bool byShooting)
{
	DbgBox("\t\t\tCGeniusAI::battleStackIsAttacked");
}

/**
 * called when it's turn of that stack
 */
BattleAction CGeniusAI::activeStack(int stackID)
{
	std::string message("\t\t\tCGeniusAI::activeStack stackID(");

	message += boost::lexical_cast<std::string>(stackID);
	message += ")";
	DbgBox(message.c_str());

	return m_battleLogic->MakeDecision(stackID);
};

