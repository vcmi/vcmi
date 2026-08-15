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
#include "battle/IBattleInfoCallback.h"
#include "bonuses/Bonus.h"
#include "combatScripts/CombatEventPayload.h"

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
class ICombatEventScript;

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

	/// One reaction to a combat event that is about to run. Which script it is and how it is ordered
	/// are decided when it is collected, so that running it is nothing but a dispatch. Units are kept
	/// by id rather than by pointer because a reaction running before it may remove either from the battle.
	struct PendingTrigger
	{
		CombatEventType event;
		int32_t self;
		int32_t other; ///< -1 when the event has no unit on the other side
		/// Order in which reactions to the same event run. Bonus order is alphabetical by ability
		/// name, which would let a mod decide what runs first by renaming an ability, so scripts that
		/// care declare a priority instead. It orders the attacker's reactions against its victims'.
		int priority = 0;
		std::shared_ptr<const Bonus> bonus;
		const ICombatEventScript * script = nullptr; ///< null for a predefined ON_COMBAT_EVENT reaction
	};

	/// Gathers everything one unit reacts to one event with, without running any of it yet, so that
	/// several units reacting to the same attack can be ordered against each other.
	void collectEventTriggers(const CBattleInfoCallback & battle, std::vector<PendingTrigger> & pending, CombatEventType event, const battle::Unit * self, const battle::Unit * other) const;
	void runEventTriggers(const CBattleInfoCallback & battle, std::vector<PendingTrigger> & pending, const CombatEventPayload & payload);
	/// Predefined reaction of the ON_COMBAT_EVENT bonus - grant a bonus, or cast a spell
	void runPredefinedReaction(const CBattleInfoCallback & battle, const Bonus & bonus, const battle::Unit * self, const battle::Unit * other);

	/// Notifies the attacker and every unit it hits of one point of the attack, as one group ordered
	/// by priority, so that a script runs before or after another whichever side of the attack it
	/// sits on. The attacker reacts to `attackerEvent`, its victims to `targetEvent`.
	void processAttackTriggers(const CBattleInfoCallback & battle, CombatEventType attackerEvent, CombatEventType targetEvent, const CStack * attacker, const CStack * defender, const CombatEventPayload & payload);

	/// Which attack of an action this is, as opposed to who makes it - what every step of one
	/// attack has to agree on.
	struct AttackDescriptor
	{
		BattleHex targetHex;
		/// hexes the attacker travelled before striking, which the damage of a charge scales with
		int distance = 0;
		/// zero-based position of this attack among those its own side makes in this action;
		/// a counterattack is its side's attack 0
		int attackIndex = 0;
		/// the attack that the legacy spell-casting abilities fire on, which is not the same as
		/// attackIndex 0 for a defender striking first
		bool first = false;
		bool ranged = false;
		bool counter = false;
	};

	MovementResult moveStack(const CBattleInfoCallback & battle, int stack, BattleHex dest); //returned value - travelled distance
	void makeAttack(const CBattleInfoCallback & battle, const CStack * attacker, const CStack * defender, const AttackDescriptor & attack);

	/// Rolls what is decided before any damage: luck, and the abilities that double it by chance.
	void rollAttackFlags(const CBattleInfoCallback & battle, const CStack * attacker, BattleAttack & bat) const;
	/// Fills in what a script reacting before the attack gets to see - who is about to be hit and how
	/// much health each of them has left. Nothing about the damage, which has not been rolled yet.
	void describeUpcomingAttack(CombatEventPayload & payload, const CStack * defender, const battle::Units & secondaryTargets) const;
	/// Everyone the attack hits besides its primary target: the extra hexes of a multiple-hex attack
	/// and, on top of those, everything the spell a spell-like attack stands for would have hit.
	/// Also sets the flags of `bat` that say which of the two this attack is.
	battle::Units collectSecondaryTargets(const CBattleInfoCallback & battle, const CStack * attacker, const CStack * defender, const AttackDescriptor & attack, BattleAttack & bat) const;
	/// Marks every hit of a spell-like attack as caused by the spell, so that the client animates it.
	void markSpellLikeAttack(const CStack * attacker, BattleAttack & bat) const;

	void handleAfterAttackCasting(const CBattleInfoCallback & battle, const CStack * attacker, const CStack * defender, const CombatEventPayload & payload);
	void attackCasting(const CBattleInfoCallback & battle, bool ranged, BonusType attackMode, const battle::Unit * attacker, const CStack * defender);

	std::set<SpellID> getSpellsForAttackCasting(const TConstBonusListPtr & spells, const CStack *defender);

	/// Rolls the damage one attacked unit takes and appends what scripts need to know about it to the payload
	void applyBattleEffects(const CBattleInfoCallback & battle, BattleAttack & bat, std::shared_ptr<battle::CUnitState> attackerState, CombatEventPayload & payload, const battle::Unit * def, int distance, bool secondary) const;

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
	void removeBonuses(const CBattleInfoCallback & battle, const battle::Unit * stack, BonusList bonuses);

public:
	explicit BattleActionProcessor(BattleProcessor * owner, CGameHandler * newGameHandler);

	void processBattleEventTriggers(const CBattleInfoCallback & battle, CombatEventType event, const battle::Unit * target, const battle::Unit * secondary, const CombatEventPayload & payload = CombatEventPayload());

	bool makeAutomaticBattleAction(const CBattleInfoCallback & battle, const BattleAction & ba);
	bool makePlayerBattleAction(const CBattleInfoCallback & battle, PlayerColor player, const BattleAction & ba);
};
