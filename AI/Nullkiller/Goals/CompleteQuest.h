/*
* CompleteQuestBehavior.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "../AIUtility.h"
#include "../../../CCallback.h"
#include "../Goals/CGoal.h"
#include "../../../lib/CGameState.h"

namespace NKAI
{
namespace Goals
{
	class CompleteQuest : public CGoal<CompleteQuest>
	{
	private:
		QuestInfo q;

	public:
		CompleteQuest(const QuestInfo & quest)
			:CGoal(COMPLETE_QUEST), q(quest)
		{
		}

		virtual Goals::TGoalVec decompose() const override;
		virtual std::string toString() const override;
		virtual bool hasHash() const override { return true; }
		virtual uint64_t getHash() const override;

		virtual bool operator==(const CompleteQuest & other) const override;

	private:
		TGoalVec tryCompleteQuest() const;
		TGoalVec missionArt() const;
		TGoalVec missionHero() const;
		TGoalVec missionArmy() const;
		TGoalVec missionResources() const;
		TGoalVec missionDestroyObj() const;
		TGoalVec missionIncreasePrimaryStat() const;
		TGoalVec missionLevel() const;
		TGoalVec missionKeymaster() const;
		std::string questToString() const;
	};
}

}
