/*
 * CCastleEvent.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CMapEvent.h"

/// The castle event builds/adds buildings/creatures for a specific town.
class DLL_LINKAGE CCastleEvent : public CMapEvent
{
public:
	CCastleEvent() = default;

	std::set<BuildingID> buildings;
	/// Faction-specific buildings for event of random town, merged into buildings once faction is picked. Not serialized - only used before game start
	std::map<FactionID, std::set<BuildingID>> randomTownBuildings;
	std::vector<si32> creatures;
	bool neutralAffected = false; ///< event also occurs while the town is neutral, without resources and message

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & static_cast<CMapEvent &>(*this);
		h & buildings;
		h & creatures;
		if(h.hasFeature(Handler::Version::NEUTRAL_TOWN_EVENTS))
			h & neutralAffected;
	}

	void serializeJson(JsonSerializeFormat & handler) override;
};
