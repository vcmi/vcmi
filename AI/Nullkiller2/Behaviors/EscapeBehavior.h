/*
* EscapeBehavior.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "lib/GameLibrary.h"
#include "../Goals/CGoal.h"

namespace NK2AI
{

struct EscapePathCandidate
{
	bool currentTileThreatensHero = false;
	bool sameDay = false;
	bool sameTile = false;
	bool blockedAction = false;
	bool singleHeroPath = false;
	bool destinationSafe = false;
	bool destinationIsSafer = false;
	bool usesDimensionDoor = false;
	float threatReduction = 0.0f;
	float movementCost = 0.0f;
};

struct EscapePathEvaluation
{
	bool accepted = false;
	float score = 0.0f;
};

EscapePathEvaluation evaluateEscapePathCandidate(const EscapePathCandidate & candidate);

namespace Goals
{
	class EscapeBehavior : public CGoal<EscapeBehavior>
	{
	public:
		EscapeBehavior()
			:CGoal(ESCAPE_BEHAVIOR)
		{
		}

		Goals::TGoalVec decompose(const Nullkiller * aiNk) const override;
		std::string toString() const override;

		bool operator==(const EscapeBehavior & other) const override
		{
			return true;
		}
	};
}

}
