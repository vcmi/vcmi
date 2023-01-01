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
#include "spells/CSpellHandler.h"

template class std::vector<VCMI_LIB_WRAP_NAMESPACE(CBonusType)>;

VCMI_LIB_NAMESPACE_BEGIN

///MacroString

MacroString::MacroString(const std::string & format)
{
	static const std::string MACRO_START = "${";
	static const std::string MACRO_END = "}";
	static const size_t MACRO_START_L = 2;
	static const size_t MACRO_END_L = 1;

	size_t end_pos = 0;
	size_t start_pos = std::string::npos;
	do
	{
		start_pos = format.find(MACRO_START, end_pos);

		if(!(start_pos == std::string::npos))
		{
			//chunk before macro
			items.push_back(Item(Item::STRING, format.substr(end_pos, start_pos - end_pos)));

			start_pos += MACRO_START_L;
			end_pos = format.find(MACRO_END, start_pos);

			if(end_pos == std::string::npos)
			{
				logBonus->warn("Format error in: %s", format);
				end_pos = start_pos;
				break;
			}
			else
			{
				items.push_back(Item(Item::MACRO, format.substr(start_pos, end_pos - start_pos)));
				end_pos += MACRO_END_L;
			}
		}
	}
	while(start_pos != std::string::npos);

	//no more macros
	items.push_back(Item(Item::STRING, format.substr(end_pos)));
}

std::string MacroString::build(const GetValue & getValue) const
{
	std::string result;

	for(const Item & i : items)
	{
		switch(i.type)
		{
		case Item::MACRO:
		{
			result += getValue(i.value);
			break;
		}
		case Item::STRING:
		{
			result += i.value;
			break;
		}
		}
	}
	return result;
}

///CBonusType

CBonusType::CBonusType()
{
	hidden = true;
	icon.clear();
	nameTemplate.clear();
	descriptionTemplate.clear();
}

CBonusType::~CBonusType()
{

}

void CBonusType::buildMacros()
{
	name = MacroString(nameTemplate);
	description = MacroString(descriptionTemplate);
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
	auto getValue = [=](const std::string & name) -> std::string
	{
		if(name == "val")
		{
			 return boost::lexical_cast<std::string>(bearer->valOfBonuses(Selector::typeSubtype(bonus->type, bonus->subtype)));
		}
		else if(name == "subtype.creature")
		{
			 const CreatureID cre(bonus->subtype);
			 return cre.toCreature()->namePl;
		}
		else if(name == "subtype.spell")
		{
			 const SpellID sp(bonus->subtype);
			 return sp.toSpell()->getNameTranslated();
		}
		else if(name == "MR")
		{
			 return boost::lexical_cast<std::string>(bearer->magicResistance());
		}
		else
		{
			 logBonus->warn("Unknown macro in bonus config: %s", name);
			 return "[error]";
		}
	};

	const CBonusType & bt = bonusTypes[bonus->type];
	if(bt.hidden)
		return "";
	const MacroString & macro = description ? bt.description : bt.name;

	return macro.build(getValue);
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
			fileName = "E_SPLVL" + boost::lexical_cast<std::string>(bonus->val) + ".bmp";
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

			logBonus->warn("Adding new bonuses not implemented (%s)", node.first);
		}
		else
		{
			CBonusType & bt = bonusTypes[it->second];

			loadItem(node.second, bt);
			logBonus->trace("Loaded bonus type %s", node.first);
		}
	}
}

void CBonusTypeHandler::loadItem(const JsonNode & source, CBonusType & dest)
{
	dest.nameTemplate = source["name"].String();
	dest.descriptionTemplate = source["description"].String();
	dest.hidden = source["hidden"].Bool(); //Null -> false

	const JsonNode & graphics = source["graphics"];

	if(!graphics.isNull())
	{
		dest.icon = graphics["icon"].String();
	}
	dest.buildMacros();
}

VCMI_LIB_NAMESPACE_END
