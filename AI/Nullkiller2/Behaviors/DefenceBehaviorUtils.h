/*
* DefenceBehaviorUtils.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include <cstdint>
#include <vector>

class CGHeroInstance;
class CGObjectInstance;
class CGTownInstance;

namespace NK2AI
{

struct AIPath;
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
	bool isDefenderReleaseAllowedForTownCapture(
		const CGHeroInstance & defender,
		const CGObjectInstance & target,
		bool targetIsEnemy,
		bool defenderMakesHomeStable,
		uint64_t remainingTownReinforcement,
		int dayOfWeek,
		int daysInWeek);
	bool isSafeSameTurnReturnPath(const CGHeroInstance & hero, const AIPath & path, float safeAttackRatio, float availableMovement);
	bool isSafeSameTurnReturnPath(const CGHeroInstance & hero, const AIPath & path, float safeAttackRatio);
}

}
