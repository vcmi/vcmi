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
		explicit OneWayPortalProbe(int3 tile)
			: CGoal(Goals::ONE_WAY_PORTAL_PROBE)
		{
			settile(tile);
		}

		bool operator==(const OneWayPortalProbe & other) const override;
		std::string toString() const override;
	};
}
}
