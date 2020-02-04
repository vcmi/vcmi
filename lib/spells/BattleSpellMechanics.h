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

struct BattleSpellCast;

namespace spells
{

class BattleSpellMechanics : public BaseMechanics
{
public:
	BattleSpellMechanics(const IBattleCast * event, std::shared_ptr<effects::Effects> effects_, std::shared_ptr<IReceptiveCheck> targetCondition_);
	virtual ~BattleSpellMechanics();

	void applyEffects(BattleStateProxy * battleState, vstd::RNG & rng, const Target & targets, bool indirect, bool ignoreImmunity) const override;

	bool canBeCast(Problem & problem) const override;
	bool canBeCastAt(const Target & target) const override;

	void cast(const PacketSender * server, vstd::RNG & rng, const Target & target) override final;
	void cast(IBattleState * battleState, vstd::RNG & rng, const Target & target) override final;

	std::vector<const CStack *> getAffectedStacks(const Target & target) const override final;

	std::vector<AimType> getTargetTypes() const override final;
	std::vector<Destination> getPossibleDestinations(size_t index, AimType aimType, const Target & current) const override final;

	bool isReceptive(const battle::Unit * target) const override;

	std::vector<BattleHex> rangeInHexes(BattleHex centralHex, bool * outDroppedHexes = nullptr) const override;

	bool counteringSelector(const Bonus * bonus) const;

private:
	std::shared_ptr<effects::Effects> effects;
	std::shared_ptr<IReceptiveCheck> targetCondition;

	std::vector<const battle::Unit *> affectedUnits;
	effects::Effects::EffectsToApply effectsToApply;

	void beforeCast(BattleSpellCast & sc, vstd::RNG & rng, const Target & target);

	void addCustomEffect(BattleSpellCast & sc, const battle::Unit * target, ui32 effect);
	void addCustomEffect(BattleSpellCast & sc, ui32 targetId, ui32 effect);

	std::set<const battle::Unit *> collectTargets() const;

	static void doRemoveEffects(const PacketSender * server, const std::vector<const battle::Unit *> & targets, const CSelector & selector);

	std::set<BattleHex> spellRangeInHexes(BattleHex centralHex) const;

	Target transformSpellTarget(const Target & aimPoint) const;
};

}

