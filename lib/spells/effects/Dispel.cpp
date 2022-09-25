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

#include <vcmi/spells/Spell.h>

#include "../ISpellMechanics.h"

#include "../../NetPacks.h"
#include "../../battle/IBattleState.h"
#include "../../battle/Unit.h"
#include "../../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

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

void Dispel::apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const
{
	const bool describe = server->describeChanges();
	SetStackEffect sse;
	BattleLogMessage blm;

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
				blm.lines.push_back(std::move(line));
			}

			std::vector<Bonus> buffer;
			auto bl = getBonuses(m, unit);

			for(auto item : *bl)
				buffer.emplace_back(*item);

			if(!buffer.empty())
				sse.toRemove.push_back(std::make_pair(unit->unitId(), buffer));
		}
	}

	if(!sse.toRemove.empty())
		server->apply(&sse);

	if(describe && !blm.lines.empty())
		server->apply(&blm);
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

std::shared_ptr<const BonusList> Dispel::getBonuses(const Mechanics * m, const battle::Unit * unit) const
{
	auto sel = [=](const Bonus * bonus)
	{
		if(bonus->source == Bonus::SPELL_EFFECT)
		{
			const Spell * sourceSpell = SpellID(bonus->sid).toSpell(m->spells());
			if(!sourceSpell)
				return false;//error

			//Special case: DISRUPTING_RAY and ACID_BREATH_DEFENSE are "immune" to dispell
			//Other even PERMANENT effects can be removed (f.e. BIND)
			if(sourceSpell->getIndex() == SpellID::DISRUPTING_RAY || sourceSpell->getIndex() == SpellID::ACID_BREATH_DEFENSE)
				return false;
			//Special case: do not remove lifetime marker
			if(sourceSpell->getIndex() == SpellID::CLONE)
				return false;
			//stack may have inherited effects
			if(sourceSpell->isAdventure())
				return false;

			if(sourceSpell->getIndex() == m->getSpellIndex())
				return false;

			auto positiveness = sourceSpell->getPositiveness();

			if(boost::logic::indeterminate(positiveness))
			{
				if(neutral)
					return true;
			}
			else if(positiveness)
			{
				if(positive)
					return true;
			}
			else
			{
				if(negative)
					return true;
			}
		}
		return false;
	};
	CSelector selector(sel);

	return unit->getBonuses(selector);
}

}
}

VCMI_LIB_NAMESPACE_END
