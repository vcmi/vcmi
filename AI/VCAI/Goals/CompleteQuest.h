/*
* CompleteQuest.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CGoal.h"
#include "../../../lib/VCMI_Lib.h"
#include "../../../lib/gameState/QuestInfo.h"

namespace Goals
{
	class DLL_EXPORT CompleteQuest : public CGoal<CompleteQuest>
	{
	private:
		const QuestInfo q;

	public:
		CompleteQuest(const QuestInfo quest)
			: CGoal(Goals::COMPLETE_QUEST), q(quest)
		{
		}

		TGoalVec getAllPossibleSubgoals() override;
		TSubgoal whatToDoToAchieve() override;
		std::string name() const override;
		std::string completeMessage() const override;
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
