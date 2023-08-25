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

VCMI_LIB_NAMESPACE_BEGIN
class CStack;
struct BattleHex;
class BattleAction;
class BattleInfo;
struct CObstacleInstance;
VCMI_LIB_NAMESPACE_END

class CGameHandler;
class BattleProcessor;

/// Controls flow of battles - battle startup actions and switching to next stack or next round after actions
class BattleFlowProcessor : boost::noncopyable
{
	BattleProcessor * owner;
	CGameHandler * gameHandler;

	const CStack * getNextStack(const BattleInfo & battle);

	bool rollGoodMorale(const BattleInfo & battle, const CStack * stack);
	bool tryMakeAutomaticAction(const BattleInfo & battle, const CStack * stack);

	void summonGuardiansHelper(const BattleInfo & battle, std::vector<BattleHex> & output, const BattleHex & targetPosition, ui8 side, bool targetIsTwoHex);
	void trySummonGuardians(const BattleInfo & battle, const CStack * stack);
	void tryPlaceMoats(const BattleInfo & battle);
	void castOpeningSpells(const BattleInfo & battle);
	void activateNextStack(const BattleInfo & battle);
	void startNextRound(const BattleInfo & battle, bool isFirstRound);

	void stackEnchantedTrigger(const BattleInfo & battle, const CStack * stack);
	void removeObstacle(const BattleInfo & battle, const CObstacleInstance & obstacle);
	void stackTurnTrigger(const BattleInfo & battle, const CStack * stack);
	void setActiveStack(const BattleInfo & battle, const CStack * stack);

	void makeStackDoNothing(const BattleInfo & battle, const CStack * next);
	bool makeAutomaticAction(const BattleInfo & battle, const CStack * stack, BattleAction & ba); //used when action is taken by stack without volition of player (eg. unguided catapult attack)

public:
	explicit BattleFlowProcessor(BattleProcessor * owner);
	void setGameHandler(CGameHandler * newGameHandler);

	void onBattleStarted(const BattleInfo & battle);
	void onTacticsEnded(const BattleInfo & battle);
	void onActionMade(const BattleInfo & battle, const BattleAction & ba);
};
