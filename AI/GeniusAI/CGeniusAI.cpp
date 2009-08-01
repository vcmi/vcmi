#include "CGeniusAI.h"
#include <iostream>

using namespace std;
using namespace GeniusAI;

#if defined (_MSC_VER) && (_MSC_VER >= 1020) || (__MINGW32__)
#include <windows.h>
#endif

void MsgBox(const char *msg, bool messageBox)
{
#if defined _DEBUG
#	if defined (_MSC_VER) && (_MSC_VER >= 1020)
	if (messageBox)
	{
		MessageBoxA(NULL, msg, "Debug message", MB_OK | MB_ICONASTERISK);
	}
#	endif
	std::cout << msg << std::endl;
#endif
}

CGeniusAI::CGeniusAI()
	: m_generalAI(),turn(0)
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
	MsgBox(info.c_str());
}

unsigned long randomFromInt(unsigned long in)
{
	return (in*214013+2531011);
}

void CGeniusAI::doHero(const CGHeroInstance * h)
{

	
	if(!h==NULL)
	{
		int3 destination, pos;
		pos=h->convertPosition(h->pos,false);
		
		vector<int3> buildingPath;

		int movement = h->movement;
		int usedMovement = 0;
		int attempts = 0;
		CPath path;
		do{
			do{
				destination=pos;
				attempts++;
				destination.x+=randomFromInt((attempts*1243)+turn)%9-4;
				destination.y+=randomFromInt((attempts*1243)+turn+1234)%9-4;
			}while((!m_cb->getPath(pos,destination,h,path)||(path.nodes[0].dist>=(movement-usedMovement))) && attempts<100);
			
			if(attempts<100)
			{
				pos = destination;
				usedMovement += path.nodes[0].dist;
				path.convert(0);
				for(int i = path.nodes.size()-2;i>=0;i--)
					buildingPath.push_back(path.nodes[i].coord);
			}
			else break;
		}while(movement-usedMovement>=50);

		for(int i = 0; i < buildingPath.size();i++)
		{
			m_cb->moveHero(h,buildingPath[i]);
			//std::cout << "(" << buildingPath[i].x << ", " << buildingPath[i].y << ")" << std::endl;
		}

	}

}

void CGeniusAI::doTown(const CGTownInstance * t)
{
	if(m_cb->howManyHeroes(true)<3)		//recrute up to 3 heroes
	{
		if(t->visitingHero==NULL)
		{
			std::vector<const CGHeroInstance *> toBuy = m_cb->getAvailableHeroes(t);
			if(toBuy[0]->army.slots.size()>1)//only buy heros with units
			{
				m_cb->recruitHero(t,toBuy[0]);
			}
		}

	}

//	m_cb->recruitCreatures(t, ui32 ID, ui32 amount)

	
}


void CGeniusAI::yourTurn()
{
	//static boost::mutex mutex;
	//boost::mutex::scoped_lock scoped_lock(mutex);
	turn++;

	std::cout << "AI Player " <<m_cb->getMySerial()<< " with " <<  m_cb->howManyHeroes(true) << " heroes. " << std::endl;

	
	std::cout << m_cb->getResourceAmount(0) << " wood. ";
	std::cout << m_cb->getResourceAmount(1) << " mercury. ";
	std::cout << m_cb->getResourceAmount(2) << " ore. ";
	std::cout << m_cb->getResourceAmount(3) << " sulfer. ";
	std::cout << m_cb->getResourceAmount(4) << " cristal. ";
	std::cout << m_cb->getResourceAmount(5) << " gems. ";
	std::cout << m_cb->getResourceAmount(6) << " gold.";
	std::cout << std::endl;


	std::vector < const CGHeroInstance *> heroes = m_cb->getHeroesInfo();	
	for(std::vector < const CGHeroInstance *>::iterator i = heroes.begin(); i < heroes.end(); i++)
		doHero(*i);

//	std::vector < const CGTownInstance *> towns = m_cb->getTownsInfo();	
//	for(std::vector < const CGTownInstance *>::iterator i = towns.begin(); i < towns.end(); i++)
//		doTown(*i);


	m_cb->endTurn();
}

void CGeniusAI::heroKilled(const CGHeroInstance * hero)
{

}

void CGeniusAI::heroCreated(const CGHeroInstance *hero)
{

}

void CGeniusAI::heroMoved(const TryMoveHero &TMH)
{
	MsgBox("** CGeniusAI::heroMoved **");

	

}

void CGeniusAI::heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback)
{
	callback(rand() % skills.size());
}

void GeniusAI::CGeniusAI::showGarrisonDialog( const CArmedInstance *up, const CGHeroInstance *down, boost::function<void()> &onEnd )
{
	onEnd();
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
	MsgBox(message.c_str());
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
	MsgBox(message.c_str());
}
/**
 * called when stack is performing attack
 */
void CGeniusAI::battleAttack(BattleAttack *ba)
{
	MsgBox("\t\t\tCGeniusAI::battleAttack");
}
/**
 * called when stack receives damage (after battleAttack())
 */
void CGeniusAI::battleStacksAttacked(std::set<BattleStackAttacked> & bsa)
{
	MsgBox("\t\t\tCGeniusAI::battleStacksAttacked");
}
/**
 * called by engine when battle starts; side=0 - left, side=1 - right
 */

void CGeniusAI::battleStart(CCreatureSet *army1, CCreatureSet *army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, bool side)
{
	assert(!m_battleLogic); //************** assert fails when AI starts two battles at same time? ***************
	m_battleLogic = new BattleAI::CBattleLogic(m_cb, army1, army2, tile, hero1, hero2, side);

	MsgBox("** CGeniusAI::battleStart **");
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

	MsgBox("** CGeniusAI::battleEnd **");
}
/**
 * called at the beggining of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
 */
void CGeniusAI::battleNewRound(int round)
{
	std::string message("\tCGeniusAI::battleNewRound - ");
	message += boost::lexical_cast<std::string>(round);
	MsgBox(message.c_str());

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
	MsgBox(message.c_str());
}
/**
 *
 */
void CGeniusAI::battleSpellCast(SpellCast *sc)
{
	MsgBox("\t\t\tCGeniusAI::battleSpellCast");
}
/**
 * called when battlefield is prepared, prior the battle beginning
 */
void CGeniusAI::battlefieldPrepared(int battlefieldType, std::vector<CObstacle*> obstacles)
{
	MsgBox("CGeniusAI::battlefieldPrepared");
}
/**
 *
 */
void CGeniusAI::battleStackMoved(int ID, int dest, bool startMoving, bool endMoving)
{
	MsgBox("\t\t\tCGeniusAI::battleStackMoved");
}
/**
 *
 */
void CGeniusAI::battleStackAttacking(int ID, int dest)
{
	MsgBox("\t\t\tCGeniusAI::battleStackAttacking");
}
/**
 *
 */
void CGeniusAI::battleStackIsAttacked(int ID, int dmg, int killed, int IDby, bool byShooting)
{
	MsgBox("\t\t\tCGeniusAI::battleStackIsAttacked");
}

/**
 * called when it's turn of that stack
 */
BattleAction CGeniusAI::activeStack(int stackID)
{
	std::string message("\t\t\tCGeniusAI::activeStack stackID(");

	message += boost::lexical_cast<std::string>(stackID);
	message += ")";
	MsgBox(message.c_str());

	return m_battleLogic->MakeDecision(stackID);
};

