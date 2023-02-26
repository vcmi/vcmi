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

#include "../lib/int3.h"
#include "../lib/GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN
struct ObjectPosInfo;
class CGObjectInstance;
VCMI_LIB_NAMESPACE_END

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

	boost::multi_array<MapObjectsList, 3> objects;

	void addObject(const CGObjectInstance * object);
	void addMovingObject(const CGObjectInstance * object, const int3 & tileFrom, const int3 & tileDest);
	void removeObject(const CGObjectInstance * object);
};
