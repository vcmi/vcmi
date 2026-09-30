/*
 * NoTargetActions.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BattleActionType.h"

/// NO_ACTION: the unit ends its turn without acting
class DLL_LINKAGE SkipTurnAction : public BattleActionType
{
public:
	bool isUnitAction() const override;
	bool validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const override;
	void apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const override;
};

/// BAD_MORALE: the unit loses its turn to bad morale
class DLL_LINKAGE BadMoraleAction : public SkipTurnAction
{
public:
	MetaString getStartLogLine(const CBattleInfoCallback & battle, const BattleAction & action) const override;
};

class DLL_LINKAGE WaitAction : public BattleActionType
{
public:
	bool isUnitAction() const override;
	bool validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const override;
	void applyStartState(battle::CUnitState & actor, const BattleAction & action) const override;
	void apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const override;
	MetaString getStartLogLine(const CBattleInfoCallback & battle, const BattleAction & action) const override;
};

class DLL_LINKAGE DefendAction : public BattleActionType
{
public:
	bool isUnitAction() const override;
	bool validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const override;
	void applyStartState(battle::CUnitState & actor, const BattleAction & action) const override;
	void apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const override;
};

class DLL_LINKAGE EndTacticsAction : public BattleActionType
{
public:
	bool isTacticsAction() const override;
	bool validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const override;
	void apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const override;
};

class DLL_LINKAGE RetreatAction : public BattleActionType
{
public:
	bool isTacticsAction() const override;
	bool isBattleEndAction() const override;
	bool validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const override;
	void apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const override;
};

class DLL_LINKAGE SurrenderAction : public BattleActionType
{
public:
	bool isTacticsAction() const override;
	bool isBattleEndAction() const override;
	bool validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const override;
	void apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const override;
};
