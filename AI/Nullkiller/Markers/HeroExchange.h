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

namespace NKAI
{
namespace Goals
{
	class DLL_EXPORT HeroExchange : public CGoal<HeroExchange>
	{
	private:
		AIPath exchangePath;

	public:
		HeroExchange(const CGHeroInstance * targetHero, const AIPath & exchangePath)
			: CGoal(Goals::HERO_EXCHANGE), exchangePath(exchangePath)
		{
			sethero(targetHero);
		}

		virtual bool operator==(const HeroExchange & other) const override;
		virtual std::string toString() const override;

		uint64_t getReinforcementArmyStrength() const;
	};
}

}
