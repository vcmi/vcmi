/*
 * RandomizationProcessor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/callback/IGameRandomizer.h"

VCMI_LIB_NAMESPACE_BEGIN

class CRandomGenerator;
class CGHeroInstance;

VCMI_LIB_NAMESPACE_END

/// Biased randomizer that has following properties:
/// - at bias value of 0 it acts as statistical random generator
/// - at bias value of 100 it guarantees that it will take at most 100/chance rolls till succesfull roll
/// - at bias value between 1..99 similar guarantee is also provided, but with larger number of rolls
/// No matter what bias is, statistical probability on large number of rolls remains the same
/// Its goal is to simulate human expectations of random distributions and reduce frustration from "bad" rolls
class BiasedRandomizer
{
	int accumulatedBias;
public:
	/// Performs coin flip with specified success chance
	/// Returns true with probability successChance percents, and false with probability 100-successChance percents
	bool roll(vstd::RNG & generator, int successChance, int biasValue);
};

class RandomizationProcessor final : public IGameRandomizer
{
	std::unique_ptr<CRandomGenerator> globalRandomNumberGenerator;

	std::map<HeroTypeID, std::unique_ptr<CRandomGenerator>> heroSeed;
	std::map<PlayerColor, std::unique_ptr<CRandomGenerator>> playerTavern;

	std::map<ObjectInstanceID, BiasedRandomizer> goodMoraleSeed;
	std::map<ObjectInstanceID, BiasedRandomizer> badMoraleSeed;
	std::map<ObjectInstanceID, BiasedRandomizer> goodLuckSeed;
	std::map<ObjectInstanceID, BiasedRandomizer> badLuckSeed;

	std::map<ObjectInstanceID, BiasedRandomizer> combatAbilitySeed;

public:
	RandomizationProcessor();

	PrimarySkill rollPrimarySkillForLevelup(const CGHeroInstance * hero);
	SecondarySkill rollSecondarySkillForLevelup(const CGHeroInstance * hero, const std::vector<SecondarySkill> & candidates);

	bool rollGoodMorale(ObjectInstanceID actor, int moraleValue);
	bool rollBadMorale(ObjectInstanceID actor, int moraleValue);
	bool rollGoodLuck(ObjectInstanceID actor, int luckValue);
	bool rollBadLuck(ObjectInstanceID actor, int luckValue);

	bool rollCombatAbility(ObjectInstanceID actor, int percentageChance);

	HeroTypeID rollHero(PlayerColor player, FactionID faction) override;

	CreatureID rollCreature() override;
	CreatureID rollCreature(int tier) override;

	ArtifactID rollArtifact() override;
	ArtifactID rollArtifact(EArtifactClass type) override;
	ArtifactID rollArtifact(std::set<ArtifactID> filtered) override;
	std::vector<ArtifactID> rollMarketArtifactSet() override;

	std::string rollTownName(FactionID faction) override;

	vstd::RNG & getDefault() override;

	void setSeed(int newSeed);

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & *globalRandomNumberGenerator;
	}
};
