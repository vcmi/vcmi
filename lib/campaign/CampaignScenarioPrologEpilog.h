/*
 * CampaignScenarioPrologEpilog.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN

struct DLL_LINKAGE CampaignScenarioPrologEpilog
{
	bool hasPrologEpilog = false;
	VideoPath prologVideo; // from CmpMovie.txt
	AudioPath prologMusic; // from CmpMusic.txt
	std::string prologText;

	template <typename Handler> void serialize(Handler &h, const int formatVersion)
	{
		h & hasPrologEpilog;
		h & prologVideo;
		h & prologMusic;
		h & prologText;
	}
};

VCMI_LIB_NAMESPACE_END
