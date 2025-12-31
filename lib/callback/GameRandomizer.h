/*
 * GameRandomizer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "callback/IGameRandomizer.h"
#include "CRandomGenerator.h"

VCMI_LIB_NAMESPACE_BEGIN

enum class EGameSettings;

class CGHeroInstance;

class DLL_LINKAGE RandomizationBias
{
	int32_t accumulatedBias = 0;

public:
	/// Performs coin flip with specified success chance
	/// Returns true with probability successChance percents, and false with probability totalWeight-successChance percents
	bool roll(vstd::RNG & generator, int successChance, int totalWeight, int biasValue);

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & accumulatedBias;
	}
};

/// Biased randomizer that has following properties:
/// - at bias value of 0 it acts as statistical random generator, just like vstd::RNG
/// - at bias value of 100 it guarantees that it will take at most 100/chance rolls till succesfull roll
/// - at bias value between 1..99 similar guarantee is also provided, but with larger number of rolls
/// No matter what bias is, statistical probability on large number of rolls remains the same
/// Its goal is to simulate human expectations of random distributions and reduce frustration from "bad" rolls
class DLL_LINKAGE RandomGeneratorWithBias
{
	CRandomGenerator generator;
	RandomizationBias bias;

public:
	explicit RandomGeneratorWithBias(int seed = 0);
	/// Performs coin flip with specified success chance
	/// Returns true with probability successChance percents, and false with probability 100-successChance percents
	bool roll(int successChance, int totalWeight, int biasValue);

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & generator;
		h & bias;
	}
};

class DLL_LINKAGE GameRandomizer final : public IGameRandomizer
{
	struct HeroSkillRandomizer
	{
		explicit HeroSkillRandomizer(int seed = 0)
			: seed(seed)
		{}

		CRandomGenerator seed;
		int8_t magicSchoolCounter = 1;
		int8_t wisdomCounter = 1;

		template<typename Handler>
		void serialize(Handler & h)
		{
			h & seed;
			h & magicSchoolCounter;
			h & wisdomCounter;
		}
	};

	const IGameInfoCallback & gameInfo;

	/// Global RNG, for use when there is no specialized instance
	CRandomGenerator globalRandomNumberGenerator;

	/// Stores number of times each artifact was placed on map via randomization
	std::map<ArtifactID, int> allocatedArtifacts;

	std::map<HeroTypeID, HeroSkillRandomizer> heroSkillSeed;

	std::map<ObjectInstanceID, RandomGeneratorWithBias> goodMoraleSeed;
	std::map<ObjectInstanceID, RandomGeneratorWithBias> badMoraleSeed;
	std::map<ObjectInstanceID, RandomGeneratorWithBias> goodLuckSeed;
	std::map<ObjectInstanceID, RandomGeneratorWithBias> badLuckSeed;
	std::map<ObjectInstanceID, RandomGeneratorWithBias> combatAbilitySeed;

	bool rollMoraleLuck(std::map<ObjectInstanceID, RandomGeneratorWithBias> & seeds, ObjectInstanceID actor, int moraleLuckValue, EGameSettings biasValue, EGameSettings diceSize, EGameSettings diceWeights);

public:
	explicit GameRandomizer(const IGameInfoCallback & gameInfo);
	~GameRandomizer();

	PrimarySkill rollPrimarySkillForLevelup(const CGHeroInstance * hero) override;
	SecondarySkill rollSecondarySkillForLevelup(const CGHeroInstance * hero, const std::set<SecondarySkill> & candidates) override;

	bool rollGoodMorale(ObjectInstanceID actor, int moraleValue);
	bool rollBadMorale(ObjectInstanceID actor, int moraleValue);
	bool rollGoodLuck(ObjectInstanceID actor, int luckValue);
	bool rollBadLuck(ObjectInstanceID actor, int luckValue);

	bool rollCombatAbility(ObjectInstanceID actor, int percentageChance);

	CreatureID rollCreature() override;
	CreatureID rollCreature(int tier) override;

	ArtifactID rollArtifact() override;
	ArtifactID rollArtifact(EArtifactClass type) override;
	ArtifactID rollArtifact(std::set<ArtifactID> filtered) override;
	std::vector<ArtifactID> rollMarketArtifactSet() override;

	vstd::RNG & getDefault() override;

	void setSeed(int newSeed);

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & globalRandomNumberGenerator;

		if (h.hasFeature(Handler::Version::RANDOMIZATION_REWORK))
		{
			h & allocatedArtifacts;
			h & heroSkillSeed;
			h & goodMoraleSeed;
			h & badMoraleSeed;
			h & goodLuckSeed;
			h & badLuckSeed;
			h & combatAbilitySeed;
		}
	}
};

VCMI_LIB_NAMESPACE_END
