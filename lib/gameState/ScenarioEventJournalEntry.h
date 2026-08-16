/*
 * ScenarioEventJournalEntry.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../texts/MetaString.h"

struct DLL_LINKAGE ScenarioEventJournalEntry
{
	ui32 day = 0;
	std::string title;
	MetaString message;

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & day;
		h & title;
		h & message;
	}
};
