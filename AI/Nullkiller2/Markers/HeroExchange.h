/*
* HeroExchange.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "../Goals/CGoal.h"
#include "../Pathfinding/AINodeStorage.h"

namespace NK2AI::Goals
{
	class DLL_EXPORT HeroExchange : public CGoal<HeroExchange>
	{
	private:
		uint64_t reinforcementArmyStrengthOverride = 0;

	public:
		AIPath exchangePath;

		HeroExchange(const CGHeroInstance * targetHero, const AIPath & exchangePath, uint64_t reinforcementArmyStrengthOverride = 0)
			: CGoal(Goals::HERO_EXCHANGE),
			exchangePath(exchangePath),
			reinforcementArmyStrengthOverride(reinforcementArmyStrengthOverride)
		{
			sethero(targetHero);
		}

		bool operator==(const HeroExchange & other) const override;
		std::string toString() const override;
		uint64_t getReinforcementArmyStrength(const Nullkiller * aiNk) const;
		uint64_t getArtifactExchangeValue() const;
	};
}
