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

#include "../VCMI_Lib.h"
#include "../IGameSettings.h"

int BonusCacheBase::getBonusValueImpl(BonusCacheEntry & currentValue, const CSelector & selector) const
{
	if (target->getTreeVersion() == currentValue.version)
	{
		return currentValue.value;
	}
	else
	{
		// NOTE: following code theoretically can fail if bonus tree was changed by another thread between two following lines
		// However, this situation should not be possible - gamestate modification should only happen in single-treaded mode with locked gamestate mutex
		int newValue = target->valOfBonuses(selector);
		currentValue.value = newValue;
		currentValue.version = target->getTreeVersion();

		return newValue;
	}
}

BonusValueCache::BonusValueCache(const IBonusBearer * target, const CSelector selector)
	:BonusCacheBase(target),selector(selector)
{}

int BonusValueCache::getValue() const
{
	return getBonusValueImpl(value, selector);
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
		VLC->engineSettings()->getVectorValue(EGameSettings::HEROES_MINIMAL_PRIMARY_SKILLS, PrimarySkill::ATTACK),
		VLC->engineSettings()->getVectorValue(EGameSettings::HEROES_MINIMAL_PRIMARY_SKILLS, PrimarySkill::DEFENSE),
		VLC->engineSettings()->getVectorValue(EGameSettings::HEROES_MINIMAL_PRIMARY_SKILLS, PrimarySkill::SPELL_POWER),
		VLC->engineSettings()->getVectorValue(EGameSettings::HEROES_MINIMAL_PRIMARY_SKILLS, PrimarySkill::KNOWLEDGE)
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
