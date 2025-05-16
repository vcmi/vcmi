/*
 * RandomizationProcessor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "RandomizationProcessor.h"

#include <vstd/RNG.h>

bool BiasedRandomizer::roll(vstd::RNG &generator, int successChance, int biasValue)
{
	int failChance = 100 - successChance;
	int newRoll = generator.nextInt(0,99);
	bool success = newRoll + accumulatedBias >= successChance;
	if (success)
		accumulatedBias -= failChance * biasValue / 100;
	else
		accumulatedBias += successChance * biasValue / 100;

	return success;
}

//void CGameInfoCallback::pickAllowedArtsSet(std::vector<ArtifactID> & out, vstd::RNG & rand)
//{
//	for (int j = 0; j < 3 ; j++)
//		out.push_back(gameState().pickRandomArtifact(rand, EArtifactClass::ART_TREASURE));
//	for (int j = 0; j < 3 ; j++)
//		out.push_back(gameState().pickRandomArtifact(rand, EArtifactClass::ART_MINOR));
//
//	out.push_back(gameState().pickRandomArtifact(rand, EArtifactClass::ART_MAJOR));
//}


//ArtifactID CGameState::pickRandomArtifact(vstd::RNG & randomGenerator, std::optional<EArtifactClass> type, std::function<bool(ArtifactID)> accepts)
//{
//	std::set<ArtifactID> potentialPicks;
//
//	// Select artifacts that satisfy provided criteria
//	for (auto const & artifactID : map->allowedArtifact)
//	{
//		if (!LIBRARY->arth->legalArtifact(artifactID))
//			continue;
//
//		const auto * artifact = artifactID.toArtifact();
//
//		assert(artifact->aClass != EArtifactClass::ART_SPECIAL); // should be filtered out when allowedArtifacts is initialized
//
//		if (type.has_value() && *type != artifact->aClass)
//			continue;
//
//		if (!accepts(artifact->getId()))
//			continue;
//
//		potentialPicks.insert(artifact->getId());
//	}
//
//	return pickRandomArtifact(randomGenerator, potentialPicks);
//}
//
//ArtifactID CGameState::pickRandomArtifact(vstd::RNG & randomGenerator, std::set<ArtifactID> potentialPicks)
//{
//	// No allowed artifacts at all - give Grail - this can't be banned (hopefully)
//	// FIXME: investigate how such cases are handled by H3 - some heavily customized user-made maps likely rely on H3 behavior
//	if (potentialPicks.empty())
//	{
//		logGlobal->warn("Failed to find artifact that matches requested parameters!");
//		return ArtifactID::GRAIL;
//	}
//
//	// Find how many times least used artifacts were picked by randomizer
//	int leastUsedTimes = std::numeric_limits<int>::max();
//	for (auto const & artifact : potentialPicks)
//		if (allocatedArtifacts[artifact] < leastUsedTimes)
//			leastUsedTimes = allocatedArtifacts[artifact];
//
//	// Pick all artifacts that were used least number of times
//	std::set<ArtifactID> preferredPicks;
//	for (auto const & artifact : potentialPicks)
//		if (allocatedArtifacts[artifact] == leastUsedTimes)
//			preferredPicks.insert(artifact);
//
//	assert(!preferredPicks.empty());
//
//	ArtifactID artID = *RandomGeneratorUtil::nextItem(preferredPicks, randomGenerator);
//	allocatedArtifacts[artID] += 1; // record +1 more usage
//	return artID;
//}
//
//ArtifactID CGameState::pickRandomArtifact(vstd::RNG & randomGenerator, std::function<bool(ArtifactID)> accepts)
//{
//	return pickRandomArtifact(randomGenerator, std::nullopt, std::move(accepts));
//}
//
//ArtifactID CGameState::pickRandomArtifact(vstd::RNG & randomGenerator, std::optional<EArtifactClass> type)
//{
//	return pickRandomArtifact(randomGenerator, type, [](const ArtifactID &) { return true; });
//}

//CreatureID CCreatureHandler::pickRandomMonster(vstd::RNG & rand, int tier) const
//{
//	std::vector<CreatureID> allowed;
//	for(const auto & creature : objects)
//	{
//		if(creature->special)
//			continue;
//
//		if(creature->excludeFromRandomization)
//			continue;
//
//		if (creature->level == tier || tier == -1)
//			allowed.push_back(creature->getId());
//	}
//
//	if(allowed.empty())
//	{
//		logGlobal->warn("Cannot pick a random creature of tier %d!", tier);
//		return CreatureID::NONE;
//	}
//
//	return *RandomGeneratorUtil::nextItem(allowed, rand);
//}
