/*
 * BattlefieldHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "BattlefieldHandler.h"
#include "CModHandler.h"
#include "battle/obstacle/ObstacleJson.h"

BattlefieldHandler::BattlefieldHandler()
{
}

BattlefieldHandler::~BattlefieldHandler()
{

}

void BattlefieldHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	auto object = std::make_shared<ObstacleJson>(data);
	obstacleConfigs.push_back(object);
}

void BattlefieldHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	auto object = std::make_shared<ObstacleJson>(data);
	obstacleConfigs[index] = object;
}

std::vector<JsonNode> BattlefieldHandler::loadLegacyData(size_t dataSize)
{
	return {};
}

std::vector<bool> BattlefieldHandler::getDefaultAllowed() const
{
	std::vector<bool> allowed;
	allowed.resize(obstacleConfigs.size(), true);
	return allowed;
}

std::vector<std::shared_ptr<ObstacleJson> > const & BattlefieldHandler::getObstacleConfigs() const
{
	return obstacleConfigs;
}
