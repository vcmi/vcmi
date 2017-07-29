/*
 * BattlefieldHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "GameConstants.h"
#include "IHandlerBase.h"
#include "../obstacle/Obstacle.h"
#include "../obstacle/ObstacleJson.h"

class ObstacleJson;
class DLL_LINKAGE BattlefieldHandler : public IHandlerBase
{
public:
	BattlefieldHandler();
	~BattlefieldHandler();

	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

	std::vector<JsonNode> loadLegacyData(size_t dataSize) override;
	std::vector<bool> getDefaultAllowed() const override;

	std::vector<std::shared_ptr<ObstacleJson> > const & getObstacleConfigs() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
	}

private:
	std::vector<std::shared_ptr<ObstacleJson> > obstacleConfigs;
};
