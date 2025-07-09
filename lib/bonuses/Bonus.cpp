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

#include "../CBonusTypeHandler.h"
#include "../CCreatureHandler.h"
#include "../CSkillHandler.h"
#include "../TerrainHandler.h"
#include "../GameLibrary.h"
#include "../callback/IGameInfoCallback.h"
#include "../mapObjects/CGObjectInstance.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../battle/BattleInfo.h"
#include "../constants/StringConstants.h"
#include "../entities/hero/CHero.h"
#include "../modding/ModUtility.h"
#include "../spells/CSpellHandler.h"
#include "../texts/CGeneralTextHandler.h"

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
std::string Bonus::Description(const IGameInfoCallback * cb, std::optional<si32> customValue) const
{
	MetaString descriptionHelper = description;
	auto valueToShow = customValue.value_or(val);

	if(descriptionHelper.empty())
	{
		// no custom description - try to generate one based on bonus source
		switch(source)
		{
			case BonusSource::ARTIFACT:
				descriptionHelper.appendName(sid.as<ArtifactID>());
				break;
			case BonusSource::SPELL_EFFECT:
				descriptionHelper.appendName(sid.as<SpellID>());
				break;
			case BonusSource::CREATURE_ABILITY:
				descriptionHelper.appendNamePlural(sid.as<CreatureID>());
				break;
			case BonusSource::SECONDARY_SKILL:
				descriptionHelper.appendTextID(sid.as<SecondarySkill>().toEntity(LIBRARY)->getNameTextID());
				break;
			case BonusSource::HERO_SPECIAL:
				descriptionHelper.appendTextID(sid.as<HeroTypeID>().toEntity(LIBRARY)->getNameTextID());
				break;
			case BonusSource::OBJECT_INSTANCE:
				const auto * object = cb->getObj(sid.as<ObjectInstanceID>());
				if (object)
					descriptionHelper.appendTextID(LIBRARY->objtypeh->getObjectName(object->ID, object->subID));
		}
	}

	if(descriptionHelper.empty())
	{
		// still no description - try to generate one based on duration
		if ((duration & BonusDuration::ONE_BATTLE) != 0)
		{
			if (val > 0)
				descriptionHelper.appendTextID("core.arraytxt.110"); //+%d Temporary until next battle"
			else
				descriptionHelper.appendTextID("core.arraytxt.109"); //-%d Temporary until next battle"

			// erase sign - already present in description string
			valueToShow = std::abs(valueToShow);
		}
	}

	if(descriptionHelper.empty())
	{
		// still no description - generate placeholder one
		descriptionHelper.appendRawString("Unknown");
	}

	if(valueToShow != 0)
	{
		descriptionHelper.replacePositiveNumber(valueToShow);
		// there is one known string that uses '%s' placeholder for bonus value:
		// "core.arraytxt.69" : "\nFountain of Fortune Visited %s",
		// So also add string replacement to handle this case
		descriptionHelper.replaceRawString(std::to_string(valueToShow));

		if(type == BonusType::CREATURE_GROWTH_PERCENT)
			descriptionHelper.appendRawString(" +" + std::to_string(valueToShow));
	}

	return descriptionHelper.toString();
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
	root["type"].String() = LIBRARY->bth->bonusToString(type);
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
		root["description"].String() = description.toString();
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
	if(hidden)
		root["hidden"].Bool() = hidden;
	return root;
}

Bonus::Bonus(BonusDuration::Type Duration, BonusType Type, BonusSource Src, si32 Val, BonusSourceID ID)
	: Bonus(Duration, Type, Src, Val, ID, BonusSubtypeID())
{}

Bonus::Bonus(BonusDuration::Type Duration, BonusType Type, BonusSource Src, si32 Val, BonusSourceID ID, BonusSubtypeID Subtype):
	duration(Duration),
	type(Type),
	subtype(Subtype),
	source(Src),
	val(Val),
	sid(ID)
{
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

Bonus::Bonus(const Bonus & inst, const BonusSourceID & sourceId)
	: Bonus(inst)
{
	sid = sourceId;
}

std::shared_ptr<Bonus> Bonus::addPropagator(const TPropagatorPtr & Propagator)
{
	propagator = Propagator;
	return this->shared_from_this();
}

DLL_LINKAGE std::ostream & operator<<(std::ostream &out, const Bonus &bonus)
{
	out << "\tType: " << LIBRARY->bth->bonusToString(bonus.type) << " \t";

#define printField(field) out << "\t" #field ": " << (int)bonus.field << "\n"
	printField(val);
	out << "\tSubtype: " << bonus.subtype.toString() << "\n";
	printField(duration);
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
		auto newLimiterList = std::make_shared<AllOfLimiter>();
		auto oldLimiterList = std::dynamic_pointer_cast<const AllOfLimiter>(limiter);

		if(oldLimiterList)
			newLimiterList->limiters = oldLimiterList->limiters;

		newLimiterList->add(Limiter);
		limiter = newLimiterList;
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
