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
#include "../constants/EntityIdentifiers.h"
#include "../IHandlerBase.h"
#include "../filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN

class ResourceTypeHandler;

namespace resources
{

class DLL_LINKAGE ResourceType : public EntityT<GameResID>
{
	friend class VCMI_LIB_WRAP_NAMESPACE(ResourceTypeHandler);

	GameResID id; //backlink

	int price;

	std::string identifier;
	std::string modScope;

public:
	int getPrice() const
	{
		return price;
	}

	std::string getJsonKey() const override { return identifier; }
	int32_t getIndex() const override { return id.getNum(); }
	GameResID getId() const override { return id;}
	int32_t getIconIndex() const override { return 0; }
	std::string getModScope() const override { return modScope; };
	void registerIcons(const IconRegistar & cb) const override {};
	std::string getNameTextID() const override;
	std::string getNameTranslated() const override;
};

}

class DLL_LINKAGE ResourceTypeHandler : public IHandlerBase
{
	std::shared_ptr<resources::ResourceType> loadObjectImpl(std::string scope, std::string name, const JsonNode & data, size_t index);
public:
	std::vector<JsonNode> loadLegacyData() override;

	/// loads single object into game. Scope is namespace of this object, same as name of source mod
	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

	std::vector<GameResID> getAllObjects() const;

	const resources::ResourceType * getById(GameResID index) const
	{
		return objects.at(index).get();
	}

private:
	std::vector<std::shared_ptr<resources::ResourceType>> objects;
};

VCMI_LIB_NAMESPACE_END
