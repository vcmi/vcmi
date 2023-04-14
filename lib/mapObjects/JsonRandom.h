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

#include "../GameConstants.h"
#include "../ResourceSet.h"

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;
typedef std::vector<JsonNode> JsonVector;
class CRandomGenerator;

struct Bonus;
struct Component;
class CStackBasicDescriptor;

namespace JsonRandom
{
	struct DLL_LINKAGE RandomStackInfo
	{
		std::vector<const CCreature *> allowedCreatures;
		si32 minAmount;
		si32 maxAmount;
	};

	DLL_LINKAGE si32 loadValue(const JsonNode & value, CRandomGenerator & rng, si32 defaultValue = 0);
	DLL_LINKAGE std::string loadKey(const JsonNode & value, CRandomGenerator & rng, std::string defaultValue = "");
	DLL_LINKAGE TResources loadResources(const JsonNode & value, CRandomGenerator & rng);
	DLL_LINKAGE TResources loadResource(const JsonNode & value, CRandomGenerator & rng);
	DLL_LINKAGE std::vector<si32> loadPrimary(const JsonNode & value, CRandomGenerator & rng);
	DLL_LINKAGE std::map<SecondarySkill, si32> loadSecondary(const JsonNode & value, CRandomGenerator & rng);

	DLL_LINKAGE ArtifactID loadArtifact(const JsonNode & value, CRandomGenerator & rng);
	DLL_LINKAGE std::vector<ArtifactID> loadArtifacts(const JsonNode & value, CRandomGenerator & rng);

	DLL_LINKAGE SpellID loadSpell(const JsonNode & value, CRandomGenerator & rng, std::vector<SpellID> spells);
	DLL_LINKAGE std::vector<SpellID> loadSpells(const JsonNode & value, CRandomGenerator & rng, const std::vector<SpellID> & spells);

	DLL_LINKAGE CStackBasicDescriptor loadCreature(const JsonNode & value, CRandomGenerator & rng);
	DLL_LINKAGE std::vector<CStackBasicDescriptor> loadCreatures(const JsonNode & value, CRandomGenerator & rng);
	DLL_LINKAGE std::vector<RandomStackInfo> evaluateCreatures(const JsonNode & value);

	DLL_LINKAGE std::vector<Bonus> loadBonuses(const JsonNode & value);
	//DLL_LINKAGE std::vector<Component> loadComponents(const JsonNode & value);
}

VCMI_LIB_NAMESPACE_END
