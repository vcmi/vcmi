/*
 * BattleLayout.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BattleHex.h"
#include "BattleSide.h"
#include "../constants/NumericConstants.h"
#include "../constants/Enumerations.h"

VCMI_LIB_NAMESPACE_BEGIN

class CArmedInstance;
class IGameInfoCallback;

struct DLL_EXPORT BattleLayout
{
	using UnitsArrayType = BattleSideArray<std::array<BattleHex, GameConstants::ARMY_SIZE>>;
	using MachinesArrayType = BattleSideArray<std::array<BattleHex, 4>>;
	using CommanderArrayType = BattleSideArray<BattleHex>;

	UnitsArrayType units;
	MachinesArrayType warMachines;
	CommanderArrayType commanders;

	bool tacticsAllowed = false;
	bool obstaclesAllowed = false;

	static BattleLayout createDefaultLayout(const IGameInfoCallback & gameInfo, const CArmedInstance * attacker, const CArmedInstance * defender);
	static BattleLayout createLayout(const IGameInfoCallback & gameInfo, const std::string & layoutName, const CArmedInstance * attacker, const CArmedInstance * defender);
};

VCMI_LIB_NAMESPACE_END
