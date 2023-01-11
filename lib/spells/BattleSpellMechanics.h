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

namespace spells
{

class BattleSpellMechanics : public BaseMechanics
{
public:
	BattleSpellMechanics(const IBattleCast * event, std::shared_ptr<effects::Effects> effects_, std::shared_ptr<IReceptiveCheck> targetCondition_);
	virtual ~BattleSpellMechanics();

	// TODO: ??? (what's the difference compared to cast?)
	void applyEffects(ServerCallback * server, const Target & targets, bool indirect, bool ignoreImmunity) const override;

	/// Returns false if spell can not be cast at all, e.g. due to not having any possible target on battlefield
	bool canBeCast(Problem & problem) const override;

	/// Returns false if spell can not be cast at specifid target
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
	std::vector<Destination> getPossibleDestinations(size_t index, AimType aimType, const Target & current) const override final;

	/// Returns true if spell can be cast on unit
	bool isReceptive(const battle::Unit * target) const override;

	/// Returns list of hexes that are affected by spell assuming cast at centralHex
	std::vector<BattleHex> rangeInHexes(BattleHex centralHex) const override;

	const Spell * getSpell() const override;

	bool counteringSelector(const Bonus * bonus) const;

private:
	std::shared_ptr<effects::Effects> effects;
	std::shared_ptr<IReceptiveCheck> targetCondition;

	std::vector<const battle::Unit *> affectedUnits;
	effects::Effects::EffectsToApply effectsToApply;

	void beforeCast(BattleSpellCast & sc, vstd::RNG & rng, const Target & target);

	std::set<const battle::Unit *> collectTargets() const;

	static void doRemoveEffects(ServerCallback * server, const std::vector<const battle::Unit *> & targets, const CSelector & selector);

	std::set<BattleHex> spellRangeInHexes(BattleHex centralHex) const;

	Target transformSpellTarget(const Target & aimPoint) const;
};

}


VCMI_LIB_NAMESPACE_END
