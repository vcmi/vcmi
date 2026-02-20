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

namespace NK2AI
{

namespace AIPathfinding
{
	bool QuestAction::canAct(const Nullkiller * aiNk, const AIPathNode * node) const
	{
		return canAct(aiNk, node->actor->hero);
	}

	bool QuestAction::canAct(const Nullkiller * aiNk, const AIPathNodeInfo & node) const
	{
		return canAct(aiNk, node.targetHero);
	}

	bool QuestAction::canAct(const Nullkiller * aiNk, const CGHeroInstance * hero) const
	{
		auto object = questInfo.getObject(aiNk->cc.get());
		auto quest = questInfo.getQuest(aiNk->cc.get());
		if(object->ID == Obj::BORDER_GATE || object->ID == Obj::BORDERGUARD)
		{
			return dynamic_cast<const IQuestObject *>(object)->checkQuest(hero);
		}

		auto notActivated = !object->wasVisited(aiNk->playerID)
			&& !quest->activeForPlayers.count(hero->getOwner());
		
		return notActivated
			|| quest->checkQuest(hero);
	}

	Goals::TSubgoal QuestAction::decompose(const Nullkiller * aiNk, const CGHeroInstance * hero) const
	{
		return Goals::sptr(Goals::CompleteQuest(questInfo, *aiNk->cc));
	}

	void QuestAction::execute(AIGateway * aiGw, const CGHeroInstance * hero) const
	{
		aiGw->moveHeroToTile(questInfo.getObject(aiGw->cc.get())->visitablePos(), HeroPtr(hero, aiGw->cc.get()));
	}

	std::string QuestAction::toString() const
	{
		return "Complete Quest";
	}
}

}
