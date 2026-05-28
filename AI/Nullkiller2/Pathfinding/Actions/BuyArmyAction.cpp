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

namespace NK2AI
{

namespace AIPathfinding
{
	void BuyArmyAction::execute(AIGateway * aiGw, const CGHeroInstance * hero) const
	{
		if(!hero->getVisitedTown())
		{
			throw cannotFulfillGoalException(
				hero->getNameTranslated() + " being at " + hero->visitablePos().toString() + " has no town to recruit creatures.");
		}

		aiGw->recruitCreatures(hero->getVisitedTown(), hero);
	}

	std::string BuyArmyAction::toString() const
	{
		return "Buy Army";
	}
}

}
