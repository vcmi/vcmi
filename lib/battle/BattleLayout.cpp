/*
 * BattleLayout.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleLayout.h"

#include "../GameSettings.h"
#include "../GameLibrary.h"
#include "../callback/IGameInfoCallback.h"
#include "../json/JsonNode.h"
#include "../mapObjects/CArmedInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

BattleLayout BattleLayout::createDefaultLayout(const IGameInfoCallback & gameInfo, const CArmedInstance * attacker, const CArmedInstance * defender)
{
	return createLayout(gameInfo, "default", attacker, defender);
}

BattleLayout BattleLayout::createLayout(const IGameInfoCallback & gameInfo, const std::string & layoutName, const CArmedInstance * attacker, const CArmedInstance * defender)
{
	const auto & loadHex = [](const JsonNode & node)
	{
		if (node.isNull())
			return BattleHex();
		else
			return BattleHex(node.Integer());
	};

	const auto & loadUnits = [](const JsonNode & node)
	{
		UnitsArrayType::value_type result;
		for (size_t i = 0; i < GameConstants::ARMY_SIZE; ++i)
		{
			if (!node[i].isNull())
				result[i] = BattleHex(node[i].Integer());
		}
		return result;
	};

	const JsonNode & configRoot = gameInfo.getSettings().getValue(EGameSettings::COMBAT_LAYOUTS);
	const JsonNode & config = configRoot[layoutName];

	BattleLayout result;

	result.commanders[BattleSide::ATTACKER] = loadHex(config["attackerCommander"]);
	result.commanders[BattleSide::DEFENDER] = loadHex(config["defenderCommander"]);

	for (size_t i = 0; i < 4; ++i)
		result.warMachines[BattleSide::ATTACKER][i] = loadHex(config["attackerWarMachines"][i]);

	for (size_t i = 0; i < 4; ++i)
		result.warMachines[BattleSide::DEFENDER][i] = loadHex(config["defenderWarMachines"][i]);

	if (attacker->formation == EArmyFormation::LOOSE && !config["attackerUnitsLoose"].isNull())
		result.units[BattleSide::ATTACKER] = loadUnits(config["attackerUnitsLoose"][attacker->stacksCount() - 1]);
	else if (attacker->formation == EArmyFormation::TIGHT && !config["attackerUnitsTight"].isNull())
		result.units[BattleSide::ATTACKER] = loadUnits(config["attackerUnitsTight"][attacker->stacksCount() - 1]);
	else
		result.units[BattleSide::ATTACKER] = loadUnits(config["attackerUnits"]);

	if (defender->formation == EArmyFormation::LOOSE && !config["defenderUnitsLoose"].isNull())
		result.units[BattleSide::DEFENDER] = loadUnits(config["defenderUnitsLoose"][defender->stacksCount() - 1]);
	else if (defender->formation == EArmyFormation::TIGHT && !config["defenderUnitsTight"].isNull())
		result.units[BattleSide::DEFENDER] = loadUnits(config["defenderUnitsTight"][defender->stacksCount() - 1]);
	else
		result.units[BattleSide::DEFENDER] = loadUnits(config["defenderUnits"]);

	result.obstaclesAllowed = config["obstaclesAllowed"].Bool();
	result.tacticsAllowed = config["tacticsAllowed"].Bool();

	return result;
}

VCMI_LIB_NAMESPACE_END
