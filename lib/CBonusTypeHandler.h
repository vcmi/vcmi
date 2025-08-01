/*
 * CBonusTypeHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "IBonusTypeHandler.h"
#include "IHandlerBase.h"
#include "bonuses/Bonus.h"

VCMI_LIB_NAMESPACE_BEGIN


class JsonNode;

class DLL_LINKAGE CBonusType : boost::noncopyable
{
public:
	CBonusType() = default;

	std::string getDescriptionTextID() const;

private:
	friend class CBonusTypeHandler;

	ImagePath icon;
	std::map<int, ImagePath> subtypeIcons;
	std::map<int, ImagePath> valueIcons;
	std::map<int, std::string> subtypeDescriptions;
	std::map<int, std::string> valueDescriptions;
	std::string identifier;

	bool creatureNature = false;
	bool hidden = true;
};

class DLL_LINKAGE CBonusTypeHandler : public IBonusTypeHandler
{
	std::vector<std::string> builtinBonusNames;
public:
	CBonusTypeHandler();
	virtual ~CBonusTypeHandler();

	std::string bonusToString(const std::shared_ptr<Bonus> & bonus, const IBonusBearer * bearer) const override;
	ImagePath bonusToGraphics(const std::shared_ptr<Bonus> & bonus) const override;

	std::vector<JsonNode> loadLegacyData() override;
	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

	const std::string & bonusToString(BonusType bonus) const;

	bool isCreatureNatureBonus(BonusType bonus) const;

	std::vector<BonusType> getAllObjets() const;
private:
	void loadItem(const JsonNode & source, CBonusType & dest, const std::string & name) const;

	std::vector<std::shared_ptr<CBonusType> > bonusTypes; //index = BonusType
};

VCMI_LIB_NAMESPACE_END

#ifndef INSTANTIATE_CBonusTypeHandler_HERE
extern template class std::vector<VCMI_LIB_WRAP_NAMESPACE(CBonusType)>;
#endif
