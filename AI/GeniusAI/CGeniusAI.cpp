#include "CGeniusAI.h"
#include <iostream>
#include "../../hch/CBuildingHandler.h"

#include "../../lib/VCMI_Lib.h"

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
	int3 interestingPos;
	int maxInteresting=0;
	for(std::set<AIObjectContainer>::const_iterator i = hgs.knownVisitableObjects.begin(); i != hgs.knownVisitableObjects.end();i++)
	{
	//	if(i->o->ID==54||i->o->ID==34)	//creatures, another hero
	//		continue;
		if(i->o->ID==53&&i->o->getOwner()==m_cb->getMyColor())//don't visit a mine if you own, there's almost no point(maybe to leave guards or because the hero's trapped).
			continue;
		
		destination = i->o->getSightCenter();

		if(hpos.z==destination.z)							//don't try to take a path from the underworld to the top or vice versa
		{
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

				// find the most interesting object that is eventually reachable, and set a similar (out of the way) position to the ultimate goal position
				int hi = rand(); //TODO: replace random numbers with some sort of ranking system

				if(hi>maxInteresting)
				{
					maxInteresting = hi;
					interestingPos = destination;
				}
			}		

		}
	}

	//find close pos with the most neighboring empty squares
	if(h.remainingMovement>0&&m_cb->getPath(hpos,interestingPos,h.h,path))
	{
		int3 bestPos = interestingPos,currentPos,destPos;
		int howGood=0;
		for(int x = -2;x <= 2;x++)
			for(int y = -2;y <= 2;y++)
			{
				currentPos=interestingPos+int3(x,y,0);
				if(m_cb->getVisitableObjs(destPos).size()!=0)			//there better not be anything there
					continue;
				if(!m_cb->getPath(hpos,destPos,h.h,path))				//it had better be reachable from the hero
					continue;
				
				int count = 0;
				for(int xx = -1;xx <= 1;xx++)
					for(int yy = -2;yy <= 1;yy++)
					{
						destPos = currentPos+int3(xx,yy,0);
						if(m_cb->getPath(currentPos,destPos,h.h,path))
							count++;
					}
				if(count > howGood)
				{
					howGood = count;
					bestPos = currentPos;
				}
			}
			
		h.interestingPos = bestPos;
		currentHeroObjectives.insert(HeroObjective(HeroObjective::finishTurn,h.h,&h));
	}

}

