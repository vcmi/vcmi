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
struct BattleStackAttacked;
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

namespace spells
{
class Spell;
}

/// Processes incoming battle action queries and applies requested action(s)
class BattleActionProcessor : boost::noncopyable
{
	struct MovementResult
	{
		/// Number of traversed hexes; undefined for flying units
		int16_t distance;
		/// Movement stopped at an obstacle
		bool obstacleHit;
		/// The destination or request was invalid
		bool invalidRequest;
	};

	BattleProcessor * owner;
	CGameHandler * gameHandler;

	/// ACTION_FINISHED recipients in first-event order. Resolution populates the list because death
	/// can stop an attack and FEROCITY can extend it.
	std::vector<uint32_t> actionParticipants;
	bool actionInProgress = false;

	/// Sentinel shared with BattleStackAttacked::attackerID
	static constexpr uint32_t noUnit = -1;

	/// Collected combat event handler with stable unit IDs and dispatch priority
	struct PendingTrigger
	{
		CombatEventType event;
		uint32_t self;
		uint32_t other; ///< `noUnit` when the event has no unit on the other side
		/// Explicit handler order, independent of ability identifiers and event side
		int priority = 0;
		std::shared_ptr<const Bonus> bonus;
		const ICombatEventScript * script = nullptr; ///< null for a predefined ON_COMBAT_EVENT reaction
	};

	/// Pending UNIT_DEATH event. `battle` disambiguates unit IDs. Triggers are captured before lethal
	/// damage removes spell-effect bonuses.
	struct PendingDeath
	{
		BattleID battle;
		uint32_t unit;
		uint32_t killer;
		uint32_t killed; ///< creatures killed by the lethal hit
		int64_t damage;
		std::vector<PendingTrigger> triggers;
	};

	/// Queued UNIT_DEATH events, dispatched after the current action to prevent nested handlers
	std::vector<PendingDeath> pendingDeaths;

	/// Collects handlers before dispatch so priority ordering applies across units
	void collectEventTriggers(const CBattleInfoCallback & battle, std::vector<PendingTrigger> & pending, CombatEventType event, const battle::Unit * self, const battle::Unit * other);

	/// Dispatches ACTION_FINISHED to all action participants
	void processActionFinishedTriggers(const CBattleInfoCallback & battle, const battle::Unit * actor);

	void runEventTriggers(const CBattleInfoCallback & battle, std::vector<PendingTrigger> & pending, const CombatEventPayload & payload);
	/// Predefined reaction of the ON_COMBAT_EVENT bonus - grant a bonus, or cast a spell
	void runPredefinedReaction(const CBattleInfoCallback & battle, const Bonus & bonus, const battle::Unit * self, const battle::Unit * other);

	/// Dispatches one attack stage to the attacker and targets in priority order
	void processAttackTriggers(const CBattleInfoCallback & battle, CombatEventType attackerEvent, CombatEventType targetEvent, const CStack * attacker, const CStack * defender, const CombatEventPayload & payload);

	/// Properties shared by every stage of one attack
	struct AttackDescriptor
	{
		BattleHex targetHex;
		/// Distance used for charge damage
		int distance = 0;
		/// Zero-based index among attacks by the same side in this action
		int attackIndex = 0;
		/// First attack eligible for legacy spell-casting abilities
		bool first = false;
		bool ranged = false;
		bool counter = false;
	};

	MovementResult moveStack(const CBattleInfoCallback & battle, int stack, BattleHex dest);
	void makeAttack(const CBattleInfoCallback & battle, const CStack * attacker, const CStack * defender, const AttackDescriptor & attack);

	/// Runs one melee attack to completion: first strike, every attacker hit, the retaliation,
	/// and expiry of bonuses that last for the sequence.
	void performAttackSequence(const CBattleInfoCallback & battle, const CStack * attacker, const CStack * defender, const BattleHex & targetHex, int distance, bool longWeaponAttack);

	/// Rolls luck, Death Blow and Ballista double damage
	void rollAttackFlags(const CBattleInfoCallback & battle, const CStack * attacker, BattleAttack & bat) const;
	/// Adds target identity and pre-attack health before damage is rolled
	void describeUpcomingAttack(CombatEventPayload & payload, const CStack * defender, const battle::Units & secondaryTargets) const;
	/// Collects extra-hex and spell-like targets and sets the corresponding attack flags
	battle::Units collectSecondaryTargets(const CBattleInfoCallback & battle, const CStack * attacker, const CStack * defender, const AttackDescriptor & attack, BattleAttack & bat) const;
	/// Associates spell-like hits with their spell for client animation
	void markSpellLikeAttack(const CStack * attacker, BattleAttack & bat) const;

	void handleAfterAttackCasting(const CBattleInfoCallback & battle, const CStack * attacker, const CStack * defender, const CombatEventPayload & payload);
	void attackCasting(const CBattleInfoCallback & battle, bool ranged, BonusType attackMode, const battle::Unit * attacker, const CStack * defender);

	std::set<SpellID> getSpellsForAttackCasting(const TConstBonusListPtr & spells, const CStack *defender);

	/// Applies damage to one target and appends its result to the event payload
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
	void removeBonuses(const CBattleInfoCallback & battle, const battle::Unit * stack, const BonusList & bonuses);

public:
	explicit BattleActionProcessor(BattleProcessor * owner, CGameHandler * newGameHandler);

	void processBattleEventTriggers(const CBattleInfoCallback & battle, CombatEventType event, const battle::Unit * target, const battle::Unit * secondary, const CombatEventPayload & payload = CombatEventPayload());

	/// Dispatches SPELL_HIT to the deliberate-cast targets stored in `unitsBefore`
	void processSpellHitTriggers(const CBattleInfoCallback & battle, const spells::Spell & spell, const battle::Unit * casterUnit, const std::vector<std::shared_ptr<const battle::CUnitState>> & unitsBefore);

	/// Queues UNIT_DEATH events from casualties, including clones
	void noteDeaths(const CBattleInfoCallback & battle, const std::vector<BattleStackAttacked> & casualties);

	/// Dispatches queued UNIT_DEATH events in batches until handlers cause no new deaths
	void flushPendingDeaths(const CBattleInfoCallback & battle);

	/// Discards queued deaths when a battle ends
	void forgetPendingDeaths(const BattleID & battleID);

	bool makeAutomaticBattleAction(const CBattleInfoCallback & battle, const BattleAction & ba);
	bool makePlayerBattleAction(const CBattleInfoCallback & battle, PlayerColor player, const BattleAction & ba);
};
