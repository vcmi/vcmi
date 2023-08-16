/*
 * BattleActionProcessor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

struct BattleLogMessage;
struct BattleAttack;
class BattleAction;
struct BattleHex;
class CStack;
enum class BonusType;

namespace battle {
class Unit;
class CUnitState;
}

VCMI_LIB_NAMESPACE_END

class CGameHandler;
class BattleProcessor;

class BattleActionProcessor : boost::noncopyable
{
	using FireShieldInfo = std::vector<std::pair<const CStack *, int64_t>>;

	BattleProcessor * owner;
	CGameHandler * gameHandler;

	int moveStack(int stack, BattleHex dest); //returned value - travelled distance
	void makeAttack(const CStack * attacker, const CStack * defender, int distance, BattleHex targetHex, bool first, bool ranged, bool counter);

	void handleAttackBeforeCasting(bool ranged, const CStack * attacker, const CStack * defender);
	void handleAfterAttackCasting(bool ranged, const CStack * attacker, const CStack * defender);
	void attackCasting(bool ranged, BonusType attackMode, const battle::Unit * attacker, const battle::Unit * defender);

	// damage, drain life & fire shield; returns amount of drained life
	int64_t applyBattleEffects(BattleAttack & bat, std::shared_ptr<battle::CUnitState> attackerState, FireShieldInfo & fireShield, const CStack * def, int distance, bool secondary);

	void sendGenericKilledLog(const CStack * defender, int32_t killed, bool multiple);
	void addGenericKilledLog(BattleLogMessage & blm, const CStack * defender, int32_t killed, bool multiple);

	bool canStackAct(const CStack * stack);

	bool doEmptyAction(const BattleAction & ba);
	bool doEndTacticsAction(const BattleAction & ba);
	bool doRetreatAction(const BattleAction & ba);
	bool doSurrenderAction(const BattleAction & ba);
	bool doHeroSpellAction(const BattleAction & ba);
	bool doWalkAction(const BattleAction & ba);
	bool doWaitAction(const BattleAction & ba);
	bool doDefendAction(const BattleAction & ba);
	bool doAttackAction(const BattleAction & ba);
	bool doShootAction(const BattleAction & ba);
	bool doCatapultAction(const BattleAction & ba);
	bool doUnitSpellAction(const BattleAction & ba);
	bool doBadMoraleAction(const BattleAction & ba);
	bool doHealAction(const BattleAction & ba);

	bool dispatchBattleAction(const BattleAction & ba);

public:
	explicit BattleActionProcessor(BattleProcessor * owner);
	void setGameHandler(CGameHandler * newGameHandler);

	bool makeBattleAction(const BattleAction &ba);

};

