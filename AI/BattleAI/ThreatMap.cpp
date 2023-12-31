/*
 * ThreatMap.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
/*
#include "ThreatMap.h"
#include "StdInc.h"

template <typename Container, typename Pred>
auto sum(const Container & c, Pred p) -> decltype(p(*std::begin(c)))
{
	double ret = 0;
	for(const auto &element : c)
	{
		ret += p(element);
	}

	return ret;
}
ThreatMap::ThreatMap(const CStack *Endangered) : endangered(Endangered)
{
	sufferedDamage.fill(0);

	for(const CStack *enemy : getCbc()->battleGetStacks())
	{
		//Consider only stacks of different owner
		if(enemy->unitSide() == endangered->unitSide())
			continue;

		//Look-up which tiles can be melee-attacked
		std::array<bool, GameConstants::BFIELD_SIZE> meleeAttackable;
		meleeAttackable.fill(false);
		auto enemyReachability = getCbc()->getReachability(enemy);
		for(int i = 0; i < GameConstants::BFIELD_SIZE; i++)
		{
			if(enemyReachability.isReachable(i))
			{
				meleeAttackable[i] = true;
				for(auto n : BattleHex(i).neighbouringTiles())
					meleeAttackable[n] = true;
			}
		}

		//Gather possible assaults
		for(int i = 0; i < GameConstants::BFIELD_SIZE; i++)
		{
			if(getCbc()->battleCanShoot(enemy, i))
				threatMap[i].push_back(BattleAttackInfo(enemy, endangered, true));
			else if(meleeAttackable[i])
			{
				BattleAttackInfo bai(enemy, endangered, false);
				bai.chargedFields = std::max(BattleHex::getDistance(enemy->position, i) - 1, 0); //TODO check real distance (BFS), not just metric
				threatMap[i].push_back(BattleAttackInfo(bai));
			}
		}
	}

	for(int i = 0; i < GameConstants::BFIELD_SIZE; i++)
	{
		sufferedDamage[i] = sum(threatMap[i], [](const BattleAttackInfo &bai) -> int
		{
			auto dmg = getCbc()->calculateDmgRange(bai);
			return (dmg.first + dmg.second)/2;
		});
	}
}
*/ // These lines may be usefull but they are't used in the code.
