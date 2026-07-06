/*
* DefenceBehavior.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include <vector>

#include "../AIUtility.h"
#include "../Goals/CGoal.h"
#include "lib/GameLibrary.h"

namespace NK2AI
{

struct HitMapInfo;

namespace Goals
{
	uint64_t estimateTownFortificationDefence(const CGTownInstance & town, bool hasDefenders);
	uint64_t estimateTownDefence(const CGTownInstance & town, const CGHeroInstance * committedDefender);
	bool isTownDefenceSufficient(uint64_t defenceStrength, const HitMapInfo & threat, float safeAttackRatio);
	int countTownThreatsCoveredByDefender(const CGTownInstance & town, const CGHeroInstance & defender, const std::vector<HitMapInfo> & threats, float safeAttackRatio);
	bool isHeroRequiredForTownDefence(const CGTownInstance & town, const CGHeroInstance & defender, const std::vector<HitMapInfo> & threats, float safeAttackRatio);
	bool shouldReserveTownDefender(const CGTownInstance & town, const CGHeroInstance & defender, const std::vector<HitMapInfo> & threats, float safeAttackRatio);
	bool shouldLockTownDefender(const CGTownInstance & town, const CGHeroInstance & defender, const HitMapInfo & threat, float safeAttackRatio);

	class DefenceBehavior : public CGoal<DefenceBehavior>
	{
	public:
		DefenceBehavior() : CGoal(DEFENCE) {}

		Goals::TGoalVec decompose(const Nullkiller * aiNk) const override;
		std::string toString() const override;

		bool operator==(const DefenceBehavior & other) const override
		{
			return true;
		}

	private:
		void evaluateDefence(Goals::TGoalVec & tasks, const CGTownInstance * town, const Nullkiller * aiNk) const;
		static void evaluateRecruitingHero(Goals::TGoalVec & tasks, const HitMapInfo & threat, const CGTownInstance * town, const Nullkiller * aiNk);
	};
}

}
