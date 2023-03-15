/*
* BattleAction.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "BuyArmyAction.h"
#include "../../AIGateway.h"
#include "../../Goals/CompleteQuest.h"
#include "../../../../lib/mapping/CMap.h" //for victory conditions

namespace NKAI
{

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<AIGateway> ai;

namespace AIPathfinding
{
	void BuyArmyAction::execute(const CGHeroInstance * hero) const
	{
		if(!hero->visitedTown)
		{
			throw cannotFulfillGoalException(
				hero->getNameTranslated() + " being at " + hero->visitablePos().toString() + " has no town to recruit creatures.");
		}

		ai->recruitCreatures(hero->visitedTown, hero);
	}

	std::string BuyArmyAction::toString() const
	{
		return "Buy Army";
	}
}

}
