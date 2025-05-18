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

/// Biased randomizer that has following properties:
/// - at bias value of 0 it acts as statistical random generator, just like vstd::RNG
/// - at bias value of 100 it guarantees that it will take at most 100/chance rolls till succesfull roll
/// - at bias value between 1..99 similar guarantee is also provided, but with larger number of rolls
/// No matter what bias is, statistical probability on large number of rolls remains the same
/// Its goal is to simulate human expectations of random distributions and reduce frustration from "bad" rolls
class BiasedRandomizer
{
	CRandomGenerator seed;
	int32_t accumulatedBias = 0;
public:
	explicit BiasedRandomizer(int seed);
	/// Performs coin flip with specified success chance
	/// Returns true with probability successChance percents, and false with probability 100-successChance percents
	bool roll(int successChance, int totalWeight, int biasValue);
};

class DLL_LINKAGE GameRandomizer final : public IGameRandomizer
{
	static constexpr int biasValueLuckMorale = 10;
	static constexpr int biasValueAbility = 25;

	struct HeroSkillRandomizer
	{
		HeroSkillRandomizer(int seed)
			:seed(seed)
		{}

		CRandomGenerator seed;
		int8_t magicSchoolCounter = 1;
		int8_t wisdomCounter = 1;
	};

	const IGameInfoCallback & gameInfo;

	/// Global RNG, for use when there is no specialized instance
	CRandomGenerator globalRandomNumberGenerator;

	/// Stores number of times each artifact was placed on map via randomization
	std::map<ArtifactID, int> allocatedArtifacts;

	std::map<HeroTypeID, HeroSkillRandomizer> heroSkillSeed;
	std::map<PlayerColor, CRandomGenerator> playerTavern;

	std::map<ObjectInstanceID, BiasedRandomizer> goodMoraleSeed;
	std::map<ObjectInstanceID, BiasedRandomizer> badMoraleSeed;
	std::map<ObjectInstanceID, BiasedRandomizer> goodLuckSeed;
	std::map<ObjectInstanceID, BiasedRandomizer> badLuckSeed;
	std::map<ObjectInstanceID, BiasedRandomizer> combatAbilitySeed;

	bool rollMoraleLuck(std::map<ObjectInstanceID, BiasedRandomizer> & seeds, ObjectInstanceID actor, int moraleLuckValue, EGameSettings diceSize, EGameSettings diceWeights);
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

//	HeroTypeID rollHero(PlayerColor player, FactionID faction) override;

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
	}
};

VCMI_LIB_NAMESPACE_END
