/*
 * ObstacleHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/EntityService.h>
#include <vcmi/Entity.h>
#include "GameConstants.h"
#include "IHandlerBase.h"
#include "Terrain.h"
#include "battle/BattleHex.h"

struct DLL_LINKAGE ObstacleInfo : public EntityT<Obstacle>
{
	Obstacle obstacle;
	si32 iconIndex;
	std::string name;
	std::string identifier;
	std::string defName;
	std::vector<Terrain> allowedTerrains;
	std::vector<std::string> allowedSpecialBfields;
	
	ui8 isAbsoluteObstacle; //there may only one such obstacle in battle and its position is always the same
	si32 width, height; //how much space to the right and up is needed to place obstacle (affects only placement algorithm)
	std::vector<si16> blockedTiles; //offsets relative to obstacle position (that is its left bottom corner)
	
	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	const std::string & getJsonKey() const override;
	const std::string & getName() const override;
	void registerIcons(const IconRegistar & cb) const override;
	Obstacle getId() const override;
	
	std::vector<BattleHex> getBlocked(BattleHex hex) const; //returns vector of hexes blocked by obstacle when it's placed on hex 'hex'
	
	bool isAppropriate(const Terrain & terrainType, const BattleField & specialBattlefield) const;
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & defName;
		h & allowedTerrains;
		h & allowedSpecialBfields;
		h & isAbsoluteObstacle;
		h & width;
		h & height;
		h & blockedTiles;
	}
};

class DLL_LINKAGE ObstacleService : public EntityServiceT<Obstacle, ObstacleInfo>
{
public:
};

class ObstacleHandler: public CHandlerBase<Obstacle, ObstacleInfo, ObstacleInfo, ObstacleService>
{
public:
	void loadObstacles();
	
	std::vector<ObstacleInfo> obstacles; //info about obstacles that may be placed on battlefield
	std::vector<ObstacleInfo> absoluteObstacles; //info about obstacles that may be placed on battlefield
	
	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & objects;
	}
};
