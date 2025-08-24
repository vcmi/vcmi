/*
* QuestAction.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "SpecialAction.h"
#include "../../../../lib/gameState/QuestInfo.h"

namespace NK2AI
{
namespace AIPathfinding
{
	class QuestAction : public SpecialAction
	{
	private:
		QuestInfo questInfo;

	public:
		QuestAction(QuestInfo questInfo)
			:questInfo(questInfo)
		{
		}

		bool canAct(const Nullkiller * aiNk, const AIPathNode * node) const override;
		bool canAct(const Nullkiller * aiNk, const AIPathNodeInfo & node) const override;
		bool canAct(const Nullkiller * aiNk, const CGHeroInstance * hero) const;

		Goals::TSubgoal decompose(const Nullkiller * aiNk, const CGHeroInstance * hero) const override;

		void execute(AIGateway * aiGw, const CGHeroInstance * hero) const override;

		std::string toString() const override;
	};
}

}
