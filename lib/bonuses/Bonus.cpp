/*
 * Bonus.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Bonus.h"
#include "Limiters.h"
#include "Updaters.h"
#include "Propagators.h"

#include "../VCMI_Lib.h"
#include "../spells/CSpellHandler.h"
#include "../CCreatureHandler.h"
#include "../CCreatureSet.h"
#include "../CHeroHandler.h"
#include "../CTownHandler.h"
#include "../CGeneralTextHandler.h"
#include "../CSkillHandler.h"
#include "../CArtHandler.h"
#include "../TerrainHandler.h"
#include "../constants/StringConstants.h"
#include "../battle/BattleInfo.h"
#include "../modding/ModUtility.h"

VCMI_LIB_NAMESPACE_BEGIN

//This constructor should be placed here to avoid side effects
CAddInfo::CAddInfo() = default;

CAddInfo::CAddInfo(si32 value)
{
	if(value != CAddInfo::NONE)
		push_back(value);
}

bool CAddInfo::operator==(si32 value) const
{
	switch(size())
	{
	case 0:
		return value == CAddInfo::NONE;
	case 1:
		return operator[](0) == value;
	default:
		return false;
	}
}

bool CAddInfo::operator!=(si32 value) const
{
	return !operator==(value);
}

si32 & CAddInfo::operator[](size_type pos)
{
	if(pos >= size())
		resize(pos + 1, CAddInfo::NONE);
	return vector::operator[](pos);
}

si32 CAddInfo::operator[](size_type pos) const
{
	return pos < size() ? vector::operator[](pos) : CAddInfo::NONE;
}

std::string CAddInfo::toString() const
{
	return toJsonNode().toCompactString();
}

JsonNode CAddInfo::toJsonNode() const
{
	if(size() < 2)
	{
		return JsonNode(operator[](0));
	}
	else
	{
		JsonNode node;
		for(si32 value : *this)
			node.Vector().emplace_back(value);
		return node;
	}
}
std::string Bonus::Description(std::optional<si32> customValue) const
{
	std::string str;

	if(description.empty())
	{
		if(stacking.empty() || stacking == "ALWAYS")
		{
			switch(source)
			{
			case BonusSource::ARTIFACT:
				str = sid.as<ArtifactID>().toEntity(VLC)->getNameTranslated();
				break;
			case BonusSource::SPELL_EFFECT:
				str = sid.as<SpellID>().toEntity(VLC)->getNameTranslated();
				break;
			case BonusSource::CREATURE_ABILITY:
				str = sid.as<CreatureID>().toEntity(VLC)->getNamePluralTranslated();
				break;
			case BonusSource::SECONDARY_SKILL:
				str = VLC->skills()->getById(sid.as<SecondarySkill>())->getNameTranslated();
				break;
			case BonusSource::HERO_SPECIAL:
				str = VLC->heroTypes()->getById(sid.as<HeroTypeID>())->getNameTranslated();
				break;
			default:
				//todo: handle all possible sources
				str = "Unknown";
				break;
			}
		}
		else
			str = stacking;
	}
	else
	{
		str = description;
	}

	if(auto value = customValue.value_or(val)) {
		//arraytxt already contains +-value
		std::string valueString = boost::str(boost::format(" %+d") % value);
		if(!boost::algorithm::ends_with(str, valueString))
			str += valueString;
	}

	return str;
}

static JsonNode additionalInfoToJson(BonusType type, CAddInfo addInfo)
{
	switch(type)
	{
	case BonusType::SPECIAL_UPGRADE:
		return JsonNode(ModUtility::makeFullIdentifier("", "creature", CreatureID::encode(addInfo[0])));
	default:
		return addInfo.toJsonNode();
	}
}

JsonNode Bonus::toJsonNode() const
{
	JsonNode root;
	// only add values that might reasonably be found in config files
	root["type"].String() = vstd::findKey(bonusNameMap, type);
	if(subtype != BonusSubtypeID())
		root["subtype"].String() = subtype.toString();
	if(additionalInfo != CAddInfo::NONE)
		root["addInfo"] = additionalInfoToJson(type, additionalInfo);
	if(source != BonusSource::OTHER)
		root["sourceType"].String() = vstd::findKey(bonusSourceMap, source);
	if(targetSourceType != BonusSource::OTHER)
		root["targetSourceType"].String() = vstd::findKey(bonusSourceMap, targetSourceType);
	if(sid != BonusSourceID())
		root["sourceID"].String() = sid.toString();
	if(val != 0)
		root["val"].Integer() = val;
	if(valType != BonusValueType::ADDITIVE_VALUE)
		root["valueType"].String() = vstd::findKey(bonusValueMap, valType);
	if(!stacking.empty())
		root["stacking"].String() = stacking;
	if(!description.empty())
		root["description"].String() = description;
	if(effectRange != BonusLimitEffect::NO_LIMIT)
		root["effectRange"].String() = vstd::findKey(bonusLimitEffect, effectRange);
	if(duration != BonusDuration::PERMANENT)
		root["duration"] = BonusDuration::toJson(duration);
	if(turnsRemain)
		root["turns"].Integer() = turnsRemain;
	if(limiter)
		root["limiters"] = limiter->toJsonNode();
	if(updater)
		root["updater"] = updater->toJsonNode();
	if(propagator)
		root["propagator"].String() = vstd::findKey(bonusPropagatorMap, propagator);
	return root;
}

Bonus::Bonus(BonusDuration::Type Duration, BonusType Type, BonusSource Src, si32 Val, BonusSourceID ID)
	: Bonus(Duration, Type, Src, Val, ID, BonusSubtypeID(), std::string())
{}

Bonus::Bonus(BonusDuration::Type Duration, BonusType Type, BonusSource Src, si32 Val, BonusSourceID ID, std::string Desc)
	: Bonus(Duration, Type, Src, Val, ID, BonusSubtypeID(), Desc)
{}

Bonus::Bonus(BonusDuration::Type Duration, BonusType Type, BonusSource Src, si32 Val, BonusSourceID ID, BonusSubtypeID Subtype)
	: Bonus(Duration, Type, Src, Val, ID, Subtype, std::string())
{}

Bonus::Bonus(BonusDuration::Type Duration, BonusType Type, BonusSource Src, si32 Val, BonusSourceID ID, BonusSubtypeID Subtype, std::string Desc):
	duration(Duration),
	type(Type),
	subtype(Subtype),
	source(Src),
	val(Val),
	sid(ID),
	description(std::move(Desc))
{
	boost::algorithm::trim(description);
	targetSourceType = BonusSource::OTHER;
}

Bonus::Bonus(BonusDuration::Type Duration, BonusType Type, BonusSource Src, si32 Val, BonusSourceID ID, BonusSubtypeID Subtype, BonusValueType ValType):
	duration(Duration),
	type(Type),
	subtype(Subtype),
	source(Src),
	val(Val),
	sid(ID),
	valType(ValType)
{
	turnsRemain = 0;
	effectRange = BonusLimitEffect::NO_LIMIT;
	targetSourceType = BonusSource::OTHER;
}

std::shared_ptr<Bonus> Bonus::addPropagator(const TPropagatorPtr & Propagator)
{
	propagator = Propagator;
	return this->shared_from_this();
}

DLL_LINKAGE std::ostream & operator<<(std::ostream &out, const Bonus &bonus)
{
	for(const auto & i : bonusNameMap)
	if(i.second == bonus.type)
		out << "\tType: " << i.first << " \t";

#define printField(field) out << "\t" #field ": " << (int)bonus.field << "\n"
	printField(val);
	out << "\tSubtype: " << bonus.subtype.toString() << "\n";
	printField(duration.to_ulong());
	printField(source);
	out << "\tSource ID: " << bonus.sid.toString() << "\n";
	if(bonus.additionalInfo != CAddInfo::NONE)
		out << "\taddInfo: " << bonus.additionalInfo.toString() << "\n";
	printField(turnsRemain);
	printField(valType);
	if(!bonus.stacking.empty())
		out << "\tstacking: \"" << bonus.stacking << "\"\n";
	printField(effectRange);
#undef printField

	if(bonus.limiter)
		out << "\tLimiter: " << bonus.limiter->toString() << "\n";
	if(bonus.updater)
		out << "\tUpdater: " << bonus.updater->toString() << "\n";

	return out;
}

std::shared_ptr<Bonus> Bonus::addLimiter(const TLimiterPtr & Limiter)
{
	if (limiter)
	{
		//If we already have limiter list, retrieve it
		auto limiterList = std::dynamic_pointer_cast<AllOfLimiter>(limiter);
		if(!limiterList)
		{
			//Create a new limiter list with old limiter and the new one will be pushed later
			limiterList = std::make_shared<AllOfLimiter>();
			limiterList->add(limiter);
			limiter = limiterList;
		}

		limiterList->add(Limiter);
	}
	else
	{
		limiter = Limiter;
	}
	return this->shared_from_this();
}

// Updaters

std::shared_ptr<Bonus> Bonus::addUpdater(const TUpdaterPtr & Updater)
{
	updater = Updater;
	return this->shared_from_this();
}

VCMI_LIB_NAMESPACE_END
