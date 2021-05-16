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
#include "../../../lib/VCMI_Lib.h"
#include "../../../CCallback.h"
#include "../Goals/CGoal.h"

namespace Goals
{
	class CompleteQuestBehavior : public CGoal<CompleteQuestBehavior>
	{
	public:
		CompleteQuestBehavior()
			:CGoal(COMPLETE_QUEST)
		{
		}

		virtual Goals::TGoalVec decompose() const override;
		virtual std::string toString() const override;

		virtual bool operator==(const CompleteQuestBehavior & other) const override
		{
			return true;
		}

	private:
		TGoalVec getQuestTasks(const QuestInfo & q) const;
		TGoalVec tryCompleteQuest(const QuestInfo & q) const;
		TGoalVec missionArt(const QuestInfo & q) const;
		TGoalVec missionHero(const QuestInfo & q) const;
		TGoalVec missionArmy(const QuestInfo & q) const;
		TGoalVec missionResources(const QuestInfo & q) const;
		TGoalVec missionDestroyObj(const QuestInfo & q) const;
		TGoalVec missionIncreasePrimaryStat(const QuestInfo & q) const;
		TGoalVec missionLevel(const QuestInfo & q) const;
		TGoalVec missionKeymaster(const QuestInfo & q) const;
		std::string questToString(const QuestInfo & q) const;
	};
}
