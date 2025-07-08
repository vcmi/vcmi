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
#include <vstd/RNG.h>
#include <vcmi/HeroClassService.h>
#include <vcmi/HeroTypeService.h>

#include "JsonBonus.h"

#include "../callback/IGameInfoCallback.h"
#include "../callback/IGameRandomizer.h"
#include "../constants/StringConstants.h"
#include "../GameLibrary.h"
#include "../CCreatureHandler.h"
#include "../spells/CSpellHandler.h"
#include "../CSkillHandler.h"
#include "../entities/artifact/CArtHandler.h"
#include "../entities/hero/CHero.h"
#include "../entities/hero/CHeroClass.h"
#include "../gameState/CGameState.h"
#include "../mapObjects/army/CStackBasicDescriptor.h"
#include "../mapObjects/IObjectInterface.h"
#include "../modding/IdentifierStorage.h"
#include "../modding/ModScope.h"

VCMI_LIB_NAMESPACE_BEGIN

std::string JsonRandomizationException::cleanupJson(const JsonNode & value)
{
	std::string result = value.toCompactString();
	for (size_t i = 0; i < result.size(); ++i)
		if (result[i] == '\n')
			result[i] = ' ';

	return result;
}

JsonRandomizationException::JsonRandomizationException(const std::string & message, const JsonNode & input)
	: std::runtime_error(message + " Input was: " + cleanupJson(input))
{}

