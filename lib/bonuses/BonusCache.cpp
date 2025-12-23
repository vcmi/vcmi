/*
 * BonusCache.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "BonusCache.h"
#include "IBonusBearer.h"

#include "BonusSelector.h"
#include "BonusList.h"

#include "../GameLibrary.h"
#include "../IGameSettings.h"
#include "../spells/SpellSchoolHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

int BonusCacheBase::getBonusValueImpl(BonusCacheEntry & currentValue, const CSelector & selector, BonusCacheMode mode) const
{
	if (target->getTreeVersion() == currentValue.version)
	{
		return currentValue.value;
	}
	else
	{
		// NOTE: following code theoretically can fail if bonus tree was changed by another thread between two following lines
		// However, this situation should not be possible - gamestate modification should only happen in single-treaded mode with locked gamestate mutex
		int newValue;

		if (mode == BonusCacheMode::VALUE)
			newValue = target->valOfBonuses(selector);
		else
			newValue = target->hasBonus(selector);
		currentValue.value = newValue;
		currentValue.version = target->getTreeVersion();

		return newValue;
	}
}

BonusValueCache::BonusValueCache(const IBonusBearer * target, const CSelector & selector)
	:BonusCacheBase(target),selector(selector)
{}

int BonusValueCache::getValue() const
{
	return getBonusValueImpl(value, selector, BonusCacheMode::VALUE);
}

bool BonusValueCache::hasBonus() const
{
	return getBonusValueImpl(value, selector, BonusCacheMode::PRESENCE);
}

MagicSchoolMasteryCache::MagicSchoolMasteryCache(const IBonusBearer * target)
	:target(target)
	,schools(LIBRARY->spellSchoolHandler->getAllObjects().size() + 1)
{}

void MagicSchoolMasteryCache::update() const
{
	static const CSelector allBonusesSelector = Selector::type()(BonusType::MAGIC_SCHOOL_SKILL);

	auto list = target->getBonuses(allBonusesSelector);
	schools[0] = list->valOfBonuses(Selector::subtype()(SpellSchool::ANY));

	for (int i = 1; i < schools.size(); ++i)
		schools[i] = list->valOfBonuses(Selector::subtype()(SpellSchool(i-1)));

	version = target->getTreeVersion();
}

int32_t MagicSchoolMasteryCache::getMastery(const SpellSchool & school) const
{
	if (target->getTreeVersion() != version)
		update();
	return schools[school.num + 1];
}

PrimarySkillsCache::PrimarySkillsCache(const IBonusBearer * target)
	:target(target)
{}

void PrimarySkillsCache::update() const
{
	static const CSelector primarySkillsSelector = Selector::type()(BonusType::PRIMARY_SKILL);
	static const CSelector attackSelector = Selector::subtype()(PrimarySkill::ATTACK);
	static const CSelector defenceSelector = Selector::subtype()(PrimarySkill::DEFENSE);
	static const CSelector spellPowerSelector = Selector::subtype()(PrimarySkill::SPELL_POWER);
	static const CSelector knowledgeSelector = Selector::subtype()(PrimarySkill::KNOWLEDGE);

	std::array<int, 4> minValues = {
		LIBRARY->engineSettings()->getVectorValue(EGameSettings::HEROES_MINIMAL_PRIMARY_SKILLS, PrimarySkill::ATTACK),
		LIBRARY->engineSettings()->getVectorValue(EGameSettings::HEROES_MINIMAL_PRIMARY_SKILLS, PrimarySkill::DEFENSE),
		LIBRARY->engineSettings()->getVectorValue(EGameSettings::HEROES_MINIMAL_PRIMARY_SKILLS, PrimarySkill::SPELL_POWER),
		LIBRARY->engineSettings()->getVectorValue(EGameSettings::HEROES_MINIMAL_PRIMARY_SKILLS, PrimarySkill::KNOWLEDGE)
	};

	auto list = target->getBonuses(primarySkillsSelector);
	skills[PrimarySkill::ATTACK] = std::max(minValues[PrimarySkill::ATTACK], list->valOfBonuses(attackSelector));
	skills[PrimarySkill::DEFENSE] = std::max(minValues[PrimarySkill::DEFENSE], list->valOfBonuses(defenceSelector));
	skills[PrimarySkill::SPELL_POWER] = std::max(minValues[PrimarySkill::SPELL_POWER], list->valOfBonuses(spellPowerSelector));
	skills[PrimarySkill::KNOWLEDGE] = std::max(minValues[PrimarySkill::KNOWLEDGE], list->valOfBonuses(knowledgeSelector));

	version = target->getTreeVersion();
}

const std::array<std::atomic<int32_t>, 4> & PrimarySkillsCache::getSkills() const
{
	if (target->getTreeVersion() != version)
		update();
	return skills;
}

int BonusCachePerTurn::getValueUncached(int turns) const
{
	std::lock_guard lock(bonusListMutex);

	int nodeTreeVersion = target->getTreeVersion();

	if (bonusListVersion != nodeTreeVersion)
	{
		bonusList = target->getBonuses(selector);
		bonusListVersion = nodeTreeVersion;
	}

	if (mode == BonusCacheMode::VALUE)
	{
		if (turns != 0)
			return bonusList->valOfBonuses(Selector::turns(turns));
		else
			return bonusList->totalValue();
	}
	else
	{
		if (turns != 0)
			return bonusList->getFirst(Selector::turns(turns)) != nullptr;
		else
			return !bonusList->empty();
	}
}

int BonusCachePerTurn::getValue(int turns) const
{
	int nodeTreeVersion = target->getTreeVersion();

	if (turns < cachedTurns)
	{
		auto & entry = cache[turns];
		if (entry.version == nodeTreeVersion)
		{
			// best case: value is in cache and up-to-date
			return entry.value;
		}
		else
		{
			// else - compute value and update it in the cache
			int newValue = getValueUncached(turns);
			entry.value = newValue;
			entry.version = nodeTreeVersion;
			return newValue;
		}
	}
	else
	{
		// non-cacheable value - compute and return (should be 0 / close to 0 calls)
		return getValueUncached(turns);
	}
}

const UnitBonusValuesProxy::SelectorsArray * UnitBonusValuesProxy::generateSelectors()
{
	static const CSelector additionalAttack = Selector::type()(BonusType::ADDITIONAL_ATTACK);
	static const CSelector selectorMelee = Selector::effectRange()(BonusLimitEffect::NO_LIMIT).Or(Selector::effectRange()(BonusLimitEffect::ONLY_MELEE_FIGHT));
	static const CSelector selectorRanged = Selector::effectRange()(BonusLimitEffect::NO_LIMIT).Or(Selector::effectRange()(BonusLimitEffect::ONLY_DISTANCE_FIGHT));
	static const CSelector minDamage = Selector::typeSubtype(BonusType::CREATURE_DAMAGE, BonusCustomSubtype::creatureDamageBoth).Or(Selector::typeSubtype(BonusType::CREATURE_DAMAGE, BonusCustomSubtype::creatureDamageMin));
	static const CSelector maxDamage = Selector::typeSubtype(BonusType::CREATURE_DAMAGE, BonusCustomSubtype::creatureDamageBoth).Or(Selector::typeSubtype(BonusType::CREATURE_DAMAGE, BonusCustomSubtype::creatureDamageMax));
	static const CSelector attack = Selector::typeSubtype(BonusType::PRIMARY_SKILL, BonusSubtypeID(PrimarySkill::ATTACK));
	static const CSelector defence = Selector::typeSubtype(BonusType::PRIMARY_SKILL, BonusSubtypeID(PrimarySkill::DEFENSE));

	static const UnitBonusValuesProxy::SelectorsArray selectors = {
		additionalAttack.And(selectorMelee), //TOTAL_ATTACKS_MELEE,
		additionalAttack.And(selectorRanged), //TOTAL_ATTACKS_RANGED,
		minDamage.And(selectorMelee), //MIN_DAMAGE_MELEE,
		minDamage.And(selectorRanged), //MIN_DAMAGE_RANGED,
		maxDamage.And(selectorMelee), //MAX_DAMAGE_MELEE,
		maxDamage.And(selectorRanged), //MAX_DAMAGE_RANGED,
		attack.And(selectorMelee),//ATTACK_MELEE,
		attack.And(selectorRanged),//ATTACK_RANGED,
		defence.And(selectorMelee),//DEFENCE_MELEE,
		defence.And(selectorRanged),//DEFENCE_RANGED,
		Selector::type()(BonusType::IN_FRENZY),//IN_FRENZY,
		Selector::type()(BonusType::HYPNOTIZED),//HYPNOTIZED,
		Selector::type()(BonusType::FORGETFULL),//FORGETFULL,
		Selector::type()(BonusType::FREE_SHOOTING).Or(Selector::type()(BonusType::SIEGE_WEAPON)),//HAS_FREE_SHOOTING,
		Selector::type()(BonusType::STACK_HEALTH),//STACK_HEALTH,
		Selector::type()(BonusType::INVINCIBLE),//INVINCIBLE,
		Selector::type()(BonusType::NONE).And(Selector::source(BonusSource::SPELL_EFFECT, BonusSourceID(SpellID(SpellID::CLONE))))
	};

	return &selectors;
}

VCMI_LIB_NAMESPACE_END
