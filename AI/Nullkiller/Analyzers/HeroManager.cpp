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
#include "../../../lib/CHeroHandler.h"

SecondarySkillEvaluator HeroManager::wariorSkillsScores = SecondarySkillEvaluator(
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

SecondarySkillEvaluator HeroManager::scountSkillsScores = SecondarySkillEvaluator(
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
	auto heroSpecial = Selector::source(Bonus::HERO_SPECIAL, hero->type->ID.getNum());
	auto secondarySkillBonus = Selector::type()(Bonus::SECONDARY_SKILL_PREMY);
	auto specialSecondarySkillBonuses = hero->getBonuses(heroSpecial.And(secondarySkillBonus));
	float specialityScore = 0.0f;

	for(auto bonus : *specialSecondarySkillBonuses)
	{
		SecondarySkill bonusSkill = SecondarySkill(bonus->subtype);
		float bonusScore = wariorSkillsScores.evaluateSecSkill(hero, bonusSkill);

		if(bonusScore > 0)
			specialityScore += bonusScore * bonusScore * bonusScore;
	}

	return specialityScore;
}

float HeroManager::evaluateFightingStrength(const CGHeroInstance * hero) const
{
	return evaluateSpeciality(hero) + wariorSkillsScores.evaluateSecSkills(hero) + hero->level * 1.5f;
}

std::vector<std::vector<const CGHeroInstance *>> clusterizeHeroes(CCallback * cb, std::vector<const CGHeroInstance *> heroes)
{
	std::vector<std::vector<const CGHeroInstance *>> clusters;

	for(auto hero : heroes)
	{
		auto paths = cb->getPathsInfo(hero);
		std::vector<const CGHeroInstance *> newCluster = {hero};

		for(auto cluster = clusters.begin(); cluster != clusters.end();)
		{
			auto hero = std::find_if(cluster->begin(), cluster->end(), [&](const CGHeroInstance * h) -> bool
			{
				return paths->getNode(h->visitablePos())->turns < SCOUT_TURN_DISTANCE_LIMIT;
			});

			if(hero != cluster->end())
			{
				vstd::concatenate(newCluster, *cluster);
				clusters.erase(cluster);
			}
			else
				cluster++;
		}

		clusters.push_back(newCluster);
	}

	return clusters;
}

void HeroManager::update()
{
	logAi->trace("Start analysing our heroes");

	std::map<const CGHeroInstance *, float> scores;
	auto myHeroes = cb->getHeroesInfo();

	for(auto & hero : myHeroes)
	{
		scores[hero] = evaluateFightingStrength(hero);
	}

	auto scoreSort = [&](const CGHeroInstance * h1, const CGHeroInstance * h2) -> bool
	{
		return scores.at(h1) > scores.at(h2);
	};

	int globalMainCount = std::min(((int)myHeroes.size() + 2) / 3, cb->getMapSize().x / 100 + 1);

	std::sort(myHeroes.begin(), myHeroes.end(), scoreSort);

	for(auto hero : myHeroes)
	{
		heroRoles[hero] = (globalMainCount--) > 0 ? HeroRole::MAIN : HeroRole::SCOUT;
	}

	for(auto cluster : clusterizeHeroes(cb, myHeroes))
	{
		std::sort(cluster.begin(), cluster.end(), scoreSort);

		auto localMainCountMax = (cluster.size() + 2) / 3;

		for(auto hero : cluster)
		{
			if(heroRoles[hero] != HeroRole::MAIN)
			{
				heroRoles[hero] = HeroRole::MAIN;
				break;
			}
			
			localMainCountMax--;

			if(localMainCountMax == 0)
				break;
		}
	}

	for(auto hero : myHeroes)
	{
		logAi->trace("Hero %s has role %s", hero->name, heroRoles[hero] == HeroRole::MAIN ? "main" : "scout");
	}
}

HeroRole HeroManager::getHeroRole(const HeroPtr & hero) const
{
	return heroRoles.at(hero);
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
			hero.name,
			skills[i].toEnum(),
			score);
	}

	return result;
}

float HeroManager::evaluateHero(const CGHeroInstance * hero) const
{
	return evaluateFightingStrength(hero);
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

		upgradesLeft += SecSkillLevel::EXPERT - heroSkill.second;
	}

	if(score >= 2 || (score >= 1 && upgradesLeft <= 1))
		score += 1.5;
}

void WisdomRule::evaluateScore(const CGHeroInstance * hero, SecondarySkill skill, float & score) const
{
	if(skill != SecondarySkill::WISDOM)
		return;

	auto wisdomLevel = hero->getSecSkillLevel(SecondarySkill::WISDOM);

	if(hero->level > 10 && wisdomLevel == SecSkillLevel::NONE)
		score += 1.5;
}

std::vector<SecondarySkill> AtLeastOneMagicRule::magicSchools = {
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
		return hero->getSecSkillLevel(skill) > SecSkillLevel::NONE;
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
