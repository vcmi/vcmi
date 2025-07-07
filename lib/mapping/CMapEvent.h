/*
 * CMapEvent.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../ResourceSet.h"
#include "../texts/MetaString.h"

VCMI_LIB_NAMESPACE_BEGIN

class JsonSerializeFormat;

/// The map event is an event which e.g. gives or takes resources of a specific
/// amount to/from players and can appear regularly or once a time.
class DLL_LINKAGE CMapEvent
{
public:
	CMapEvent();
	virtual ~CMapEvent() = default;

	bool occursToday(int currentDay) const;
	bool affectsPlayer(PlayerColor player, bool isHuman) const;

	std::string name;
	MetaString message;
	TResources resources;
	std::set<PlayerColor> players;
	bool humanAffected;
	bool computerAffected;
	ui32 firstOccurrence;
	ui32 nextOccurrence; /// specifies after how many days the event will occur the next time; 0 if event occurs only one time

	std::vector<ObjectInstanceID> deletedObjectsInstances;

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & name;
		h & message;
		h & resources;
		h & players;
		h & humanAffected;
		h & computerAffected;
		h & firstOccurrence;
		h & nextOccurrence;
		h & deletedObjectsInstances;
	}

	virtual void serializeJson(JsonSerializeFormat & handler);
};

VCMI_LIB_NAMESPACE_END
