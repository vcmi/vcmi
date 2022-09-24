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

VCMI_LIB_NAMESPACE_BEGIN

namespace vstd
{
	class RNG;
}

struct CPackForClient;
struct BattleLogMessage;
struct BattleStackMoved;
struct BattleUnitsChanged;
struct SetStackEffect;
struct StacksInjured;
struct BattleObstaclesChanged;
struct CatapultAttack;

class DLL_LINKAGE ServerCallback
{
public:
	virtual ~ServerCallback() = default;

	virtual void complain(const std::string & problem) = 0;
	virtual bool describeChanges() const = 0;

	virtual vstd::RNG * getRNG() = 0;

	virtual void apply(CPackForClient * pack) = 0;

	virtual void apply(BattleLogMessage * pack) = 0;
	virtual void apply(BattleStackMoved * pack) = 0;
	virtual void apply(BattleUnitsChanged * pack) = 0;
	virtual void apply(SetStackEffect * pack) = 0;
	virtual void apply(StacksInjured * pack) = 0;
	virtual void apply(BattleObstaclesChanged * pack) = 0;
	virtual void apply(CatapultAttack * pack) = 0;
};

VCMI_LIB_NAMESPACE_END
