/*
 * BattleSpellMechanics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "ISpellMechanics.h"

#include "effects/Effects.h"

VCMI_LIB_NAMESPACE_BEGIN

struct BattleSpellCast;

namespace battle
{
	using Units = boost::container::small_vector<const Unit *, 4>;
}

namespace spells
{

class BattleSpellMechanics : public BaseMechanics
{
public:
	BattleSpellMechanics(const IBattleCast * event, std::shared_ptr<effects::Effects> effects_, std::shared_ptr<IReceptiveCheck> targetCondition_);
	virtual ~BattleSpellMechanics();

	void forEachEffect(const std::function<bool(const spells::effects::Effect &)> & fn) const override final;

	// TODO: ??? (what's the difference compared to cast?)
	void applyEffects(ServerCallback * server, const Target & targets, bool indirect, bool ignoreImmunity) const override;

	/// Returns false if spell can not be cast at all, e.g. due to not having any possible target on battlefield
	bool canBeCast(Problem & problem) const override;

	/// Returns false if spell can not be cast at specified target
	bool canBeCastAt(const Target & target, Problem & problem) const override;

	// TODO: ??? (what's the difference compared to applyEffects?)
	void cast(ServerCallback * server, const Target & target) override final;
	// TODO: ??? (what's the difference compared to cast?)
	void castEval(ServerCallback * server, const Target & target) override final;

	/// Returns list of affected stack using currently configured target
	std::vector<const CStack *> getAffectedStacks(const Target & target) const override final;

	/// Returns list of target types that can be targeted by spell
	std::vector<AimType> getTargetTypes() const override final;

	/// Returns vector of all possible destinations for specified aim type
	/// index - ???
	/// current - ???
	std::vector<Destination> getPossibleDestinations(size_t index, AimType aimType, const Target & current, bool fast) const override final;

	/// Returns true if spell can be cast on unit
	bool isReceptive(const battle::Unit * target) const override;
	bool isSmart() const override;

	/// Returns list of hexes that are affected by spell assuming cast at centralHex
	BattleHexArray rangeInHexes(const BattleHex & centralHex) const override;

	Target canonicalizeTarget(const Target & aim) const override;

	const Spell * getSpell() const override;

	bool counteringSelector(const Bonus * bonus) const;

private:
	std::shared_ptr<effects::Effects> effects;
	std::shared_ptr<IReceptiveCheck> targetCondition;

	battle::Units affectedUnits;
	effects::Effects::EffectsToApply effectsToApply;

	void beforeCast(BattleSpellCast & sc, vstd::RNG & rng, const Target & target);
	bool isReflected(const battle::Unit * unit, vstd::RNG & rng);
	void reflect(BattleSpellCast & sc, vstd::RNG & rng, const battle::Unit * unit);
	const battle::Unit * getRandomUnit(vstd::RNG & rng, const BattleSide & side);

	std::set<const battle::Unit *> collectTargets() const;

	void doRemoveEffects(ServerCallback * server, const battle::Units & targets, const CSelector & selector);

	BattleHexArray spellRangeInHexes(const BattleHex & centralHex) const;

	Target transformSpellTarget(const Target & aimPoint) const;

	bool canCastAtTarget(const battle::Unit * target) const;
};

}


VCMI_LIB_NAMESPACE_END
