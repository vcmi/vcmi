/*
 * ObstacleHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ObstacleHandler.h"
#include "BattleFieldHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

int32_t ObstacleInfo::getIndex() const
{
	return obstacle.getNum();
}

int32_t ObstacleInfo::getIconIndex() const
{
	return iconIndex;
}

const std::string & ObstacleInfo::getName() const
{
	return identifier;
}

const std::string & ObstacleInfo::getJsonKey() const
{
	return identifier;
}

void ObstacleInfo::registerIcons(const IconRegistar & cb) const
{
}

Obstacle ObstacleInfo::getId() const
{
	return obstacle;
}


std::vector<BattleHex> ObstacleInfo::getBlocked(BattleHex hex) const
{
	std::vector<BattleHex> ret;
	if(isAbsoluteObstacle)
	{
		assert(!hex.isValid());
		range::copy(blockedTiles, std::back_inserter(ret));
		return ret;
	}
	
	for(int offset : blockedTiles)
	{
		BattleHex toBlock = hex + offset;
		if((hex.getY() & 1) && !(toBlock.getY() & 1))
			toBlock += BattleHex::LEFT;
		
		if(!toBlock.isValid())
			logGlobal->error("Misplaced obstacle!");
		else
			ret.push_back(toBlock);
	}
	
	return ret;
}

bool ObstacleInfo::isAppropriate(const TerrainId terrainType, const BattleField & battlefield) const
{
	auto bgInfo = battlefield.getInfo();
	
	if(bgInfo->isSpecial)
		return vstd::contains(allowedSpecialBfields, bgInfo->identifier);
	
	return vstd::contains(allowedTerrains, terrainType);
}

ObstacleInfo * ObstacleHandler::loadFromJson(const std::string & scope, const JsonNode & json, const std::string & identifier, size_t index)
{
	auto * info = new ObstacleInfo(Obstacle(index), identifier);
	
	info->animation = json["animation"].String();
	info->width = json["width"].Integer();
	info->height = json["height"].Integer();
	for(auto & t : json["allowedTerrain"].Vector())
		info->allowedTerrains.emplace_back(VLC->terrainTypeHandler->getInfoByName(t.String())->id);
	for(auto & t : json["specialBattlefields"].Vector())
		info->allowedSpecialBfields.emplace_back(t.String());
	info->blockedTiles = json["blockedTiles"].convertTo<std::vector<si16>>();
	info->isAbsoluteObstacle = json["absolute"].Bool();
	
	objects.push_back(info);
	
	return info;
}

std::vector<JsonNode> ObstacleHandler::loadLegacyData(size_t dataSize)
{
	return {};
}

std::vector<bool> ObstacleHandler::getDefaultAllowed() const
{
	return {};
}

const std::vector<std::string> & ObstacleHandler::getTypeNames() const
{
	static const std::vector<std::string> types = { "obstacle" };
	return types;
}

VCMI_LIB_NAMESPACE_END
