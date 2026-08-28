/*
 * ParallelObjectRandomizer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../callback/IGameRandomizer.h"
#include "../CRandomGenerator.h"

class CGObjectInstance;

/// True if the object must be randomized on the main thread instead of the parallel pool,
/// because it depends on state shared across objects (linked towns, allocatedArtifacts bias,
/// unused-hero-type pool).
bool objectNeedsSerialRandomization(const CGObjectInstance * object);

/// True if the object's initObj() may roll an artifact, so must run serially for reproducibility
bool objectNeedsSerialInit(const CGObjectInstance * object);

/// IGameRandomizer for randomizing/initializing one map object (pickRandomObject()/initObj())
/// on a worker thread, using a private RNG stream seeded up-front on the main thread. Only
/// rollCreature()/getDefault() are used by the parallel-bucket objects; everything else throws
/// as a misuse safety net.
class ParallelObjectRandomizer final : public IGameRandomizer
{
	CRandomGenerator generator;

public:
	explicit ParallelObjectRandomizer(int seed);

	CreatureID rollCreature() override;
	CreatureID rollCreature(int tier) override;
	vstd::RNG & getDefault() override;

	ArtifactID rollArtifact() override;
	ArtifactID rollArtifact(EArtifactClass type) override;
	ArtifactID rollArtifact(std::set<ArtifactID> filtered) override;
	std::vector<ArtifactID> rollMarketArtifactSet() override;
	PrimarySkill rollPrimarySkillForLevelup(const CGHeroInstance * hero) override;
	SecondarySkill rollSecondarySkillForLevelup(const CGHeroInstance * hero, const std::set<SecondarySkill> & candidates) override;
	std::vector<SecondarySkill> rollSecondarySkills(const CGHeroInstance * hero) override;
};
