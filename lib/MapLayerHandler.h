/*
 * MapLayerHandler.h, part of VCMI engine
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

class DLL_LINKAGE MapLayerType : public EntityT<MapLayerId>
{
	friend class MapLayerTypeHandler;
	std::string identifier;
	std::string modScope;
	MapLayerId id;

public:
	int32_t getIndex() const override { return id.getNum(); }
	int32_t getIconIndex() const override { return 0; }
	std::string getJsonKey() const override;
	std::string getModScope() const override;
	void registerIcons(const IconRegistar & cb) const override {}
	MapLayerId getId() const override { return id;}
	void updateFrom(const JsonNode & data) {};

	std::string getNameTextID() const override;
	std::string getNameTranslated() const override;

	MapLayerType();
};

class DLL_LINKAGE MapLayerTypeService : public EntityServiceT<MapLayerId, MapLayerType>
{
};

class DLL_LINKAGE MapLayerTypeHandler : public CHandlerBase<MapLayerId, MapLayerType, MapLayerType, MapLayerTypeService>
{
public:
	std::shared_ptr<MapLayerType> loadFromJson(
		const std::string & scope,
		const JsonNode & json,
		const std::string & identifier,
		size_t index) override;

	MapLayerTypeHandler();

	const std::vector<std::string> & getTypeNames() const override;
	std::vector<JsonNode> loadLegacyData() override;
};

VCMI_LIB_NAMESPACE_END
