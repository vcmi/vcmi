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
	return name;
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

bool ObstacleInfo::isAppropriate(const Terrain & terrainType, const BattleField & battlefield) const
{
	auto bgInfo = battlefield.getInfo();
	
	if(bgInfo->isSpecial)
		return vstd::contains(allowedSpecialBfields, bgInfo->identifier);
	
	return vstd::contains(allowedTerrains, terrainType);
}

void ObstacleHandler::loadObstacles()
{
	auto loadObstacles = [](const JsonNode & node, bool absolute, std::vector<ObstacleInfo> & out)
	{
		for(const JsonNode &obs : node.Vector())
		{
			out.emplace_back();
			ObstacleInfo & obi = out.back();
			obi.defName = obs["defname"].String();
			obi.width =  static_cast<si32>(obs["width"].Float());
			obi.height = static_cast<si32>(obs["height"].Float());
			for(auto & t : obs["allowedTerrain"].Vector())
				obi.allowedTerrains.emplace_back(t.String());
			for(auto & t : obs["specialBattlefields"].Vector())
				obi.allowedSpecialBfields.emplace_back(t.String());
			obi.blockedTiles = obs["blockedTiles"].convertTo<std::vector<si16> >();
			obi.isAbsoluteObstacle = absolute;
		}
	};
	
	//auto allConfigs = VLC->modh->getActiveMods();
	//allConfigs.insert(allConfigs.begin(), "core");
	/*for(auto & mod : allConfigs)
	{
		if(!CResourceHandler::get(mod)->existsResource(ResourceID("config/obstacles.json")))
			continue;
		
		const JsonNode config(mod, ResourceID("config/obstacles.json"));
		loadObstacles(config["obstacles"], false, obstacles);
		loadObstacles(config["absoluteObstacles"], true, absoluteObstacles);
	}*/
}
