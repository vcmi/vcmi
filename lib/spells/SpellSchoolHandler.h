/*
 * SpellSchoolHandler.h, part of VCMI engine
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

class SpellSchoolHandler;

namespace spells
{

class DLL_LINKAGE SpellSchoolType : public EntityT<SpellSchool>
{
	friend class VCMI_LIB_WRAP_NAMESPACE(SpellSchoolHandler);

	SpellSchool id; //backlink
	AnimationPath spellBordersPath;
	AnimationPath schoolBookmarkPath;
	ImagePath schoolHeaderPath;

	std::string identifier;
	std::string modScope;

public:
	AnimationPath getSpellBordersPath() const
	{
		return spellBordersPath;
	}

	AnimationPath getSchoolBookmarkPath() const
	{
		return schoolBookmarkPath;
	}

	ImagePath getSchoolHeaderPath() const
	{
		return schoolHeaderPath;
	}

	std::string getJsonKey() const override { return identifier; }
	int32_t getIndex() const override { return id.getNum(); }
	SpellSchool getId() const override { return id;}
	int32_t getIconIndex() const override { return 0; }
	std::string getModScope() const override { return modScope; };
	void registerIcons(const IconRegistar & cb) const override {};
	std::string getNameTextID() const override;
	std::string getNameTranslated() const override;
};

}

class DLL_LINKAGE SpellSchoolHandler : public IHandlerBase
{
	std::shared_ptr<spells::SpellSchoolType> loadObjectImpl(std::string scope, std::string name, const JsonNode & data, size_t index);
public:
	std::vector<JsonNode> loadLegacyData() override;

	/// loads single object into game. Scope is namespace of this object, same as name of source mod
	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

	std::vector<SpellSchool> getAllObjects() const;

	const spells::SpellSchoolType * getById(SpellSchool index) const
	{
		return objects.at(index).get();
	}

private:
	std::vector<std::shared_ptr<spells::SpellSchoolType>> objects;
};

VCMI_LIB_NAMESPACE_END
