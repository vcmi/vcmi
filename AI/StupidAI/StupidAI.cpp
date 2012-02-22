#include "StdInc.h"
#include "../../lib/AI_Base.h"
#include "StupidAI.h"
#include "../../lib/BattleState.h"
#include "../../CCallback.h"
#include "../../lib/CCreatureHandler.h"

CBattleCallback * cbc;

CStupidAI::CStupidAI(void)
	: side(-1), cb(NULL)
{
	print("created");
}


CStupidAI::~CStupidAI(void)
{
	print("destroyed");
}

void CStupidAI::init( CBattleCallback * CB )
{
	print("init called, saving ptr to IBattleCallback");
	cbc = cb = CB;
}

void CStupidAI::actionFinished( const BattleAction *action )
{
	print("actionFinished called");
}

void CStupidAI::actionStarted( const BattleAction *action )
{
	print("actionStarted called");
}

struct EnemyInfo
{
	const CStack * s;
	int adi, adr;
	std::vector<BattleHex> attackFrom; //for melee fight
	EnemyInfo(const CStack * _s) : s(_s)
	{}
	void calcDmg(const CStack * ourStack)
	{
		TDmgRange retal, dmg = cbc->battleEstimateDamage(ourStack, s, &retal);
		adi = (dmg.first + dmg.second) / 2;
		adr = (retal.first + retal.second) / 2;
	}

	bool operator==(const EnemyInfo& ei) const
	{
		return s == ei.s;
	}
};

bool isMoreProfitable(const EnemyInfo &ei1, const EnemyInfo& ei2) 
{
	return (ei1.adi-ei1.adr) < (ei2.adi - ei2.adr);
}

int distToNearestNeighbour(BattleHex hex, const std::vector<int> & dists, BattleHex *chosenHex = NULL)
{
	int ret = 1000000;
	BOOST_FOREACH(BattleHex n, hex.neighbouringTiles())
	{
		if(dists[n] >= 0 && dists[n] < ret)
		{
			ret = dists[n];
			if(chosenHex)
				*chosenHex = n;
		}
	}

	return ret;
}

bool isCloser(const EnemyInfo & ei1, const EnemyInfo & ei2, const std::vector<int> & dists)
{
	return distToNearestNeighbour(ei1.s->position, dists) < distToNearestNeighbour(ei2.s->position, dists);
}

static bool willSecondHexBlockMoreEnemyShooters(const BattleHex &h1, const BattleHex &h2)
{
	int shooters[2] = {0}; //count of shooters on hexes

	for(int i = 0; i < 2; i++)
		BOOST_FOREACH(BattleHex neighbour, (i ? h2 : h1).neighbouringTiles())
			if(const CStack *s = cbc->battleGetStackByPos(neighbour))
				if(s->getCreature()->isShooting())
						shooters[i]++;

	return shooters[0] < shooters[1];
}

BattleAction CStupidAI::activeStack( const CStack * stack )
{
	//boost::this_thread::sleep(boost::posix_time::seconds(2));
	print("activeStack called");
	std::vector<BattleHex> avHexes = cb->battleGetAvailableHexes(stack, false);
	std::vector<int> dists = cb->battleGetDistances(stack);
	std::vector<EnemyInfo> enemiesShootable, enemiesReachable, enemiesUnreachable;

	if(stack->type->idNumber == 145) //catapult
	{
		BattleAction attack;
		static const int wallHexes[] = {50, 183, 182, 130, 62, 29, 12, 95};
		attack.destinationTile = wallHexes[ rand()%ARRAY_COUNT(wallHexes) ];
		attack.actionType = BattleAction::CATAPULT;
		attack.additionalInfo = 0;
		attack.side = side;
		attack.stackNumber = stack->ID;

		return attack;
	}

	BOOST_FOREACH(const CStack *s, cb->battleGetStacks(CBattleCallback::ONLY_ENEMY))
	{
		if(cb->battleCanShoot(stack, s->position))
		{
			enemiesShootable.push_back(s);
		}
		else
		{
			BOOST_FOREACH(BattleHex hex, avHexes)
			{
				if(CStack::isMeleeAttackPossible(stack, s, hex))
				{
					std::vector<EnemyInfo>::iterator i = std::find(enemiesReachable.begin(), enemiesReachable.end(), s);
					if(i == enemiesReachable.end())
					{
						enemiesReachable.push_back(s);
						i = enemiesReachable.begin() + (enemiesReachable.size() - 1);
					}

					i->attackFrom.push_back(hex);
				}
			}

			if(!vstd::contains(enemiesReachable, s) && s->position.isValid())
				enemiesUnreachable.push_back(s);
		}
	}

	if(enemiesShootable.size())
	{
		const EnemyInfo &ei= *std::max_element(enemiesShootable.begin(), enemiesShootable.end(), isMoreProfitable);
		return BattleAction::makeShotAttack(stack, ei.s);
	}
	else if(enemiesReachable.size())
	{
		const EnemyInfo &ei= *std::max_element(enemiesReachable.begin(), enemiesReachable.end(), &isMoreProfitable);
		return BattleAction::makeMeleeAttack(stack, ei.s, *std::max_element(ei.attackFrom.begin(), ei.attackFrom.end(), &willSecondHexBlockMoreEnemyShooters));
	}
	else
	{
		const EnemyInfo &ei= *std::min_element(enemiesUnreachable.begin(), enemiesUnreachable.end(), boost::bind(isCloser, _1, _2, boost::ref(dists)));
		if(distToNearestNeighbour(ei.s->position, dists) < GameConstants::BFIELD_SIZE)
		{
			return goTowards(stack, ei.s->position);
		}
	}

	return BattleAction::makeDefend(stack);
}

