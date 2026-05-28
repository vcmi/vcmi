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

#include <vcmi/HeroClassService.h>
#include <vcmi/HeroTypeService.h>
#include <vstd/RNG.h>
#include <vstd/StringUtils.h>

#include "JsonBonus.h"

#include "../CCreatureHandler.h"
#include "../CSkillHandler.h"
#include "../GameLibrary.h"
#include "../callback/IGameInfoCallback.h"
#include "../callback/IGameRandomizer.h"
#include "../constants/StringConstants.h"
#include "../entities/ResourceTypeHandler.h"
#include "../entities/artifact/CArtHandler.h"
#include "../entities/hero/CHero.h"
#include "../entities/hero/CHeroClass.h"
#include "../gameState/CGameState.h"
#include "../mapObjects/IObjectInterface.h"
#include "../mapObjects/army/CStackBasicDescriptor.h"
#include "../modding/IdentifierStorage.h"
#include "../modding/ModScope.h"
#include "../spells/CSpellHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

std::string JsonRandomizationException::cleanupJson(const JsonNode & value)
{
	std::string result = value.toCompactString();
	for(size_t i = 0; i < result.size(); ++i)
		if(result[i] == '\n')
			result[i] = ' ';

	return result;
}

JsonRandomizationException::JsonRandomizationException(const std::string & message, const JsonNode & input)
	: std::runtime_error(message + " Input was: " + cleanupJson(input))
{
}

JsonRandom::JsonRandom(IGameInfoCallback * cb, IGameRandomizer & gameRandomizer)
	: GameCallbackHolder(cb), gameRandomizer(gameRandomizer), rng(gameRandomizer.getDefault()), jsonKeyExtractor(cb)
{
}

si32 JsonRandom::loadValue(const JsonNode & value, const Variables & variables, si32 defaultValue)
{
	if(value.isNull())
		return defaultValue;
	if(value.isNumber())
		return value.Integer();
	if(value.isString())
		return jsonKeyExtractor.loadVariable("number", value.String(), variables, defaultValue);

	if(value.isVector())
	{
		const auto & vector = value.Vector();

		size_t index = rng.nextInt64(0, vector.size() - 1);
		return loadValue(vector[index], variables, 0);
	}
	if(value.isStruct())
	{
		if(!value["amount"].isNull())
			return loadValue(value["amount"], variables, defaultValue);
		si32 min = loadValue(value["min"], variables, 0);
		si32 max = loadValue(value["max"], variables, 0);
		return rng.nextInt64(min, max);
	}
	return defaultValue;
}

TResources JsonRandom::loadResources(const JsonNode & value, const Variables & variables)
{
	TResources ret;

	if(value.isVector())
	{
		for(const auto & entry : value.Vector())
			ret += loadResource(entry, variables);
		return ret;
	}

	for(auto & i : LIBRARY->resourceTypeHandler->getAllObjects())
	{
		ret[i] = loadValue(value[i.toResource()->getJsonKey()], variables);
	}
	return ret;
}

TResources JsonRandom::loadResource(const JsonNode & value, const Variables & variables)
{
	GameResID resourceID = loadResourceType(value, variables);
	si32 resourceAmount = loadValue(value, variables, 0);

	TResources ret;
	ret[resourceID] = resourceAmount;
	return ret;
}

GameResID JsonRandom::loadResourceType(const JsonNode & value, const Variables & variables)
{
	auto defaultResources = LIBRARY->resourceTypeHandler->getAllObjects();

	std::set<GameResID> potentialPicks = jsonKeyExtractor.filterKeys(value, std::set<GameResID>(defaultResources.begin(), defaultResources.end()), variables);
	GameResID resourceID = *RandomGeneratorUtil::nextItem(potentialPicks, rng);
	return resourceID;
}

