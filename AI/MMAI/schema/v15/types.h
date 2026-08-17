/*
 * types.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "encoding.h"
#include "graph.h"
#include "schema/base.h"

namespace MMAI::Schema::V15
{

enum class CombatResult : uint8_t
{
	LEFT_WINS,
	RIGHT_WINS,
	DRAW,
	NONE,

	_count
};

enum class WallHP : uint8_t
{
	HP0, // destroyed or not a wall
	HP1,
	HP2,
	HP3,

	_count
};

enum class ActionType : uint8_t
{
	// RETREAT,  // prevent retreats for now
	WAIT,
	DEFEND,
	MOVE,
	AMOVE,
	SHOOT,
	_count
};

class IAttackLog
{
public:
	virtual int getDamageDealt() const = 0;
	virtual int getDamageDealtPermille() const = 0;
	virtual int getUnitsKilled() const = 0;
	virtual int getValueKilled() const = 0;
	virtual int getValueKilledPermille() const = 0;
	virtual ~IAttackLog() = default;
};

using AttackLogs = std::vector<const IAttackLog *>;

// This is returned as std::any by IState
// => MMAI_DLL_LINKAGE is needed to ensure std::any_cast sees the same symbol
class MMAI_DLL_LINKAGE ISupplementaryData
{
public:
	enum class Type : uint8_t
	{
		REGULAR,
		ANSI_RENDER
	};

	virtual Type getType() const = 0;
	virtual const Graph::IGraph * getGraph() const = 0;
	virtual AttackLogs getAttackLogs() const = 0;
	virtual std::string getAnsiRender() const = 0;
	virtual ~ISupplementaryData() = default;
};
}
