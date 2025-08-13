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

namespace NKAI
{

const SecondarySkillEvaluator HeroManager::wariorSkillsScores = SecondarySkillEvaluator(
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

const SecondarySkillEvaluator HeroManager::scountSkillsScores = SecondarySkillEvaluator(
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
	auto role = getHeroRole(hero);

	if(role == HeroRole::MAIN)
		return wariorSkillsScores.evaluateSecSkill(hero, skill);

	return scountSkillsScores.evaluateSecSkill(hero, skill);
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
			float bonusScore = wariorSkillsScores.evaluateSecSkill(hero, bonusSkill);

			if(bonusScore > 0)
				specialityScore += bonusScore * bonusScore * bonusScore;
		}
	}

	return specialityScore;
}

float HeroManager::evaluateFightingStrength(const CGHeroInstance * hero) const
{
	return evaluateSpeciality(hero) + wariorSkillsScores.evaluateSecSkills(hero) + hero->getBasePrimarySkillValue(PrimarySkill::ATTACK) + hero->getBasePrimarySkillValue(PrimarySkill::DEFENSE) + hero->getBasePrimarySkillValue(PrimarySkill::SPELL_POWER) + hero->getBasePrimarySkillValue(PrimarySkill::KNOWLEDGE);
}

void HeroManager::update()
{
	logAi->trace("Start analysing our heroes");

	std::map<const CGHeroInstance *, float> scores;
	auto myHeroes = cb->getHeroesInfo();

	for(auto & hero : myHeroes)
	{
		scores[hero] = evaluateFightingStrength(hero);
		knownFightingStrength[hero->id] = hero->getHeroStrength();
	}

	auto scoreSort = [&](const CGHeroInstance * h1, const CGHeroInstance * h2) -> bool
	{
		return scores.at(h1) > scores.at(h2);
	};

	int globalMainCount = std::min(((int)myHeroes.size() + 2) / 3, cb->getMapSize().x / 50 + 1);

	//vstd::amin(globalMainCount, 1 + (cb->getTownsInfo().size() / 3));
	if(cb->getTownsInfo().size() < 4 && globalMainCount > 2)
	{
		globalMainCount = 2;
	}

	std::sort(myHeroes.begin(), myHeroes.end(), scoreSort);
	heroRoles.clear();

	for(auto hero : myHeroes)
	{
		if(hero->patrol.patrolling)
		{
			heroRoles[hero] = HeroRole::MAIN;
		}
		else
		{
			heroRoles[hero] = (globalMainCount--) > 0 ? HeroRole::MAIN : HeroRole::SCOUT;
		}
	}

	for(auto hero : myHeroes)
	{
		logAi->trace("Hero %s has role %s", hero->getNameTranslated(), heroRoles[hero] == HeroRole::MAIN ? "main" : "scout");
	}
}

HeroRole HeroManager::getHeroRole(const HeroPtr & hero) const
{
	if (heroRoles.find(hero) != heroRoles.end())
		return heroRoles.at(hero);
	else
		return HeroRole::SCOUT;
}

const std::map<HeroPtr, HeroRole> & HeroManager::getHeroRoles() const
{
	return heroRoles;
}

int HeroManager::selectBestSkill(const HeroPtr & hero, const std::vector<SecondarySkill> & skills) const
{
	auto role = getHeroRole(hero);
	auto & evaluator = role == HeroRole::MAIN ? wariorSkillsScores : scountSkillsScores;

	int result = 0;
	float resultScore = -100;

	for(int i = 0; i < skills.size(); i++)
	{
		auto score = evaluator.evaluateSecSkill(hero.get(), skills[i]);

		if(score > resultScore)
		{
			resultScore = score;
			result = i;
		}

		logAi->trace(
			"Hero %s is proposed to learn %d with score %f",
			hero.name(),
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
	int heroCount = cb->getHeroCount(ai->playerID, includeGarrisoned);

	return heroCount >= ai->settings->getMaxRoamingHeroes()
		|| heroCount >= cb->getSettings().getInteger(EGameSettings::HEROES_PER_PLAYER_ON_MAP_CAP)
		|| heroCount >= cb->getSettings().getInteger(EGameSettings::HEROES_PER_PLAYER_TOTAL_CAP);
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

	if(cb->getResourceAmount(EGameResID::GOLD) < GameConstants::HERO_GOLD_COST)
		return false;

	if(heroCapReached())
		return false;

	if(!cb->getAvailableHeroes(town).size())
		return false;

	return true;
}

const CGTownInstance * HeroManager::findTownWithTavern() const
{
	for(const CGTownInstance * t : cb->getTownsInfo())
		if(townHasFreeTavern(t))
			return t;

	return nullptr;
}

const CGHeroInstance * HeroManager::findHeroWithGrail() const
{
	for(const CGHeroInstance * h : cb->getHeroesInfo())
	{
		if(h->hasArt(ArtifactID::GRAIL))
			return h;
	}
	return nullptr;
}

const CGHeroInstance * HeroManager::findWeakHeroToDismiss(uint64_t armyLimit, const CGTownInstance* townToSpare) const
{
	const CGHeroInstance * weakestHero = nullptr;
	auto myHeroes = ai->cb->getHeroesInfo();

	for(auto existingHero : myHeroes)
	{
		if(ai->getHeroLockedReason(existingHero) == HeroLockedReason::DEFENCE
			|| existingHero->getArmyStrength() >armyLimit
			|| getHeroRole(existingHero) == HeroRole::MAIN
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