void CGeniusAI::HeroObjective::fulfill(CGeniusAI & cg,HypotheticalGameState & hgs)
{
	HypotheticalGameState::HeroModel * h;
	int3 hpos, destination;
	CPath path;
	CPath path2;
	
	switch(type)
	{
	case finishTurn:
		h = whoCanAchieve.front();
		hpos = h->pos;
		destination = h->interestingPos;
		if(!cg.m_cb->getPath(hpos,destination,h->h,path)) {cout << "AI error: invalid destination" << endl; return;}
//		path.convert(0);
		destination = h->pos;
		for(int i = path.nodes.size()-2;i>=0;i--)		//find closest coord that we can get to
			if(cg.m_cb->getPath(hpos,path.nodes[i].coord,h->h,path2)&&path2.nodes[0].dist<=h->remainingMovement)
				destination = path.nodes[i].coord;



		break;
	case visit:
		//std::cout << "trying to visit " << object->hoverName << std::endl;
		h = whoCanAchieve[rand()%whoCanAchieve.size()];//TODO:replace with best hero for the job
		hpos = h->pos;
		destination = object->getSightCenter();

		break;
		


	}
	if(type == visit||type == finishTurn)
	if(cg.m_cb->getPath(hpos,destination,h->h,path))
	{
		path.convert(0);
		
		if(cg.m_state.get() != NO_BATTLE)
			cg.m_state.waitUntil(NO_BATTLE);//wait for battle end
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

void CGeniusAI::addTownObjectives(HypotheticalGameState::TownModel &t, HypotheticalGameState & hgs)
{
	//recruitHero
	//buildBuilding
	//recruitCreatures
	//upgradeCreatures
	if(hgs.heroModels.size()<3&&hgs.resourceAmounts[6]>=2500) //recruitHero
	{
		bool heroAtTown = false;
		for(int i = 0; i < hgs.heroModels.size();i++)
			if(hgs.heroModels[i].pos==t.t->getSightCenter())
				heroAtTown = true;
		if(!heroAtTown  &&  vstd::contains(t.t->builtBuildings, 5)) //no visiting hero and built tavern

		{
			for(int i =0; i < hgs.AvailableHeroesToBuy.size();i++)
				if(hgs.AvailableHeroesToBuy[i]!=NULL&&hgs.AvailableHeroesToBuy[i]->getArmyStrength()>4000)//only buy heros with units
				{//TODO: recruit heros of own race.
					TownObjective to(AIObjective::recruitHero,&t,0);
					currentTownObjectives.insert(to);
				}
		}
	}
		//buildBuilding
	if(!t.hasBuilt)
	{
		std::map<int, CBuilding *> thisTownsBuildings = VLC->buildh->buildings[t.t->subID];// m_cb->getCBuildingsByID(t.t);
		for(std::map<int, CBuilding *>::iterator i = thisTownsBuildings.begin(); i != thisTownsBuildings.end();i++)
		{
			if(m_cb->canBuildStructure(t.t,i->first)==7)
			{
				TownObjective to(AIObjective::buildBuilding,&t,i->first);
				currentTownObjectives.insert(to);
				//cout <<"can build " << i->first << " "<< i->second->Name() << endl;
			}
		}
	}
	
	//recruitCreatures

	for(int i = 0; i < t.creaturesToRecruit.size() ;i++)
	{
		if(t.creaturesToRecruit[i].first==0) continue;
		int ID = t.creaturesToRecruit[i].second.back();
		
		const CCreature *creature = &VLC->creh->creatures[ID];//m_cb->getCCreatureByID(ID);
		bool canAfford = true;
		for(int ii = 0;ii<creature->cost.size();ii++)
			if(creature->cost[ii]>hgs.resourceAmounts[ii])
				canAfford = false;
		if(!canAfford) continue;
				
		//cout << "town has " << t.t->creatures[i].first  << " "<< creature->namePl << " (AI Strength " << creature->AIValue << ")." << endl;
		TownObjective to(AIObjective::recruitCreatures,&t,i);
		currentTownObjectives.insert(to);

	}
	//upgradeCreatures
/*	for(int i = 0; i < t.creaturesToRecruit.size() ;i++)
	{
		if(t.creaturesToRecruit[i].first==0) continue;
		int ID = t.creaturesToRecruit[i].second.back();
		
		const CCreature *creature = m_cb->getCCreatureByID(ID);
		bool canAfford = true;
		for(int ii = 0;ii<creature->cost.size();ii++)
			if(creature->cost[ii]>hgs.resourceAmounts[ii])
				canAfford = false;
		if(!canAfford) continue;
				
		//cout << "town has " << t.t->creatures[i].first  << " "<< creature->namePl << " (AI Strength " << creature->AIValue << ")." << endl;
		TownObjective to(AIObjective::recruitCreatures,&t,i);
		currentTownObjectives.insert(to);

	}*/

}

void CGeniusAI::TownObjective::fulfill(CGeniusAI & cg,HypotheticalGameState &hgs)
{
	CBuilding * b;
	const CCreature *creature;
	HypotheticalGameState::HeroModel hm;
	switch(type)
	{
	case recruitHero:
		cg.m_cb->recruitHero(whichTown->t,hgs.AvailableHeroesToBuy[which]);
		hm = HypotheticalGameState::HeroModel(hgs.AvailableHeroesToBuy[which]);
		hm.pos = whichTown->t->getSightCenter();
		hm.remainingMovement = hm.h->maxMovePoints(true);
		hgs.heroModels.push_back(hm);
		hgs.resourceAmounts[6]-=2500;
		break;
	case buildBuilding:
		b = VLC->buildh->buildings[whichTown->t->subID][which];//cg.m_cb->getCBuildingsByID(whichTown->t)[which];
		if(cg.m_cb->canBuildStructure(whichTown->t,which)==7)
		{
			cout << "built " << b->Name() << "." << endl;
			cg.m_cb->buildBuilding(whichTown->t,which);
			for(int i = 0; b && i < b->resources.size();i++)
				hgs.resourceAmounts[i]-=b->resources[i];
			whichTown->hasBuilt=true;
		}
		break;
	case recruitCreatures:
		int ID = whichTown->creaturesToRecruit[which].second.back();				//buy upgraded if possible
		creature = &VLC->creh->creatures[ID];//cg.m_cb->getCCreatureByID(ID);
		int howMany = whichTown->creaturesToRecruit[which].first;
		for(int i = 0; i < creature->cost.size();i++)
			howMany = min(howMany,creature->cost[i]?hgs.resourceAmounts[i]/creature->cost[i]:INT_MAX);
		if(howMany == 0) cout << "tried to recruit without enough money.";
		cout << "recruiting " << howMany  << " "<< creature->namePl << " (Total AI Strength " << creature->AIValue*howMany << ")." << endl;
		cg.m_cb->recruitCreatures(whichTown->t,ID,howMany);

		break;
		

	}
	
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
	m_cb->waitTillRealize = true;
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
	
	m_cb->waitTillRealize = false;
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

