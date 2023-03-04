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

#include "JsonNode.h"
#include "filesystem/Filesystem.h"

#include "GameConstants.h"
#include "CCreatureHandler.h"
#include "CGeneralTextHandler.h"
#include "spells/CSpellHandler.h"

template class std::vector<VCMI_LIB_WRAP_NAMESPACE(CBonusType)>;

VCMI_LIB_NAMESPACE_BEGIN

///CBonusType

CBonusType::CBonusType():
	hidden(true)
{}

std::string CBonusType::getNameTextID() const
{
	return TextIdentifier( "core", "bonus", identifier, "name").get();
}

std::string CBonusType::getDescriptionTextID() const
{
	return TextIdentifier( "core", "bonus", identifier, "description").get();
}

///CBonusTypeHandler

CBonusTypeHandler::CBonusTypeHandler()
{
	//register predefined bonus types

	#define BONUS_NAME(x) \
	do { \
		bonusTypes.push_back(CBonusType()); \
	} while(0);


	BONUS_LIST;
	#undef BONUS_NAME

	load();
}

CBonusTypeHandler::~CBonusTypeHandler()
{
	//dtor
}

std::string CBonusTypeHandler::bonusToString(const std::shared_ptr<Bonus> & bonus, const IBonusBearer * bearer, bool description) const
{
	const CBonusType & bt = bonusTypes[bonus->type];
	if(bt.hidden)
		return "";

	std::string textID = description ? bt.getDescriptionTextID() : bt.getNameTextID();
	std::string text = VLC->generaltexth->translate(textID);

	if (text.find("${val}") != std::string::npos)
		boost::algorithm::replace_all(text, "${val}", std::to_string(bearer->valOfBonuses(Selector::typeSubtype(bonus->type, bonus->subtype))));

	if (text.find("${subtype.creature}") != std::string::npos)
		boost::algorithm::replace_all(text, "${subtype.creature}", CreatureID(bonus->subtype).toCreature()->getNamePluralTranslated());

	if (text.find("${subtype.spell}") != std::string::npos)
		boost::algorithm::replace_all(text, "${subtype.spell}", SpellID(bonus->subtype).toSpell()->getNameTranslated());

	if (text.find("${MR}") != std::string::npos)
		boost::algorithm::replace_all(text, "${MR}", std::to_string(bearer->magicResistance()));

	return text;
}

std::string CBonusTypeHandler::bonusToGraphics(const std::shared_ptr<Bonus> & bonus) const
{
	std::string fileName;
	bool fullPath = false;

	switch(bonus->type)
	{
	case Bonus::SECONDARY_SKILL_PREMY:
		if(bonus->subtype == SecondarySkill::RESISTANCE)
		{
			fileName = "E_DWARF.bmp";
		}
		break;
	case Bonus::SPELL_IMMUNITY:
	{
		fullPath = true;
		const CSpell * sp = SpellID(bonus->subtype).toSpell();
		fileName = sp->getIconImmune();
		break;
	}
	case Bonus::FIRE_IMMUNITY:
		switch(bonus->subtype)
		{
		case 0:
			fileName = "E_SPFIRE.bmp";
			break;//all
		case 1:
			fileName = "E_SPFIRE1.bmp";
			break;//not positive
		case 2:
			fileName = "E_FIRE.bmp";
			break;//direct damage
		}
		break;
	case Bonus::WATER_IMMUNITY:
		switch(bonus->subtype)
		{
		case 0:
			fileName = "E_SPWATER.bmp";
			break;//all
		case 1:
			fileName = "E_SPWATER1.bmp";
			break;//not positive
		case 2:
			fileName = "E_SPCOLD.bmp";
			break;//direct damage
		}
		break;
	case Bonus::AIR_IMMUNITY:
		switch(bonus->subtype)
		{
		case 0:
			fileName = "E_SPAIR.bmp";
			break;//all
		case 1:
			fileName = "E_SPAIR1.bmp";
			break;//not positive
		case 2:
			fileName = "E_LIGHT.bmp";
			break;//direct damage
		}
		break;
	case Bonus::EARTH_IMMUNITY:
		switch(bonus->subtype)
		{
		case 0:
			fileName = "E_SPEATH.bmp";
			break;//all
		case 1:
		case 2://no specific icon for direct damage immunity
			fileName = "E_SPEATH1.bmp";
			break;//not positive
		}
		break;
	case Bonus::LEVEL_SPELL_IMMUNITY:
	{
		if(vstd::iswithin(bonus->val, 1, 5))
		{
			fileName = "E_SPLVL" + std::to_string(bonus->val) + ".bmp";
		}
		break;
	}
	case Bonus::KING:
	{
		if(vstd::iswithin(bonus->val, 0, 3))
		{
			fileName = "E_KING" + std::to_string(std::max(1, bonus->val)) + ".bmp";
		}
		break;
	}
	case Bonus::GENERAL_DAMAGE_REDUCTION:
	{
		switch(bonus->subtype)
		{
		case 0:
			fileName = "DamageReductionMelee.bmp";
			break;
		case 1:
			fileName = "DamageReductionRanged.bmp";
			break;
		}
		break;
	}

	default:
		{
			const CBonusType & bt = bonusTypes[bonus->type];
			fileName = bt.icon;
			fullPath = true;
		}
		break;
	}

	if(!fileName.empty() && !fullPath)
		fileName = "zvs/Lib1.res/" + fileName;
	return fileName;
}

void CBonusTypeHandler::load()
{
	const JsonNode gameConf(ResourceID("config/gameConfig.json"));
	const JsonNode config(JsonUtils::assembleFromFiles(gameConf["bonuses"].convertTo<std::vector<std::string>>()));
	load(config);
}

void CBonusTypeHandler::load(const JsonNode & config)
{
	for(auto & node : config.Struct())
	{
		auto it = bonusNameMap.find(node.first);

		if(it == bonusNameMap.end())
		{
			//TODO: new bonus
//			CBonusType bt;
//			loadItem(node.second, bt);
//
//			auto new_id = bonusTypes.size();
//
//			bonusTypes.push_back(bt);

			logBonus->warn("Unrecognized bonus name! (%s)", node.first);
		}
		else
		{
			CBonusType & bt = bonusTypes[it->second];

			loadItem(node.second, bt, node.first);
			logBonus->trace("Loaded bonus type %s", node.first);
		}
	}
}

void CBonusTypeHandler::loadItem(const JsonNode & source, CBonusType & dest, const std::string & name)
{
	dest.identifier = name;
	dest.hidden = source["hidden"].Bool(); //Null -> false

	if (!dest.hidden)
	{
		VLC->generaltexth->registerString( "core", dest.getNameTextID(), source["name"].String());
		VLC->generaltexth->registerString( "core", dest.getDescriptionTextID(), source["description"].String());
	}

	const JsonNode & graphics = source["graphics"];

	if(!graphics.isNull())
		dest.icon = graphics["icon"].String();
}

VCMI_LIB_NAMESPACE_END
