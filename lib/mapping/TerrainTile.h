/*
 * TerrainTile.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameLibrary.h"
#include "../TerrainHandler.h"
#include "../mapObjects/CGObjectInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

class TerrainType;
class RiverType;
class RoadType;
class CGObjectInstance;

/// The terrain tile describes the terrain type and the visual representation of the terrain.
/// Furthermore the struct defines whether the tile is visitable or/and blocked and which objects reside in it.
struct DLL_LINKAGE TerrainTile
{
	TerrainTile();

	/// Gets true if the terrain is not a rock. If from is water/land, same type is also required.
	inline bool entrableTerrain() const;
	inline bool entrableTerrain(const TerrainTile * from) const;
	inline bool entrableTerrain(bool allowLand, bool allowSea) const;
	/// Checks for blocking objects and terraint type (water / land).
	bool isClear(const TerrainTile * from = nullptr) const;
	/// Gets the ID of the top visitable object or -1 if there is none.
	ObjectInstanceID topVisitableObj(bool excludeTop = false) const;
	inline bool isWater() const;
	inline bool isLand() const;
	EDiggingStatus getDiggingStatus(bool excludeTop = true) const;
	inline bool hasFavorableWinds() const;

	inline bool visitable() const;
	inline bool blocked() const;

	inline const TerrainType * getTerrain() const;
	inline const RiverType * getRiver() const;
	inline const RoadType * getRoad() const;

	inline TerrainId getTerrainID() const;
	inline RiverId getRiverID() const;
	inline RoadId getRoadID() const;

	inline bool hasRiver() const;
	inline bool hasRoad() const;

	TerrainId terrainType;
	RiverId riverType;
	RoadId roadType;
	ui8 terView;
	ui8 riverDir;
	ui8 roadDir;
	/// first two bits - how to rotate terrain graphic (next two - river graphic, next two - road);
	///	7th bit - whether tile is coastal (allows disembarking if land or block movement if water); 8th bit - Favorable Winds effect
	ui8 extTileFlags;

	std::vector<ObjectInstanceID> visitableObjects;
	std::vector<ObjectInstanceID> blockingObjects;

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & terrainType;
		h & terView;
		h & riverType;
		h & riverDir;
		h & roadType;
		h & roadDir;
		h & extTileFlags;

		if(h.hasFeature(Handler::Version::NO_RAW_POINTERS_IN_SERIALIZER))
		{
			h & visitableObjects;
			h & blockingObjects;
		}
		else
		{
			std::vector<std::shared_ptr<CGObjectInstance>> objectPtrs;
			h & objectPtrs;
			for(const auto & ptr : objectPtrs)
				visitableObjects.push_back(ptr->id);
			h & objectPtrs;
			for(const auto & ptr : objectPtrs)
				blockingObjects.push_back(ptr->id);
		}
	}
};

inline bool TerrainTile::hasFavorableWinds() const
{
	return extTileFlags & 128;
}

inline bool TerrainTile::isWater() const
{
	return getTerrain()->isWater();
}

inline bool TerrainTile::isLand() const
{
	return getTerrain()->isLand();
}

inline bool TerrainTile::visitable() const
{
	return !visitableObjects.empty();
}

inline bool TerrainTile::blocked() const
{
	return !blockingObjects.empty();
}

inline bool TerrainTile::hasRiver() const
{
	return getRiverID() != RiverId::NO_RIVER;
}

inline bool TerrainTile::hasRoad() const
{
	return getRoadID() != RoadId::NO_ROAD;
}

inline const TerrainType * TerrainTile::getTerrain() const
{
	return terrainType.toEntity(LIBRARY);
}

inline const RiverType * TerrainTile::getRiver() const
{
	return riverType.toEntity(LIBRARY);
}

inline const RoadType * TerrainTile::getRoad() const
{
	return roadType.toEntity(LIBRARY);
}

inline TerrainId TerrainTile::getTerrainID() const
{
	return terrainType;
}

inline RiverId TerrainTile::getRiverID() const
{
	return riverType;
}

inline RoadId TerrainTile::getRoadID() const
{
	return roadType;
}

inline bool TerrainTile::entrableTerrain() const
{
	return entrableTerrain(true, true);
}

inline bool TerrainTile::entrableTerrain(const TerrainTile * from) const
{
	const TerrainType * terrainFrom = from->getTerrain();
	return entrableTerrain(terrainFrom->isLand(), terrainFrom->isWater());
}

inline bool TerrainTile::entrableTerrain(bool allowLand, bool allowSea) const
{
	const TerrainType * terrain = getTerrain();
	return terrain->isPassable() && ((allowSea && terrain->isWater()) || (allowLand && terrain->isLand()));
}

VCMI_LIB_NAMESPACE_END
