#pragma once

#include "../GameConstants.h"
#include "../ResourceSet.h"

/*
 * JsonRandom.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class JsonNode;
typedef std::vector<JsonNode> JsonVector;
class CRandomGenerator;

class Bonus;
class Component;
class CStackBasicDescriptor;

namespace JsonRandom
{
	struct RandomStackInfo
	{
		std::vector<const CCreature *> allowedCreatures;
		si32 minAmount;
		si32 maxAmount;
	};

	si32 loadValue(const JsonNode & value, CRandomGenerator & rng, si32 defaultValue = 0);
	TResources loadResources(const JsonNode & value, CRandomGenerator & rng);
	std::vector<si32> loadPrimary(const JsonNode & value, CRandomGenerator & rng);
	std::map<SecondarySkill, si32> loadSecondary(const JsonNode & value, CRandomGenerator & rng);

	ArtifactID loadArtifact(const JsonNode & value, CRandomGenerator & rng);
	std::vector<ArtifactID> loadArtifacts(const JsonNode & value, CRandomGenerator & rng);

	SpellID loadSpell(const JsonNode & value, CRandomGenerator & rng, std::vector<SpellID> spells);
	std::vector<SpellID> loadSpells(const JsonNode & value, CRandomGenerator & rng, std::vector<SpellID> spells);

	CStackBasicDescriptor loadCreature(const JsonNode & value, CRandomGenerator & rng);
	std::vector<CStackBasicDescriptor> loadCreatures(const JsonNode & value, CRandomGenerator & rng);
	std::vector<RandomStackInfo> evaluateCreatures(const JsonNode & value);

	std::vector<Bonus> DLL_LINKAGE loadBonuses(const JsonNode & value);
	std::vector<Component> loadComponents(const JsonNode & value);
}
