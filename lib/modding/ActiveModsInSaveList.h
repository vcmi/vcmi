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

VCMI_LIB_NAMESPACE_BEGIN

class ActiveModsInSaveList
{
	std::vector<TModID> getActiveMods();
	const ModVerificationInfo & getVerificationInfo(TModID mod);

	/// Checks whether provided mod list is compatible with current VLC and throws on failure
	void verifyActiveMods(const std::vector<std::pair<TModID, ModVerificationInfo>> & modList);
public:
	template <typename Handler> void serialize(Handler &h)
	{
		if(h.saving)
		{
			std::vector<TModID> activeMods = getActiveMods();
			h & activeMods;
			for(const auto & m : activeMods)
				h & getVerificationInfo(m);
		}
		else
		{
			std::vector<TModID> saveActiveMods;
			h & saveActiveMods;

			std::vector<std::pair<TModID, ModVerificationInfo>> saveModInfos(saveActiveMods.size());
			for(int i = 0; i < saveActiveMods.size(); ++i)
			{
				saveModInfos[i].first = saveActiveMods[i];
				h & saveModInfos[i].second;
			}

			verifyActiveMods(saveModInfos);
		}
	}
};

VCMI_LIB_NAMESPACE_END
