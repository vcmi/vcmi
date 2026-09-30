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

namespace battle
{
class Unit;
class CUnitState;
}

namespace spells
{
class Problem;
}

/// Rules of one kind of battle action: when it is allowed and how it changes the battle. Stateless and shared by all actions of the kind
class DLL_LINKAGE BattleActionType
{
public:
	virtual ~BattleActionType() = default;

	/// Returns the type of the action, or nullptr if the action type is still handled by the legacy code
	static const BattleActionType * find(const BattleAction & action);

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
	/// Checks that clicking the hovered hex makes a valid action. By default validates the action from build
	virtual bool isLegal(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const;
	/// Cursor, text and shaded hexes for the hovered hex, the blocked ones where the option is not legal
	virtual ActionPreview preview(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const;
	/// Action made by clicking the hovered hex
	virtual BattleAction build(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const;

	/// Checks that the side may make the action. Runs on the server after StartAction is applied, and in isLegal
	virtual bool validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const = 0;
	/// Updates the turn state of the acting unit when StartAction is applied. Called only for unit actions outside of the tactics phase
	virtual void applyStartState(battle::CUnitState & actor, const BattleAction & action) const;
	virtual void apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const = 0;

	/// Battle log line that clients show when the action starts, empty if none
	virtual MetaString getStartLogLine(const CBattleInfoCallback & battle, const BattleAction & action) const;

	/// Adds the options of every action type for the unit
	static void collectAllOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out);

	/// Checks that the unit exists, is alive and may act now
	static bool checkUnitCanAct(const CBattleInfoCallback & battle, const battle::Unit * unit, spells::Problem & problem);

protected:
	/// Returns the unit making the action. Throws if the battle has no such unit
	static const battle::Unit & getActor(const CBattleInfoCallback & battle, const BattleAction & action);
};