JsonRandom::JsonRandom(IGameInfoCallback * cb, IGameRandomizer & gameRandomizer)
	: GameCallbackHolder(cb)
	, gameRandomizer(gameRandomizer)
	, rng(gameRandomizer.getDefault())
{
}

	si32 JsonRandom::loadVariable(const std::string & variableGroup, const std::string & value, const Variables & variables, si32 defaultValue)
	{
		if (value.empty() || value[0] != '@')
		{
			logMod->warn("Invalid syntax in load value! Can not load value from '%s'", value);
			return defaultValue;
		}

		std::string variableID = variableGroup + value;

		if (variables.count(variableID) == 0)
		{
			logMod->warn("Invalid syntax in load value! Unknown variable '%s'", value);
			return defaultValue;
		}
		return variables.at(variableID);
	}

	si32 JsonRandom::loadValue(const JsonNode & value, const Variables & variables, si32 defaultValue)
	{
		if(value.isNull())
			return defaultValue;
		if(value.isNumber())
			return value.Integer();
		if (value.isString())
			return loadVariable("number", value.String(), variables, defaultValue);

		if(value.isVector())
		{
			const auto & vector = value.Vector();

			size_t index= rng.nextInt64(0, vector.size()-1);
			return loadValue(vector[index], variables, 0);
		}
		if(value.isStruct())
		{
			if (!value["amount"].isNull())
				return loadValue(value["amount"], variables, defaultValue);
			si32 min = loadValue(value["min"], variables, 0);
			si32 max = loadValue(value["max"], variables, 0);
			return rng.nextInt64(min, max);
		}
		return defaultValue;
	}

	template<typename IdentifierType>
	IdentifierType JsonRandom::decodeKey(const std::string & modScope, const std::string & value, const Variables & variables)
	{
		if (value.empty() || value[0] != '@')
			return IdentifierType(LIBRARY->identifiers()->getIdentifier(modScope, IdentifierType::entityType(), value).value_or(-1));
		else
			return loadVariable(IdentifierType::entityType(), value, variables, IdentifierType::NONE);
	}

	template<typename IdentifierType>
	IdentifierType JsonRandom::decodeKey(const JsonNode & value, const Variables & variables)
	{
		if (value.String().empty() || value.String()[0] != '@')
			return IdentifierType(LIBRARY->identifiers()->getIdentifier(IdentifierType::entityType(), value).value_or(-1));
		else
			return loadVariable(IdentifierType::entityType(), value.String(), variables, IdentifierType::NONE);
	}

	template<>
	ArtifactPosition JsonRandom::decodeKey(const JsonNode & value, const Variables & variables)
	{
		return ArtifactPosition::decode(value.String());
	}

	template<>
	PlayerColor JsonRandom::decodeKey(const JsonNode & value, const Variables & variables)
	{
		return PlayerColor(*LIBRARY->identifiers()->getIdentifier("playerColor", value));
	}

	template<>
	PrimarySkill JsonRandom::decodeKey(const JsonNode & value, const Variables & variables)
	{
		return PrimarySkill(*LIBRARY->identifiers()->getIdentifier("primarySkill", value));
	}

	template<>
	PrimarySkill JsonRandom::decodeKey(const std::string & modScope, const std::string & value, const Variables & variables)
	{
		if (value.empty() || value[0] != '@')
			return PrimarySkill(*LIBRARY->identifiers()->getIdentifier(modScope, "primarySkill", value));
		else
			return PrimarySkill(loadVariable("primarySkill", value, variables, PrimarySkill::NONE.getNum()));
	}

	/// Method that allows type-specific object filtering
	/// Default implementation is to accept all input objects
	template<typename IdentifierType>
	std::set<IdentifierType> JsonRandom::filterKeysTyped(const JsonNode & value, const std::set<IdentifierType> & valuesSet)
	{
		return valuesSet;
	}

	template<>
	std::set<ArtifactID> JsonRandom::filterKeysTyped(const JsonNode & value, const std::set<ArtifactID> & valuesSet)
	{
		assert(value.isStruct());

		std::set<EArtifactClass> allowedClasses;
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
			const CArtifact * art = artID.toArtifact();

			if(!vstd::iswithin(art->getPrice(), minValue, maxValue))
				continue;

			if(!allowedClasses.empty() && !allowedClasses.count(art->aClass))
				continue;

			if(!cb->isAllowed(art->getId()))
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
	std::set<SpellID> JsonRandom::filterKeysTyped(const JsonNode & value, const std::set<SpellID> & valuesSet)
	{
		std::set<SpellID> result = valuesSet;

		if (!value["level"].isNull())
		{
			int32_t spellLevel = value["level"].Integer();

			vstd::erase_if(result, [=](const SpellID & spell)
			{
				return LIBRARY->spellh->getById(spell)->getLevel() != spellLevel;
			});
		}

		if (!value["school"].isNull())
		{
			int32_t schoolID = LIBRARY->identifiers()->getIdentifier("spellSchool", value["school"]).value();

			vstd::erase_if(result, [=](const SpellID & spell)
			{
				return !LIBRARY->spellh->getById(spell)->hasSchool(SpellSchool(schoolID));
			});
		}
		return result;
	}

	template<typename IdentifierType>
	std::set<IdentifierType> JsonRandom::filterKeys(const JsonNode & value, const std::set<IdentifierType> & valuesSet, const Variables & variables)
	{
		if(value.isString())
			return { decodeKey<IdentifierType>(value, variables) };

		assert(value.isStruct());

		if(value.isStruct())
		{
			if(!value["type"].isNull())
				return filterKeys(value["type"], valuesSet, variables);

			std::set<IdentifierType> filteredTypes = filterKeysTyped(value, valuesSet);

			if(!value["anyOf"].isNull())
			{
				std::set<IdentifierType> filteredAnyOf;
				for (auto const & entry : value["anyOf"].Vector())
				{
					std::set<IdentifierType> subset = filterKeys(entry, valuesSet, variables);
					filteredAnyOf.insert(subset.begin(), subset.end());
				}

				vstd::erase_if(filteredTypes, [&filteredAnyOf](const IdentifierType & filteredValue)
				{
					return filteredAnyOf.count(filteredValue) == 0;
				});
			}

			if(!value["noneOf"].isNull())
			{
				for (auto const & entry : value["noneOf"].Vector())
				{
					std::set<IdentifierType> subset = filterKeys(entry, valuesSet, variables);
					for (auto bannedEntry : subset )
						filteredTypes.erase(bannedEntry);
				}
			}

			return filteredTypes;
		}
		return valuesSet;
	}

	TResources JsonRandom::loadResources(const JsonNode & value, const Variables & variables)
	{
		TResources ret;

		if (value.isVector())
		{
			for (const auto & entry : value.Vector())
				ret += loadResource(entry, variables);
			return ret;
		}

		for (size_t i=0; i<GameConstants::RESOURCE_QUANTITY; i++)
		{
			ret[i] = loadValue(value[GameConstants::RESOURCE_NAMES[i]], variables);
		}
		return ret;
	}

	TResources JsonRandom::loadResource(const JsonNode & value, const Variables & variables)
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

		std::set<GameResID> potentialPicks = filterKeys(value, defaultResources, variables);
		GameResID resourceID = *RandomGeneratorUtil::nextItem(potentialPicks, rng);
		si32 resourceAmount = loadValue(value, variables, 0);

		TResources ret;
		ret[resourceID] = resourceAmount;
		return ret;
	}

	PrimarySkill JsonRandom::loadPrimary(const JsonNode & value, const Variables & variables)
	{
		std::set<PrimarySkill> defaultSkills{
			PrimarySkill::ATTACK,
			PrimarySkill::DEFENSE,
			PrimarySkill::SPELL_POWER,
			PrimarySkill::KNOWLEDGE
		};
		std::set<PrimarySkill> potentialPicks = filterKeys(value, defaultSkills, variables);
		return *RandomGeneratorUtil::nextItem(potentialPicks, rng);
	}

	std::vector<si32> JsonRandom::loadPrimaries(const JsonNode & value, const Variables & variables)
	{
		std::vector<si32> ret(GameConstants::PRIMARY_SKILLS, 0);
		std::set<PrimarySkill> defaultSkills{
			PrimarySkill::ATTACK,
			PrimarySkill::DEFENSE,
			PrimarySkill::SPELL_POWER,
			PrimarySkill::KNOWLEDGE
		};

		if(value.isStruct())
		{
			for(const auto & pair : value.Struct())
			{
				PrimarySkill id = decodeKey<PrimarySkill>(pair.second.getModScope(), pair.first, variables);
				ret[id.getNum()] += loadValue(pair.second, variables);
			}
		}
		if(value.isVector())
		{
			for(const auto & element : value.Vector())
			{
				std::set<PrimarySkill> potentialPicks = filterKeys(element, defaultSkills, variables);
				PrimarySkill skillID = *RandomGeneratorUtil::nextItem(potentialPicks, rng);

				defaultSkills.erase(skillID);
				ret[skillID.getNum()] += loadValue(element, variables);
			}
		}
		return ret;
	}

	SecondarySkill JsonRandom::loadSecondary(const JsonNode & value, const Variables & variables)
	{
		std::set<SecondarySkill> defaultSkills;
		for(const auto & skill : LIBRARY->skillh->objects)
			if (cb->isAllowed(skill->getId()))
				defaultSkills.insert(skill->getId());

		std::set<SecondarySkill> potentialPicks = filterKeys(value, defaultSkills, variables);
		return *RandomGeneratorUtil::nextItem(potentialPicks, rng);
	}

	std::map<SecondarySkill, si32> JsonRandom::loadSecondaries(const JsonNode & value, const Variables & variables)
	{
		std::map<SecondarySkill, si32> ret;
		if(value.isStruct())
		{
			for(const auto & pair : value.Struct())
			{
				SecondarySkill id = decodeKey<SecondarySkill>(pair.second.getModScope(), pair.first, variables);
				ret[id] = loadValue(pair.second, variables);
			}
		}
		if(value.isVector())
		{
			std::set<SecondarySkill> defaultSkills;
			for(const auto & skill : LIBRARY->skillh->objects)
				if (cb->isAllowed(skill->getId()))
					defaultSkills.insert(skill->getId());

			for(const auto & element : value.Vector())
			{
				std::set<SecondarySkill> potentialPicks = filterKeys(element, defaultSkills, variables);
				SecondarySkill skillID = *RandomGeneratorUtil::nextItem(potentialPicks, rng);

				defaultSkills.erase(skillID); //avoid dupicates
				ret[skillID] = loadValue(element, variables);
			}
		}
		return ret;
	}

	ArtifactID JsonRandom::loadArtifact(const JsonNode & value, const Variables & variables)
	{
		std::set<ArtifactID> allowedArts;
		for(const auto & artifact : LIBRARY->arth->objects)
			if (cb->isAllowed(artifact->getId()) && LIBRARY->arth->legalArtifact(artifact->getId()))
				allowedArts.insert(artifact->getId());

		std::set<ArtifactID> potentialPicks = filterKeys(value, allowedArts, variables);

		return gameRandomizer.rollArtifact(potentialPicks);
	}

	std::vector<ArtifactID> JsonRandom::loadArtifacts(const JsonNode & value, const Variables & variables)
	{
		std::vector<ArtifactID> ret;
		for (const JsonNode & entry : value.Vector())
		{
			ret.push_back(loadArtifact(entry, variables));
		}
		return ret;
	}

	std::vector<ArtifactPosition> JsonRandom::loadArtifactSlots(const JsonNode & value, const Variables & variables)
	{
		std::set<ArtifactPosition> allowedSlots;
		for(ArtifactPosition pos(0); pos < ArtifactPosition::BACKPACK_START; ++pos)
			allowedSlots.insert(pos);

		std::vector<ArtifactPosition> ret;
		for (const JsonNode & entry : value.Vector())
		{
			std::set<ArtifactPosition> potentialPicks = filterKeys(entry, allowedSlots, variables);
			ret.push_back(*RandomGeneratorUtil::nextItem(potentialPicks, rng));
		}
		return ret;
	}

	SpellID JsonRandom::loadSpell(const JsonNode & value, const Variables & variables)
	{
		std::set<SpellID> defaultSpells;
		for(const auto & spell : LIBRARY->spellh->objects)
			if (cb->isAllowed(spell->getId()) && !spell->isSpecial())
				defaultSpells.insert(spell->getId());

		std::set<SpellID> potentialPicks = filterKeys(value, defaultSpells, variables);

		if (potentialPicks.empty())
		{
			logMod->warn("Failed to select suitable random spell!");
			return SpellID::NONE;
		}
		return *RandomGeneratorUtil::nextItem(potentialPicks, rng);
	}

	std::vector<SpellID> JsonRandom::loadSpells(const JsonNode & value, const Variables & variables)
	{
		std::vector<SpellID> ret;
		for (const JsonNode & entry : value.Vector())
		{
			ret.push_back(loadSpell(entry, variables));
		}
		return ret;
	}

	std::vector<PlayerColor> JsonRandom::loadColors(const JsonNode & value, const Variables & variables)
	{
		std::vector<PlayerColor> ret;
		std::set<PlayerColor> defaultPlayers;
		for(size_t i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
			defaultPlayers.insert(PlayerColor(i));

		for(auto & entry : value.Vector())
		{
			std::set<PlayerColor> potentialPicks = filterKeys(entry, defaultPlayers, variables);
			ret.push_back(*RandomGeneratorUtil::nextItem(potentialPicks, rng));
		}

		return ret;
	}

	std::vector<HeroTypeID> JsonRandom::loadHeroes(const JsonNode & value)
	{
		std::vector<HeroTypeID> ret;
		for(auto & entry : value.Vector())
		{
			ret.push_back(LIBRARY->heroTypes()->getByIndex(LIBRARY->identifiers()->getIdentifier("hero", entry.String()).value())->getId());
		}
		return ret;
	}

	std::vector<HeroClassID> JsonRandom::loadHeroClasses(const JsonNode & value)
	{
		std::vector<HeroClassID> ret;
		for(auto & entry : value.Vector())
		{
			ret.push_back(LIBRARY->heroClasses()->getByIndex(LIBRARY->identifiers()->getIdentifier("heroClass", entry.String()).value())->getId());
		}
		return ret;
	}

	CStackBasicDescriptor JsonRandom::loadCreature(const JsonNode & value, const Variables & variables)
	{
		CStackBasicDescriptor stack;

		std::set<CreatureID> defaultCreatures;
		for(const auto & creature : LIBRARY->creh->objects)
			if (!creature->special)
				defaultCreatures.insert(creature->getId());

		std::set<CreatureID> potentialPicks = filterKeys(value, defaultCreatures, variables);
		CreatureID pickedCreature;

		if (!potentialPicks.empty())
			pickedCreature = *RandomGeneratorUtil::nextItem(potentialPicks, rng);
		else
			throw JsonRandomizationException("No potential creatures to pick!", value);

		if (!pickedCreature.hasValue())
			throw JsonRandomizationException("Invalid creature picked!", value);

		stack.setType(pickedCreature.toCreature());
		stack.setCount(loadValue(value, variables));
		if (!value["upgradeChance"].isNull() && !stack.getCreature()->upgrades.empty())
		{
			if (int(value["upgradeChance"].Float()) > rng.nextInt(99)) // select random upgrade
			{
				stack.setType(RandomGeneratorUtil::nextItem(stack.getCreature()->upgrades, rng)->toCreature());
			}
		}
		return stack;
	}

	std::vector<CStackBasicDescriptor> JsonRandom::loadCreatures(const JsonNode & value, const Variables & variables)
	{
		std::vector<CStackBasicDescriptor> ret;
		for (const JsonNode & node : value.Vector())
		{
			ret.push_back(loadCreature(node, variables));
		}
		return ret;
	}

	std::vector<JsonRandom::RandomStackInfo> JsonRandom::evaluateCreatures(const JsonNode & value, const Variables & variables)
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
			CreatureID creatureID(LIBRARY->identifiers()->getIdentifier("creature", node["type"]).value());
			const CCreature * crea = creatureID.toCreature();
			info.allowedCreatures.push_back(crea);
			if (node["upgradeChance"].Float() > 0)
			{
				for(const auto & creaID : crea->upgrades)
					info.allowedCreatures.push_back(creaID.toCreature());
			}
			ret.push_back(info);
		}
		return ret;
	}

	std::vector<std::shared_ptr<Bonus>> JsonRandom::loadBonuses(const JsonNode & value)
	{
		std::vector<std::shared_ptr<Bonus>> ret;
		for (const JsonNode & entry : value.Vector())
		{
			if(auto bonus = JsonUtils::parseBonus(entry))
				ret.push_back(bonus);
		}
		return ret;
	}

VCMI_LIB_NAMESPACE_END
