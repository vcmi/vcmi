/*
 * Dispel.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Dispel.h"
#include "Registry.h"
#include "../ISpellMechanics.h"
#include "../CSpellHandler.h"

#include "../../NetPacks.h"
#include "../../battle/IBattleState.h"
#include "../../battle/Unit.h"
#include "../../serializer/JsonSerializeFormat.h"

static const std::string EFFECT_NAME = "core:dispel";

namespace spells
{
namespace effects
{

VCMI_REGISTER_SPELL_EFFECT(Dispel, EFFECT_NAME);

Dispel::Dispel()
	: UnitEffect()
{

}

Dispel::~Dispel() = default;

void Dispel::apply(BattleStateProxy * battleState, RNG & rng, const Mechanics * m, const EffectTarget & target) const
{
	SetStackEffect sse;
	prepareEffects(sse, rng, m, target, battleState->describe);

	if(!sse.toRemove.empty())
		battleState->apply(&sse);
}

bool Dispel::isValidTarget(const Mechanics * m, const battle::Unit * unit) const
{
	if(getBonuses(m, unit)->empty())
		return false;

	return UnitEffect::isValidTarget(m, unit);
}

void Dispel::serializeJsonUnitEffect(JsonSerializeFormat & handler)
{
	handler.serializeBool("dispelPositive", positive);
	handler.serializeBool("dispelNegative", negative);
	handler.serializeBool("dispelNeutral", neutral);
}

std::shared_ptr<BonusList> Dispel::getBonuses(const Mechanics * m, const battle::Unit * unit) const
{
	auto addSelector = [=](const Bonus * bonus)
	{
		if(bonus->source == Bonus::SPELL_EFFECT)
		{
			const CSpell * sourceSpell = SpellID(bonus->sid).toSpell();
			if(!sourceSpell)
				return false;//error
			if(bonus->sid == m->getSpellIndex())
				return false;

			if(positive && sourceSpell->isPositive())
				return true;
			if(negative && sourceSpell->isNegative())
				return true;
			if(neutral && sourceSpell->isNeutral())
				return true;

		}
		return false;
	};
	CSelector selector = CSelector(mainSelector).And(CSelector(addSelector));

	return unit->getBonuses(selector);
}

bool Dispel::mainSelector(const Bonus * bonus)
{
	if(bonus->source == Bonus::SPELL_EFFECT)
	{
		const CSpell * sourceSpell = SpellID(bonus->sid).toSpell();
		if(!sourceSpell)
			return false;//error
		//Special case: DISRUPTING_RAY is "immune" to dispell
		//Other even PERMANENT effects can be removed (f.e. BIND)
		if(sourceSpell->id == SpellID::DISRUPTING_RAY)
			return false;
		//Special case:do not remove lifetime marker
		if(sourceSpell->id == SpellID::CLONE)
			return false;
		//stack may have inherited effects
		if(sourceSpell->isAdventureSpell())
			return false;
		return true;
	}
	//not spell effect
	return false;
}

void Dispel::prepareEffects(SetStackEffect & pack, RNG & rng, const Mechanics * m, const EffectTarget & target, bool describe) const
{
	for(auto & t : target)
	{
		const battle::Unit * unit = t.unitValue;
		if(unit)
		{
			//special case for DISPEL_HELPFUL_SPELLS
			if(describe && positive && !negative && !neutral)
			{
				MetaString line;
				unit->addText(line, MetaString::GENERAL_TXT, -555, true);
				unit->addNameReplacement(line, true);
				pack.battleLog.push_back(std::move(line));
			}

			std::vector<Bonus> buffer;
			auto bl = getBonuses(m, unit);

			for(auto item : *bl)
				buffer.emplace_back(*item);

			if(!buffer.empty())
				pack.toRemove.push_back(std::make_pair(unit->unitId(), buffer));
		}
	}
}

}
}
