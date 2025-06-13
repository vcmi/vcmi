/*
 * CBonusTypeHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#define INSTANTIATE_CBonusTypeHandler_HERE

#include "CBonusTypeHandler.h"

#include "filesystem/Filesystem.h"

#include "CCreatureHandler.h"
#include "GameConstants.h"
#include "GameLibrary.h"
#include "modding/ModScope.h"
#include "modding/IdentifierStorage.h"
#include "spells/CSpellHandler.h"
#include "texts/CGeneralTextHandler.h"
#include "json/JsonUtils.h"

VCMI_LIB_NAMESPACE_BEGIN

///CBonusType

std::string CBonusType::getDescriptionTextID() const
{
	return TextIdentifier( "core", "bonus", identifier, "description").get();
}

///CBonusTypeHandler

CBonusTypeHandler::CBonusTypeHandler()
{
	//register predefined bonus types

#define BONUS_NAME(x) { #x },
	builtinBonusNames = {
		BONUS_LIST
	};
#undef BONUS_NAME

	for (int i = 0; i < builtinBonusNames.size(); ++i)
		bonusTypes.push_back(std::make_shared<CBonusType>());

	for (int i = 0; i < builtinBonusNames.size(); ++i)
		registerObject(ModScope::scopeBuiltin(), "bonus", builtinBonusNames[i], i);
}

CBonusTypeHandler::~CBonusTypeHandler() = default;

std::string CBonusTypeHandler::bonusToString(const std::shared_ptr<Bonus> & bonus, const IBonusBearer * bearer) const
{
	const CBonusType & bt = *bonusTypes.at(vstd::to_underlying(bonus->type));
	int bonusValue = bearer->valOfBonuses(bonus->type, bonus->subtype);
	if(bt.hidden)
		return "";

	std::string textID = bt.getDescriptionTextID();
	std::string text = LIBRARY->generaltexth->translate(textID);

	auto subtype = bonus->subtype.getNum();
	if (bt.subtypeDescriptions.count(subtype))
	{
		std::string fullTextID = textID + '.' + bt.subtypeDescriptions.at(subtype);
		text = LIBRARY->generaltexth->translate(fullTextID);
	}
	else if (bt.valueDescriptions.count(bonusValue))
	{
		std::string fullTextID = textID + '.' + bt.valueDescriptions.at(bonusValue);
		text = LIBRARY->generaltexth->translate(fullTextID);
	}

	if (text.find("${val}") != std::string::npos)
		boost::algorithm::replace_all(text, "${val}", std::to_string(bonusValue));

	if (text.find("${subtype.creature}") != std::string::npos && bonus->subtype.as<CreatureID>().hasValue())
		boost::algorithm::replace_all(text, "${subtype.creature}", bonus->subtype.as<CreatureID>().toCreature()->getNamePluralTranslated());

	if (text.find("${subtype.spell}") != std::string::npos && bonus->subtype.as<SpellID>().hasValue())
		boost::algorithm::replace_all(text, "${subtype.spell}", bonus->subtype.as<SpellID>().toSpell()->getNameTranslated());

	return text;
}

ImagePath CBonusTypeHandler::bonusToGraphics(const std::shared_ptr<Bonus> & bonus) const
{
	const CBonusType & bt = *bonusTypes.at(vstd::to_underlying(bonus->type));

	if (bonus->type == BonusType::SPELL_IMMUNITY && bonus->subtype.as<SpellID>().hasValue())
	{
		const CSpell * sp = bonus->subtype.as<SpellID>().toSpell();
		return sp->getIconImmune();
	}

	if (bt.subtypeIcons.count(bonus->subtype.getNum()))
		return bt.subtypeIcons.at(bonus->subtype.getNum());

	if (bt.valueIcons.count(bonus->val))
		return bt.valueIcons.at(bonus->val);

	return bt.icon;
}

std::vector<JsonNode> CBonusTypeHandler::loadLegacyData()
{
	return {};
}

void CBonusTypeHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	if (vstd::contains(builtinBonusNames, name))
	{
		//h3 bonus
		BonusType bonus = static_cast<BonusType>(vstd::find_pos(builtinBonusNames, name));
		CBonusType & bt =*bonusTypes.at(vstd::to_underlying(bonus));
		loadItem(data, bt, name);
		logBonus->trace("Loaded bonus type %s", name);
	}
	else
	{
		// new bonus
		registerObject(scope, "bonus", name, bonusTypes.size());
		bonusTypes.push_back(std::make_shared<CBonusType>());
		loadItem(data, *bonusTypes.back(), name);
		logBonus->trace("New bonus type %s", name);
	}
}

void CBonusTypeHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	assert(0);
}

void CBonusTypeHandler::loadItem(const JsonNode & source, CBonusType & dest, const std::string & name) const
{
	dest.identifier = name;
	dest.hidden = source["hidden"].Bool(); //Null -> false
	dest.creatureNature = source["creatureNature"].Bool(); //Null -> false

	if (!dest.hidden)
		LIBRARY->generaltexth->registerString( "vcmi", dest.getDescriptionTextID(), source["description"]);

	const JsonNode & graphics = source["graphics"];

	if(!graphics.isNull())
		dest.icon = ImagePath::fromJson(graphics["icon"]);

	for (const auto & additionalIcon : graphics["subtypeIcons"].Struct())
	{
		auto path = ImagePath::fromJson(additionalIcon.second);
		LIBRARY->identifiers()->requestIdentifier(additionalIcon.second.getModScope(), additionalIcon.first, [&dest, path](int32_t index)
		{
			dest.subtypeIcons[index] = path;
		});
	}

	for (const auto & additionalIcon : graphics["valueIcons"].Struct())
	{
		auto path = ImagePath::fromJson(additionalIcon.second);
		int value = std::stoi(additionalIcon.first);
		dest.valueIcons[value] = path;
	}

	for (const auto & additionalDescription : source["subtypeDescriptions"].Struct())
	{
		LIBRARY->generaltexth->registerString( "vcmi", dest.getDescriptionTextID() + "." + additionalDescription.first, additionalDescription.second);
		auto stringID = additionalDescription.first;
		LIBRARY->identifiers()->requestIdentifier(additionalDescription.second.getModScope(), additionalDescription.first, [&dest, stringID](int32_t index)
		{
			dest.subtypeDescriptions[index] = stringID;
		});
	}

	for (const auto & additionalDescription : source["valueDescriptions"].Struct())
	{
		LIBRARY->generaltexth->registerString( "vcmi", dest.getDescriptionTextID() + "." + additionalDescription.first, additionalDescription.second);
		auto stringID = additionalDescription.first;
		int value = std::stoi(additionalDescription.first);
		dest.valueDescriptions[value] = stringID;
	}
}

const std::string & CBonusTypeHandler::bonusToString(BonusType bonus) const
{
	return bonusTypes.at(static_cast<int>(bonus))->identifier;
}

bool CBonusTypeHandler::isCreatureNatureBonus(BonusType bonus) const
{
	return bonusTypes.at(static_cast<int>(bonus))->creatureNature;
}

std::vector<BonusType> CBonusTypeHandler::getAllObjets() const
{
	std::vector<BonusType> ret;
	for (int i = 0; i < bonusTypes.size(); ++i)
		ret.push_back(static_cast<BonusType>(i));

	return ret;
}

VCMI_LIB_NAMESPACE_END
