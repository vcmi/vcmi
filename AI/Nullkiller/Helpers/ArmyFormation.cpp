/*
* ArmyFormation.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "ArmyFormation.h"
#include "../../../lib/mapObjects/CGTownInstance.h"

namespace NKAI
{

void ArmyFormation::rearrangeArmyForWhirlpool(const CGHeroInstance * hero)
{
	addSingleCreatureStacks(hero);
}

void ArmyFormation::addSingleCreatureStacks(const CGHeroInstance * hero)
{
	auto freeSlots = hero->getFreeSlots();

	while(!freeSlots.empty())
	{
		TSlots::const_iterator weakestCreature = vstd::minElementByFun(hero->Slots(), [](const auto & slot) -> int
			{
				return slot.second->getCount() == 1
					? std::numeric_limits<int>::max()
					: slot.second->getCreatureID().toCreature()->getAIValue();
			});

		if(weakestCreature == hero->Slots().end() || weakestCreature->second->getCount() == 1)
		{
			break;
		}

		cb->splitStack(hero, hero, weakestCreature->first, freeSlots.back(), 1);
		freeSlots.pop_back();
	}
}

void ArmyFormation::rearrangeArmyForSiege(const CGTownInstance * town, const CGHeroInstance * attacker)
{
	addSingleCreatureStacks(attacker);

	if(town->fortLevel() > CGTownInstance::FORT)
	{
		std::vector<const CStackInstance *> stacks;

		for(const auto & slot : attacker->Slots())
			stacks.push_back(slot.second.get());

		boost::sort(
			stacks,
			[](const CStackInstance * slot1, const CStackInstance * slot2) -> bool
			{
				auto cre1 = slot1->getCreatureID().toCreature();
				auto cre2 = slot2->getCreatureID().toCreature();
				auto flying = cre1->hasBonusOfType(BonusType::FLYING) - cre2->hasBonusOfType(BonusType::FLYING);
			
				if(flying != 0) return flying < 0;
				else return cre1->getAIValue() < cre2->getAIValue();
			});

		for(int i = 0; i < stacks.size(); i++)
		{
			auto pos = stacks[i]->getArmy()->findStack(stacks[i]);

			if(pos.getNum() != i)
				cb->swapCreatures(attacker, attacker, static_cast<SlotID>(i), pos);
		}
	}
}

}
