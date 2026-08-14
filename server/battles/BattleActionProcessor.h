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
#include "combatScripts/CombatEventPayload.h"
#include "json/JsonNode.h"

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
class CUnitState;
}

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

	BattleProcessor * owner;
	CGameHandler * gameHandler;

	/// One reaction to a combat event that is about to run, kept by unit id rather than by pointer
	/// because a reaction running before it may remove either unit from the battle.
	struct PendingTrigger
	{
		std::shared_ptr<Bonus> bonus;
		CombatEventType event;
		int32_t self;
		int32_t other; ///< -1 when the event has no unit on the other side
	};

	void collectEventTriggers(const CBattleInfoCallback & battle, std::vector<PendingTrigger> & pending, CombatEventType event, const battle::Unit * self, const battle::Unit * other) const;
	void runEventTriggers(const CBattleInfoCallback & battle, std::vector<PendingTrigger> & pending, const CombatEventPayload & payload);
	/// Predefined reaction of the ON_COMBAT_EVENT bonus - grant a bonus, or cast a spell
	void runPredefinedReaction(const CBattleInfoCallback & battle, const Bonus & bonus, const battle::Unit * self, const battle::Unit * other);

	MovementResult moveStack(const CBattleInfoCallback & battle, int stack, BattleHex dest); //returned value - travelled distance
	/// attackIndex is the zero-based position of this attack among those its own side makes in this action;
	/// a counterattack is its side's attack 0. `first` instead marks the attack that the legacy
	/// spell-casting abilities fire on, which is not the same thing for a defender striking first.
	void makeAttack(const CBattleInfoCallback & battle, const CStack * attacker, const CStack * defender, int distance, const BattleHex & targetHex, int attackIndex, bool first, bool ranged, bool counter);

	void handleAfterAttackCasting(const CBattleInfoCallback & battle, const CStack * attacker, const CStack * defender, const CombatEventPayload & payload);
	void attackCasting(const CBattleInfoCallback & battle, bool ranged, BonusType attackMode, const battle::Unit * attacker, const CStack * defender);

	std::set<SpellID> getSpellsForAttackCasting(const TConstBonusListPtr & spells, const CStack *defender);

	/// Rolls the damage one attacked unit takes and appends what scripts need to know about it to the payload
	void applyBattleEffects(const CBattleInfoCallback & battle, BattleAttack & bat, std::shared_ptr<battle::CUnitState> attackerState, CombatEventPayload & payload, const CStack * def, int distance, bool secondary) const;

	void sendGenericKilledLog(const CBattleInfoCallback & battle, const CStack * defender, int32_t killed, bool multiple);
	void addGenericKilledLog(BattleLogMessage & blm, const CStack * defender, int32_t killed, bool multiple) const;
	void addGenericDamageLog(BattleLogMessage& blm, const std::shared_ptr<battle::CUnitState> &attackerState, int64_t damageDealt) const;

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
	bool doWalkAndSpellcastAction(const CBattleInfoCallback & battle, const BattleAction & ba);
	bool doShootAction(const CBattleInfoCallback & battle, const BattleAction & ba);
	bool doCatapultAction(const CBattleInfoCallback & battle, const BattleAction & ba);
	bool doUnitSpellAction(const CBattleInfoCallback & battle, const BattleAction & ba);
	bool doHealAction(const CBattleInfoCallback & battle, const BattleAction & ba);

	bool dispatchBattleAction(const CBattleInfoCallback & battle, const BattleAction & ba);
	bool makeBattleActionImpl(const CBattleInfoCallback & battle, const BattleAction & ba);
	void removeBonuses(const CBattleInfoCallback & battle, const CStack * stack, BonusList bonuses);

public:
	explicit BattleActionProcessor(BattleProcessor * owner, CGameHandler * newGameHandler);

	void processBattleEventTriggers(const CBattleInfoCallback & battle, CombatEventType event, const battle::Unit * target, const battle::Unit * secondary, const CombatEventPayload & payload = CombatEventPayload());

	bool makeAutomaticBattleAction(const CBattleInfoCallback & battle, const BattleAction & ba);
	bool makePlayerBattleAction(const CBattleInfoCallback & battle, PlayerColor player, const BattleAction & ba);
};