PrimarySkill JsonRandom::loadPrimary(const JsonNode & value, const Variables & variables)
{
	std::set<PrimarySkill> defaultSkills{PrimarySkill::ATTACK, PrimarySkill::DEFENSE, PrimarySkill::SPELL_POWER, PrimarySkill::KNOWLEDGE};
	std::set<PrimarySkill> potentialPicks = jsonKeyExtractor.filterKeys(value, defaultSkills, variables);
	return *RandomGeneratorUtil::nextItem(potentialPicks, rng);
}

std::vector<si32> JsonRandom::loadPrimaries(const JsonNode & value, const Variables & variables)
{
	std::vector<si32> ret(GameConstants::PRIMARY_SKILLS, 0);
	std::set<PrimarySkill> defaultSkills{PrimarySkill::ATTACK, PrimarySkill::DEFENSE, PrimarySkill::SPELL_POWER, PrimarySkill::KNOWLEDGE};

	if(value.isStruct())
	{
		for(const auto & pair : value.Struct())
		{
			PrimarySkill id = jsonKeyExtractor.decodeKey<PrimarySkill>(pair.second.getModScope(), pair.first, variables);
			ret[id.getNum()] += loadValue(pair.second, variables);
		}
	}
	if(value.isVector())
	{
		for(const auto & element : value.Vector())
		{
			std::set<PrimarySkill> potentialPicks = jsonKeyExtractor.filterKeys(element, defaultSkills, variables);
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
		if(cb->isAllowed(skill->getId()) && !skill->isSpecial())
			defaultSkills.insert(skill->getId());

	std::set<SecondarySkill> potentialPicks = jsonKeyExtractor.filterKeys(value, defaultSkills, variables);
	return *RandomGeneratorUtil::nextItem(potentialPicks, rng);
}

std::map<SecondarySkill, si32> JsonRandom::loadSecondaries(const JsonNode & value, const Variables & variables)
{
	std::map<SecondarySkill, si32> ret;
	if(value.isStruct())
	{
		for(const auto & pair : value.Struct())
		{
			SecondarySkill id = jsonKeyExtractor.decodeKey<SecondarySkill>(pair.second.getModScope(), pair.first, variables);
			ret[id] = loadValue(pair.second, variables);
		}
	}
	if(value.isVector())
	{
		std::set<SecondarySkill> defaultSkills;
		for(const auto & skill : LIBRARY->skillh->objects)
			if(cb->isAllowed(skill->getId()) && !skill->isSpecial())
				defaultSkills.insert(skill->getId());

		for(const auto & element : value.Vector())
		{
			std::set<SecondarySkill> potentialPicks = jsonKeyExtractor.filterKeys(element, defaultSkills, variables);
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
		if(cb->isAllowed(artifact->getId()) && LIBRARY->arth->legalArtifact(artifact->getId()))
			allowedArts.insert(artifact->getId());

	std::set<ArtifactID> potentialPicks = jsonKeyExtractor.filterKeys(value, allowedArts, variables);

	return gameRandomizer.rollArtifact(potentialPicks);
}

std::vector<ArtifactID> JsonRandom::loadArtifacts(const JsonNode & value, const Variables & variables)
{
	std::vector<ArtifactID> ret;
	for(const JsonNode & entry : value.Vector())
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
	for(const JsonNode & entry : value.Vector())
	{
		std::set<ArtifactPosition> potentialPicks = jsonKeyExtractor.filterKeys(entry, allowedSlots, variables);
		ret.push_back(*RandomGeneratorUtil::nextItem(potentialPicks, rng));
	}
	return ret;
}

SpellID JsonRandom::loadSpell(const JsonNode & value, const Variables & variables)
{
	std::set<SpellID> defaultSpells;
	for(const auto & spell : LIBRARY->spellh->objects)
		if(cb->isAllowed(spell->getId()) && !spell->isSpecial())
			defaultSpells.insert(spell->getId());

	std::set<SpellID> potentialPicks = jsonKeyExtractor.filterKeys(value, defaultSpells, variables);

	if(potentialPicks.empty())
	{
		logMod->warn("Failed to select suitable random spell!");
		return SpellID::NONE;
	}
	return *RandomGeneratorUtil::nextItem(potentialPicks, rng);
}

std::vector<SpellID> JsonRandom::loadSpells(const JsonNode & value, const Variables & variables)
{
	std::vector<SpellID> ret;
	for(const JsonNode & entry : value.Vector())
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
		std::set<PlayerColor> potentialPicks = jsonKeyExtractor.filterKeys(entry, defaultPlayers, variables);
		ret.push_back(*RandomGeneratorUtil::nextItem(potentialPicks, rng));
	}

	return ret;
}

std::vector<HeroTypeID> JsonRandom::loadHeroes(const JsonNode & value)
{
	std::vector<HeroTypeID> ret;
	for(auto & entry : value.Vector())
	{
		ret.push_back(LIBRARY->heroTypes()->getByIndex(LIBRARY->identifiers()->getIdentifier("hero", entry).value())->getId());
	}
	return ret;
}

std::vector<HeroClassID> JsonRandom::loadHeroClasses(const JsonNode & value)
{
	std::vector<HeroClassID> ret;
	for(auto & entry : value.Vector())
	{
		ret.push_back(LIBRARY->heroClasses()->getByIndex(LIBRARY->identifiers()->getIdentifier("heroClass", entry).value())->getId());
	}
	return ret;
}

CreatureID JsonRandom::loadCreatureType(const JsonNode & value, const Variables & variables)
{
	std::set<CreatureID> defaultCreatures;
	for(const auto & creature : LIBRARY->creh->objects)
		if(!creature->special)
			defaultCreatures.insert(creature->getId());

	std::set<CreatureID> potentialPicks = jsonKeyExtractor.filterKeys(value, defaultCreatures, variables);
	CreatureID pickedCreature;

	if(!potentialPicks.empty())
		pickedCreature = *RandomGeneratorUtil::nextItem(potentialPicks, rng);
	else
		throw JsonRandomizationException("No potential creatures to pick!", value);

	if(!pickedCreature.hasValue())
		throw JsonRandomizationException("Invalid creature picked!", value);

	return pickedCreature;
}

CStackBasicDescriptor JsonRandom::loadCreature(const JsonNode & value, const Variables & variables)
{
	CStackBasicDescriptor stack;

	CreatureID pickedCreature = loadCreatureType(value, variables);
	stack.setType(pickedCreature.toCreature());
	stack.setCount(loadValue(value, variables));
	if(!value["upgradeChance"].isNull() && !stack.getCreature()->upgrades.empty())
	{
		if(int(value["upgradeChance"].Float()) > rng.nextInt(99)) // select random upgrade
		{
			stack.setType(RandomGeneratorUtil::nextItem(stack.getCreature()->upgrades, rng)->toCreature());
		}
	}
	return stack;
}

std::vector<CStackBasicDescriptor> JsonRandom::loadCreatures(const JsonNode & value, const Variables & variables)
{
	std::vector<CStackBasicDescriptor> ret;
	for(const JsonNode & node : value.Vector())
	{
		ret.push_back(loadCreature(node, variables));
	}
	return ret;
}

std::vector<JsonRandom::RandomStackInfo> JsonRandom::evaluateCreatures(const JsonNode & value, const Variables & variables)
{
	std::vector<RandomStackInfo> ret;
	for(const JsonNode & node : value.Vector())
	{
		RandomStackInfo info;

		if(!node["amount"].isNull())
			info.minAmount = info.maxAmount = static_cast<si32>(node["amount"].Float());
		else
		{
			info.minAmount = static_cast<si32>(node["min"].Float());
			info.maxAmount = static_cast<si32>(node["max"].Float());
		}
		CreatureID creatureID(LIBRARY->identifiers()->getIdentifier("creature", node["type"]).value());
		const CCreature * crea = creatureID.toCreature();
		info.allowedCreatures.push_back(crea);
		if(node["upgradeChance"].Float() > 0)
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
	for(const JsonNode & entry : value.Vector())
	{
		if(auto bonus = JsonUtils::parseBonus(entry))
			ret.push_back(bonus);
	}
	return ret;
}

VCMI_LIB_NAMESPACE_END
