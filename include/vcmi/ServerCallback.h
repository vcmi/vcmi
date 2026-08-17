/*
 * ServerCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "scripting/ApiTags.h"

namespace vstd
{
	class RNG;
}

namespace battle
{
	class Unit;
}

class IBattleInfoCallback;

struct CPackForClient;
struct BattleLogMessage;
struct BattleStackMoved;
struct BattleUnitsChanged;
struct SetStackEffect;
struct StacksInjured;
struct BattleObstaclesChanged;
struct CatapultAttack;

class DLL_LINKAGE ServerCallback : public scripting::ApiRawPointer<ServerCallback>
{
public:
	virtual ~ServerCallback() = default;

	virtual void complain(const std::string & problem) = 0;
	virtual bool describeChanges() const = 0;

	virtual vstd::RNG * getRNG() = 0;

	/// Rolls a chance-based combat ability of the given unit
	virtual bool rollCombatAbility(const IBattleInfoCallback & battle, const battle::Unit & actor, int percentageChance) = 0;

	virtual void apply(CPackForClient & pack) = 0;

	virtual void apply(BattleLogMessage & pack) = 0;
	virtual void apply(BattleStackMoved & pack) = 0;
	virtual void apply(BattleUnitsChanged & pack) = 0;
	virtual void apply(SetStackEffect & pack) = 0;
	virtual void apply(StacksInjured & pack) = 0;
	virtual void apply(BattleObstaclesChanged & pack) = 0;
	virtual void apply(CatapultAttack & pack) = 0;
};
