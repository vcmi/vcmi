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

namespace NK2AI
{
namespace Goals
{
	class CompleteQuest : public CGoal<CompleteQuest>
	{
		QuestInfo q;
		CCallback & cc;

	public:
		CompleteQuest(const QuestInfo & quest, CCallback & cc) : CGoal(COMPLETE_QUEST), q(quest), cc(cc) {}

		Goals::TGoalVec decompose(const Nullkiller * aiNk) const override;
		std::string toString() const override;
		bool hasHash() const override { return true; }
		uint64_t getHash() const override;

		bool operator==(const CompleteQuest & other) const override;

	private:
		TGoalVec tryCompleteQuest(const Nullkiller * aiNk) const;
		TGoalVec missionArt(const Nullkiller * aiNk) const;
		TGoalVec missionHero(const Nullkiller * aiNk) const;
		TGoalVec missionArmy(const Nullkiller * aiNk) const;
		TGoalVec missionResources(const Nullkiller * aiNk) const;
		TGoalVec missionDestroyObj(const Nullkiller * aiNk) const;
		TGoalVec missionIncreasePrimaryStat(const Nullkiller * aiNk) const;
		TGoalVec missionLevel(const Nullkiller * aiNk) const;
		TGoalVec missionKeymaster(const Nullkiller * aiNk) const;
		std::string questToString() const;
	};
}

}
