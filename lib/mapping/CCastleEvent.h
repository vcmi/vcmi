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

VCMI_LIB_NAMESPACE_BEGIN

/// The castle event builds/adds buildings/creatures for a specific town.
class DLL_LINKAGE CCastleEvent : public CMapEvent
{
public:
	CCastleEvent() = default;

	std::set<BuildingID> buildings;
	std::vector<si32> creatures;

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & static_cast<CMapEvent &>(*this);
		h & buildings;
		h & creatures;
	}

	void serializeJson(JsonSerializeFormat & handler) override;
};

VCMI_LIB_NAMESPACE_END
