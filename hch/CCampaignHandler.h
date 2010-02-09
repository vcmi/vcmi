#ifndef __CCAMPAIGNHANDLER_H__
#define __CCAMPAIGNHANDLER_H__

#include "../global.h"
#include <string>
#include <vector>

/*
 * CCampaignHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class DLL_EXPORT CCampaignHeader
{
public:
	si32 version; //5 - AB, 6 - SoD, WoG - ?!?
	ui8 mapVersion; //CampText.txt's format
	std::string name, description;
	ui8 difficultyChoosenByPlayer;
	ui8 music; //CmpMusic.txt, start from 0

	template <typename Handler> void serialize(Handler &h, const int formatVersion)
	{
		h & version & mapVersion & name & description & difficultyChoosenByPlayer & music;
	}
};


class DLL_EXPORT CCampaignHandler
{
	CCampaignHeader getHeader( const std::string & name, int size );
public:
	std::vector<CCampaignHeader> getCampaignHeaders();
};


#endif __CCAMPAIGNHANDLER_H__