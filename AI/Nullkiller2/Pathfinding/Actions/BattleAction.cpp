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
#include "BattleAction.h"
#include "../../AIGateway.h"
#include "../../Goals/CompleteQuest.h"

namespace NK2AI
{

namespace AIPathfinding
{
	void BattleAction::execute(AIGateway * aiGw, const CGHeroInstance * hero) const
	{
		aiGw->moveHeroToTile(targetTile, HeroPtr(hero, aiGw->cc.get()));
	}

	std::string BattleAction::toString() const
	{
		return "Battle at " + targetTile.toString();
	}
}

}
