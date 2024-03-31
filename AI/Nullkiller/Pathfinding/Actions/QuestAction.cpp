/*
* QuestAction.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "QuestAction.h"
#include "../../AIGateway.h"
#include "../../Goals/CompleteQuest.h"

namespace NKAI
{

namespace AIPathfinding
{
	bool QuestAction::canAct(const Nullkiller * ai, const AIPathNode * node) const
	{
		return canAct(ai, node->actor->hero);
	}

	bool QuestAction::canAct(const Nullkiller * ai, const AIPathNodeInfo & node) const
	{
		return canAct(ai, node.targetHero);
	}

	bool QuestAction::canAct(const Nullkiller * ai, const CGHeroInstance * hero) const
	{
		if(questInfo.obj->ID == Obj::BORDER_GATE || questInfo.obj->ID == Obj::BORDERGUARD)
		{
			return dynamic_cast<const IQuestObject *>(questInfo.obj)->checkQuest(hero);
		}

		return questInfo.quest->activeForPlayers.count(hero->getOwner())
			|| questInfo.quest->checkQuest(hero);
	}

	Goals::TSubgoal QuestAction::decompose(const Nullkiller * ai, const CGHeroInstance * hero) const
	{
		return Goals::sptr(Goals::CompleteQuest(questInfo));
	}

	void QuestAction::execute(AIGateway * ai, const CGHeroInstance * hero) const
	{
		ai->moveHeroToTile(questInfo.obj->visitablePos(), hero);
	}

	std::string QuestAction::toString() const
	{
		return "Complete Quest";
	}
}

}
