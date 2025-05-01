/*
 * JsonRandom.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "GameCallbackHolder.h"
#include "GameConstants.h"
#include "ResourceSet.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{
class RNG;
}

class JsonNode;
using JsonVector = std::vector<JsonNode>;

struct Bonus;
struct Component;
class CStackBasicDescriptor;

class JsonRandomizationException : public std::runtime_error
{
	std::string cleanupJson(const JsonNode & value);
public:
	JsonRandomizationException(const std::string & message, const JsonNode & input);
};

class JsonRandom : public GameCallbackHolder
{
public:
	using Variables = std::map<std::string, int>;

private:
	template<typename IdentifierType>
	std::set<IdentifierType> filterKeys(const JsonNode & value, const std::set<IdentifierType> & valuesSet, const Variables & variables);

	template<typename IdentifierType>
	std::set<IdentifierType> filterKeysTyped(const JsonNode & value, const std::set<IdentifierType> & valuesSet);

	template<typename IdentifierType>
	IdentifierType decodeKey(const std::string & modScope, const std::string & value, const Variables & variables);

	template<typename IdentifierType>
	IdentifierType decodeKey(const JsonNode & value, const Variables & variables);

	si32 loadVariable(const std::string & variableGroup, const std::string & value, const Variables & variables, si32 defaultValue);

public:
	using GameCallbackHolder::GameCallbackHolder;

	struct RandomStackInfo
	{
		std::vector<const CCreature *> allowedCreatures;
		si32 minAmount;
		si32 maxAmount;
	};

	si32 loadValue(const JsonNode & value, vstd::RNG & rng, const Variables & variables, si32 defaultValue = 0);

	TResources loadResources(const JsonNode & value, vstd::RNG & rng, const Variables & variables);
	TResources loadResource(const JsonNode & value, vstd::RNG & rng, const Variables & variables);
	PrimarySkill loadPrimary(const JsonNode & value, vstd::RNG & rng, const Variables & variables);
	std::vector<si32> loadPrimaries(const JsonNode & value, vstd::RNG & rng, const Variables & variables);
	SecondarySkill loadSecondary(const JsonNode & value, vstd::RNG & rng, const Variables & variables);
	std::map<SecondarySkill, si32> loadSecondaries(const JsonNode & value, vstd::RNG & rng, const Variables & variables);

	ArtifactID loadArtifact(const JsonNode & value, vstd::RNG & rng, const Variables & variables);
	std::vector<ArtifactID> loadArtifacts(const JsonNode & value, vstd::RNG & rng, const Variables & variables);
	std::vector<ArtifactPosition> loadArtifactSlots(const JsonNode & value, vstd::RNG & rng, const Variables & variables);

	SpellID loadSpell(const JsonNode & value, vstd::RNG & rng, const Variables & variables);
	std::vector<SpellID> loadSpells(const JsonNode & value, vstd::RNG & rng, const Variables & variables);

	CStackBasicDescriptor loadCreature(const JsonNode & value, vstd::RNG & rng, const Variables & variables);
	std::vector<CStackBasicDescriptor> loadCreatures(const JsonNode & value, vstd::RNG & rng, const Variables & variables);
	std::vector<RandomStackInfo> evaluateCreatures(const JsonNode & value, const Variables & variables);

	std::vector<PlayerColor> loadColors(const JsonNode & value, vstd::RNG & rng, const Variables & variables);
	std::vector<HeroTypeID> loadHeroes(const JsonNode & value, vstd::RNG & rng);
	std::vector<HeroClassID> loadHeroClasses(const JsonNode & value, vstd::RNG & rng);

	static std::vector<Bonus> loadBonuses(const JsonNode & value);
};

VCMI_LIB_NAMESPACE_END
