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

/// IGameRandomizer for randomizing one map object on a worker thread, using a private RNG
/// stream seeded up-front on the main thread. Only rollCreature()/getDefault() are used by
/// the parallel-bucket objects; everything else throws as a misuse safety net.
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

/// IGameRandomizer for CGObjectInstance::initObj() on a worker thread during initMapObjects().
/// Like ParallelObjectRandomizer for creatures/RNG, but delegates rollArtifact() to the shared
/// gameRandomizer since artifact-granting objects need its cross-object allocation bias state.
class ParallelInitRandomizer final : public IGameRandomizer
{
	CRandomGenerator generator;
	IGameRandomizer & sharedRandomizer;

public:
	ParallelInitRandomizer(int seed, IGameRandomizer & sharedRandomizer);

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
