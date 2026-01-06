/*
 * GameRandomizer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "GameRandomizer.h"

#include "IGameInfoCallback.h"

#include "../../lib/CRandomGenerator.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CSkillHandler.h"
#include "../../lib/IGameSettings.h"
#include "../../lib/entities/artifact/CArtHandler.h"
#include "../../lib/entities/artifact/EArtifactClass.h"
#include "../../lib/entities/hero/CHeroClass.h"
#include "../../lib/mapObjects/CGHeroInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

bool RandomizationBias::roll(vstd::RNG & generator, int successChance, int totalWeight, int biasValue)
{
	assert(successChance > 0);
	assert(totalWeight >= successChance);

	int failChance = totalWeight - successChance;
	int newRoll = generator.nextInt(1, totalWeight);
	// accumulated bias is stored as premultiplied to avoid precision loss on division
	// so multiply everything else in equation to compensate
	// precision loss is small, and generally insignificant, but better to play it safe
	bool success = newRoll * totalWeight - accumulatedBias <= successChance * totalWeight;
	if(success)
		accumulatedBias -= failChance * biasValue;
	else
		accumulatedBias += successChance * biasValue;

	return success;
}

RandomGeneratorWithBias::RandomGeneratorWithBias(int seed)
	: generator(seed)
{
}

bool RandomGeneratorWithBias::roll(int successChance, int totalWeight, int biasValue)
{
	return bias.roll(generator, successChance, totalWeight, biasValue);
}

GameRandomizer::GameRandomizer(const IGameInfoCallback & gameInfo)
	: gameInfo(gameInfo)
{
}

GameRandomizer::~GameRandomizer() = default;


bool GameRandomizer::rollMoraleLuck(std::map<ObjectInstanceID, RandomGeneratorWithBias> & seeds, ObjectInstanceID actor, int moraleLuckValue, EGameSettings biasValueSetting, EGameSettings diceSizeSetting, EGameSettings chanceVectorSetting)
{
	assert(moraleLuckValue > 0);
	auto chanceVector = gameInfo.getSettings().getVector(chanceVectorSetting);
	int diceSize = gameInfo.getSettings().getInteger(diceSizeSetting);
	int biasValue = gameInfo.getSettings().getInteger(biasValueSetting);
	size_t chanceIndex = std::min<size_t>(chanceVector.size(), moraleLuckValue) - 1; // array index, so 0-indexed

	if(!seeds.count(actor))
		seeds.try_emplace(actor, getDefault().nextInt());

	if(chanceVector.empty())
		return false;

	return seeds.at(actor).roll(chanceVector[chanceIndex], diceSize, biasValue);
}

bool GameRandomizer::rollGoodMorale(ObjectInstanceID actor, int moraleValue)
{
	return rollMoraleLuck(goodMoraleSeed, actor, moraleValue, EGameSettings::COMBAT_MORALE_BIAS, EGameSettings::COMBAT_MORALE_DICE_SIZE, EGameSettings::COMBAT_GOOD_MORALE_CHANCE);
}

bool GameRandomizer::rollBadMorale(ObjectInstanceID actor, int moraleValue)
{
	return rollMoraleLuck(badMoraleSeed, actor, moraleValue, EGameSettings::COMBAT_MORALE_BIAS, EGameSettings::COMBAT_MORALE_DICE_SIZE, EGameSettings::COMBAT_BAD_MORALE_CHANCE);
}

bool GameRandomizer::rollGoodLuck(ObjectInstanceID actor, int luckValue)
{
	return rollMoraleLuck(goodLuckSeed, actor, luckValue, EGameSettings::COMBAT_LUCK_BIAS, EGameSettings::COMBAT_LUCK_DICE_SIZE, EGameSettings::COMBAT_GOOD_LUCK_CHANCE);
}

bool GameRandomizer::rollBadLuck(ObjectInstanceID actor, int luckValue)
{
	return rollMoraleLuck(badLuckSeed, actor, luckValue, EGameSettings::COMBAT_LUCK_BIAS, EGameSettings::COMBAT_LUCK_DICE_SIZE, EGameSettings::COMBAT_BAD_LUCK_CHANCE);
}

bool GameRandomizer::rollCombatAbility(ObjectInstanceID actor, int percentageChance)
{
	if(!combatAbilitySeed.count(actor))
		combatAbilitySeed.try_emplace(actor, getDefault().nextInt());

	if(percentageChance <= 0)
		return false;

	if(percentageChance >= 100)
		return true;

	int biasValue = gameInfo.getSettings().getInteger(EGameSettings::COMBAT_ABILITY_BIAS);

	return combatAbilitySeed.at(actor).roll(percentageChance, 100, biasValue);
}

CreatureID GameRandomizer::rollCreature()
{
	std::vector<CreatureID> allowed;
	for(const auto & creatureID : LIBRARY->creh->getDefaultAllowed())
	{
		const auto * creaturePtr = creatureID.toCreature();
		if(!creaturePtr->excludeFromRandomization)
			allowed.push_back(creaturePtr->getId());
	}

	if(allowed.empty())
		throw std::runtime_error("Cannot pick a random creature!");

	return *RandomGeneratorUtil::nextItem(allowed, getDefault());
}

CreatureID GameRandomizer::rollCreature(int tier)
{
	std::vector<CreatureID> allowed;
	for(const auto & creatureID : LIBRARY->creh->getDefaultAllowed())
	{
		const auto * creaturePtr = creatureID.toCreature();
		if(creaturePtr->excludeFromRandomization)
			continue;

		if(creaturePtr->getLevel() == tier)
			allowed.push_back(creaturePtr->getId());
	}

	if(allowed.empty())
		throw std::runtime_error("Cannot pick a random creature!");

	return *RandomGeneratorUtil::nextItem(allowed, getDefault());
}

ArtifactID GameRandomizer::rollArtifact()
{
	std::set<ArtifactID> potentialPicks;

	for(const auto & artifactID : LIBRARY->arth->getDefaultAllowed())
	{
		if(!LIBRARY->arth->legalArtifact(artifactID))
			continue;

		potentialPicks.insert(artifactID);
	}

	return rollArtifact(potentialPicks);
}

ArtifactID GameRandomizer::rollArtifact(EArtifactClass type)
{
	std::set<ArtifactID> potentialPicks;

	for(const auto & artifactID : LIBRARY->arth->getDefaultAllowed())
	{
		if(!LIBRARY->arth->legalArtifact(artifactID))
			continue;

		if(!gameInfo.isAllowed(artifactID))
			continue;

		const auto * artifact = artifactID.toArtifact();

		if(type != artifact->aClass)
			continue;

		potentialPicks.insert(artifactID);
	}

	return rollArtifact(potentialPicks);
}

ArtifactID GameRandomizer::rollArtifact(std::set<ArtifactID> potentialPicks)
{
	// No allowed artifacts at all - give Grail - this can't be banned (hopefully)
	// FIXME: investigate how such cases are handled by H3 - some heavily customized user-made maps likely rely on H3 behavior
	if(potentialPicks.empty())
	{
		logGlobal->warn("Failed to find artifact that matches requested parameters!");
		return ArtifactID::GRAIL;
	}

	// Find how many times least used artifacts were picked by randomizer
	int leastUsedTimes = std::numeric_limits<int>::max();
	for(const auto & artifact : potentialPicks)
		if(allocatedArtifacts[artifact] < leastUsedTimes)
			leastUsedTimes = allocatedArtifacts[artifact];

	// Pick all artifacts that were used least number of times
	std::set<ArtifactID> preferredPicks;
	for(const auto & artifact : potentialPicks)
		if(allocatedArtifacts[artifact] == leastUsedTimes)
			preferredPicks.insert(artifact);

	assert(!preferredPicks.empty());

	ArtifactID artID = *RandomGeneratorUtil::nextItem(preferredPicks, getDefault());
	allocatedArtifacts[artID] += 1; // record +1 more usage
	return artID;
}

std::vector<ArtifactID> GameRandomizer::rollMarketArtifactSet()
{
	return {
		rollArtifact(EArtifactClass::ART_TREASURE),
		rollArtifact(EArtifactClass::ART_TREASURE),
		rollArtifact(EArtifactClass::ART_TREASURE),
		rollArtifact(EArtifactClass::ART_MINOR),
		rollArtifact(EArtifactClass::ART_MINOR),
		rollArtifact(EArtifactClass::ART_MINOR),
		rollArtifact(EArtifactClass::ART_MAJOR)
	};
}

vstd::RNG & GameRandomizer::getDefault()
{
	return globalRandomNumberGenerator;
}

void GameRandomizer::setSeed(int newSeed)
{
	globalRandomNumberGenerator.setSeed(newSeed);
}

PrimarySkill GameRandomizer::rollPrimarySkillForLevelup(const CGHeroInstance * hero)
{
	if(!heroSkillSeed.count(hero->getHeroTypeID()))
		heroSkillSeed.try_emplace(hero->getHeroTypeID(), getDefault().nextInt());

	const bool isLowLevelHero = hero->level < GameConstants::HERO_HIGH_LEVEL;
	const auto & skillChances = isLowLevelHero ? hero->getHeroClass()->primarySkillLowLevel : hero->getHeroClass()->primarySkillHighLevel;
	auto & heroRng = heroSkillSeed.at(hero->getHeroTypeID());

	if(hero->isCampaignYog())
	{
		// Yog can only receive Attack or Defence on level-up
		std::vector<int> yogChances = {skillChances[0], skillChances[1]};
		return static_cast<PrimarySkill>(RandomGeneratorUtil::nextItemWeighted(yogChances, heroRng.seed));
	}
	return static_cast<PrimarySkill>(RandomGeneratorUtil::nextItemWeighted(skillChances, heroRng.seed));
}

SecondarySkill GameRandomizer::rollSecondarySkillForLevelup(const CGHeroInstance * hero, const std::set<SecondarySkill> & options)
{
	if(!heroSkillSeed.count(hero->getHeroTypeID()))
		heroSkillSeed.try_emplace(hero->getHeroTypeID(), getDefault().nextInt());

	auto & heroRng = heroSkillSeed.at(hero->getHeroTypeID());

	auto getObligatorySkills = [](CSkill::Obligatory obl)
	{
		std::set<SecondarySkill> obligatory;
		for(auto i = 0; i < LIBRARY->skillh->size(); i++)
			if((*LIBRARY->skillh)[SecondarySkill(i)]->obligatory(obl))
				obligatory.insert(i); //Always return all obligatory skills

		return obligatory;
	};

	auto intersect = [](const std::set<SecondarySkill> & left, const std::set<SecondarySkill> & right)
	{
		std::set<SecondarySkill> intersection;
		std::set_intersection(left.begin(), left.end(), right.begin(), right.end(), std::inserter(intersection, intersection.begin()));
		return intersection;
	};

	std::set<SecondarySkill> wisdomList = getObligatorySkills(CSkill::Obligatory::MAJOR);
	std::set<SecondarySkill> schoolList = getObligatorySkills(CSkill::Obligatory::MINOR);

	bool wantsWisdom = heroRng.wisdomCounter >= hero->maxlevelsToWisdom();
	bool wantsSchool = heroRng.magicSchoolCounter >= hero->maxlevelsToMagicSchool();
	bool selectWisdom = wantsWisdom && !intersect(options, wisdomList).empty();
	bool selectSchool = wantsSchool && !intersect(options, schoolList).empty();

	std::set<SecondarySkill> actualCandidates;

	if(selectWisdom)
		actualCandidates = intersect(options, wisdomList);
	else if(selectSchool)
		actualCandidates = intersect(options, schoolList);
	else
		actualCandidates = options;

	assert(!actualCandidates.empty());

	std::vector<int> weights;
	std::vector<SecondarySkill> skills;

	for(const auto & possible : actualCandidates)
	{
		skills.push_back(possible);
		if(hero->getHeroClass()->secSkillProbability.count(possible) != 0)
		{
			int weight = hero->getHeroClass()->secSkillProbability.at(possible);
			weights.push_back(std::max(1, weight));
		}
		else
			weights.push_back(1); // H3 behavior - banned skills have minimal (1) chance to be picked
	}

	int selectedIndex = RandomGeneratorUtil::nextItemWeighted(weights, heroRng.seed);
	SecondarySkill selectedSkill = skills.at(selectedIndex);

	if((*LIBRARY->skillh)[selectedSkill]->obligatory(CSkill::Obligatory::MAJOR))
		heroRng.wisdomCounter = 0;
	if((*LIBRARY->skillh)[selectedSkill]->obligatory(CSkill::Obligatory::MINOR))
		heroRng.magicSchoolCounter = 0;

	return selectedSkill;
}

std::vector<SecondarySkill> GameRandomizer::rollSecondarySkills(const CGHeroInstance * hero)
{
	auto & heroRng = heroSkillSeed.at(hero->getHeroTypeID());

	//deterministic secondary skills
	++heroRng.magicSchoolCounter;
	++heroRng.wisdomCounter;

	std::set<SecondarySkill> basicAndAdv;
	std::set<SecondarySkill> none;
	std::vector<SecondarySkill>	skills;

	if (hero->canLearnSkill())
	{
		for(int i = 0; i < LIBRARY->skillh->size(); i++)
			if (hero->canLearnSkill(SecondarySkill(i)))
				none.insert(SecondarySkill(i));
	}

	for(const auto & elem : hero->secSkills)
	{
		if(elem.second < MasteryLevel::EXPERT)
			basicAndAdv.insert(elem.first);
		none.erase(elem.first);
	}

	int maxUpgradedSkills = hero->cb->getSettings().getInteger(EGameSettings::LEVEL_UP_UPGRADED_SKILLS_AMOUNT);
	int maxTotalSkills = hero->cb->getSettings().getInteger(EGameSettings::LEVEL_UP_TOTAL_SKILLS_AMOUNT);
	int newSkillsAvailable = none.size();
	int upgradedSkillsToSelect = std::max(maxUpgradedSkills, maxTotalSkills - newSkillsAvailable);

	while (skills.size() < upgradedSkillsToSelect && !basicAndAdv.empty())
	{
		skills.push_back(rollSecondarySkillForLevelup(hero, basicAndAdv));
		basicAndAdv.erase(skills.back());
	}

	while (skills.size() < maxTotalSkills && !none.empty())
	{
		skills.push_back(rollSecondarySkillForLevelup(hero, none));
		none.erase(skills.back());
	}
	return skills;
}

VCMI_LIB_NAMESPACE_END
