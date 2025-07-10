/*
 * BattleFlowProcessor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/battle/BattleSide.h"
#include "../lib/battle/BattleUnitTurnReason.h"

VCMI_LIB_NAMESPACE_BEGIN
class CStack;
class BattleHex;
class BattleHexArray;
class BattleAction;
class CBattleInfoCallback;
struct CObstacleInstance;
namespace battle
{
class Unit;
}
VCMI_LIB_NAMESPACE_END

class CGameHandler;
class BattleProcessor;

/// Controls flow of battles - battle startup actions and switching to next stack or next round after actions
class BattleFlowProcessor : boost::noncopyable
{
	BattleProcessor * owner;
	CGameHandler * gameHandler;

	const CStack * getNextStack(const CBattleInfoCallback & battle);

	bool rollGoodMorale(const CBattleInfoCallback & battle, const CStack * stack);
	bool tryMakeAutomaticAction(const CBattleInfoCallback & battle, const CStack * stack);
	bool tryActivateMoralePenalty(const CBattleInfoCallback & battle, const CStack * stack);
	bool tryActivateBerserkPenalty(const CBattleInfoCallback & battle, const CStack * stack);
	bool tryAutomaticActionOfWarMachines(const CBattleInfoCallback & battle, const CStack * stack);
	bool tryMakeAutomaticActionOfBallistaOrTowers(const CBattleInfoCallback & battle, const CStack * stack);
	bool tryMakeAutomaticActionOfCatapult(const CBattleInfoCallback & battle, const CStack * stack);
	bool tryMakeAutomaticActionOfFirstAidTent(const CBattleInfoCallback & battle, const CStack * stack);

	void summonGuardiansHelper(const CBattleInfoCallback & battle, BattleHexArray & output, const BattleHex & targetPosition, BattleSide side, bool targetIsTwoHex);
	void trySummonGuardians(const CBattleInfoCallback & battle, const CStack * stack);
	void tryPlaceMoats(const CBattleInfoCallback & battle);
	void castOpeningSpells(const CBattleInfoCallback & battle);
	void activateNextStack(const CBattleInfoCallback & battle);
	void startNextRound(const CBattleInfoCallback & battle, bool isFirstRound);

	void stackEnchantedTrigger(const CBattleInfoCallback & battle, const CStack * stack);
	void removeObstacle(const CBattleInfoCallback & battle, const CObstacleInstance & obstacle);
	void stackTurnTrigger(const CBattleInfoCallback & battle, const CStack * stack);
	void setActiveStack(const CBattleInfoCallback & battle, const battle::Unit * stack, BattleUnitTurnReason reason);

	void makeStackDoNothing(const CBattleInfoCallback & battle, const CStack * next);
	bool makeAutomaticAction(const CBattleInfoCallback & battle, const CStack * stack, const BattleAction & ba); //used when action is taken by stack without volition of player (eg. unguided catapult attack)

public:
	explicit BattleFlowProcessor(BattleProcessor * owner, CGameHandler * newGameHandler);

	void onBattleStarted(const CBattleInfoCallback & battle);
	void onTacticsEnded(const CBattleInfoCallback & battle);
	void onActionMade(const CBattleInfoCallback & battle, const BattleAction & ba);
};
