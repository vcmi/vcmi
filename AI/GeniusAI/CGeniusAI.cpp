#include "CGeniusAI.h"
#include <iostream>
#include <fstream>
#include <math.h>
#include <boost/lexical_cast.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/if.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost::lambda;

using namespace GeniusAI;

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#include <windows.h>
#endif

void MsgBox(const char *msg, bool messageBox = false)
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

void CGeniusAI::init(ICallback *CB)
{
	m_cb = CB;
	human = false;
	playerID = m_cb->getMyColor();
	serialID = m_cb->getMySerial();
	std::string info = std::string("GeniusAI initialized for player ") + boost::lexical_cast<std::string>(playerID);
	m_battleLogic = NULL;
	MsgBox(info.c_str());
}

void CGeniusAI::yourTurn()
{
	m_cb->endTurn();
}

void CGeniusAI::heroKilled(const CGHeroInstance *)
{
}

void CGeniusAI::heroCreated(const CGHeroInstance *)
{
}

void CGeniusAI::heroMoved(const HeroMoveDetails &)
{
}

void CGeniusAI::heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback)
{
	callback(rand() % skills.size());
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
void CGeniusAI::battleStackAttacked(BattleStackAttacked * bsa)
{
	MsgBox("\t\t\tCGeniusAI::battleStackAttacked");
}
/**
 * called by engine when battle starts; side=0 - left, side=1 - right
 */
void CGeniusAI::battleStart(CCreatureSet *army1, CCreatureSet *army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, bool side)
{
	assert(!m_battleLogic);
	m_battleLogic = new CBattleLogic(m_cb);
	assert(m_battleLogic);

	MsgBox("** CGeniusAI::battleStart **");
}
/**
 *
 */
void CGeniusAI::battleEnd(BattleResult *br)
{
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
void CGeniusAI::battleStackMoved(int ID, int dest)
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
void CGeniusAI::battleSpellCasted(SpellCasted *sc)
{
	MsgBox("\t\t\tCGeniusAI::battleSpellCasted");
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

/*
ui8 side; //who made this action: false - left, true - right player
	ui32 stackNumber;//stack ID, -1 left hero, -2 right hero,
	ui8 actionType; //    
		0 = Cancel BattleAction   
		1 = Hero cast a spell   
		2 = Walk   
		3 = Defend   
		4 = Retreat from the battle   
		5 = Surrender   
		6 = Walk and Attack   
		7 = Shoot    
		8 = Wait   
		9 = Catapult 
		10 = Monster casts a spell (i.e. Faerie Dragons)
	ui16 destinationTile;
	si32 additionalInfo; // e.g. spell number if type is 1 || 10; tile to attack if type is 6
*/

/**
 *	Implementation of CBattleHelper class.
 */

CBattleHelper::CBattleHelper():
	InfiniteDistance(0xffff),
	BattlefieldHeight(11),
	BattlefieldWidth(15),
	m_voteForDistance(10),
	m_voteForDistanceFromShooters(20),
	m_voteForHitPoints(10),
	m_voteForMaxDamage(10),
	m_voteForMaxSpeed(10),
	m_voteForMinDamage(10)
{
	// loads votes
	std::fstream f;
	f.open("AI\\CBattleHelper.txt", std::ios::in);
	if (f)
	{
		char c_line[512];
		std::string line;
		while (std::getline(f, line, '\n'))
		{
			//f.getline(c_line, sizeof(c_line), '\n');
			//std::string line(c_line);
			//std::getline(f, line);
			std::vector<std::string> parts;
			boost::algorithm::split(parts, line, boost::algorithm::is_any_of("="));
			if (parts.size() >= 2)
			{
				boost::algorithm::trim(parts[0]);
				boost::algorithm::trim(parts[1]);
				if (parts[0].compare("m_voteForDistance") == 0)
				{
					try 
					{
						m_voteForDistance = boost::lexical_cast<int>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				}
				else if (parts[0].compare("m_voteForDistanceFromShooters") == 0)
				{
					try 
					{
						m_voteForDistanceFromShooters = boost::lexical_cast<int>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				}
				else if (parts[0].compare("m_voteForHitPoints") == 0)
				{
					try 
					{
						m_voteForHitPoints = boost::lexical_cast<int>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				}
				else if (parts[0].compare("m_voteForMaxDamage") == 0)
				{
					try 
					{
						m_voteForMaxDamage = boost::lexical_cast<int>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				}
				else if (parts[0].compare("m_voteForMaxSpeed") == 0)
				{
					try 
					{
						m_voteForMaxSpeed = boost::lexical_cast<int>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				}
				else if (parts[0].compare("m_voteForMinDamage") == 0)
				{
					try 
					{
						m_voteForMinDamage = boost::lexical_cast<int>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				}
			}
		}
		f.close();
	}
}

CBattleHelper::~CBattleHelper()
{}

int CBattleHelper::GetBattleFieldPosition(int x, int y)
{
	return x + 17 * (y - 1);
}

int CBattleHelper::DecodeXPosition(int battleFieldPosition)
{
	return battleFieldPosition - (DecodeYPosition(battleFieldPosition) - 1) * 17;
}

int CBattleHelper::DecodeYPosition(int battleFieldPosition)
{
	double y = battleFieldPosition / 17;
	if (y - (int)y > 0.0)
	{
		return (int)y + 1;
	}
	return (int)y;
}


int CBattleHelper::GetShortestDistance(int pointA, int pointB)
{
	/**
	 * TODO: function hasn't been checked!
	 */
	int x1 = DecodeXPosition(pointA);
	int y1 = DecodeYPosition(pointA);
	//
	int x2 = DecodeXPosition(pointB);
	int y2 = DecodeYPosition(pointB);
	//
	double dx = x1 - x2;
	double dy = y1 - y2;

	return (int)sqrt(dx * dx + dy * dy);
}

int CBattleHelper::GetDistanceWithObstacles(int pointA, int pointB)
{
	// TODO - implement this!
	return GetShortestDistance(pointA, pointB);
}

/**
 *	Implementation of CBattleLogic class.
 */

CBattleLogic::CBattleLogic(ICallback *cb) :
	m_cb(cb),
	m_iCurrentTurn(-2),
	m_bIsAttacker(false)
{
	const int max_enemy_creatures = 12;
	m_statMaxDamage.reserve(max_enemy_creatures);
	m_statMinDamage.reserve(max_enemy_creatures);
	
	m_statMaxSpeed.reserve(max_enemy_creatures);
	m_statDistance.reserve(max_enemy_creatures);
	m_statDistanceFromShooters.reserve(max_enemy_creatures);
	m_statHitPoints.reserve(max_enemy_creatures);
}

CBattleLogic::~CBattleLogic()
{}

void CBattleLogic::SetCurrentTurn(int turn)
{
	m_iCurrentTurn = turn;
}

void CBattleLogic::MakeStatistics(int currentCreatureId)
{
	typedef std::map<int, CStack> map_stacks;
	map_stacks allStacks = m_cb->battleGetStacks();
	const CStack *currentStack = m_cb->battleGetStackByID(currentCreatureId);
	/*
	// find all creatures belong to the enemy
	std::for_each(allStacks.begin(), allStacks.end(),
		if_(bind<ui8>(&CStack::attackerOwned, bind<CStack>(&map_stacks::value_type::second, _1)) == m_bIsAttacker)
		[
			var(enemy)[ret<int>(bind<int>(&map_stacks::value_type::first, _1))] = 
				ret<CStack>(bind<CStack>(&map_stacks::value_type::second, _1))
		]
	);
	// fill other containers
	// max damage
	std::for_each(enemy.begin(), enemy.end(),
		var(m_statMaxDamage)[ret<int>(bind<int>(&map_stacks::value_type::first, _1))] =
			ret<int>(bind<int>(&CCreature::damageMax, bind<CCreature*>(&CStack::creature, 
				bind<CStack>(&map_stacks::value_type::second, _1))))
	);
	// min damage
	std::for_each(enemy.begin(), enemy.end(),
		var(m_statMinDamage)[ret<int>(bind<int>(&map_stacks::value_type::first, _1))] =
			ret<int>(bind<int>(&CCreature::damageMax, bind<CCreature*>(&CStack::creature, 
				bind<CStack>(&map_stacks::value_type::second, _1))))
	);
	*/
	m_statMaxDamage.clear();
	m_statMinDamage.clear();
	m_statHitPoints.clear();
	m_statMaxSpeed.clear();
	m_statDistanceFromShooters.clear();
	m_statDistance.clear();
	m_statDistance.clear();

	for (map_stacks::const_iterator it = allStacks.begin(); it != allStacks.end(); ++it)
	{
		if (it->second.attackerOwned != m_bIsAttacker)
		{
			int id = it->first;
			const CStack *st = &it->second;
			if (st->amount < 1)
			{
				continue;
			}
			// make stats
			m_statMaxDamage.push_back(std::pair<int, int>(id, st->creature->damageMax * st->amount));
			m_statMinDamage.push_back(std::pair<int, int>(id, st->creature->damageMin * st->amount));
			m_statHitPoints.push_back(std::pair<int, int>(id, st->creature->hitPoints * st->amount));
			m_statMaxSpeed.push_back(std::pair<int, int>(id, st->creature->speed));
			
			if (st->creature->isShooting() && st->shots > 0)
			{
				m_statDistanceFromShooters.push_back(std::pair<int, int>(id, m_battleHelper.GetShortestDistance(currentStack->position, st->position)));
			}

			if (currentStack->creature->isFlying() || (currentStack->creature->isShooting() && currentStack->shots > 0))
			{
				m_statDistance.push_back(std::pair<int, int>(id, m_battleHelper.GetShortestDistance(currentStack->position, st->position)));
			}
			else
			{
				m_statDistance.push_back(std::pair<int, int>(id, m_battleHelper.GetDistanceWithObstacles(currentStack->position, st->position)));
			}
		}
	}
	// sort max damage
	std::sort(m_statMaxDamage.begin(), m_statMaxDamage.end(), 
		bind(&creature_stat::value_type::second, _1) > bind(&creature_stat::value_type::second, _2));
	// sort min damage
	std::sort(m_statMinDamage.begin(), m_statMinDamage.end(), 
		bind(&creature_stat::value_type::second, _1) > bind(&creature_stat::value_type::second, _2));
	// sort max speed
	std::sort(m_statMaxSpeed.begin(), m_statMaxSpeed.end(), 
		bind(&creature_stat::value_type::second, _1) > bind(&creature_stat::value_type::second, _2));
	// sort distance
	std::sort(m_statDistance.begin(), m_statDistance.end(), 
		bind(&creature_stat::value_type::second, _1) < bind(&creature_stat::value_type::second, _2));
	// sort distance from shooters
	std::sort(m_statDistanceFromShooters.begin(), m_statDistanceFromShooters.end(), 
		bind(&creature_stat::value_type::second, _1) < bind(&creature_stat::value_type::second, _2));
	// sort hit points
	std::sort(m_statHitPoints.begin(), m_statHitPoints.end(), 
		bind(&creature_stat::value_type::second, _1) > bind(&creature_stat::value_type::second, _2));
}

BattleAction CBattleLogic::MakeDecision(int stackID)
{
	MakeStatistics(stackID);
	// first approach based on the statistics and weights
	// if this solution was fine we would develop this idea
	int creature_to_attack = -1;
	//
	std::map<int, int> votes;

	for (creature_stat::iterator it = m_statMaxDamage.begin(); it != m_statMaxDamage.end(); ++it)
	{
		votes[it->first]  = 0;
	}

	votes[m_statMaxDamage.begin()->first] += m_battleHelper.GetVoteForMaxDamage();
	votes[m_statMinDamage.begin()->first] += m_battleHelper.GetVoteForMinDamage();
	if (m_statDistanceFromShooters.size()) 
	{
		votes[m_statDistanceFromShooters.begin()->first] += m_battleHelper.GetVoteForDistanceFromShooters();
	}
	votes[m_statDistance.begin()->first] += m_battleHelper.GetVoteForDistance();
	votes[m_statHitPoints.begin()->first] += m_battleHelper.GetVoteForHitPoints();
	votes[m_statMaxSpeed.begin()->first] += m_battleHelper.GetVoteForMaxSpeed();


	// get creature to attack

	int max_vote = 0;

	for (std::map<int, int>::iterator it = votes.begin(); it != votes.end(); ++it)
	{
		if (it->second > max_vote)
		{
			max_vote = it->second;
			creature_to_attack = it->first;
		}
	}

	if (creature_to_attack == -1)
	{
		// defend
		return MakeDefend(stackID);
	}
	if (m_cb->battleCanShoot(stackID, m_cb->battleGetPos(creature_to_attack)))
	{
		// shoot
		BattleAction ba; 
		ba.side = 1;
		ba.actionType = action_shoot; // shoot
		ba.stackNumber = stackID;
		ba.destinationTile = m_cb->battleGetPos(creature_to_attack);
		return ba;
	}
	else
	{
		// go or go&attack
		int dest_tile = -1; //m_cb->battleGetPos(creature_to_attack) + 1;
		std::vector<int> av_tiles = GetAvailableHexesForAttacker(m_cb->battleGetStackByID(creature_to_attack));
		if (av_tiles.size() < 1)
		{
			// TODO: shouldn't be like that
			return MakeDefend(stackID);
		}
		
		// get the best tile - now the nearest

		int prev_distance = m_battleHelper.InfiniteDistance;

		for (std::vector<int>::iterator it = av_tiles.begin(); it != av_tiles.end(); ++it)
		{
			int dist = m_battleHelper.GetDistanceWithObstacles(*it, m_cb->battleGetPos(stackID));
			if (dist < prev_distance)
			{
				prev_distance = dist;
				dest_tile = *it;
			}
		}

		std::vector<int> fields = m_cb->battleGetAvailableHexes(stackID);
		
		BattleAction ba; 
		ba.side = 1;
		//ba.actionType = 6; // go and attack
		ba.stackNumber = stackID;
		ba.destinationTile = dest_tile;
		ba.additionalInfo = m_cb->battleGetPos(creature_to_attack);
		
		int nearest_dist = m_battleHelper.InfiniteDistance;
		int nearest_pos = -1;

		for (std::vector<int>::const_iterator it = fields.begin(); it != fields.end(); ++it)
		{
			if (*it == dest_tile)
			{
				// attack!
				ba.actionType = action_walk_and_attack;
				return ba;
			}
			int d = m_battleHelper.GetDistanceWithObstacles(dest_tile, *it);
			if (d < nearest_dist)
			{
				nearest_dist = d;
				nearest_pos = *it;
			}
		}
		ba.actionType = action_walk;
		ba.destinationTile = nearest_pos;
		ba.additionalInfo  = -1;
		return ba;
	}
}

std::vector<int> CBattleLogic::GetAvailableHexesForAttacker(CStack *defender)
{
	int x = m_battleHelper.DecodeXPosition(defender->position);
	int y = m_battleHelper.DecodeYPosition(defender->position);
	// TOTO: should be std::vector<int> but for debug purpose std::pair is used
	std::vector<std::pair<int, int> > candidates;
	std::vector<int> fields;
	// find neighbourhood
	bool upLimit = false;
	bool downLimit = false;
	bool leftLimit = false;
	bool rightLimit = false;
	
	if (x == 1)
	{
		leftLimit = true;
	}
	else if (x == m_battleHelper.BattlefieldWidth) // x+ 1
	{
		rightLimit = true;
	}
	
	if (y == 1)
	{
		upLimit = true;
	}
	else if (y == m_battleHelper.BattlefieldHeight)
	{
		downLimit = true;
	}

	if (!downLimit)
	{
		candidates.push_back(std::pair<int, int>(x, y + 1));
	}
	if (!downLimit && !leftLimit)
	{
		candidates.push_back(std::pair<int, int>(x - 1, y + 1));
	}
	if (!downLimit && !rightLimit)
	{
		candidates.push_back(std::pair<int, int>(x + 1, y  + 1));
	}
	if (!upLimit)
	{
		candidates.push_back(std::pair<int, int>(x, y - 1));
	}
	if (!upLimit && !leftLimit)
	{
		candidates.push_back(std::pair<int, int>(x - 1, y - 1));
	}
	if (!upLimit && !rightLimit)
	{
		candidates.push_back(std::pair<int, int>(x + 1, y - 1));
	}
	// check if these field are empty
	for (std::vector<std::pair<int, int> >::iterator it = candidates.begin(); it != candidates.end(); ++it)
	{
		int new_pos = m_battleHelper.GetBattleFieldPosition(it->first, it->second);
		CStack *st = m_cb->battleGetStackByPos(new_pos);
		// int obstacle = m_cb->battleGetObstaclesAtTile(new_pos); // TODO: wait for battleGetObstaclesAtTile function
		if (st == NULL || st->amount < 1)
		{
			fields.push_back(new_pos);
		}
	}
	return fields;
}

BattleAction CBattleLogic::MakeDefend(int stackID)
{
	BattleAction ba; 
	ba.side = 1;
	ba.actionType = action_defend;
	ba.stackNumber = stackID;
	ba.additionalInfo = -1;
	return ba;
}