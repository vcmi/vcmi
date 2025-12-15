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
#include "../texts/MetaString.h"

VCMI_LIB_NAMESPACE_BEGIN

struct DLL_LINKAGE CampaignScenarioPrologEpilog
{
	bool hasPrologEpilog = false;
	std::pair<VideoPath, VideoPath> prologVideo;
	AudioPath prologMusic;
	AudioPath prologVoice;
	MetaString prologText;

	template <typename Handler> void serialize(Handler &h)
	{
		h & hasPrologEpilog;
		if(h.version >= Handler::Version::CAMPAIGN_VIDEO)
			h & prologVideo;
		else
			h & prologVideo.first;
		h & prologMusic;
		h & prologVoice;
		h & prologText;
	}
};

VCMI_LIB_NAMESPACE_END
