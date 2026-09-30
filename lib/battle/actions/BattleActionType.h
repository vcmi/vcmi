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

	/// Checks that the side may make the action. Runs on the server after StartAction is applied
	virtual bool validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const = 0;
	/// Updates the turn state of the acting unit when StartAction is applied. Called only for unit actions outside of the tactics phase
	virtual void applyStartState(battle::CUnitState & actor, const BattleAction & action) const;
	virtual void apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const = 0;

	/// Battle log line that clients show when the action starts, empty if none
	virtual MetaString getStartLogLine(const CBattleInfoCallback & battle, const BattleAction & action) const;

	/// Checks that the unit exists, is alive and may act now
	static bool checkUnitCanAct(const CBattleInfoCallback & battle, const battle::Unit * unit, spells::Problem & problem);
};