void CStupidAI::battleAttack(const BattleAttack *ba)
{
	print("battleAttack called");
}

void CStupidAI::battleStacksAttacked(const std::vector<BattleStackAttacked> & bsa)
{
	print("battleStacksAttacked called");
}

void CStupidAI::battleEnd(const BattleResult *br) 
{
	print("battleEnd called");
}

// void CStupidAI::battleResultsApplied() 
// {
// 	print("battleResultsApplied called");
// }

void CStupidAI::battleNewRoundFirst(int round) 
{
	print("battleNewRoundFirst called");
}

void CStupidAI::battleNewRound(int round) 
{
	print("battleNewRound called");
}

void CStupidAI::battleStackMoved(const CStack * stack, std::vector<BattleHex> dest, int distance) 
{
	print("battleStackMoved called");;
}

void CStupidAI::battleSpellCast(const BattleSpellCast *sc) 
{
	print("battleSpellCast called");
}

void CStupidAI::battleStacksEffectsSet(const SetStackEffect & sse) 
{
	print("battleStacksEffectsSet called");
}

void CStupidAI::battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool Side) 
{
	print("battleStart called");
	side = Side;
}

void CStupidAI::battleStacksHealedRes(const std::vector<std::pair<ui32, ui32> > & healedStacks, bool lifeDrain, bool tentHeal, si32 lifeDrainFrom) 
{
	print("battleStacksHealedRes called");
}

void CStupidAI::battleNewStackAppeared(const CStack * stack) 
{
	print("battleNewStackAppeared called");
}

void CStupidAI::battleObstaclesRemoved(const std::set<si32> & removedObstacles) 
{
	print("battleObstaclesRemoved called");
}

void CStupidAI::battleCatapultAttacked(const CatapultAttack & ca) 
{
	print("battleCatapultAttacked called");
}

void CStupidAI::battleStacksRemoved(const BattleStacksRemoved & bsr) 
{
	print("battleStacksRemoved called");
}

void CStupidAI::print(const std::string &text) const
{
	tlog6 << "CStupidAI [" << this <<"]: " << text << std::endl;
}

BattleAction CStupidAI::goTowards(const CStack * stack, BattleHex hex)
{
	BattleHex realDest = hex;
	BattleHex predecessors[GameConstants::BFIELD_SIZE];
	std::vector<int> dists = cb->battleGetDistances(stack, hex);
	if(distToNearestNeighbour(hex, dists, &realDest) > GameConstants::BFIELD_SIZE)
	{
		print("goTowards: Cannot reach");
		return BattleAction::makeDefend(stack);
	}

	dists = cb->battleGetDistances(stack, realDest, predecessors);
	std::vector<BattleHex> avHexes = cb->battleGetAvailableHexes(stack, false);

	if(!avHexes.size())
	{
		print("goTowards: Stack cannot move! That's " + stack->nodeName());
		return BattleAction::makeDefend(stack);
	}

	while(1)
	{
		assert(realDest.isValid());
		if(vstd::contains(avHexes, hex))
			return BattleAction::makeMove(stack, hex);

		hex = predecessors[hex];
	}
}

