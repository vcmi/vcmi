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
		if (value.isVector())
		{
			const auto & vector = value.Vector();

			size_t index= rng.getIntRange(0, vector.size()-1)();
			return loadValue(vector[index], rng, 0);
		}
		if (!value["amount"].isNull())
			return static_cast<si32>(loadValue(value["amount"], rng, defaultValue));
		si32 min = static_cast<si32>(loadValue(value["min"], rng, 0));
		si32 max = static_cast<si32>(loadValue(value["max"], rng, 0));
		return rng.getIntRange(min, max)();
	}

	DLL_LINKAGE std::string loadKey(const JsonNode & value, CRandomGenerator & rng, std::string defaultValue)
	{
		if (value.isNull())
			return defaultValue;
		if (value.isString())
			return value.String();
		if (!value["type"].isNull())
			return value["type"].String();

		if (value["list"].isNull())
			return defaultValue;

		const auto & resourceList = value["list"].Vector();

		if (resourceList.empty())
			return defaultValue;

		si32 index = rng.getIntRange(0, resourceList.size() - 1 )();

		return resourceList[index].String();
	}

	TResources loadResources(const JsonNode & value, CRandomGenerator & rng)
	{
		TResources ret;

		if (value.isVector())
		{
			for (const auto & entry : value.Vector())
				ret += loadResource(entry, rng);
			return ret;
		}

		for (size_t i=0; i<GameConstants::RESOURCE_QUANTITY; i++)
		{
			ret[i] = loadValue(value[GameConstants::RESOURCE_NAMES[i]], rng);
		}
		return ret;
	}

	TResources loadResource(const JsonNode & value, CRandomGenerator & rng)
	{
		std::string resourceName = loadKey(value, rng, "");
		si32 resourceAmount = loadValue(value, rng, 0);
		
		if(!value["simple"].isNull())
		{
			si32 index = *RandomGeneratorUtil::nextItem(std::vector<si32>{Res::WOOD, Res::ORE}, rng);
			resourceName = GameConstants::RESOURCE_NAMES[index];
		}
		if(!value["precious"].isNull())
		{
			si32 index = *RandomGeneratorUtil::nextItem(std::vector<si32>{Res::MERCURY, Res::GEMS, Res::SULFUR, Res::CRYSTAL}, rng);
			resourceName = GameConstants::RESOURCE_NAMES[index];
		}
		if(!value["random"].isNull())
		{
			//enumeratign resources to exclude mithril
			si32 index = *RandomGeneratorUtil::nextItem(std::vector<si32>{Res::WOOD, Res::ORE, Res::MERCURY, Res::GEMS, Res::SULFUR, Res::CRYSTAL, Res::GOLD}, rng);
			if(index == Res::GOLD)
				resourceAmount *= 100;
			resourceName = GameConstants::RESOURCE_NAMES[index];
		}
		
		si32 resourceID(VLC->modh->identifiers.getIdentifier(value.meta, "resource", resourceName).get());

		TResources ret;
		ret[resourceID] = resourceAmount;
		return ret;
	}


	std::vector<si32> loadPrimary(const JsonNode & value, CRandomGenerator & rng)
	{
		std::vector<si32> ret;
		for(const auto & name : PrimarySkill::names)
		{
			ret.push_back(loadValue(value[name], rng));
		}
		
		if(!value["random"].isNull())
		{
			*RandomGeneratorUtil::nextItem(ret, rng) = loadValue(value["random"], rng);
		}
		
		return ret;
	}

	std::map<SecondarySkill, si32> loadSecondary(const JsonNode & value, CRandomGenerator & rng)
	{
		std::map<SecondarySkill, si32> ret;
		for(const auto & pair : value.Struct())
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
			allowedClasses.insert(CArtHandler::stringToClass(value["class"].String()));
		else
			for(const auto & entry : value["class"].Vector())
				allowedClasses.insert(CArtHandler::stringToClass(entry.String()));

		if (value["slot"].getType() == JsonNode::JsonType::DATA_STRING)
			allowedPositions.insert(ArtifactPosition(value["class"].String()));
		else
			for(const auto & entry : value["slot"].Vector())
				allowedPositions.insert(ArtifactPosition(entry.String()));

		if (!value["minValue"].isNull()) minValue = static_cast<ui32>(value["minValue"].Float());
		if (!value["maxValue"].isNull()) maxValue = static_cast<ui32>(value["maxValue"].Float());

		return VLC->arth->pickRandomArtifact(rng, [=](const ArtifactID & artID) -> bool
		{
			CArtifact * art = VLC->arth->objects[artID];

			if(!vstd::iswithin(art->price, minValue, maxValue))
				return false;

			if(!allowedClasses.empty() && !allowedClasses.count(art->aClass))
				return false;

			if(!allowedPositions.empty())
			{
				for(const auto & pos : art->possibleSlots[ArtBearer::HERO])
				{
					if(allowedPositions.count(pos))
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

		vstd::erase_if(spells, [=](const SpellID & spell)
		{
			return VLC->spellh->getById(spell)->getLevel() != si32(value["level"].Float());
		});

		return SpellID(*RandomGeneratorUtil::nextItem(spells, rng));
	}

	std::vector<SpellID> loadSpells(const JsonNode & value, CRandomGenerator & rng, const std::vector<SpellID> & spells)
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
				for(const auto & creaID : crea->upgrades)
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
