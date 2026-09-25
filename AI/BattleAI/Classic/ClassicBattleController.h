/*
 * ClassicBattleController.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../../lib/battle/AutocombatPreferences.h"
#include "../../../lib/battle/BattleHex.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class BattleAction;
class CBattleCallback;
class CStack;
class Environment;
class IClassicBattleAIRng;
class ClassicDecisionTrace;

enum class BattleSide : int8_t;

/// Runs the original Heroes III tactical decision pipeline over VCMI battle state.
class ClassicBattleController
{
	std::shared_ptr<Environment> env;
	std::shared_ptr<CBattleCallback> cb;
	PlayerColor playerID;
	BattleSide side;
	std::shared_ptr<IClassicBattleAIRng> randomGenerator;
	std::shared_ptr<ClassicDecisionTrace> trace;
	BattleID currentBattleID = BattleID::NONE;
	std::vector<uint32_t> tacticsStackIDs;
	size_t tacticsCursor = 0;
	uint32_t movingTacticsStack = std::numeric_limits<uint32_t>::max();

	void advanceTactics();

public:
	ClassicBattleController(
		std::shared_ptr<Environment> env,
		std::shared_ptr<CBattleCallback> cb,
		PlayerColor playerID,
		std::shared_ptr<IClassicBattleAIRng> randomGenerator,
		std::shared_ptr<ClassicDecisionTrace> trace
	);

	void battleStart(const BattleID & battleID, BattleSide side);
	void battleEnd(const BattleID & battleID);
	BattleAction decideStackAction(
		const BattleID & battleID,
		const CStack * stack,
		const AutocombatPreferences & preferences);
	void activeStack(const BattleID & battleID, const CStack * stack, const AutocombatPreferences & preferences);
	void yourTacticPhase(const BattleID & battleID, int distance, const AutocombatPreferences & preferences);
	void actionFinished(const BattleID & battleID, const BattleAction & action);
};
