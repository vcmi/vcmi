/*
 * ParallelObjectRandomizer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ParallelObjectRandomizer.h"

#include "../CCreatureHandler.h"
#include "../GameLibrary.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGObjectInstance.h"
#include "../mapObjects/CGTownInstance.h"
#include "../mapObjects/MiscObjects.h"

namespace
{
	std::vector<CreatureID> allowedCreaturesOfTier(int tier)
	{
		std::vector<CreatureID> allowed;
		for(const auto & creatureID : LIBRARY->creh->getDefaultAllowed())
		{
			const auto * creaturePtr = creatureID.toCreature();
			if(creaturePtr->excludeFromRandomization)
				continue;
			if(!LIBRARY->objtypeh->knownSubObjects(Obj::MONSTER).contains(creatureID.getNum()))
				continue;
			if(tier >= 0 && creaturePtr->getLevel() != tier)
				continue;
			allowed.push_back(creaturePtr->getId());
		}
		return allowed;
	}

	CreatureID rollCreatureOfTier(CRandomGenerator & generator, int tier)
	{
		auto allowed = allowedCreaturesOfTier(tier);

		if(allowed.empty())
			throw std::runtime_error("Cannot pick a random creature!");

		return *RandomGeneratorUtil::nextItem(allowed, generator);
	}
}

bool objectNeedsSerialRandomization(const CGObjectInstance * object)
{
	if (dynamic_cast<const CGTownInstance *>(object))
		return true;

	if (dynamic_cast<const CGArtifact *>(object))
		return true;

	if (const auto * hero = dynamic_cast<const CGHeroInstance *>(object))
		return hero->ID == Obj::RANDOM_HERO;

	return false;
}

ParallelObjectRandomizer::ParallelObjectRandomizer(int seed)
	: generator(seed)
{
}

CreatureID ParallelObjectRandomizer::rollCreature()
{
	return rollCreatureOfTier(generator, -1);
}

CreatureID ParallelObjectRandomizer::rollCreature(int tier)
{
	return rollCreatureOfTier(generator, tier);
}

vstd::RNG & ParallelObjectRandomizer::getDefault()
{
	return generator;
}

[[noreturn]] static void unsupported(const char * randomizerName, const char * what, const char * reason)
{
	throw std::logic_error(std::string(randomizerName) + ": " + what + "() is not supported " + reason);
}

static const char * OBJECT_RANDOMIZATION_REASON = "- object should have been in the serial randomization bucket";
static const char * OBJECT_INIT_REASON = "during object initialization";

ArtifactID ParallelObjectRandomizer::rollArtifact() { unsupported("ParallelObjectRandomizer", "rollArtifact", OBJECT_RANDOMIZATION_REASON); }
ArtifactID ParallelObjectRandomizer::rollArtifact(EArtifactClass type) { unsupported("ParallelObjectRandomizer", "rollArtifact", OBJECT_RANDOMIZATION_REASON); }
ArtifactID ParallelObjectRandomizer::rollArtifact(std::set<ArtifactID> filtered) { unsupported("ParallelObjectRandomizer", "rollArtifact", OBJECT_RANDOMIZATION_REASON); }
std::vector<ArtifactID> ParallelObjectRandomizer::rollMarketArtifactSet() { unsupported("ParallelObjectRandomizer", "rollMarketArtifactSet", OBJECT_RANDOMIZATION_REASON); }
PrimarySkill ParallelObjectRandomizer::rollPrimarySkillForLevelup(const CGHeroInstance * hero) { unsupported("ParallelObjectRandomizer", "rollPrimarySkillForLevelup", OBJECT_RANDOMIZATION_REASON); }
SecondarySkill ParallelObjectRandomizer::rollSecondarySkillForLevelup(const CGHeroInstance * hero, const std::set<SecondarySkill> & candidates) { unsupported("ParallelObjectRandomizer", "rollSecondarySkillForLevelup", OBJECT_RANDOMIZATION_REASON); }
std::vector<SecondarySkill> ParallelObjectRandomizer::rollSecondarySkills(const CGHeroInstance * hero) { unsupported("ParallelObjectRandomizer", "rollSecondarySkills", OBJECT_RANDOMIZATION_REASON); }

ParallelInitRandomizer::ParallelInitRandomizer(int seed, IGameRandomizer & sharedRandomizer)
	: generator(seed)
	, sharedRandomizer(sharedRandomizer)
{
}

CreatureID ParallelInitRandomizer::rollCreature()
{
	return rollCreatureOfTier(generator, -1);
}

CreatureID ParallelInitRandomizer::rollCreature(int tier)
{
	return rollCreatureOfTier(generator, tier);
}

vstd::RNG & ParallelInitRandomizer::getDefault()
{
	return generator;
}

ArtifactID ParallelInitRandomizer::rollArtifact() { return sharedRandomizer.rollArtifact(); }
ArtifactID ParallelInitRandomizer::rollArtifact(EArtifactClass type) { return sharedRandomizer.rollArtifact(type); }
ArtifactID ParallelInitRandomizer::rollArtifact(std::set<ArtifactID> filtered) { return sharedRandomizer.rollArtifact(std::move(filtered)); }

std::vector<ArtifactID> ParallelInitRandomizer::rollMarketArtifactSet() { unsupported("ParallelInitRandomizer", "rollMarketArtifactSet", OBJECT_INIT_REASON); }
PrimarySkill ParallelInitRandomizer::rollPrimarySkillForLevelup(const CGHeroInstance * hero) { unsupported("ParallelInitRandomizer", "rollPrimarySkillForLevelup", OBJECT_INIT_REASON); }
SecondarySkill ParallelInitRandomizer::rollSecondarySkillForLevelup(const CGHeroInstance * hero, const std::set<SecondarySkill> & candidates) { unsupported("ParallelInitRandomizer", "rollSecondarySkillForLevelup", OBJECT_INIT_REASON); }
std::vector<SecondarySkill> ParallelInitRandomizer::rollSecondarySkills(const CGHeroInstance * hero) { unsupported("ParallelInitRandomizer", "rollSecondarySkills", OBJECT_INIT_REASON); }
