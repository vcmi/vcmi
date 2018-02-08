/*
 * CustomSpellMechanics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "ISpellMechanics.h"
#include "CDefaultSpellMechanics.h"

#include "effects/Effects.h"

namespace spells
{

class CustomSpellMechanics : public DefaultSpellMechanics
{
public:
	CustomSpellMechanics(const IBattleCast * event, std::shared_ptr<effects::Effects> e);
	virtual ~CustomSpellMechanics();

	void applyEffects(const SpellCastEnvironment * env, const Target & targets) const override;
	void applyIndirectEffects(const SpellCastEnvironment * env, const Target & targets) const override;

	void applyEffectsForced(const SpellCastEnvironment * env, const Target & targets) const override;

	bool canBeCast(Problem & problem) const override;
	bool canBeCastAt(BattleHex destination) const override;

	void beforeCast(vstd::RNG & rng, const Target & target, std::vector <const CStack*> & reflected) override;

	void applyCastEffects(const SpellCastEnvironment * env, const Target & target) const override;

	void cast(IBattleState * battleState, vstd::RNG & rng, const Target & target) override;

	std::vector<const CStack *> getAffectedStacks(BattleHex destination) const override final;

	std::vector<AimType> getTargetTypes() const override final;
	std::vector<Destination> getPossibleDestinations(size_t index, AimType aimType, const Target & current) const override final;

	std::vector<BattleHex> rangeInHexes(BattleHex centralHex, bool * outDroppedHexes = nullptr) const override;

private:
	effects::Effects::EffectsToApply effectsToApply;

	std::shared_ptr<effects::Effects> effects;

	std::set<const battle::Unit *> collectTargets() const;

	Target transformSpellTarget(const Target & aimPoint) const;
};

} //namespace spells
