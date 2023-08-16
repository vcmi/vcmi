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
struct CObstacleInstance;
VCMI_LIB_NAMESPACE_END

class CGameHandler;
class BattleProcessor;

class BattleFlowProcessor : boost::noncopyable
{
	BattleProcessor * owner;
	CGameHandler * gameHandler;

	const CStack * getNextStack();

	bool rollGoodMorale(const CStack * stack);
	bool tryMakeAutomaticAction(const CStack * stack);

	void summonGuardiansHelper(std::vector<BattleHex> & output, const BattleHex & targetPosition, ui8 side, bool targetIsTwoHex);
	void activateNextStack();
	void startNextRound(bool isFirstRound);

	void stackEnchantedTrigger(const CStack * stack);
	void removeObstacle(const CObstacleInstance &obstacle);
	void stackTurnTrigger(const CStack *stack);

	void makeStackDoNothing(const CStack * next);
	bool makeAutomaticAction(const CStack *stack, BattleAction &ba); //used when action is taken by stack without volition of player (eg. unguided catapult attack)

public:
	explicit BattleFlowProcessor(BattleProcessor * owner);
	void setGameHandler(CGameHandler * newGameHandler);

	void onBattleStarted();
	void onTacticsEnded();
	void onActionMade(const BattleAction &ba);
};
