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
#include "../Goals/CGoal.h"
#include "../../../lib/gameState/QuestInfo.h"

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

		Goals::TGoalVec decompose(const Nullkiller * ai) const override;
		std::string toString() const override;
		bool hasHash() const override { return true; }
		uint64_t getHash() const override;

		bool operator==(const CompleteQuest & other) const override;

	private:
		TGoalVec tryCompleteQuest(const Nullkiller * ai) const;
		TGoalVec missionArt(const Nullkiller * ai) const;
		TGoalVec missionHero(const Nullkiller * ai) const;
		TGoalVec missionArmy(const Nullkiller * ai) const;
		TGoalVec missionResources(const Nullkiller * ai) const;
		TGoalVec missionDestroyObj(const Nullkiller * ai) const;
		TGoalVec missionIncreasePrimaryStat(const Nullkiller * ai) const;
		TGoalVec missionLevel(const Nullkiller * ai) const;
		TGoalVec missionKeymaster(const Nullkiller * ai) const;
		std::string questToString() const;
	};
}

}
