/*
 * QuestAssertions.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "QuestAssertions.h"

namespace quest_test
{

void expectQuestMission(const CQuest & actual, const ExpectedMission & expected,
                        const char * file, int line)
{
	::testing::ScopedTrace trace(file, line, "EXPECT_QUEST_MISSION");

	EXPECT_EQ(actual.lastDay, expected.lastDay);

	// `hasExtraCreatures` is set by the quest object's init (allowsFullArmyRemoval),
	// not the H3M wire format — ignore it for limiter-equality purposes.
	Rewardable::Limiter normalized = expected.limiter;
	normalized.hasExtraCreatures = actual.mission.hasExtraCreatures;
	EXPECT_EQ(actual.mission, normalized);

	if(!expected.firstVisitText.empty())
	{
		EXPECT_EQ(expected.firstVisitText, actual.firstVisitText.toString());
	}
	if(!expected.nextVisitText.empty())
	{
		EXPECT_EQ(expected.nextVisitText, actual.nextVisitText.toString());
	}
	if(!expected.completedText.empty())
	{
		EXPECT_EQ(expected.completedText, actual.completedText.toString());
	}
}

} // namespace quest_test
