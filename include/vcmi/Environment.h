/*
 * Environment.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class Services;

class IGameInfoCallback;
class IBattleInfoCallback;
class BattleID;

namespace events
{
	class EventBus;
}

class DLL_LINKAGE Environment
{
public:
	using BattleCb = IBattleInfoCallback;
	using GameCb = IGameInfoCallback;

	virtual ~Environment() = default;

	virtual const Services * services() const = 0;
	virtual const BattleCb * battle(const BattleID & battleID) const = 0;
	virtual const GameCb * game() const = 0;
	virtual vstd::CLoggerBase * logger() const = 0;
	virtual events::EventBus * eventBus() const = 0;
};

VCMI_LIB_NAMESPACE_END
