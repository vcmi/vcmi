/*
 * StupidAI.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "../../lib/AI_Base.h"
#include "StupidAI.h"
#include "../../lib/CStack.h"
#include "../../CCallback.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/battle/BattleAction.h"
#include "../../lib/battle/BattleInfo.h"

static std::shared_ptr<CBattleCallback> cbc;

CStupidAI::CStupidAI()
	: side(-1)
	, wasWaitingForRealize(false)
	, wasUnlockingGs(false)
{
	print("created");
}

CStupidAI::~CStupidAI()
{
	print("destroyed");
	if(cb)
	{
		//Restore previous state of CB - it may be shared with the main AI (like VCAI)
		cb->waitTillRealize = wasWaitingForRealize;
		cb->unlockGsWhenWaiting = wasUnlockingGs;
	}
}

void CStupidAI::initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB)
{
	print("init called, saving ptr to IBattleCallback");
	env = ENV;
	cbc = cb = CB;

	wasWaitingForRealize = CB->waitTillRealize;
	wasUnlockingGs = CB->unlockGsWhenWaiting;
	CB->waitTillRealize = false;
	CB->unlockGsWhenWaiting = false;
}

void CStupidAI::initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB, AutocombatPreferences autocombatPreferences)
{
	initBattleInterface(ENV, CB);
}

void CStupidAI::actionFinished(const BattleID & battleID, const BattleAction &action)
{
	print("actionFinished called");
}

void CStupidAI::actionStarted(const BattleID & battleID, const BattleAction &action)
{
	print("actionStarted called");
}

class EnemyInfo
{
public:
	const CStack * s;
	int adi, adr;
	std::vector<BattleHex> attackFrom; //for melee fight
	EnemyInfo(const CStack * _s) : s(_s), adi(0), adr(0)
	{}
	void calcDmg(const BattleID & battleID, const CStack * ourStack)
	{
		// FIXME: provide distance info for Jousting bonus
		DamageEstimation retal;
		DamageEstimation dmg = cbc->getBattle(battleID)->battleEstimateDamage(ourStack, s, 0, &retal);
		adi = static_cast<int>((dmg.damage.min + dmg.damage.max) / 2);
		adr = static_cast<int>((retal.damage.min + retal.damage.max) / 2);
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

static bool willSecondHexBlockMoreEnemyShooters(const BattleID & battleID, const BattleHex &h1, const BattleHex &h2)
{
	int shooters[2] = {0}; //count of shooters on hexes

	for(int i = 0; i < 2; i++)
	{
		for (auto & neighbour : (i ? h2 : h1).neighbouringTiles())
			if(const auto * s = cbc->getBattle(battleID)->battleGetUnitByPos(neighbour))
				if(s->isShooter())
					shooters[i]++;
	}

	return shooters[0] < shooters[1];
}

void CStupidAI::yourTacticPhase(const BattleID & battleID, int distance)
{
	cb->battleMakeTacticAction(battleID, BattleAction::makeEndOFTacticPhase(cb->getBattle(battleID)->battleGetTacticsSide()));
}

void CStupidAI::activeStack(const BattleID & battleID, const CStack * stack)
{
	//boost::this_thread::sleep_for(boost::chrono::seconds(2));
	print("activeStack called for " + stack->nodeName());
	ReachabilityInfo dists = cb->getBattle(battleID)->getReachability(stack);
	std::vector<EnemyInfo> enemiesShootable, enemiesReachable, enemiesUnreachable;

	if(stack->creatureId() == CreatureID::CATAPULT)
	{
		BattleAction attack;
		static const std::vector<int> wallHexes = {50, 183, 182, 130, 78, 29, 12, 95};
		auto seletectedHex = *RandomGeneratorUtil::nextItem(wallHexes, CRandomGenerator::getDefault());
		attack.aimToHex(seletectedHex);
		attack.actionType = EActionType::CATAPULT;
		attack.side = side;
		attack.stackNumber = stack->unitId();

		cb->battleMakeUnitAction(battleID, attack);
		return;
	}
	else if(stack->hasBonusOfType(BonusType::SIEGE_WEAPON))
	{
		cb->battleMakeUnitAction(battleID, BattleAction::makeDefend(stack));
		return;
	}

	for (const CStack *s : cb->getBattle(battleID)->battleGetStacks(CBattleInfoEssentials::ONLY_ENEMY))
	{
		if(cb->getBattle(battleID)->battleCanShoot(stack, s->getPosition()))
		{
			enemiesShootable.push_back(s);
		}
		else
		{
			std::vector<BattleHex> avHexes = cb->getBattle(battleID)->battleGetAvailableHexes(stack, false);

			for (BattleHex hex : avHexes)
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

			if(!vstd::contains(enemiesReachable, s) && s->getPosition().isValid())
				enemiesUnreachable.push_back(s);
		}
	}

	for ( auto & enemy : enemiesReachable )
		enemy.calcDmg(battleID, stack);

	for ( auto & enemy : enemiesShootable )
		enemy.calcDmg(battleID, stack);

	if(enemiesShootable.size())
	{
		const EnemyInfo &ei= *std::max_element(enemiesShootable.begin(), enemiesShootable.end(), isMoreProfitable);
		cb->battleMakeUnitAction(battleID, BattleAction::makeShotAttack(stack, ei.s));
		return;
	}
	else if(enemiesReachable.size())
	{
		const EnemyInfo &ei= *std::max_element(enemiesReachable.begin(), enemiesReachable.end(), &isMoreProfitable);
		BattleHex targetHex = *std::max_element(ei.attackFrom.begin(), ei.attackFrom.end(), [&](auto a, auto b) { return willSecondHexBlockMoreEnemyShooters(battleID, a, b);});

		cb->battleMakeUnitAction(battleID, BattleAction::makeMeleeAttack(stack, ei.s->getPosition(), targetHex));
		return;
	}
	else if(enemiesUnreachable.size()) //due to #955 - a buggy battle may occur when there are no enemies
	{
		auto closestEnemy = vstd::minElementByFun(enemiesUnreachable, [&](const EnemyInfo & ei) -> int
		{
			return dists.distToNearestNeighbour(stack, ei.s);
		});

		if(dists.distToNearestNeighbour(stack, closestEnemy->s) < GameConstants::BFIELD_SIZE)
		{
			cb->battleMakeUnitAction(battleID, goTowards(battleID, stack, closestEnemy->s->getAttackableHexes(stack)));
			return;
		}
	}

	cb->battleMakeUnitAction(battleID, BattleAction::makeDefend(stack));
	return;
}

void CStupidAI::battleAttack(const BattleID & battleID, const BattleAttack *ba)
{
	print("battleAttack called");
}

void CStupidAI::battleStacksAttacked(const BattleID & battleID, const std::vector<BattleStackAttacked> & bsa, bool ranged)
{
	print("battleStacksAttacked called");
}

void CStupidAI::battleEnd(const BattleID & battleID, const BattleResult *br, QueryID queryID)
{
	print("battleEnd called");
}

// void CStupidAI::battleResultsApplied()
// {
// 	print("battleResultsApplied called");
// }

void CStupidAI::battleNewRoundFirst(const BattleID & battleID)
{
	print("battleNewRoundFirst called");
}

void CStupidAI::battleNewRound(const BattleID & battleID)
{
	print("battleNewRound called");
}

void CStupidAI::battleStackMoved(const BattleID & battleID, const CStack * stack, std::vector<BattleHex> dest, int distance, bool teleport)
{
	print("battleStackMoved called");
}

void CStupidAI::battleSpellCast(const BattleID & battleID, const BattleSpellCast *sc)
{
	print("battleSpellCast called");
}

void CStupidAI::battleStacksEffectsSet(const BattleID & battleID, const SetStackEffect & sse)
{
	print("battleStacksEffectsSet called");
}

void CStupidAI::battleStart(const BattleID & battleID, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool Side, bool replayAllowed)
{
	print("battleStart called");
	side = Side;
}

void CStupidAI::battleCatapultAttacked(const BattleID & battleID, const CatapultAttack & ca)
{
	print("battleCatapultAttacked called");
}

void CStupidAI::print(const std::string &text) const
{
	logAi->trace("CStupidAI  [%p]: %s", this, text);
}

BattleAction CStupidAI::goTowards(const BattleID & battleID, const CStack * stack, std::vector<BattleHex> hexes) const
{
	auto reachability = cb->getBattle(battleID)->getReachability(stack);
	auto avHexes = cb->getBattle(battleID)->battleGetAvailableHexes(reachability, stack, false);

	if(!avHexes.size() || !hexes.size()) //we are blocked or dest is blocked
	{
		return BattleAction::makeDefend(stack);
	}

	std::sort(hexes.begin(), hexes.end(), [&](BattleHex h1, BattleHex h2) -> bool
	{
		return reachability.distances[h1] < reachability.distances[h2];
	});

	for(auto hex : hexes)
	{
		if(vstd::contains(avHexes, hex))
			return BattleAction::makeMove(stack, hex);

		if(stack->coversPos(hex))
		{
			logAi->warn("Warning: already standing on neighbouring tile!");
			//We shouldn't even be here...
			return BattleAction::makeDefend(stack);
		}
	}

	BattleHex bestNeighbor = hexes.front();

	if(reachability.distances[bestNeighbor] > GameConstants::BFIELD_SIZE)
	{
		return BattleAction::makeDefend(stack);
	}

	if(stack->hasBonusOfType(BonusType::FLYING))
	{
		// Flying stack doesn't go hex by hex, so we can't backtrack using predecessors.
		// We just check all available hexes and pick the one closest to the target.
		auto nearestAvailableHex = vstd::minElementByFun(avHexes, [&](BattleHex hex) -> int
		{
			return BattleHex::getDistance(bestNeighbor, hex);
		});

		return BattleAction::makeMove(stack, *nearestAvailableHex);
	}
	else
	{
		BattleHex currentDest = bestNeighbor;
		while(1)
		{
			if(!currentDest.isValid())
			{
				logAi->error("CBattleAI::goTowards: internal error");
				return BattleAction::makeDefend(stack);
			}

			if(vstd::contains(avHexes, currentDest))
				return BattleAction::makeMove(stack, currentDest);

			currentDest = reachability.predecessors[currentDest];
		}
	}
}
