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
#include "bonuses/Bonus.h"

VCMI_LIB_NAMESPACE_BEGIN

struct BattleLogMessage;
struct BattleAttack;
class BattleAction;
class CBattleInfoCallback;
class BattleHex;
class CStack;
class PlayerColor;
enum class BonusType : uint16_t;

namespace battle
{
class Unit;
struct HealInfo;
class CUnitState;
}

VCMI_LIB_NAMESPACE_END

class CGameHandler;
class BattleProcessor;

/// Processes incoming battle action queries and applies requested action(s)
class BattleActionProcessor : boost::noncopyable
{
	struct MovementResult
	{
		/// number of tiles unit moved through, unset for flying units
		int16_t distance;
		/// Unit failed to complete movement due to stepping into obstacle
		bool obstacleHit;
		/// Unit was unable to move to destination, e.g. invalid request
		bool invalidRequest;
	};

	using FireShieldInfo = std::vector<std::pair<const CStack *, int64_t>>;

	BattleProcessor * owner;
	CGameHandler * gameHandler;

	MovementResult moveStack(const CBattleInfoCallback & battle, int stack, BattleHex dest); //returned value - travelled distance
	void makeAttack(const CBattleInfoCallback & battle, const CStack * attacker, const CStack * defender, int distance, const BattleHex & targetHex, bool first, bool ranged, bool counter);

	void handleAttackBeforeCasting(const CBattleInfoCallback & battle, bool ranged, const CStack * attacker, const CStack * defender);

	void handleDeathStare(const CBattleInfoCallback &battle, bool ranged, const CStack *attacker, const CStack *defender);

	void handleAfterAttackCasting(const CBattleInfoCallback & battle, bool ranged, const CStack * attacker, const CStack * defender);
	void attackCasting(const CBattleInfoCallback & battle, bool ranged, BonusType attackMode, const battle::Unit * attacker, const CStack * defender);

	std::set<SpellID> getSpellsForAttackCasting(const TConstBonusListPtr & spells, const CStack *defender);

	// damage, drain life & fire shield; returns amount of drained life
	void applyBattleEffects(const CBattleInfoCallback & battle, BattleAttack & bat, std::shared_ptr<battle::CUnitState> attackerState, FireShieldInfo & fireShield, const CStack * def, battle::HealInfo & healInfo, int distance, bool secondary) const;

	void sendGenericKilledLog(const CBattleInfoCallback & battle, const CStack * defender, int32_t killed, bool multiple);
	void addGenericKilledLog(BattleLogMessage & blm, const CStack * defender, int32_t killed, bool multiple) const;
	void addGenericDamageLog(BattleLogMessage& blm, const std::shared_ptr<battle::CUnitState> &attackerState, int64_t damageDealt) const;
	void addGenericDrainedLifeLog(BattleLogMessage& blm, const std::shared_ptr<battle::CUnitState> &attackerState, const CStack* defender, int64_t drainedLife) const;
	void addGenericResurrectedLog(BattleLogMessage& blm, const std::shared_ptr<battle::CUnitState> &attackerState, const CStack* defender, int64_t resurrected) const;

	bool canStackAct(const CBattleInfoCallback & battle, const CStack * stack);

	bool doEmptyAction(const CBattleInfoCallback & battle, const BattleAction & ba);
	bool doEndTacticsAction(const CBattleInfoCallback & battle, const BattleAction & ba);
	bool doRetreatAction(const CBattleInfoCallback & battle, const BattleAction & ba);
	bool doSurrenderAction(const CBattleInfoCallback & battle, const BattleAction & ba);
	bool doHeroSpellAction(const CBattleInfoCallback & battle, const BattleAction & ba);
	bool doWalkAction(const CBattleInfoCallback & battle, const BattleAction & ba);
	bool doWaitAction(const CBattleInfoCallback & battle, const BattleAction & ba);
	bool doDefendAction(const CBattleInfoCallback & battle, const BattleAction & ba);
	bool doAttackAction(const CBattleInfoCallback & battle, const BattleAction & ba);
	bool doShootAction(const CBattleInfoCallback & battle, const BattleAction & ba);
	bool doCatapultAction(const CBattleInfoCallback & battle, const BattleAction & ba);
	bool doUnitSpellAction(const CBattleInfoCallback & battle, const BattleAction & ba);
	bool doHealAction(const CBattleInfoCallback & battle, const BattleAction & ba);

	bool dispatchBattleAction(const CBattleInfoCallback & battle, const BattleAction & ba);
	bool makeBattleActionImpl(const CBattleInfoCallback & battle, const BattleAction & ba);

public:
	explicit BattleActionProcessor(BattleProcessor * owner, CGameHandler * newGameHandler);

	bool makeAutomaticBattleAction(const CBattleInfoCallback & battle, const BattleAction & ba);
	bool makePlayerBattleAction(const CBattleInfoCallback & battle, PlayerColor player, const BattleAction & ba);
};
