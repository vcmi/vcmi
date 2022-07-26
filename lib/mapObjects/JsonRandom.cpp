/*
 * JsonRandom.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "JsonRandom.h"

#include "../JsonNode.h"
#include "../CRandomGenerator.h"
#include "../StringConstants.h"
#include "../VCMI_Lib.h"
#include "../CModHandler.h"
#include "../CArtHandler.h"
#include "../CCreatureHandler.h"
#include "../CCreatureSet.h"
#include "../spells/CSpellHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace JsonRandom
{
	si32 loadValue(const JsonNode & value, CRandomGenerator & rng, si32 defaultValue)
	{
		if (value.isNull())
			return defaultValue;
		if (value.isNumber())
			return static_cast<si32>(value.Float());
		if (!value["amount"].isNull())
			return static_cast<si32>(value["amount"].Float());
		si32 min = static_cast<si32>(value["min"].Float());
		si32 max = static_cast<si32>(value["max"].Float());
		return rng.getIntRange(min, max)();
	}

	TResources loadResources(const JsonNode & value, CRandomGenerator & rng)
	{
		TResources ret;
		for (size_t i=0; i<GameConstants::RESOURCE_QUANTITY; i++)
		{
			ret[i] = loadValue(value[GameConstants::RESOURCE_NAMES[i]], rng);
		}
		return ret;
	}

	std::vector<si32> loadPrimary(const JsonNode & value, CRandomGenerator & rng)
	{
		std::vector<si32> ret;
		for (auto & name : PrimarySkill::names)
		{
			ret.push_back(loadValue(value[name], rng));
		}
		return ret;
	}

	std::map<SecondarySkill, si32> loadSecondary(const JsonNode & value, CRandomGenerator & rng)
	{
		std::map<SecondarySkill, si32> ret;
		for (auto & pair : value.Struct())
		{
			SecondarySkill id(VLC->modh->identifiers.getIdentifier(pair.second.meta, "skill", pair.first).get());
			ret[id] = loadValue(pair.second, rng);
		}
		return ret;
	}

	ArtifactID loadArtifact(const JsonNode & value, CRandomGenerator & rng)
	{
		if (value.getType() == JsonNode::JsonType::DATA_STRING)
			return ArtifactID(VLC->modh->identifiers.getIdentifier("artifact", value).get());

		std::set<CArtifact::EartClass> allowedClasses;
		std::set<ArtifactPosition> allowedPositions;
		ui32 minValue = 0;
		ui32 maxValue = std::numeric_limits<ui32>::max();

		if (value["class"].getType() == JsonNode::JsonType::DATA_STRING)
			allowedClasses.insert(VLC->arth->stringToClass(value["class"].String()));
		else
			for (auto & entry : value["class"].Vector())
				allowedClasses.insert(VLC->arth->stringToClass(entry.String()));

		if (value["slot"].getType() == JsonNode::JsonType::DATA_STRING)
			allowedPositions.insert(VLC->arth->stringToSlot(value["class"].String()));
		else
			for (auto & entry : value["slot"].Vector())
				allowedPositions.insert(VLC->arth->stringToSlot(entry.String()));

		if (!value["minValue"].isNull()) minValue = static_cast<ui32>(value["minValue"].Float());
		if (!value["maxValue"].isNull()) maxValue = static_cast<ui32>(value["maxValue"].Float());

		return VLC->arth->pickRandomArtifact(rng, [=](ArtifactID artID) -> bool
		{
			CArtifact * art = VLC->arth->objects[artID];

			if (!vstd::iswithin(art->price, minValue, maxValue))
				return false;

			if (!allowedClasses.empty() && !allowedClasses.count(art->aClass))
				return false;

			if (!allowedPositions.empty())
			{
				for (auto pos : art->possibleSlots[ArtBearer::HERO])
				{
					if (allowedPositions.count(pos))
						return true;
				}
				return false;
			}
			return true;
		});
	}

	std::vector<ArtifactID> loadArtifacts(const JsonNode & value, CRandomGenerator & rng)
	{
		std::vector<ArtifactID> ret;
		for (const JsonNode & entry : value.Vector())
		{
			ret.push_back(loadArtifact(entry, rng));
		}
		return ret;
	}

	SpellID loadSpell(const JsonNode & value, CRandomGenerator & rng, std::vector<SpellID> spells)
	{
		if (value.getType() == JsonNode::JsonType::DATA_STRING)
			return SpellID(VLC->modh->identifiers.getIdentifier("spell", value).get());
		if (value["type"].getType() == JsonNode::JsonType::DATA_STRING)
			return SpellID(VLC->modh->identifiers.getIdentifier("spell", value["type"]).get());

		vstd::erase_if(spells, [=](SpellID spell)
		{
			return VLC->spellh->objects[spell]->level != si32(value["level"].Float());
		});

		return SpellID(*RandomGeneratorUtil::nextItem(spells, rng));
	}

	std::vector<SpellID> loadSpells(const JsonNode & value, CRandomGenerator & rng, std::vector<SpellID> spells)
	{
		// possible extensions: (taken from spell json config)
		// "type": "adventure",//"adventure", "combat", "ability"
		// "school": {"air":true, "earth":true, "fire":true, "water":true},
		// "level": 1,

		std::vector<SpellID> ret;
		for (const JsonNode & entry : value.Vector())
		{
			ret.push_back(loadSpell(entry, rng, spells));
		}
		return ret;
	}

	CStackBasicDescriptor loadCreature(const JsonNode & value, CRandomGenerator & rng)
	{
		CStackBasicDescriptor stack;
		stack.type = VLC->creh->objects[VLC->modh->identifiers.getIdentifier("creature", value["type"]).get()];
		stack.count = loadValue(value, rng);
		if (!value["upgradeChance"].isNull() && !stack.type->upgrades.empty())
		{
			if (int(value["upgradeChance"].Float()) > rng.nextInt(99)) // select random upgrade
			{
				stack.type = VLC->creh->objects[*RandomGeneratorUtil::nextItem(stack.type->upgrades, rng)];
			}
		}
		return stack;
	}

	std::vector<CStackBasicDescriptor> loadCreatures(const JsonNode & value, CRandomGenerator & rng)
	{
		std::vector<CStackBasicDescriptor> ret;
		for (const JsonNode & node : value.Vector())
		{
			ret.push_back(loadCreature(node, rng));
		}
		return ret;
	}

	std::vector<RandomStackInfo> evaluateCreatures(const JsonNode & value)
	{
		std::vector<RandomStackInfo> ret;
		for (const JsonNode & node : value.Vector())
		{
			RandomStackInfo info;

			if (!node["amount"].isNull())
				info.minAmount = info.maxAmount = static_cast<si32>(node["amount"].Float());
			else
			{
				info.minAmount = static_cast<si32>(node["min"].Float());
				info.maxAmount = static_cast<si32>(node["max"].Float());
			}
			const CCreature * crea = VLC->creh->objects[VLC->modh->identifiers.getIdentifier("creature", node["type"]).get()];
			info.allowedCreatures.push_back(crea);
			if (node["upgradeChance"].Float() > 0)
			{
				for (auto creaID : crea->upgrades)
					info.allowedCreatures.push_back(VLC->creh->objects[creaID]);
			}
			ret.push_back(info);
		}
		return ret;
	}

	//std::vector<Component> loadComponents(const JsonNode & value)
	//{
	//	std::vector<Component> ret;
	//	return ret;
	//	//TODO
	//}

	std::vector<Bonus> DLL_LINKAGE loadBonuses(const JsonNode & value)
	{
		std::vector<Bonus> ret;
		for (const JsonNode & entry : value.Vector())
		{
			auto bonus = JsonUtils::parseBonus(entry);
			ret.push_back(*bonus);
		}
		return ret;
	}

}

VCMI_LIB_NAMESPACE_END
