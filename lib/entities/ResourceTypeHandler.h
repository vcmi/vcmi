/*
 * ResourceTypeHandler.h, part of VCMI engine
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
#include <vcmi/ResourceType.h>
#include <vcmi/ResourceTypeService.h>
#include "../constants/EntityIdentifiers.h"
#include "../IHandlerBase.h"
#include "../filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN

class ResourceTypeHandler;

class DLL_LINKAGE Resource : public ResourceType
{
	friend class ResourceTypeHandler;

	GameResID id; //backlink

	int price;
	std::string iconSmall;
	std::string iconMedium;
	std::string iconLarge;

	std::string identifier;
	std::string modScope;

public:
	int getPrice() const override { return price; }

	std::string getJsonKey() const override { return identifier; }
	int32_t getIndex() const override { return id.getNum(); }
	GameResID getId() const override { return id;}
	int32_t getIconIndex() const override { return id.getNum(); }
	std::string getModScope() const override { return modScope; };
	void registerIcons(const IconRegistar & cb) const override;
	std::string getNameTextID() const override;
	std::string getNameTranslated() const override;
};

class DLL_LINKAGE ResourceTypeHandler : public CHandlerBase<GameResID, ResourceType, Resource, ResourceTypeService>
{
public:
	std::shared_ptr<Resource> loadFromJson(const std::string & scope,
										const JsonNode & json,
										const std::string & identifier,
										size_t index) override;
	
	const std::vector<std::string> & getTypeNames() const override;
	std::vector<JsonNode> loadLegacyData() override;

	std::vector<GameResID> getAllObjects() const;
};

VCMI_LIB_NAMESPACE_END
