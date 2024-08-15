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
#include "battle/BattleHex.h"
#include "filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE ObstacleInfo : public EntityT<Obstacle>
{
public:
	ObstacleInfo(): obstacle(-1), width(0), height(0), isAbsoluteObstacle(false), iconIndex(0), isForegroundObstacle(false)
	{}
	
	ObstacleInfo(Obstacle obstacle, std::string identifier)
	: obstacle(obstacle), identifier(identifier), iconIndex(obstacle.getNum()), width(0), height(0), isAbsoluteObstacle(false), isForegroundObstacle(false)
	{
	}
	
	Obstacle obstacle;
	si32 iconIndex;
	std::string modScope;
	std::string identifier;
	AudioPath appearSound;
	AnimationPath appearAnimation;
	AnimationPath animation;
	std::vector<TerrainId> allowedTerrains;
	std::vector<std::string> allowedSpecialBfields;
	
	bool isAbsoluteObstacle; //there may only one such obstacle in battle and its position is always the same
	bool isForegroundObstacle;
	si32 width; //how much space to the right and up is needed to place obstacle (affects only placement algorithm)
	si32 height;
	std::vector<si16> blockedTiles; //offsets relative to obstacle position (that is its left bottom corner)
	
	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	std::string getJsonKey() const override;
	std::string getModScope() const override;
	std::string getNameTranslated() const override;
	std::string getNameTextID() const override;
	void registerIcons(const IconRegistar & cb) const override;
	Obstacle getId() const override;
	
	std::vector<BattleHex> getBlocked(BattleHex hex) const; //returns vector of hexes blocked by obstacle when it's placed on hex 'hex'
	
	bool isAppropriate(const TerrainId terrainType, const BattleField & specialBattlefield) const;
};

class DLL_LINKAGE ObstacleService : public EntityServiceT<Obstacle, ObstacleInfo>
{
public:
};

class ObstacleHandler: public CHandlerBase<Obstacle, ObstacleInfo, ObstacleInfo, ObstacleService>
{
public:
	std::shared_ptr<ObstacleInfo> loadFromJson(const std::string & scope,
										const JsonNode & json,
										const std::string & identifier,
										size_t index) override;
	
	const std::vector<std::string> & getTypeNames() const override;
	std::vector<JsonNode> loadLegacyData() override;
};

VCMI_LIB_NAMESPACE_END
