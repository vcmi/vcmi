/*
 * TerrainHandler.h, part of VCMI engine
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
#include "filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE RoadType : public EntityT<RoadId>
{
	friend class RoadTypeHandler;
	std::string identifier;
	std::string modScope;
	RoadId id;

public:
	int32_t getIndex() const override { return id.getNum(); }
	int32_t getIconIndex() const override { return 0; }
	std::string getJsonKey() const override;
	std::string getModScope() const override;
	void registerIcons(const IconRegistar & cb) const override {}
	RoadId getId() const override { return id;}
	void updateFrom(const JsonNode & data) {};

	std::string getNameTextID() const override;
	std::string getNameTranslated() const override;

	AnimationPath tilesFilename;
	std::string shortIdentifier;
	ui8 movementCost;

	RoadType();
};

class DLL_LINKAGE RoadTypeService : public EntityServiceT<RoadId, RoadType>
{
public:
};

class DLL_LINKAGE RoadTypeHandler : public CHandlerBase<RoadId, RoadType, RoadType, RoadTypeService>
{
public:
	std::shared_ptr<RoadType> loadFromJson(
		const std::string & scope,
		const JsonNode & json,
		const std::string & identifier,
		size_t index) override;

	RoadTypeHandler();

	const std::vector<std::string> & getTypeNames() const override;
	std::vector<JsonNode> loadLegacyData() override;
};

VCMI_LIB_NAMESPACE_END
