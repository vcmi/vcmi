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

static std::shared_ptr<CBattleCallback> cbc;

CStupidAI::CStupidAI()
	: side(-1)
{
	print("created");
}


CStupidAI::~CStupidAI()
{
	print("destroyed");
}

void CStupidAI::init(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB)
{
	print("init called, saving ptr to IBattleCallback");
	env = ENV;
	cbc = cb = CB;
}

void CStupidAI::actionFinished(const BattleAction &action)
{
	print("actionFinished called");
}

void CStupidAI::actionStarted(const BattleAction &action)
{
	print("actionStarted called");
}

struct EnemyInfo
{
	const CStack * s;
	int adi, adr;
	std::vector<BattleHex> attackFrom; //for melee fight
	EnemyInfo(const CStack * _s) : s(_s), adi(0), adr(0)
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

namespace {

int distToNearestNeighbour(BattleHex hex, const ReachabilityInfo::TDistances& dists, BattleHex *chosenHex = nullptr)
{
	int ret = 1000000;
	for(auto & n: hex.neighbouringTiles())
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

bool isCloser(const EnemyInfo & ei1, const EnemyInfo & ei2, const ReachabilityInfo::TDistances & dists)
{
	return distToNearestNeighbour(ei1.s->getPosition(), dists) < distToNearestNeighbour(ei2.s->getPosition(), dists);
}

}

static bool willSecondHexBlockMoreEnemyShooters(const BattleHex &h1, const BattleHex &h2)
{
	int shooters[2] = {0}; //count of shooters on hexes

	for(int i = 0; i < 2; i++)
		for (auto & neighbour : (i ? h2 : h1).neighbouringTiles())
			if(const CStack *s = cbc->battleGetStackByPos(neighbour))
				if(s->getCreature()->isShooting())
						shooters[i]++;

	return shooters[0] < shooters[1];
}

BattleAction CStupidAI::activeStack( const CStack * stack )
{
	//boost::this_thread::sleep(boost::posix_time::seconds(2));
	print("activeStack called for " + stack->nodeName());
	auto dists = cb->battleGetDistances(stack, stack->getPosition());
	std::vector<EnemyInfo> enemiesShootable, enemiesReachable, enemiesUnreachable;

	if(stack->type->idNumber == CreatureID::CATAPULT)
	{
		BattleAction attack;
		static const std::vector<int> wallHexes = {50, 183, 182, 130, 78, 29, 12, 95};
		auto seletectedHex = *RandomGeneratorUtil::nextItem(wallHexes, CRandomGenerator::getDefault());
		attack.aimToHex(seletectedHex);
		attack.actionType = EActionType::CATAPULT;
		attack.side = side;
		attack.stackNumber = stack->ID;

		return attack;
	}
	else if(stack->hasBonusOfType(Bonus::SIEGE_WEAPON))
	{
		return BattleAction::makeDefend(stack);
	}

	for (const CStack *s : cb->battleGetStacks(CBattleCallback::ONLY_ENEMY))
	{
		if(cb->battleCanShoot(stack, s->getPosition()))
		{
			enemiesShootable.push_back(s);
		}
		else
		{
			std::vector<BattleHex> avHexes = cb->battleGetAvailableHexes(stack);

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
		enemy.calcDmg( stack );

	for ( auto & enemy : enemiesShootable )
		enemy.calcDmg( stack );

	if(enemiesShootable.size())
	{
		const EnemyInfo &ei= *std::max_element(enemiesShootable.begin(), enemiesShootable.end(), isMoreProfitable);
		return BattleAction::makeShotAttack(stack, ei.s);
	}
	else if(enemiesReachable.size())
	{
		const EnemyInfo &ei= *std::max_element(enemiesReachable.begin(), enemiesReachable.end(), &isMoreProfitable);
		return BattleAction::makeMeleeAttack(stack, ei.s->getPosition(), *std::max_element(ei.attackFrom.begin(), ei.attackFrom.end(), &willSecondHexBlockMoreEnemyShooters));
	}
	else if(enemiesUnreachable.size()) //due to #955 - a buggy battle may occur when there are no enemies
	{
		assert(enemiesUnreachable.size());
		const EnemyInfo &ei= *std::min_element(enemiesUnreachable.begin(), enemiesUnreachable.end(), std::bind(isCloser, _1, _2, std::ref(dists)));
		assert(ei.s);
		if(distToNearestNeighbour(ei.s->getPosition(), dists) < GameConstants::BFIELD_SIZE)
		{
			return goTowards(stack, ei.s->getPosition());
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
	print("battleStackMoved called");
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

void CStupidAI::battleCatapultAttacked(const CatapultAttack & ca)
{
	print("battleCatapultAttacked called");
}

void CStupidAI::print(const std::string &text) const
{
	logAi->trace("CStupidAI  [%p]: %s", this, text);
}

BattleAction CStupidAI::goTowards(const CStack * stack, BattleHex destination)
{
	assert(destination.isValid());
	auto reachability = cb->getReachability(stack);
	auto avHexes = cb->battleGetAvailableHexes(reachability, stack);

	if(vstd::contains(avHexes, destination))
		return BattleAction::makeMove(stack, destination);

	auto destNeighbours = destination.neighbouringTiles();
	if(vstd::contains_if(destNeighbours, [&](BattleHex n) { return stack->coversPos(destination); }))
	{
		logAi->warn("Warning: already standing on neighbouring tile!");
		//We shouldn't even be here...
		return BattleAction::makeDefend(stack);
	}

	vstd::erase_if(destNeighbours, [&](BattleHex hex){ return !reachability.accessibility.accessible(hex, stack); });

	if(!avHexes.size() || !destNeighbours.size()) //we are blocked or dest is blocked
	{
		print("goTowards: Stack cannot move! That's " + stack->nodeName());
		return BattleAction::makeDefend(stack);
	}

	if(stack->hasBonusOfType(Bonus::FLYING))
	{
		// Flying stack doesn't go hex by hex, so we can't backtrack using predecessors.
		// We just check all available hexes and pick the one closest to the target.
		auto distToDestNeighbour = [&](BattleHex hex) -> int
		{
			auto nearestNeighbourToHex = vstd::minElementByFun(destNeighbours, [&](BattleHex a)
			{
				return BattleHex::getDistance(a, hex);
			});

			return BattleHex::getDistance(*nearestNeighbourToHex, hex);
		};

		auto nearestAvailableHex = vstd::minElementByFun(avHexes, distToDestNeighbour);
		return BattleAction::makeMove(stack, *nearestAvailableHex);
	}
	else
	{
		BattleHex bestNeighbor = destination;
		if(distToNearestNeighbour(destination, reachability.distances, &bestNeighbor) > GameConstants::BFIELD_SIZE)
		{
			print("goTowards: Cannot reach");
			return BattleAction::makeDefend(stack);
		}

		BattleHex currentDest = bestNeighbor;
		while(1)
		{
			assert(currentDest.isValid());
			if(vstd::contains(avHexes, currentDest))
				return BattleAction::makeMove(stack, currentDest);

			currentDest = reachability.predecessors[currentDest];
		}
	}
}
