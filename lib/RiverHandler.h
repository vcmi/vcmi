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

struct DLL_LINKAGE RiverPaletteAnimation
{
	/// index of first color to cycle
	int32_t start;
	/// total numbers of colors to cycle
	int32_t length;
};

class DLL_LINKAGE RiverType : public EntityT<RiverId>
{
	friend class RiverTypeHandler;
	std::string identifier;
	std::string modScope;
	RiverId id;

public:
	int32_t getIndex() const override { return id.getNum(); }
	int32_t getIconIndex() const override { return 0; }
	std::string getJsonKey() const override;
	void registerIcons(const IconRegistar & cb) const override {}
	RiverId getId() const override { return id;}
	void updateFrom(const JsonNode & data) {};

	std::string getNameTextID() const override;
	std::string getNameTranslated() const override;

	AnimationPath tilesFilename;
	std::string shortIdentifier;
	std::string deltaName;

	std::vector<RiverPaletteAnimation> paletteAnimation;

	RiverType();
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

	const std::vector<std::string> & getTypeNames() const override;
	std::vector<JsonNode> loadLegacyData() override;
};

VCMI_LIB_NAMESPACE_END
