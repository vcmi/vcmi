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

#include "../int3.h"
#include "../networkPacks/Component.h"
#include "../texts/MetaString.h"

struct DLL_LINKAGE ScenarioEventJournalEntry
{
	ui32 day = 0;
	MetaString message;
	int3 location = int3(-1, -1, -1);
	std::vector<Component> components;

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & day;
		h & message;
		h & location;
		h & components;
	}
};

struct DLL_LINKAGE ScenarioEventJournalInfo
{
	int3 location = int3(-1, -1, -1);

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & location;
	}
};
