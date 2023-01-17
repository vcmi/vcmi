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

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE RiverType : public EntityT<RiverId>
{
	friend class RiverTypeHandler;
	std::string identifier;
	RiverId id;

	const std::string & getName() const override { return identifier;}
public:
	int32_t getIndex() const override { return id.getNum(); }
	int32_t getIconIndex() const override { return 0; }
	const std::string & getJsonKey() const override { return identifier;}
	void registerIcons(const IconRegistar & cb) const override {}
	RiverId getId() const override { return id;}
	void updateFrom(const JsonNode & data) {};

	std::string getNameTextID() const;
	std::string getNameTranslated() const;

	std::string tilesFilename;
	std::string shortIdentifier;
	std::string deltaName;

	RiverType();

	template <typename Handler> void serialize(Handler& h, const int version)
	{
		h & tilesFilename;
		h & identifier;
		h & deltaName;
		h & id;
	}
};

class DLL_LINKAGE RiverTypeService : public EntityServiceT<RiverId, RiverType>
{
public:
};

class DLL_LINKAGE RiverTypeHandler : public CHandlerBase<RiverId, RiverType, RiverType, RiverTypeService>
{
public:
	virtual RiverType * loadFromJson(
		const std::string & scope,
		const JsonNode & json,
		const std::string & identifier,
		size_t index) override;

	RiverTypeHandler();

	virtual const std::vector<std::string> & getTypeNames() const override;
	virtual std::vector<JsonNode> loadLegacyData(size_t dataSize) override;
	virtual std::vector<bool> getDefaultAllowed() const override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & objects;
	}
};

VCMI_LIB_NAMESPACE_END
