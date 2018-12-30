/*
* mock_VCAI_CGoal.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "../../AI/VCAI/Goals/Goals.h"

namespace Goals
{
	class InvalidGoalMock : public Invalid
	{
	public:
		//MOCK_METHOD1(accept, void(VCAI *));
		//MOCK_METHOD1(accept, float(FuzzyHelper *));
		//MOCK_METHOD1(fulfillsMe, bool(TSubgoal));
		//bool operator==(AbstractGoal & g) override
		//{
		//	return false;
		//};
		//MOCK_METHOD0(getAllPossibleSubgoals, TGoalVec());
		//MOCK_METHOD0(whatToDoToAchieve, TSubgoal());

	};

	class GatherArmyGoalMock : public GatherArmy
	{
	};
}
