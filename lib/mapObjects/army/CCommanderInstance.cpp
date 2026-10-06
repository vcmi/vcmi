/*
 * CCommanderInstance.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CCommanderInstance.h"

#include "../../GameLibrary.h"
#include "../../callback/IGameInfoCallback.h"
#include "../../entities/hero/CHeroHandler.h"

CCommanderInstance::CCommanderInstance(IGameInfoCallback * cb)
	: CStackInstance(cb)
{
}

CCommanderInstance::CCommanderInstance(IGameInfoCallback * cb, const CreatureID & id)
	: CStackInstance(cb, BonusNodeType::COMMANDER, false)
	, name("Commando")
{
	alive = true;
	setCount(1);
	setType(nullptr);
	secondarySkills.resize(ECommander::SPELL_POWER + 1);
	setType(id);
	//TODO - parse them
}

void CCommanderInstance::setAlive(bool Alive)
{
	//TODO: helm of immortality
	alive = Alive;
	if(!alive)
	{
		removeBonusesRecursive(Bonus::UntilCommanderKilled);
	}
}

bool CCommanderInstance::canGainExperience() const
{
	return alive;
}

int CCommanderInstance::getExpRank() const
{
	return level;
}

int CCommanderInstance::getLevel() const
{
	return level;
}

void CCommanderInstance::levelUp()
{
	level++;
	for(const auto & bonus : LIBRARY->creh->commanderLevelPremy)
	{ //grant all regular level-up bonuses
		accumulateBonus(bonus);
	}
}

ArtBearer CCommanderInstance::bearerType() const
{
	return ArtBearer::COMMANDER;
}

bool CCommanderInstance::gainsLevel() const
{
	return level < cb->getHeroLevelLimit() && getTotalExperience() >= LIBRARY->heroh->reqExp(level + 1);
}

void CCommanderInstance::levelUpAutomatically(vstd::RNG & rand)
{
	const auto skills = getLevelUpSkillChoices();

	levelUp();
	if(skills.empty())
		return;

	const int skill = *RandomGeneratorUtil::nextItem(skills, rand);
	for(const auto & bonus : getSkillBonuses(skill))
		accumulateBonus(std::make_shared<Bonus>(bonus));

	if(skill <= ECommander::SPELL_POWER)
		secondarySkills.at(skill) += 1;
	else
		specialSkills.insert(skill);
}

std::vector<int> CCommanderInstance::getLevelUpSkillChoices() const
{
	std::vector<int> result;

	for(int i = 0; i <= ECommander::SPELL_POWER; ++i)
	{
		if(secondarySkills.at(i) < ECommander::MAX_SKILL_LEVEL)
			result.push_back(i);
	}

	int i = 100;
	for(const auto & specialSkill : LIBRARY->creh->skillRequirements)
	{
		if(secondarySkills.at(specialSkill.second.first) >= ECommander::MAX_SKILL_LEVEL - 1
			&& secondarySkills.at(specialSkill.second.second) >= ECommander::MAX_SKILL_LEVEL - 1
			&& !vstd::contains(specialSkills, i))
			result.push_back(i);
		++i;
	}
	return result;
}

std::vector<Bonus> CCommanderInstance::getSkillBonuses(int skill) const
{
	std::vector<Bonus> result;

	if(skill > ECommander::SPELL_POWER)
	{
		for(const auto & bonus : LIBRARY->creh->skillRequirements.at(skill - 100).first)
			result.push_back(*bonus);
		return result;
	}

	const auto difference = [this](int skillToTest) -> int
	{
		int s = std::min(skillToTest, static_cast<int>(ECommander::SPELL_POWER)); //spell power level controls also casts and resistance
		const auto & skillLevels = LIBRARY->creh->skillLevels.at(skillToTest);
		return skillLevels.at(secondarySkills.at(s)) - (secondarySkills.at(s) ? skillLevels.at(secondarySkills.at(s) - 1) : 0);
	};

	Bonus bonus;
	bonus.source = BonusSource::COMMANDER;
	bonus.valType = BonusValueType::BASE_NUMBER;

	switch(skill)
	{
		case ECommander::ATTACK:
			bonus.type = BonusType::PRIMARY_SKILL;
			bonus.subtype = BonusSubtypeID(PrimarySkill::ATTACK);
			break;
		case ECommander::DEFENSE:
			bonus.type = BonusType::PRIMARY_SKILL;
			bonus.subtype = BonusSubtypeID(PrimarySkill::DEFENSE);
			break;
		case ECommander::HEALTH:
			bonus.type = BonusType::STACK_HEALTH;
			bonus.valType = BonusValueType::PERCENT_TO_ALL; //TODO: check how it accumulates in original WoG with artifacts such as vial of life blood, elixir of life etc.
			break;
		case ECommander::DAMAGE:
			bonus.type = BonusType::CREATURE_DAMAGE;
			bonus.subtype = BonusCustomSubtype::creatureDamageBoth;
			bonus.valType = BonusValueType::PERCENT_TO_ALL;
			break;
		case ECommander::SPEED:
			bonus.type = BonusType::STACKS_SPEED;
			break;
		case ECommander::SPELL_POWER:
			bonus.type = BonusType::SPELL_DAMAGE_REDUCTION;
			bonus.subtype = BonusSubtypeID(SpellSchool::ANY);
			bonus.val = difference(ECommander::RESISTANCE);
			result.push_back(bonus);
			bonus.type = BonusType::CREATURE_SPELL_POWER;
			bonus.val = difference(ECommander::SPELL_POWER) * 100; //like hero with spellpower = ability level
			result.push_back(bonus);
			bonus.type = BonusType::CASTS;
			bonus.val = difference(ECommander::CASTS);
			result.push_back(bonus);
			bonus.type = BonusType::CREATURE_ENCHANT_POWER;
			break;
	}

	bonus.val = difference(skill);
	result.push_back(bonus);
	return result;
}
