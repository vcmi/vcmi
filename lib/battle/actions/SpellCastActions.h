/*
 * SpellCastActions.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BattleActionType.h"

namespace spells
{
class Caster;
class Mechanics;
enum class Mode;
}

/// Base of the spell cast types: the caster casts the spell of the action at its target.
/// The player chooses the destinations of the target one by one, as the mechanics of the spell describe them
class SpellCastAction : public BattleActionType
{
public:
	std::optional<battle::Destination> buildTargetStep(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const override;
	ActionPreview preview(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const override;
	std::optional<BattleAction> build(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const override;

	bool validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const override;

protected:
	virtual spells::Mode getMode() const = 0;
	/// Returns the hero of the side or the acting unit, nullptr if there is none
	virtual const spells::Caster * getCaster(const CBattleInfoCallback & battle, BattleSide side, const battle::Unit * actor) const = 0;
	/// Checks that the caster exists and may cast the spell now, apart from the rules of the spell
	virtual bool validateCaster(const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const = 0;
	virtual BattleAction makeAction(const ActionOption & option, const ActionContext & context, const battle::Target & target) const = 0;

	/// Mechanics of the option's spell for the caster of the context, nullptr if there is no caster
	std::unique_ptr<spells::Mechanics> getMechanics(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const;
};

/// HERO_SPELL: the hero of the side casts a spell, which doesn't end the turn of the active unit
class HeroSpellAction : public SpellCastAction
{
public:
	void apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const override;

protected:
	spells::Mode getMode() const override;
	const spells::Caster * getCaster(const CBattleInfoCallback & battle, BattleSide side, const battle::Unit * actor) const override;
	bool validateCaster(const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const override;
	BattleAction makeAction(const ActionOption & option, const ActionContext & context, const battle::Target & target) const override;
};

/// MONSTER_SPELL: a SPELLCASTER unit casts one of its spells
class UnitSpellAction : public SpellCastAction
{
public:
	bool isUnitAction() const override;

	/// Option of the unit to cast the spell, with its panel button
	ActionOption makeOption(SpellID spell) const;
	void collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const override;
	int getPriority(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const battle::Unit * target) const override;

	std::optional<battle::UnitTurnStatus> getStartTurnStatus(const BattleAction & action) const override;
	bool usesUnitSpellcast() const override;
	void apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const override;

protected:
	spells::Mode getMode() const override;
	const spells::Caster * getCaster(const CBattleInfoCallback & battle, BattleSide side, const battle::Unit * actor) const override;
	bool validateCaster(const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const override;
	BattleAction makeAction(const ActionOption & option, const ActionContext & context, const battle::Target & target) const override;
};

/// RANDOM_SPELL: a RANDOM_SPELLCASTER unit casts a spell on another unit of its side. The server rolls the spell out of the beneficial ones
class RandomSpellAction : public BattleActionType
{
public:
	bool isUnitAction() const override;

	void collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const override;
	int getPriority(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const battle::Unit * target) const override;
	ActionPreview preview(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const override;
	std::optional<BattleAction> build(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const override;

	bool validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const override;
	bool usesUnitSpellcast() const override;
	void apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const override;
};

/// WALK_AND_CAST: an ADJACENT_SPELLCASTER unit moves next to a unit and casts the spell of the ability on it.
/// The action targets the hex to cast from, followed by the unit
class WalkAndCastAction : public BattleActionType
{
public:
	bool isUnitAction() const override;

	void collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const override;
	int getPriority(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const battle::Unit * target) const override;
	std::vector<ActionApproach> getApproaches(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const BattleHex & hoveredHex) const override;
	ActionPreview preview(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const override;
	std::optional<BattleAction> build(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const override;

	bool validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const override;
	void apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const override;
};
