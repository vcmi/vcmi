/*
 * ActiveModsInSaveList.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "ModVerificationInfo.h"

/// List of mods stored in saved games created before ESerializationVersion::SAVE_CHUNKS, newer saves store it in save metadata
class ActiveModsInSaveList
{
	/// Checks whether provided mod list is compatible with current LIBRARY and throws on failure
	void verifyActiveMods(const std::map<TModID, ModVerificationInfo> & modList);
public:
	template <typename Handler> void serialize(Handler &h)
	{
		static_assert(!Handler::saving, "Mods of a saved game are written as part of save metadata");

		std::vector<TModID> saveActiveMods;
		h & saveActiveMods;

		std::map<TModID, ModVerificationInfo> saveModInfos;
		for(int i = 0; i < saveActiveMods.size(); ++i)
			h & saveModInfos[saveActiveMods[i]];

		verifyActiveMods(saveModInfos);
	}
};
