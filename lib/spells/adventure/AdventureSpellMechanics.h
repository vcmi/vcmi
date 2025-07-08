/*
 * AdventureSpellMechanics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../ISpellMechanics.h"

VCMI_LIB_NAMESPACE_BEGIN

enum class ESpellCastResult
{
	OK, // cast successful
	CANCEL, // cast failed but it is not an error, no mana has been spent
	PENDING,
	ERROR // error occurred, for example invalid request from player
};

class AdventureSpellMechanics : public IAdventureSpellMechanics
{
public:
	AdventureSpellMechanics(const CSpell * s);

	bool canBeCast(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster) const final;
	bool canBeCastAt(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const final;

	bool adventureCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const override final;

protected:
	///actual adventure cast implementation
	virtual ESpellCastResult applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const;
	virtual ESpellCastResult beginCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const;
	virtual void endCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const;
	virtual bool canBeCastImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster) const;
	virtual bool canBeCastAtImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const;

	void performCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const;
};

VCMI_LIB_NAMESPACE_END
