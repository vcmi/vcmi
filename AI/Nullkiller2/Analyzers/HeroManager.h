/*
* HeroManager.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "../AIUtility.h"

#include "../../../lib/GameConstants.h"
#include "../../../lib/GameLibrary.h"

namespace NK2AI
{

class DLL_EXPORT ISecondarySkillRule
{
public:
	virtual ~ISecondarySkillRule() = default;
	virtual void evaluateScore(const CGHeroInstance * hero, SecondarySkill skill, float & score) const = 0;
};

class DLL_EXPORT SecondarySkillEvaluator
{
private:
	std::vector<std::shared_ptr<ISecondarySkillRule>> evaluationRules;

public:
	SecondarySkillEvaluator(std::vector<std::shared_ptr<ISecondarySkillRule>> evaluationRules);
	float evaluateSecSkills(const CGHeroInstance * hero) const;
	float evaluateSecSkill(const CGHeroInstance * hero, SecondarySkill skill) const;
};

class DLL_EXPORT HeroManager
{
private:
	static const SecondarySkillEvaluator mainSkillsEvaluator;
	static const SecondarySkillEvaluator scoutSkillsEvaluator;

	CCallback * cc; //this is enough, but we downcast from CCallback
	const Nullkiller * aiNk;
	std::map<HeroPtr, HeroRole> heroToRoleMap; // can get out of sync with cc->getHeroesInfo() and include lost heroes
	std::map<ObjectInstanceID, float> knownFightingStrength;

public:
	HeroManager(CCallback * cc, const Nullkiller * aiNk) : cc(cc), aiNk(aiNk) {}
	HeroRole getHeroRoleOrDefaultInefficient(const CGHeroInstance * hero) const;
	HeroRole getHeroRoleOrDefault(const HeroPtr & heroPtr) const;
	int selectBestSkillIndex(const HeroPtr & heroPtr, const std::vector<SecondarySkill> & skills) const;
	void update();
	float evaluateSecSkill(SecondarySkill skill, const CGHeroInstance * hero) const;
	float evaluateHero(const CGHeroInstance * hero) const;
	bool canRecruitHero(const CGTownInstance * t = nullptr) const;
	bool heroCapReached(bool includeGarrisoned = true) const;
	const CGHeroInstance * findHeroWithGrail() const;
	const CGHeroInstance * findWeakHeroToDismiss(uint64_t armyLimit, const CGTownInstance * townToSpare = nullptr) const;
	float getMagicStrength(const CGHeroInstance * hero) const;
	float getFightingStrengthCached(const CGHeroInstance * hero) const;

private:
	float evaluateFightingStrength(const CGHeroInstance * hero) const;
	float evaluateSpeciality(const CGHeroInstance * hero) const;
	const CGTownInstance * findTownWithTavern() const;
};

// basic skill scores. missing skills will have score of 0
class DLL_EXPORT SecondarySkillScoreMap : public ISecondarySkillRule
{
private:
	std::map<SecondarySkill, float> scoreMap;

public:
	SecondarySkillScoreMap(std::map<SecondarySkill, float> scoreMap);
	void evaluateScore(const CGHeroInstance * hero, SecondarySkill skill, float & score) const override;
};

// Controls when to upgrade existing skills and when get new
class ExistingSkillRule : public ISecondarySkillRule
{
public:
	void evaluateScore(const CGHeroInstance * hero, SecondarySkill skill, float & score) const override;
};

// Allows to get wisdom at 12 lvl
class WisdomRule : public ISecondarySkillRule
{
public:
	void evaluateScore(const CGHeroInstance * hero, SecondarySkill skill, float & score) const override;
};

// Dynamically controls scores for magic skills
class AtLeastOneMagicRule : public ISecondarySkillRule
{
private:
	static const std::vector<SecondarySkill> magicSchools;

public:
	void evaluateScore(const CGHeroInstance * hero, SecondarySkill skill, float & score) const override;
};

}
