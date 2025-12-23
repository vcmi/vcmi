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
	std::vector<TModID> getActiveGameplayAffectingMods();
	ModVerificationInfo getVerificationInfo(TModID mod);

	/// Checks whether provided mod list is compatible with current LIBRARY and throws on failure
	void verifyActiveMods(const std::map<TModID, ModVerificationInfo> & modList);
public:
	template <typename Handler> void serialize(Handler &h)
	{
		if(h.saving)
		{
			std::vector<TModID> activeMods = getActiveGameplayAffectingMods();
			h & activeMods;
			for(const auto & m : activeMods)
			{
				ModVerificationInfo info = getVerificationInfo(m);
				h & info;
			}
		}
		else
		{
			std::vector<TModID> saveActiveMods;
			h & saveActiveMods;

			std::map<TModID, ModVerificationInfo> saveModInfos;
			for(int i = 0; i < saveActiveMods.size(); ++i)
			{
				ModVerificationInfo data;
				h & saveModInfos[saveActiveMods[i]];
			}

			verifyActiveMods(saveModInfos);
		}
	}
};

VCMI_LIB_NAMESPACE_END
