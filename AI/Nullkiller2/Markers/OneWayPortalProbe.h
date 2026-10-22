/*
 * OneWayPortalProbe.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#pragma once

#include "../Goals/CGoal.h"

namespace NK2AI
{
namespace Goals
{
	class DLL_EXPORT OneWayPortalProbe : public CGoal<OneWayPortalProbe>
	{
	public:
		/// Objects behind the portal the hero crosses for; such a crossing is valued by them instead of running ahead of gathering
		std::vector<ObjectInstanceID> rewardsBehind;

		explicit OneWayPortalProbe(int3 tile, const CGHeroInstance * hero = nullptr, std::vector<ObjectInstanceID> rewardsBehind = {})
			: CGoal(Goals::ONE_WAY_PORTAL_PROBE)
			, rewardsBehind(std::move(rewardsBehind))
		{
			settile(tile);
			sethero(hero);
		}

		bool operator==(const OneWayPortalProbe & other) const override;
		std::string toString() const override;
	};
}
}
