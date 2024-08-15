/*
 * CMapDefines.h, part of VCMI engine
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

class TerrainType;
class RiverType;
class RoadType;
class CGObjectInstance;
class CGTownInstance;
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

	template <typename Handler>
	void serialize(Handler & h)
	{
		h & name;
		h & message;
		h & resources;
		if (h.version >= Handler::Version::EVENTS_PLAYER_SET)
		{
			h & players;
		}
		else
		{
			ui8 playersMask = 0;
			h & playersMask;
			for (int i = 0; i < 8; ++i)
				if ((playersMask & (1 << i)) != 0)
					players.insert(PlayerColor(i));
		}
		h & humanAffected;
		h & computerAffected;
		h & firstOccurrence;
		h & nextOccurrence;
	}
	
	virtual void serializeJson(JsonSerializeFormat & handler);
};

/// The castle event builds/adds buildings/creatures for a specific town.
class DLL_LINKAGE CCastleEvent: public CMapEvent
{
public:
	CCastleEvent() = default;

	std::set<BuildingID> buildings;
	std::vector<si32> creatures;

	template <typename Handler>
	void serialize(Handler & h)
	{
		h & static_cast<CMapEvent &>(*this);
		h & buildings;
		h & creatures;
	}
	
	void serializeJson(JsonSerializeFormat & handler) override;
};

/// The terrain tile describes the terrain type and the visual representation of the terrain.
/// Furthermore the struct defines whether the tile is visitable or/and blocked and which objects reside in it.
struct DLL_LINKAGE TerrainTile
{
	TerrainTile();

	/// Gets true if the terrain is not a rock. If from is water/land, same type is also required.
	bool entrableTerrain(const TerrainTile * from = nullptr) const;
	bool entrableTerrain(bool allowLand, bool allowSea) const;
	/// Checks for blocking objects and terraint type (water / land).
	bool isClear(const TerrainTile * from = nullptr) const;
	/// Gets the ID of the top visitable object or -1 if there is none.
	Obj topVisitableId(bool excludeTop = false) const;
	CGObjectInstance * topVisitableObj(bool excludeTop = false) const;
	bool isWater() const;
	EDiggingStatus getDiggingStatus(const bool excludeTop = true) const;
	bool hasFavorableWinds() const;

	const TerrainType * terType;
	const RiverType * riverType;
	const RoadType * roadType;
	ui8 terView;
	ui8 riverDir;
	ui8 roadDir;
	/// first two bits - how to rotate terrain graphic (next two - river graphic, next two - road);
	///	7th bit - whether tile is coastal (allows disembarking if land or block movement if water); 8th bit - Favorable Winds effect
	ui8 extTileFlags;
	bool visitable;
	bool blocked;

	std::vector<CGObjectInstance *> visitableObjects;
	std::vector<CGObjectInstance *> blockingObjects;

	template <typename Handler>
	void serialize(Handler & h)
	{
		h & terType;
		h & terView;
		h & riverType;
		h & riverDir;
		h & roadType;
		h & roadDir;
		h & extTileFlags;
		h & visitable;
		h & blocked;
		h & visitableObjects;
		h & blockingObjects;
	}
};

VCMI_LIB_NAMESPACE_END
