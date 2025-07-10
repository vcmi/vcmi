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

class IAdventureSpellEffect;

class AdventureSpellMechanics final : public IAdventureSpellMechanics, boost::noncopyable
{
	struct LevelOptions
	{
		std::unique_ptr<IAdventureSpellEffect> effect;
		std::vector<std::shared_ptr<Bonus>> bonuses;
		int castsPerDay;
		int castsPerDayXL;
	};

	std::array<LevelOptions, GameConstants::SPELL_SCHOOL_LEVELS> levelOptions;

public:
	AdventureSpellMechanics(const CSpell * s);
	~AdventureSpellMechanics();

	const LevelOptions & getLevel(const spells::Caster * caster) const;

	bool canBeCast(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster) const final;
	bool canBeCastAt(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const final;
	bool isTargetInRange(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const final;
	bool adventureCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const final;

	void giveBonuses(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const;
	void performCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const;
};

VCMI_LIB_NAMESPACE_END
