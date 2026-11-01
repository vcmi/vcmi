/*
 * RumorState.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../constants/EntityIdentifiers.h"
#include "../texts/MetaString.h"

/// Rumor told in taverns during current week, and rumors told since their pool was last exhausted
struct DLL_LINKAGE RumorState
{
	MetaString text; ///< empty if there is no rumor to tell
	std::set<RumorId> usedRumors;
	std::set<int32_t> usedMapRumors; ///< indexes in CMap::rumors

	template <typename Handler> void serialize(Handler &h)
	{
		if (h.hasFeature(Handler::Version::RUMOR_REWORK))
		{
			h & text;
			h & usedRumors;
			h & usedMapRumors;
		}
		else
		{
			int32_t oldType = 0;
			std::map<int32_t, std::pair<int32_t, int32_t>> oldLastRumors;
			h & oldType;
			h & oldLastRumors;

			text = MetaString::createFromTextID("core.randtvrn.0");
		}
	}
};
