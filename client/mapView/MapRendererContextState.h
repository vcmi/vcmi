/*
 * MapRendererContext.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/GameConstants.h"
#include "../../lib/mapping/MapTilesStorage.h"
#include "../../lib/int3.h"

#include <boost/container/small_vector.hpp>

struct ObjectPosInfo;
class CGObjectInstance;

class IMapRendererContext;

// from VwSymbol.def
enum class EWorldViewIcon
{
	TOWN = 0,
	HERO = 1,
	ARTIFACT = 2,
	TELEPORT = 3,
	GATE = 4,
	MINE_WOOD = 5,
	MINE_MERCURY = 6,
	MINE_STONE = 7,
	MINE_SULFUR = 8,
	MINE_CRYSTAL = 9,
	MINE_GEM = 10,
	MINE_GOLD = 11,
	RES_WOOD = 12,
	RES_MERCURY = 13,
	RES_STONE = 14,
	RES_SULFUR = 15,
	RES_CRYSTAL = 16,
	RES_GEM = 17,
	RES_GOLD = 18,

	ICONS_PER_PLAYER = 19,
	ICONS_TOTAL = 19 * 9 // 8 players + neutral set at the end
};

struct MapRendererContextState
{
public:
	MapRendererContextState();

	using MapObject = ObjectInstanceID;
	using MapObjectsList = std::vector<MapObject>;
	using ObjectTilesList = boost::container::small_vector<int3, 16>;

	/// Objects drawn on every tile. Ordered like in H3, apart from heroes and boats that are put last
	MapTilesStorage<MapObjectsList> objects;

	/// Objects on every tile of their footprint, drawn or not, ordered by draw layer - the way H3 keeps them.
	/// Heroes and boats are not part of it
	MapTilesStorage<MapObjectsList> orderedObjects;
	std::map<ObjectInstanceID, ObjectTilesList> usedTiles;

	/// Heroes and boats are not ordered with other objects, they are drawn between fixed layers of a tile
	static bool usesFixedDrawSlot(const CGObjectInstance * object);

	void addObject(const CGObjectInstance * object);
	void addMovingObject(const CGObjectInstance * object, const int3 & tileFrom, const int3 & tileDest);
	void removeObject(const CGObjectInstance * object);

private:
	void updateVisibleObjects(const int3 & tile);
};
