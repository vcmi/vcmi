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
#include "../../VCAI.h"
#include "../../Behaviors/CompleteQuestBehavior.h"
#include "../../../../lib/mapping/CMap.h" //for victory conditions

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;

namespace AIPathfinding
{
	void BattleAction::execute(const CGHeroInstance * hero) const
	{
		ai->moveHeroToTile(targetTile, hero);
	}

	std::string BattleAction::toString() const
	{
		return "Battle at " + targetTile.toString();
	}

	bool QuestAction::canAct(const AIPathNode * node) const
	{
		QuestInfo q = questInfo;
		return q.quest->checkQuest(node->actor->hero);
	}

	Goals::TSubgoal QuestAction::decompose(const CGHeroInstance * hero) const
	{
		return Goals::sptr(Goals::Invalid());
	}

	void QuestAction::execute(const CGHeroInstance * hero) const
	{
		ai->moveHeroToTile(questInfo.obj->visitablePos(), hero);
	}

	std::string QuestAction::toString() const
	{
		return "Complete Quest";
	}
}