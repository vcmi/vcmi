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
#include "json/JsonNode.h"
#include "modding/IdentifierStorage.h"
#include "GameLibrary.h"

VCMI_LIB_NAMESPACE_BEGIN

int32_t ObstacleInfo::getIndex() const
{
	return obstacle.getNum();
}

int32_t ObstacleInfo::getIconIndex() const
{
	return iconIndex;
}

std::string ObstacleInfo::getJsonKey() const
{
	return modScope + ':' + identifier;
}

std::string ObstacleInfo::getModScope() const
{
	return modScope;
}

std::string ObstacleInfo::getNameTranslated() const
{
	return identifier;
}

std::string ObstacleInfo::getNameTextID() const
{
	return identifier; // TODO?
}

void ObstacleInfo::registerIcons(const IconRegistar & cb) const
{
}

Obstacle ObstacleInfo::getId() const
{
	return obstacle;
}

BattleHexArray ObstacleInfo::getBlocked(const BattleHex & hex) const
{
	if(isAbsoluteObstacle)
	{
		assert(!hex.isValid());
		return BattleHexArray(blockedTiles);
	}
	
	BattleHexArray ret;
	for(int offset : blockedTiles)
	{
		BattleHex toBlock = hex.toInt() + offset;
		if((hex.getY() & 1) && !(toBlock.getY() & 1))
			toBlock += BattleHex::LEFT;
		
		if(!toBlock.isValid())
			logGlobal->error("Misplaced obstacle!");
		else
			ret.insert(toBlock);
	}
	
	return ret;
}

bool ObstacleInfo::isAppropriate(const TerrainId terrainType, const BattleField & battlefield) const
{
	const auto * bgInfo = battlefield.getInfo();

	if(bgInfo->isSpecial)
		return vstd::contains(allowedSpecialBfields, bgInfo->identifier);
	
	return vstd::contains(allowedTerrains, terrainType);
}

std::shared_ptr<ObstacleInfo> ObstacleHandler::loadFromJson(const std::string & scope, const JsonNode & json, const std::string & identifier, size_t index)
{
	assert(identifier.find(':') == std::string::npos);

	auto info = std::make_shared<ObstacleInfo>(Obstacle(index), identifier);
	
	info->modScope = scope;
	info->animation = AnimationPath::fromJson(json["animation"]);
	info->width = json["width"].Integer();
	info->height = json["height"].Integer();
	for(const auto & t : json["allowedTerrains"].Vector())
	{
		LIBRARY->identifiers()->requestIdentifierIfFound("terrain", t, [info](int32_t identifier){
			info->allowedTerrains.emplace_back(identifier);
		});
	}
	for(const auto & t : json["specialBattlefields"].Vector())

		info->allowedSpecialBfields.emplace_back(t.String());
	info->blockedTiles = json["blockedTiles"].convertTo<std::vector<si16>>();
	info->isAbsoluteObstacle = json["absolute"].Bool();
	info->isForegroundObstacle = json["foreground"].Bool();

	return info;
}

std::vector<JsonNode> ObstacleHandler::loadLegacyData()
{
	return {};
}

const std::vector<std::string> & ObstacleHandler::getTypeNames() const
{
	static const std::vector<std::string> types = { "obstacle" };
	return types;
}

VCMI_LIB_NAMESPACE_END
