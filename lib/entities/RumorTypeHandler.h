/*
 * RumorTypeHandler.h, part of VCMI engine
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

/// Rumor that can be told in taverns
class RumorType : public EntityT<RumorId>
{
	friend class RumorTypeHandler;

	std::string identifier;
	std::string modScope;
	std::string textID;
	RumorId id;
	FactionID faction = FactionID::NONE;
	int weight = 1;

public:
	int32_t getIndex() const override { return id.getNum(); }
	int32_t getIconIndex() const override { return id.getNum(); }
	std::string getJsonKey() const override;
	std::string getModScope() const override;
	void registerIcons(const IconRegistar & cb) const override {}
	RumorId getId() const override { return id; }

	/// Text of the rumor
	std::string getNameTextID() const override;
	std::string getNameTranslated() const override;

	/// Rumor can only be told if a town of this faction is on the map, or always if NONE
	FactionID getFaction() const { return faction; }
	/// Relative frequency of this rumor; currently unused
	int getWeight() const { return weight; }
};

class RumorTypeService : public EntityServiceT<RumorId, RumorType>
{
};

class DLL_LINKAGE RumorTypeHandler : public CHandlerBase<RumorId, RumorType, RumorType, RumorTypeService>
{
	std::vector<RumorId> allObjects;

public:
	std::shared_ptr<RumorType> loadFromJson(LibraryLoadContext & context, const std::string & scope,
		const JsonNode & json,
		const std::string & identifier,
		size_t index) override;

	void afterLoadFinalization(LibraryLoadContext & context) override;

	const std::vector<std::string> & getTypeNames() const override;
	std::vector<JsonNode> loadLegacyData(LibraryLoadContext & context) override;

	/// All rumors with valid text
	const std::vector<RumorId> & getAllObjects() const;
};
