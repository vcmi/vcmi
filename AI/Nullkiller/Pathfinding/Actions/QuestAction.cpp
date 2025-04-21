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
#include "../../../../lib/mapObjects/CQuest.h"

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
		auto object = questInfo.getObject(ai->cb.get());
		auto quest = questInfo.getQuest(ai->cb.get());
		if(object->ID == Obj::BORDER_GATE || object->ID == Obj::BORDERGUARD)
		{
			return dynamic_cast<const IQuestObject *>(object)->checkQuest(hero);
		}

		auto notActivated = !object->wasVisited(ai->playerID)
			&& !quest->activeForPlayers.count(hero->getOwner());
		
		return notActivated
			|| quest->checkQuest(hero);
	}

	Goals::TSubgoal QuestAction::decompose(const Nullkiller * ai, const CGHeroInstance * hero) const
	{
		return Goals::sptr(Goals::CompleteQuest(questInfo));
	}

	void QuestAction::execute(AIGateway * ai, const CGHeroInstance * hero) const
	{
		ai->moveHeroToTile(questInfo.getObject(ai->myCb.get())->visitablePos(), hero);
	}

	std::string QuestAction::toString() const
	{
		return "Complete Quest";
	}
}

}
