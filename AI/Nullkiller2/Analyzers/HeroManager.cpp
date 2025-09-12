/*
* HeroManager.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "../StdInc.h"
#include "../Engine/Nullkiller.h"
#include "../../../lib/mapObjects/MapObjects.h"
#include "../../../lib/IGameSettings.h"
#include "../../../lib/spells/ISpellMechanics.h"
#include "../../../lib/spells/adventure/TownPortalEffect.h"

namespace NK2AI
{

const SecondarySkillEvaluator HeroManager::mainSkillsEvaluator = SecondarySkillEvaluator(
	{
		std::make_shared<SecondarySkillScoreMap>(
			std::map<SecondarySkill, float>
			{
				{SecondarySkill::DIPLOMACY, 2},
				{SecondarySkill::LOGISTICS, 2},
				{SecondarySkill::EARTH_MAGIC, 2},
				{SecondarySkill::ARMORER, 2},
				{SecondarySkill::OFFENCE, 2},
				{SecondarySkill::AIR_MAGIC, 1},
				{SecondarySkill::WISDOM, 1},
				{SecondarySkill::LEADERSHIP, 1},
				{SecondarySkill::INTELLIGENCE, 1},
				{SecondarySkill::RESISTANCE, 1},
				{SecondarySkill::MYSTICISM, -1},
				{SecondarySkill::SORCERY, -1},
				{SecondarySkill::ESTATES, -1},
				{SecondarySkill::FIRST_AID, -1},
				{SecondarySkill::LEARNING, -1},
				{SecondarySkill::SCHOLAR, -1},
				{SecondarySkill::EAGLE_EYE, -1},
				{SecondarySkill::NAVIGATION, -1}
			}),
		std::make_shared<ExistingSkillRule>(),
		std::make_shared<WisdomRule>(),
		std::make_shared<AtLeastOneMagicRule>()
	});

const SecondarySkillEvaluator HeroManager::scoutSkillsEvaluator = SecondarySkillEvaluator(
	{
		std::make_shared<SecondarySkillScoreMap>(
			std::map<SecondarySkill, float>
			{
				{SecondarySkill::LOGISTICS, 2},
				{SecondarySkill::ESTATES, 2},
				{SecondarySkill::PATHFINDING, 1},
				{SecondarySkill::SCHOLAR, 1}
			}),
		std::make_shared<ExistingSkillRule>()
	});

float HeroManager::evaluateSecSkill(SecondarySkill skill, const CGHeroInstance * hero) const
{
	auto role = getHeroRoleOrDefaultInefficient(hero);

	if(role == HeroRole::MAIN)
		return mainSkillsEvaluator.evaluateSecSkill(hero, skill);

	return scoutSkillsEvaluator.evaluateSecSkill(hero, skill);
}

float HeroManager::evaluateSpeciality(const CGHeroInstance * hero) const
{
	auto heroSpecial = Selector::source(BonusSource::HERO_SPECIAL, BonusSourceID(hero->getHeroTypeID()));
	auto secondarySkillBonus = Selector::targetSourceType()(BonusSource::SECONDARY_SKILL);
	auto specialSecondarySkillBonuses = hero->getBonuses(heroSpecial.And(secondarySkillBonus), "HeroManager::evaluateSpeciality");
	auto secondarySkillBonuses = hero->getBonusesFrom(BonusSource::SECONDARY_SKILL);
	float specialityScore = 0.0f;

	for(auto bonus : *secondarySkillBonuses)
	{
		auto hasBonus = !!specialSecondarySkillBonuses->getFirst(Selector::typeSubtype(bonus->type, bonus->subtype));

		if(hasBonus)
		{
			SecondarySkill bonusSkill = bonus->sid.as<SecondarySkill>();
			float bonusScore = mainSkillsEvaluator.evaluateSecSkill(hero, bonusSkill);

			if(bonusScore > 0)
				specialityScore += bonusScore * bonusScore * bonusScore;
		}
	}

	return specialityScore;
}

float HeroManager::evaluateFightingStrength(const CGHeroInstance * hero) const
{
	// TODO: Mircea: Shouldn't we count bonuses from artifacts when generating the fighting strength? That could make a huge difference
	return evaluateSpeciality(hero) + mainSkillsEvaluator.evaluateSecSkills(hero) + hero->getBasePrimarySkillValue(PrimarySkill::ATTACK) + hero->getBasePrimarySkillValue(PrimarySkill::DEFENSE) + hero->getBasePrimarySkillValue(PrimarySkill::SPELL_POWER) + hero->getBasePrimarySkillValue(PrimarySkill::KNOWLEDGE);
}

void HeroManager::update()
{
	logAi->trace("Start analysing our heroes");

	std::map<const CGHeroInstance *, float> scores;
	auto myHeroes = cc->getHeroesInfo();

	for(auto & hero : myHeroes)
	{
		scores[hero] = evaluateFightingStrength(hero);
		knownFightingStrength[hero->id] = hero->getHeroStrength();
	}

	auto scoreSort = [&](const CGHeroInstance * h1, const CGHeroInstance * h2) -> bool
	{
		return scores.at(h1) > scores.at(h2);
	};

	int globalMainCount = std::min(((int)myHeroes.size() + 2) / 3, cc->getMapSize().x / 50 + 1);
	//vstd::amin(globalMainCount, 1 + (cb->getTownsInfo().size() / 3));
	if(cc->getTownsInfo().size() < 4 && globalMainCount > 2)
	{
		globalMainCount = 2;
	}
	// TODO: Mircea: Make it dependent on myHeroes.size() or better?
	logAi->trace("Max number of main heroes (globalMainCount) is %d", globalMainCount);

	std::sort(myHeroes.begin(), myHeroes.end(), scoreSort);
	heroToRoleMap.clear();

	for(const auto hero : myHeroes)
	{
		HeroPtr heroPtr(hero, aiNk->cc.get());
		HeroRole role;
		if(hero->patrol.patrolling)
		{
			role = MAIN;
		}
		else
		{
			role = globalMainCount-- > 0 ? MAIN : SCOUT;
		}

		heroToRoleMap[heroPtr] = role;
		logAi->trace("Hero %s has role %s", heroPtr.nameOrDefault(), role == MAIN ? "main" : "scout");
	}
}

HeroRole HeroManager::getHeroRoleOrDefaultInefficient(const  CGHeroInstance * hero) const
{
	return getHeroRoleOrDefault(HeroPtr(hero, aiNk->cc.get()));
}

// TODO: Mircea: Do we need this map on HeroPtr or is enough just on hero?
HeroRole HeroManager::getHeroRoleOrDefault(const HeroPtr & heroPtr) const
{
	if(heroToRoleMap.find(heroPtr) != heroToRoleMap.end())
		return heroToRoleMap.at(heroPtr);
	return HeroRole::SCOUT;
}

int HeroManager::selectBestSkillIndex(const HeroPtr & heroPtr, const std::vector<SecondarySkill> & skills) const
{
	const auto role = getHeroRoleOrDefault(heroPtr);
	const auto & evaluator = role == MAIN ? mainSkillsEvaluator : scoutSkillsEvaluator;
	int result = 0;
	float resultScore = -100;

	for(int i = 0; i < skills.size(); i++)
	{
		const auto score = evaluator.evaluateSecSkill(heroPtr.getUnverified(), skills[i]);
		if(score > resultScore)
		{
			resultScore = score;
			result = i;
		}

		logAi->trace(
			"Hero %s is offered to learn %d with score %f",
			heroPtr.nameOrDefault(),
			skills[i].toEnum(),
			score);
	}

	return result;
}

float HeroManager::evaluateHero(const CGHeroInstance * hero) const
{
	return evaluateFightingStrength(hero);
}

bool HeroManager::heroCapReached(bool includeGarrisoned) const
{
	int heroCount = cc->getHeroCount(aiNk->playerID, includeGarrisoned);
	int maxAllowed = aiNk->settings->getMaxRoamingHeroes();
	if (aiNk->settings->getMaxRoamingHeroesPerTown() > 0)
	{
		maxAllowed += cc->howManyTowns() * aiNk->settings->getMaxRoamingHeroesPerTown();
	}

	return heroCount >= maxAllowed
		|| heroCount >= cc->getSettings().getInteger(EGameSettings::HEROES_PER_PLAYER_ON_MAP_CAP)
		|| heroCount >= cc->getSettings().getInteger(EGameSettings::HEROES_PER_PLAYER_TOTAL_CAP);
}

float HeroManager::getFightingStrengthCached(const CGHeroInstance * hero) const
{
	auto cached = knownFightingStrength.find(hero->id);

	//FIXME: fallback to hero->getFightingStrength() is VERY slow on higher difficulties (no object graph? map reveal?)
	return cached != knownFightingStrength.end() ? cached->second : hero->getHeroStrength();
}

float HeroManager::getMagicStrength(const CGHeroInstance * hero) const
{
	auto manaLimit = hero->manaLimit();
	auto spellPower = hero->getPrimSkillLevel(PrimarySkill::SPELL_POWER);

	auto score = 0.0f;

	// FIXME: this will not cover spells give by scrolls / tomes. Intended?
	for(auto spellId : hero->getSpellsInSpellbook())
	{
		auto spell = spellId.toSpell();

		if (!spell->isAdventure())
			continue;

		auto schoolLevel = hero->getSpellSchoolLevel(spell);
		auto townPortalEffect = spell->getAdventureMechanics().getEffectAs<TownPortalEffect>(hero);

		score += (spell->getLevel() + 1) * (schoolLevel + 1) * 0.05f;

		if (spell->getAdventureMechanics().givesBonus(hero, BonusType::FLYING_MOVEMENT))
			score += 0.3;

		if(townPortalEffect != nullptr && schoolLevel != 0)
			score += 0.6f;
	}

	vstd::amin(score, 1);

	score *= std::min(1.0f, spellPower / 10.0f);

	vstd::amin(score, 1);

	score *= std::min(1.0f, manaLimit / 100.0f);

	return std::min(score, 1.0f);
}

bool HeroManager::canRecruitHero(const CGTownInstance * town) const
{
	if(!town)
		town = findTownWithTavern();

	if(!town || !townHasFreeTavern(town))
		return false;

	if(cc->getResourceAmount(EGameResID::GOLD) < GameConstants::HERO_GOLD_COST)
		return false;

	if(heroCapReached())
		return false;

	if(!cc->getAvailableHeroes(town).size())
		return false;

	return true;
}

const CGTownInstance * HeroManager::findTownWithTavern() const
{
	for(const CGTownInstance * t : cc->getTownsInfo())
		if(townHasFreeTavern(t))
			return t;

	return nullptr;
}

const CGHeroInstance * HeroManager::findHeroWithGrail() const
{
	for(const CGHeroInstance * h : cc->getHeroesInfo())
	{
		if(h->hasArt(ArtifactID::GRAIL))
			return h;
	}
	return nullptr;
}

const CGHeroInstance * HeroManager::findWeakHeroToDismiss(uint64_t armyLimit, const CGTownInstance* townToSpare) const
{
	const CGHeroInstance * weakestHero = nullptr;
	auto myHeroes = aiNk->cc->getHeroesInfo();

	for(auto existingHero : myHeroes)
	{
		if(aiNk->getHeroLockedReason(existingHero) == HeroLockedReason::DEFENCE
			|| existingHero->getArmyStrength() >armyLimit
			|| getHeroRoleOrDefaultInefficient(existingHero) == HeroRole::MAIN
			|| existingHero->movementPointsRemaining()
			|| (townToSpare != nullptr && existingHero->getVisitedTown() == townToSpare)
			|| existingHero->artifactsWorn.size() > (existingHero->hasSpellbook() ? 2 : 1))
		{
			continue;
		}

		if(!weakestHero || weakestHero->getHeroStrength() > existingHero->getHeroStrength())
		{
			weakestHero = existingHero;
		}
	}

	return weakestHero;
}

SecondarySkillScoreMap::SecondarySkillScoreMap(std::map<SecondarySkill, float> scoreMap)
	:scoreMap(scoreMap)
{
}

void SecondarySkillScoreMap::evaluateScore(const CGHeroInstance * hero, SecondarySkill skill, float & score) const
{
	auto it = scoreMap.find(skill);

	if(it != scoreMap.end())
	{
		score = it->second;
	}
}

void ExistingSkillRule::evaluateScore(const CGHeroInstance * hero, SecondarySkill skill, float & score) const
{
	int upgradesLeft = 0;

	for(auto & heroSkill : hero->secSkills)
	{
		if(heroSkill.first == skill)
			return;

		upgradesLeft += MasteryLevel::EXPERT - heroSkill.second;
	}

	if(score >= 2 || (score >= 1 && upgradesLeft <= 1))
		score += 1.5;
}

void WisdomRule::evaluateScore(const CGHeroInstance * hero, SecondarySkill skill, float & score) const
{
	if(skill != SecondarySkill::WISDOM)
		return;

	auto wisdomLevel = hero->getSecSkillLevel(SecondarySkill::WISDOM);

	if(hero->level > 10 && wisdomLevel == MasteryLevel::NONE)
		score += 1.5;
}

const std::vector<SecondarySkill> AtLeastOneMagicRule::magicSchools = {
	SecondarySkill::AIR_MAGIC,
	SecondarySkill::EARTH_MAGIC,
	SecondarySkill::FIRE_MAGIC,
	SecondarySkill::WATER_MAGIC
};

void AtLeastOneMagicRule::evaluateScore(const CGHeroInstance * hero, SecondarySkill skill, float & score) const
{
	if(!vstd::contains(magicSchools, skill))
		return;
	
	bool heroHasAnyMagic = vstd::contains_if(magicSchools, [&](SecondarySkill skill) -> bool
	{
		return hero->getSecSkillLevel(skill) > MasteryLevel::NONE;
	});

	if(!heroHasAnyMagic)
		score += 1;
}

SecondarySkillEvaluator::SecondarySkillEvaluator(std::vector<std::shared_ptr<ISecondarySkillRule>> evaluationRules)
	: evaluationRules(evaluationRules)
{
}

float SecondarySkillEvaluator::evaluateSecSkills(const CGHeroInstance * hero) const
{
	float totalScore = 0;

	for(auto skill : hero->secSkills)
	{
		totalScore += skill.second * evaluateSecSkill(hero, skill.first);
	}

	return totalScore;
}

float SecondarySkillEvaluator::evaluateSecSkill(const CGHeroInstance * hero, SecondarySkill skill) const
{
	float score = 0;

	for(auto rule : evaluationRules)
		rule->evaluateScore(hero, skill, score);

	return score;
}

}
