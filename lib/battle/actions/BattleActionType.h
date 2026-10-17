/*
 * BattleActionType.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ActionOption.h"

#include "../../texts/MetaString.h"

class BattleAction;
class CBattleInfoCallback;
class IBattleActionEnvironment;
class IGameInfoCallback;
struct StartAction;
enum class EActionType : int8_t;

namespace battle
{
class Unit;
enum class UnitTurnStatus : int8_t;
}

namespace spells
{
class Mechanics;
class Problem;
}

/// Rules of one kind of battle action: when it is allowed and how it changes the battle. Stateless and shared by all actions of the kind
class DLL_LINKAGE BattleActionType
{
public:
	virtual ~BattleActionType() = default;

	/// Returns the type of the action, or nullptr for a value outside of EActionType
	static const BattleActionType * find(const BattleAction & action);
	static const BattleActionType * find(EActionType actionType);

	/// Made by the acting unit on its turn, instead of by the side or its hero
	virtual bool isUnitAction() const;
	/// Allowed during the tactics phase
	virtual bool isTacticsAction() const;
	/// Ends the battle, and is applied without StartAction and EndAction
	virtual bool isBattleEndAction() const;

	/// Adds the options that the type offers to the unit on its turn. None by default
	virtual void collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const;
	/// Order in which the client tries options for a hovered hex, lower first. Target is the unit on the hex, or nullptr
	virtual int getPriority(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const battle::Unit * target) const;
	/// Ways to reach the hovered hex that the player selects between with the cursor position. None if the option needs no approach
	virtual std::vector<ActionApproach> getApproaches(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const BattleHex & hoveredHex) const;
	/// Checks that clicking the hovered hex makes a valid action: build returns an action that passes validate
	bool isLegal(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const;
	/// Cursor, text and shaded hexes for the hovered hex, the blocked ones where the option is not legal
	virtual ActionPreview preview(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const;
	/// Destination that clicking the hovered hex adds to the partial target, if the hex is valid for it and more destinations follow. None by default
	virtual std::optional<battle::Destination> buildTargetStep(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const;
	/// Action made by clicking the hovered hex, or nothing if the option has no action for the hex
	virtual std::optional<BattleAction> build(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const;

	/// Checks that the side may make the action. Runs on the server before StartAction is applied, and in isLegal
	virtual bool validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const = 0;
	/// Turn status of the acting unit once the action starts, or nothing to keep it. Asked only for unit actions outside of the tactics phase
	virtual std::optional<battle::UnitTurnStatus> getStartTurnStatus(const BattleAction & action) const;
	/// Uses up the spellcasting of the acting unit for the turn
	virtual bool usesUnitSpellcast() const;
	virtual void apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const = 0;

	/// Option of the hero to cast the spell
	static ActionOption makeHeroSpellOption(SpellID spell);
	/// Option of the acting unit to cast the spell, with its panel button
	static ActionOption makeUnitSpellOption(SpellID spell);
	/// StartAction pack of the action, with the changes it makes to the turn state of the acting unit
	static StartAction makeStartAction(const CBattleInfoCallback & battle, const BattleAction & action);
	/// Adds the options of every action type for the unit
	static void collectAllOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out);

	/// Checks that the unit exists, is alive and may act now
	static bool checkUnitCanAct(const CBattleInfoCallback & battle, const battle::Unit * unit, spells::Problem & problem);

protected:
	/// Returns the unit making the action. Throws if the battle has no such unit
	static const battle::Unit & getActor(const CBattleInfoCallback & battle, const BattleAction & action);
	/// Unit named by the destination, or the living unit on its hex; nullptr if there is none
	static const battle::Unit * getDestinationUnit(const CBattleInfoCallback & battle, const battle::Destination & destination);
	/// Approaches from every direction to a hex that the actor can stop at to act on the hovered hex
	static std::vector<ActionApproach> getReachableApproaches(const CBattleInfoCallback & battle, const battle::Unit & actor, const BattleHex & hoveredHex, bool longWeapon);
	/// Adds the area of the spell aimed at the hovered hex to the shaded hexes, and the units it affects to the affected units
	static void addSpellArea(ActionPreview & preview, const spells::Mechanics & mechanics, const BattleHex & hoveredHex);
};
