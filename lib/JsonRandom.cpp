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

#include <vstd/StringUtils.h>

#include "JsonNode.h"
#include "CRandomGenerator.h"
#include "constants/StringConstants.h"
#include "VCMI_Lib.h"
#include "CArtHandler.h"
#include "CCreatureHandler.h"
#include "CCreatureSet.h"
#include "spells/CSpellHandler.h"
#include "CSkillHandler.h"
#include "CHeroHandler.h"
#include "IGameCallback.h"
#include "mapObjects/IObjectInterface.h"
#include "modding/IdentifierStorage.h"
#include "modding/ModScope.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace JsonRandom
{
	si32 loadValue(const JsonNode & value, CRandomGenerator & rng, si32 defaultValue)
	{
		if(value.isNull())
			return defaultValue;
		if(value.isNumber())
			return static_cast<si32>(value.Float());
		if(value.isVector())
		{
			const auto & vector = value.Vector();

			size_t index= rng.getIntRange(0, vector.size()-1)();
			return loadValue(vector[index], rng, 0);
		}
		if(value.isStruct())
		{
			if (!value["amount"].isNull())
				return static_cast<si32>(loadValue(value["amount"], rng, defaultValue));
			si32 min = static_cast<si32>(loadValue(value["min"], rng, 0));
			si32 max = static_cast<si32>(loadValue(value["max"], rng, 0));
			return rng.getIntRange(min, max)();
		}
		return defaultValue;
	}

	template<typename IdentifierType>
	IdentifierType decodeKey(const JsonNode & value)
	{
		return IdentifierType(*VLC->identifiers()->getIdentifier(IdentifierType::entityType(), value));
	}

	template<>
	PrimarySkill decodeKey(const JsonNode & value)
	{
		return PrimarySkill(*VLC->identifiers()->getIdentifier("primarySkill", value));
	}

	/// Method that allows type-specific object filtering
	/// Default implementation is to accept all input objects
	template<typename IdentifierType>
	std::set<IdentifierType> filterKeysTyped(const JsonNode & value, const std::set<IdentifierType> & valuesSet)
	{
		return valuesSet;
	}

	template<>
	std::set<ArtifactID> filterKeysTyped(const JsonNode & value, const std::set<ArtifactID> & valuesSet)
	{
		assert(value.isStruct());

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
			allowedPositions.insert(ArtifactPosition::decode(value["class"].String()));
		else
			for(const auto & entry : value["slot"].Vector())
				allowedPositions.insert(ArtifactPosition::decode(entry.String()));

		if (!value["minValue"].isNull())
			minValue = static_cast<ui32>(value["minValue"].Float());
		if (!value["maxValue"].isNull())
			maxValue = static_cast<ui32>(value["maxValue"].Float());

		std::set<ArtifactID> result;

		for (auto const & artID : valuesSet)
		{
			CArtifact * art = VLC->arth->objects[artID];

			if(!vstd::iswithin(art->getPrice(), minValue, maxValue))
				continue;

			if(!allowedClasses.empty() && !allowedClasses.count(art->aClass))
				continue;

			if(!IObjectInterface::cb->isAllowed(1, art->getIndex()))
				continue;

			if(!allowedPositions.empty())
			{
				bool positionAllowed = false;
				for(const auto & pos : art->getPossibleSlots().at(ArtBearer::HERO))
				{
					if(allowedPositions.count(pos))
						positionAllowed = true;
				}

				if (!positionAllowed)
					continue;
			}

			result.insert(artID);
		}
		return result;
	}

	template<>
	std::set<SpellID> filterKeysTyped(const JsonNode & value, const std::set<SpellID> & valuesSet)
	{
		std::set<SpellID> result = valuesSet;

		if (!value["level"].isNull())
		{
			int32_t spellLevel = value["level"].Float();

			vstd::erase_if(result, [=](const SpellID & spell)
			{
				return VLC->spellh->getById(spell)->getLevel() != spellLevel;
			});
		}

		if (!value["school"].isNull())
		{
			int32_t schoolID = VLC->identifiers()->getIdentifier("spellSchool", value["school"]).value();

			vstd::erase_if(result, [=](const SpellID & spell)
			{
				return !VLC->spellh->getById(spell)->hasSchool(SpellSchool(schoolID));
			});
		}
		return result;
	}

	template<typename IdentifierType>
	std::set<IdentifierType> filterKeys(const JsonNode & value, const std::set<IdentifierType> & valuesSet)
	{
		if(value.isString())
			return { decodeKey<IdentifierType>(value) };

		assert(value.isStruct());

		if(value.isStruct())
		{
			if(!value["type"].isNull())
				return filterKeys(value["type"], valuesSet);

			std::set<IdentifierType> filteredTypes = filterKeysTyped(value, valuesSet);

			if(!value["anyOf"].isNull())
			{
				std::set<IdentifierType> filteredAnyOf;
				for (auto const & entry : value["anyOf"].Vector())
				{
					std::set<IdentifierType> subset = filterKeys(entry, valuesSet);
					filteredAnyOf.insert(subset.begin(), subset.end());
				}

				vstd::erase_if(filteredTypes, [&](const IdentifierType & value)
				{
					return filteredAnyOf.count(value) == 0;
				});
			}

			if(!value["noneOf"].isNull())
			{
				for (auto const & entry : value["noneOf"].Vector())
				{
					std::set<IdentifierType> subset = filterKeys(entry, valuesSet);
					for (auto bannedEntry : subset )
						filteredTypes.erase(bannedEntry);
				}
			}

			return filteredTypes;
		}
		return valuesSet;
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
		std::set<GameResID> defaultResources{
			GameResID::WOOD,
			GameResID::MERCURY,
			GameResID::ORE,
			GameResID::SULFUR,
			GameResID::CRYSTAL,
			GameResID::GEMS,
			GameResID::GOLD
		};

		std::set<GameResID> potentialPicks = filterKeys(value, defaultResources);
		GameResID resourceID = *RandomGeneratorUtil::nextItem(potentialPicks, rng);
		si32 resourceAmount = loadValue(value, rng, 0);

		TResources ret;
		ret[resourceID] = resourceAmount;
		return ret;
	}


	std::vector<si32> loadPrimary(const JsonNode & value, CRandomGenerator & rng)
	{
		std::vector<si32> ret;
		if(value.isStruct())
		{
			for(const auto & name : NPrimarySkill::names)
			{
				ret.push_back(loadValue(value[name], rng));
			}
		}
		if(value.isVector())
		{
			std::set<PrimarySkill> defaultSkills{
				PrimarySkill::ATTACK,
				PrimarySkill::DEFENSE,
				PrimarySkill::SPELL_POWER,
				PrimarySkill::KNOWLEDGE
			};

			ret.resize(GameConstants::PRIMARY_SKILLS, 0);

			for(const auto & element : value.Vector())
			{
				std::set<PrimarySkill> potentialPicks = filterKeys(element, defaultSkills);
				PrimarySkill skillID = *RandomGeneratorUtil::nextItem(potentialPicks, rng);

				defaultSkills.erase(skillID);
				ret[static_cast<int>(skillID)] += loadValue(element, rng);
			}
		}
		return ret;
	}

	std::map<SecondarySkill, si32> loadSecondary(const JsonNode & value, CRandomGenerator & rng)
	{
		std::map<SecondarySkill, si32> ret;
		if(value.isStruct())
		{
			for(const auto & pair : value.Struct())
			{
				SecondarySkill id(VLC->identifiers()->getIdentifier(pair.second.meta, "skill", pair.first).value());
				ret[id] = loadValue(pair.second, rng);
			}
		}
		if(value.isVector())
		{
			std::set<SecondarySkill> defaultSkills;
			for(const auto & skill : VLC->skillh->objects)
				if (IObjectInterface::cb->isAllowed(2, skill->getIndex()))
					defaultSkills.insert(skill->getId());

			for(const auto & element : value.Vector())
			{
				std::set<SecondarySkill> potentialPicks = filterKeys(element, defaultSkills);
				SecondarySkill skillID = *RandomGeneratorUtil::nextItem(potentialPicks, rng);

				defaultSkills.erase(skillID); //avoid dupicates
				ret[skillID] = loadValue(element, rng);
			}
		}
		return ret;
	}

	ArtifactID loadArtifact(const JsonNode & value, CRandomGenerator & rng)
	{
		std::set<ArtifactID> allowedArts;
		for (auto const * artifact : VLC->arth->allowedArtifacts)
			allowedArts.insert(artifact->getId());

		std::set<ArtifactID> potentialPicks = filterKeys(value, allowedArts);

		return VLC->arth->pickRandomArtifact(rng, potentialPicks);
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
		std::set<SpellID> defaultSpells;
		for(const auto & spell : VLC->spellh->objects)
			if (IObjectInterface::cb->isAllowed(0, spell->getIndex()))
				defaultSpells.insert(spell->getId());

		std::set<SpellID> potentialPicks = filterKeys(value, defaultSpells);

		if (potentialPicks.empty())
		{
			logMod->warn("Failed to select suitable random spell!");
			return SpellID::NONE;
		}
		return *RandomGeneratorUtil::nextItem(potentialPicks, rng);
	}

	std::vector<SpellID> loadSpells(const JsonNode & value, CRandomGenerator & rng, const std::vector<SpellID> & spells)
	{
		std::vector<SpellID> ret;
		for (const JsonNode & entry : value.Vector())
		{
			ret.push_back(loadSpell(entry, rng, spells));
		}
		return ret;
	}

	std::vector<PlayerColor> loadColors(const JsonNode & value, CRandomGenerator & rng)
	{
		std::vector<PlayerColor> ret;
		std::set<std::string> def;
		
		for(auto & color : GameConstants::PLAYER_COLOR_NAMES)
			def.insert(color);
		
		for(auto & entry : value.Vector())
		{
			auto key = loadKey(entry, rng, def);
			auto pos = vstd::find_pos(GameConstants::PLAYER_COLOR_NAMES, key);
			if(pos < 0)
				logMod->warn("Unable to determine player color %s", key);
			else
				ret.emplace_back(pos);
		}
		return ret;
	}

	std::vector<HeroTypeID> loadHeroes(const JsonNode & value, CRandomGenerator & rng)
	{
		std::vector<HeroTypeID> ret;
		for(auto & entry : value.Vector())
		{
			ret.push_back(VLC->heroTypes()->getByIndex(VLC->identifiers()->getIdentifier("hero", entry.String()).value())->getId());
		}
		return ret;
	}

	std::vector<HeroClassID> loadHeroClasses(const JsonNode & value, CRandomGenerator & rng)
	{
		std::vector<HeroClassID> ret;
		for(auto & entry : value.Vector())
		{
			ret.push_back(VLC->heroClasses()->getByIndex(VLC->identifiers()->getIdentifier("heroClass", entry.String()).value())->getId());
		}
		return ret;
	}

	CStackBasicDescriptor loadCreature(const JsonNode & value, CRandomGenerator & rng)
	{
		CStackBasicDescriptor stack;

		std::set<CreatureID> defaultCreatures;
		for(const auto & creature : VLC->creh->objects)
			if (!creature->special)
				defaultCreatures.insert(creature->getId());

		std::set<CreatureID> potentialPicks = filterKeys(value, defaultCreatures);
		CreatureID pickedCreature;

		if (!potentialPicks.empty())
			pickedCreature = *RandomGeneratorUtil::nextItem(potentialPicks, rng);
		else
			logMod->warn("Failed to select suitable random creature!");

		stack.type = VLC->creh->objects[pickedCreature];
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
			const CCreature * crea = VLC->creh->objects[VLC->identifiers()->getIdentifier("creature", node["type"]).value()];
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
			if(auto bonus = JsonUtils::parseBonus(entry))
				ret.push_back(*bonus);
		}
		return ret;
	}

}

VCMI_LIB_NAMESPACE_END
