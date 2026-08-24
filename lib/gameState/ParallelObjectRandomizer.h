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

/// True if object->pickRandomObject() must run on the main thread, in the object's fixed
/// map order, rather than concurrently with other objects. There are two distinct reasons:
/// - CGTownInstance: CGDwelling looks up a linked town by instance name/identifier and
///   expects it to already be randomized (see CGDwelling::randomizeFaction), so every town
///   must be resolved before any (parallel-bucket) dwelling runs.
/// - CGArtifact and CGHeroInstance(RANDOM_HERO): these draw from randomizer state that is
///   shared *across* object instances (GameRandomizer::allocatedArtifacts least-used-artifact
///   bias map, and CGameState's unused-hero-type pool respectively), so their relative draw
///   order must stay fixed to remain reproducible.
/// Everything else (creatures, resources, dwellings, non-random heroes, ...) only ever
/// touches its own, already-parsed data plus stateless LIBRARY content, and is safe to
/// randomize concurrently using a private RNG stream (see ParallelObjectRandomizer below).
bool objectNeedsSerialRandomization(const CGObjectInstance * object);

/// IGameRandomizer used to randomize a single map object on a worker thread. Wraps a
/// private RNG stream, seeded once on the main thread (from the real randomizer) before
/// any worker thread starts, so concurrent objects never contend on shared state and
/// results stay reproducible regardless of actual thread scheduling.
///
/// Only rollCreature()/getDefault() are implemented for real: those are the only
/// IGameRandomizer members reachable from the pickRandomObject() overrides that
/// objectNeedsSerialRandomization() allows into the parallel bucket (CGDwelling,
/// CGCreature, CGResource). Everything else throws as a correctness safety net - hitting
/// one would mean an object was misclassified into the parallel bucket.
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

/// IGameRandomizer used to initialize a single map object (CGObjectInstance::initObj) on a
/// worker thread during CGameState::initMapObjects(). Same private-RNG-stream approach as
/// ParallelObjectRandomizer above for rollCreature()/getDefault(), reseeded once on the main
/// thread so those draws stay reproducible regardless of thread scheduling.
///
/// Unlike ParallelObjectRandomizer, the rollArtifact() overloads are delegated straight
/// through to the real (shared) gameRandomizer instead of being rejected: initObj()
/// implementations that grant artifacts (CGArtifact, Rewardable::Info-driven objects and
/// town buildings, ...) need GameRandomizer's cross-object "least used artifact" bias
/// bookkeeping (allocatedArtifacts), which only exists on that single shared instance.
/// GameRandomizer now internally serializes access to that state (see
/// GameRandomizer::artifactRollMutex), so concurrent delegation from multiple worker threads
/// is memory-safe. Note this does trade away full determinism for artifact-granting objects
/// specifically: unlike the seed-split creature/RNG draws, the *order* in which concurrently
/// running objects reach the shared rollArtifact() call - and therefore which of them wins a
/// "least used so far" pick when several are tied - depends on actual thread scheduling.
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
