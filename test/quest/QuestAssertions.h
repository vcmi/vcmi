/*
 * QuestAssertions.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "StdInc.h"

#include "../../lib/mapObjects/CQuest.h"
#include "../../lib/rewardable/Limiter.h"

/// What a test expects a loaded CQuest to look like.
struct ExpectedMission
{
	Rewardable::Limiter limiter;
	int                 lastDay  = -1;

	/// Per-visit text checks. Empty string means "don't check this slot".
	std::string firstVisitText;
	std::string nextVisitText;
	std::string completedText;
};

namespace quest_test
{

void expectQuestMission(const CQuest & actual, const ExpectedMission & expected,
                        const char * file, int line);

} // namespace quest_test

/// Usage: EXPECT_QUEST_MISSION(seer->getQuest(), expected);
#define EXPECT_QUEST_MISSION(quest, expected) \
	::quest_test::expectQuestMission((quest), (expected), __FILE__, __LINE__)
