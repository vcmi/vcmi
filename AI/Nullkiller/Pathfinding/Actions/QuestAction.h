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

namespace NKAI
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

		virtual bool canAct(const AIPathNode * node) const override;

		virtual Goals::TSubgoal decompose(const CGHeroInstance * hero) const override;

		virtual void execute(const CGHeroInstance * hero) const override;

		virtual std::string toString() const override;
	};
}

}
