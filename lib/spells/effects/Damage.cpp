/*
 * Damage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Damage.h"
#include "Registry.h"
#include "../CSpellHandler.h"
#include "../ISpellMechanics.h"

#include "../../NetPacks.h"
#include "../../CStack.h"
#include "../../battle/IBattleState.h"
#include "../../battle/CBattleInfoCallback.h"
#include "../../CGeneralTextHandler.h"
#include "../../serializer/JsonSerializeFormat.h"

#include <vcmi/spells/Spell.h>

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

void Damage::apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const
{
	StacksInjured stacksInjured;
	BattleLogMessage blm;
	size_t targetIndex = 0;
	const battle::Unit * firstTarget = nullptr;
	const bool describe = server->describeChanges();

	int64_t damageToDisplay = 0;
	uint32_t killed = 0;
	bool multiple = false;

	for(const auto & t : target)
	{
		const battle::Unit * unit = t.unitValue;
		if(unit && unit->alive())
		{
			BattleStackAttacked bsa;
			bsa.damageAmount = damageForTarget(targetIndex, m, unit);
			bsa.stackAttacked = unit->unitId();
			bsa.attackerID = -1;
			auto newState = unit->acquireState();
			CStack::prepareAttacked(bsa, *server->getRNG(), newState);

			if(describe)
			{
				if(!firstTarget)
					firstTarget = unit;
				else
					multiple = true;
				damageToDisplay += bsa.damageAmount;
				killed += bsa.killedAmount;
			}
			stacksInjured.stacks.push_back(bsa);
		}
		targetIndex++;
	}

	if(describe && firstTarget && damageToDisplay > 0)
	{
		describeEffect(blm.lines, m, firstTarget, killed, damageToDisplay, multiple);
	}

	if(!stacksInjured.stacks.empty())
		server->apply(&stacksInjured);

	if(!blm.lines.empty())
		server->apply(&blm);
}

bool Damage::isReceptive(const Mechanics * m, const battle::Unit * unit) const
{
	if(!UnitEffect::isReceptive(m, unit))
		return false;

	bool isImmune = m->getSpell()->isMagical() && (unit->getBonusBearer()->valOfBonuses(BonusType::SPELL_DAMAGE_REDUCTION, SpellSchool(ESpellSchool::ANY)) >= 100); //General spell damage immunity
	//elemental immunity for damage
	m->getSpell()->forEachSchool([&](const ESpellSchool & cnf, bool & stop)
	{
		isImmune |= (unit->getBonusBearer()->valOfBonuses(BonusType::SPELL_DAMAGE_REDUCTION, SpellSchool(cnf)) >= 100); //100% reduction is immunity
	});

	return !isImmune;
}

void Damage::serializeJsonUnitEffect(JsonSerializeFormat & handler)
{
	handler.serializeBool("killByPercentage", killByPercentage);
	handler.serializeBool("killByCount", killByCount);
}

int64_t Damage::damageForTarget(size_t targetIndex, const Mechanics * m, const battle::Unit * target) const
{
	int64_t baseDamage;

	if(killByPercentage)
	{
		int64_t amountToKill = target->getCount() * m->getEffectValue() / 100;
		baseDamage = amountToKill * target->getMaxHealth();
	}
	else if(killByCount)
	{
		baseDamage = m->getEffectValue() * target->getMaxHealth();
	}
	else
	{
		baseDamage = m->adjustEffectValue(target);
	}

	if(chainLength > 1 && targetIndex > 0)
	{
		double indexedFactor = std::pow(chainFactor, static_cast<double>(targetIndex));
		return static_cast<int64_t>(indexedFactor * baseDamage);
	}

	return baseDamage;
}

void Damage::describeEffect(std::vector<MetaString> & log, const Mechanics * m, const battle::Unit * firstTarget, uint32_t kills, int64_t damage, bool multiple) const
{
	if(m->getSpellIndex() == SpellID::DEATH_STARE && !multiple)
	{
		MetaString line;
		if(kills > 1)
		{
			line.appendLocalString(EMetaText::GENERAL_TXT, 119); //%d %s die under the terrible gaze of the %s.
			line.replaceNumber(kills);
			firstTarget->addNameReplacement(line, true);
		}
		else
		{
			line.appendLocalString(EMetaText::GENERAL_TXT, 118); //One %s dies under the terrible gaze of the %s.
			firstTarget->addNameReplacement(line, false);
		}
		m->caster->getCasterName(line);
		log.push_back(line);
	}
	else if(m->getSpellIndex() == SpellID::THUNDERBOLT && !multiple)
	{
		{
			MetaString line;
			firstTarget->addText(line, EMetaText::GENERAL_TXT, -367, true);
			firstTarget->addNameReplacement(line, true);
			log.push_back(line);
		}

		{
			MetaString line;
			//todo: handle newlines in metastring
			std::string text = VLC->generaltexth->allTexts[343]; //Does %d points of damage.
			boost::algorithm::trim(text);
			line.appendRawString(text);
			line.replaceNumber(static_cast<int>(damage)); //no more text afterwards
			log.push_back(line);
		}
	}
	else
	{
		{
			MetaString line;
			line.appendLocalString(EMetaText::GENERAL_TXT, 376); // Spell %s does %d damage
			line.replaceLocalString(EMetaText::SPELL_NAME, m->getSpellIndex());
			line.replaceNumber(static_cast<int>(damage));

			log.push_back(line);
		}

		if (kills > 0)
		{
			MetaString line;

			if(kills > 1)
			{
				line.appendLocalString(EMetaText::GENERAL_TXT, 379); // %d %s perishes
				line.replaceNumber(kills);

				if(multiple)
					line.replaceLocalString(EMetaText::GENERAL_TXT, 43); // creatures
				else
					firstTarget->addNameReplacement(line, true);
			}
			else // single creature killed
			{
				line.appendLocalString(EMetaText::GENERAL_TXT, 378); // one %s perishes
				if(multiple)
					line.replaceLocalString(EMetaText::GENERAL_TXT, 42); // creature
				else
					firstTarget->addNameReplacement(line, false);
			}

			log.push_back(line);
		}
	}
}

}
}

VCMI_LIB_NAMESPACE_END
