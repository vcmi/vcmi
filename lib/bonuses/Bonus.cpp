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
#include "CBonusSystemNode.h"
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
	return toJsonNode().toJson(true);
}

JsonNode CAddInfo::toJsonNode() const
{
	if(size() < 2)
	{
		return JsonUtils::intNode(operator[](0));
	}
	else
	{
		JsonNode node(JsonNode::JsonType::DATA_VECTOR);
		for(si32 value : *this)
			node.Vector().push_back(JsonUtils::intNode(value));
		return node;
	}
}
std::string Bonus::Description(std::optional<si32> customValue) const
{
	std::ostringstream str;

	if(description.empty())
	{
		if(stacking.empty() || stacking == "ALWAYS")
		{
			switch(source)
			{
			case BonusSource::ARTIFACT:
				str << ArtifactID(sid).toArtifact(VLC->artifacts())->getNameTranslated();
				break;
			case BonusSource::SPELL_EFFECT:
				str << SpellID(sid).toSpell(VLC->spells())->getNameTranslated();
				break;
			case BonusSource::CREATURE_ABILITY:
				str << CreatureID(sid).toCreature(VLC->creatures())->getNamePluralTranslated();
				break;
			case BonusSource::SECONDARY_SKILL:
				str << VLC->skills()->getByIndex(sid)->getNameTranslated();
				break;
			case BonusSource::HERO_SPECIAL:
				str << VLC->heroTypes()->getByIndex(sid)->getNameTranslated();
				break;
			default:
				//todo: handle all possible sources
				str << "Unknown";
				break;
			}
		}
		else
			str << stacking;
	}
	else
	{
		str << description;
	}

	if(auto value = customValue.value_or(val))
		str << " " << std::showpos << value;

	return str.str();
}

static JsonNode subtypeToJson(BonusType type, int subtype)
{
	switch(type)
	{
	case BonusType::PRIMARY_SKILL:
		return JsonUtils::stringNode("primSkill." + NPrimarySkill::names[subtype]);
	case BonusType::SPECIAL_SPELL_LEV:
	case BonusType::SPECIFIC_SPELL_DAMAGE:
	case BonusType::SPELL:
	case BonusType::SPECIAL_PECULIAR_ENCHANT:
	case BonusType::SPECIAL_ADD_VALUE_ENCHANT:
	case BonusType::SPECIAL_FIXED_VALUE_ENCHANT:
		return JsonUtils::stringNode(ModUtility::makeFullIdentifier("", "spell", SpellID::encode(subtype)));
	case BonusType::IMPROVED_NECROMANCY:
	case BonusType::SPECIAL_UPGRADE:
		return JsonUtils::stringNode(ModUtility::makeFullIdentifier("", "creature", CreatureID::encode(subtype)));
	case BonusType::GENERATE_RESOURCE:
		return JsonUtils::stringNode("resource." + GameConstants::RESOURCE_NAMES[subtype]);
	default:
		return JsonUtils::intNode(subtype);
	}
}

static JsonNode additionalInfoToJson(BonusType type, CAddInfo addInfo)
{
	switch(type)
	{
	case BonusType::SPECIAL_UPGRADE:
		return JsonUtils::stringNode(ModUtility::makeFullIdentifier("", "creature", CreatureID::encode(addInfo[0])));
	default:
		return addInfo.toJsonNode();
	}
}

JsonNode Bonus::toJsonNode() const
{
	JsonNode root(JsonNode::JsonType::DATA_STRUCT);
	// only add values that might reasonably be found in config files
	root["type"].String() = vstd::findKey(bonusNameMap, type);
	if(subtype != -1)
		root["subtype"] = subtypeToJson(type, subtype);
	if(additionalInfo != CAddInfo::NONE)
		root["addInfo"] = additionalInfoToJson(type, additionalInfo);
	if(turnsRemain != 0)
		root["turns"].Integer() = turnsRemain;
	if(source != BonusSource::OTHER)
		root["sourceType"].String() = vstd::findKey(bonusSourceMap, source);
	if(targetSourceType != BonusSource::OTHER)
		root["targetSourceType"].String() = vstd::findKey(bonusSourceMap, targetSourceType);
	if(sid != 0)
		root["sourceID"].Integer() = sid;
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

Bonus::Bonus(BonusDuration::Type Duration, BonusType Type, BonusSource Src, si32 Val, ui32 ID, std::string Desc, si32 Subtype):
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

Bonus::Bonus(BonusDuration::Type Duration, BonusType Type, BonusSource Src, si32 Val, ui32 ID, si32 Subtype, BonusValueType ValType):
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
	printField(subtype);
	printField(duration.to_ulong());
	printField(source);
	printField(sid);
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
