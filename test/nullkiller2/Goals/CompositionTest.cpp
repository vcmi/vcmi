/*
 * CompositionTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Goals/Composition.h"

namespace
{
class RecordingGoal : public NK2AI::Goals::ElementarGoal<RecordingGoal>
{
	int id = 0;
	std::vector<int> * events = nullptr;
	bool fulfill = false;
	bool fail = false;

public:
	RecordingGoal(int id, std::vector<int> & eventsLog, bool fulfill, bool fail = false)
		: ElementarGoal(NK2AI::Goals::EXPLORE_NEIGHBOUR_TILE)
		, id(id)
		, events(&eventsLog)
		, fulfill(fulfill)
		, fail(fail)
	{
	}

	bool operator==(const RecordingGoal & other) const override
	{
		return id == other.id;
	}

	void accept(NK2AI::AIGateway *) override
	{
		events->push_back(id);

		if(fail)
			throw NK2AI::cannotFulfillGoalException("expected failure");

		if(fulfill)
			throw NK2AI::goalFulfilledException(NK2AI::Goals::sptr(*this));
	}

	std::string toString() const override
	{
		return "RecordingGoal " + std::to_string(id);
	}
};
}

TEST(Nullkiller2_Goals_Composition, continuesSequenceAfterFulfilledSubgoal)
{
	std::vector<int> events;
	NK2AI::Goals::Composition composition;

	composition.addNextSequence({
		NK2AI::Goals::sptr(RecordingGoal(1, events, true)),
		NK2AI::Goals::sptr(RecordingGoal(2, events, false))
	});

	EXPECT_NO_THROW(composition.accept(nullptr));
	EXPECT_EQ(events, (std::vector<int>{1, 2}));
}

TEST(Nullkiller2_Goals_Composition, propagatesFailureAfterCompletedSubgoal)
{
	std::vector<int> events;
	NK2AI::Goals::Composition composition;

	composition.addNextSequence({
		NK2AI::Goals::sptr(RecordingGoal(1, events, true)),
		NK2AI::Goals::sptr(RecordingGoal(2, events, false, true)),
		NK2AI::Goals::sptr(RecordingGoal(3, events, false))
	});

	EXPECT_THROW(composition.accept(nullptr), NK2AI::cannotFulfillGoalException);
	EXPECT_EQ(events, (std::vector<int>{1, 2}));
}
