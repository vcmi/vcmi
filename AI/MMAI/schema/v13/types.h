/*
 * types.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <array>
#include <bitset>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "schema/base.h"

namespace MMAI::Schema::V13
{
enum class Encoding : int
{
	/*
	 * Represent `v` as `n` bits, where `bits[1..v+1]=1`.
	 * If `v=-1` (a.k.a. "NULL"), only the bit at index 0 will be `1`.
	 *
	 * Examples:
	 * * `v=3`,  `n=5` => `[0,1,1,1,1]`
	 * * `v=0`,  `n=5` => `[0,1,0,0,0]`
	 * * `v=-1`, `n=5` => `[1,0,0,0,0]`
	 */
	ACCUMULATING_EXPLICIT_NULL,

	/*
	 * Represent `v` as `n` bits, where `bits[0..v]=1`.
	 * If `v=-1` (a.k.a. "NULL"), all bits will be `0`.
	 *
	 * Examples:
	 * * `v=3`,  `n=5` => `[1,1,1,1,0]`
	 * * `v=0`,  `n=5` => `[1,0,0,0,0]`
	 * * `v=-1`, `n=5` => `[0,0,0,0,0]`
	 */
	ACCUMULATING_IMPLICIT_NULL,
	/*
	 * Represent `v` as `n` bits, where `bits[0..v]=1`.
	 * If `v=-1` (a.k.a. "NULL"), all bits will be `-1`.
	 *
	 * Examples:
	 * * `v=3`,  `n=5` => `[1,1,1,1,0]`
	 * * `v=0`,  `n=5` => `[1,0,0,0,0]`
	 * * `v=-1`, `n=5` => `[-1,-1,-1,-1,-1]`
	 */
	ACCUMULATING_MASKING_NULL,

	/*
	 * Represent `v` as `n` bits, where `bits[0..v]=1`.
	 * If `v=-1` (a.k.a. "NULL"), an error will be thrown.
	 *
	 * Examples:
	 * * `v=3`,  `n=5` => `[1,1,1,1,0]`
	 * * `v=0`,  `n=5` => `[1,0,0,0,0]`
	 * * `v=-1`, `n=5` => (error)
	 */
	ACCUMULATING_STRICT_NULL,

	/*
	 * Represent `v` as `n` bits, where `bits[0..v]=1`.
	 * If `v=-1` (a.k.a. "NULL"), it will be treated as `0`.
	 *
	 * Examples:
	 * * `v=3`,  `n=5` => `[1,1,1,1,0]`
	 * * `v=0`,  `n=5` => `[1,0,0,0,0]`
	 * * `v=-1`, `n=5` => `[1,0,0,0,0]`
	 */
	ACCUMULATING_ZERO_NULL,

	/*
	 * Represent `v<<1` as `n`-bit binary (unsigned, LSB at index 0).
	 * If `v=-1` (a.k.a. "NULL"), only the bit at index 0 will be `1`.
	 *
	 * Examples:
	 * * `v=3`,  `n=5` => `[0,1,1,0,0]`
	 * * `v=0`,  `n=5` => `[0,0,0,0,0]`
	 * * `v=-1`, `n=5` => `[1,0,0,0,0]`
	 */
	BINARY_EXPLICIT_NULL,

	/*
	 * Represent `v` as an `n`-bit binary (LSB at index 0)
	 * If `v=-1` (a.k.a. "NULL"), all bits will be `-1`.
	 *
	 * Examples:
	 * * `v=3`,  `n=5` => `[1,1,0,0,0]`
	 * * `v=0`,  `n=5` => `[0,0,0,0,0]`
	 * * `v=-1`, `n=5` => `[-1,-1,-1,-1,-1]`
	 */
	BINARY_MASKING_NULL,

	/*
	 * Represent `v` as an `n`-bit binary (LSB at index 0)
	 * If `v=-1` (a.k.a. "NULL"), an error will be thrown.
	 *
	 * Examples:
	 * * `v=3`,  `n=5` => `[1,1,0,0,0]`
	 * * `v=0`,  `n=5` => `[0,0,0,0,0]`
	 * * `v=-1`, `n=5` => (error)
	 */
	BINARY_STRICT_NULL,

	/*
	 * Represent `v` as `n`-bit binary (unsigned, LSB at index 0).
	 * If `v=-1` (a.k.a. "NULL"), it will be treated as `0`.
	 *
	 * Examples:
	 * * `v=3`,  `n=5` => `[1,1,0,0,0]`
	 * * `v=0`,  `n=5` => `[0,0,0,0,0]`
	 * * `v=-1`, `n=5` => `[0,0,0,0,0]`
	 */
	BINARY_ZERO_NULL,
	// XXX: BINARY_ZERO_NULL obsoletes BINARY_IMPLICIT_NULL

	/*
	 * Represent `v` as `n` bits, where `bits[v+1]=1`.
	 *
	 * Examples:
	 * * `v=3`,  `n=5` => `[0,0,0,0,1]`
	 * * `v=0`,  `n=5` => `[0,1,0,0,0]`
	 * * `v=-1`, `n=5` => `[1,0,0,0,0]`
	 */
	CATEGORICAL_EXPLICIT_NULL,

	/*
	 * Represent `v` as `n` bits, where `bits[v]=1`.
	 * If `v=-1` (a.k.a. "NULL"), all bits will be `0`.
	 *
	 * Examples:
	 * * `v=3`,  `n=5` => `[0,0,0,1,0]`
	 * * `v=0`,  `n=5` => `[1,0,0,0,0]`
	 * * `v=-1`, `n=5` => `[0,0,0,0,0]`
	 */
	CATEGORICAL_IMPLICIT_NULL,

	/*
	 * Represent `v` as `n` bits, where `bits[v]=1`.
	 * If `v=-1` (a.k.a. "NULL"), all bits will be `-1`.
	 *
	 * Examples:
	 * * `v=3`,  `n=5` => `[0,0,0,1,0]`
	 * * `v=0`,  `n=5` => `[1,0,0,0,0]`
	 * * `v=-1`, `n=5` => `[-1,-1,-1,-1,-1]`
	 */
	CATEGORICAL_MASKING_NULL,

	/*
	 * Represent `v` as `n` bits, where `bits[v]=1`.
	 * If `v=-1` (a.k.a. "NULL"), an error will be thrown.
	 *
	 * Examples:
	 * * `v=3`,  `n=5` => `[0,0,0,1,0]`
	 * * `v=0`,  `n=5` => `[1,0,0,0,0]`
	 * * `v=-1`, `n=5` => (error)
	 */
	CATEGORICAL_STRICT_NULL,

	/*
	 * Represent `v` as `n` bits, where `bits[v]=1`.
	 * If `v=-1` (a.k.a. "NULL"), it will be treated as `0`.
	 *
	 * Examples:
	 * * `v=3`,  `n=5` => `[0,0,0,1,0]`
	 * * `v=0`,  `n=5` => `[1,0,0,0,0]`
	 * * `v=-1`, `n=5` => `[1,0,0,0,0]`
	 */
	CATEGORICAL_ZERO_NULL,

	/*
	 * Normalize `v+1` exponentially with base `vmax`
	 * and represent it as `[0, vnorm]`.
	 * If `v=-1` (a.k.a. "NULL"), the result will be `[1, 0]
	 *
	 * Examples:
	 * * `v=3`,  `vmax=10` => `[0, 0.6]`
	 * * `v=0`,  `vmax=10` => `[0, 0]`
	 * * `v=-1`, `vmax=10` => `[1, 0]`
	 */
	EXPNORM_EXPLICIT_NULL,

	/*
	 * Normalize `v+1` exponentially with base `vmax`.
	 * If `v=0`, en error will be thrown.
	 * If `v=-1` (a.k.a. "NULL"), it will not be normalized.
	 *
	 * Examples:
	 * * `v=3`,  `vmax=10` => `0.6`
	 * * `v=0`,  `vmax=10` => `0`
	 * * `v=-1`, `vmax=10` => `-1`
	 */
	EXPNORM_MASKING_NULL,

	/*
	 * Normalize `v+1` exponentially with base `vmax`.
	 * If `v=-1` (a.k.a. "NULL"), an error will be thrown.
	 *
	 * Examples:
	 * * `v=3`,  `vmax=10` => `0.6`
	 * * `v=0`,  `vmax=10` => `0`
	 * * `v=-1`, `vmax=10` => (error)
	 */
	EXPNORM_STRICT_NULL,

	/*
	 * Normalize `v+1` exponentially with base `vmax`.
	 * If `v=0`, en error will be thrown.
	 * If `v=-1` (a.k.a. "NULL"), it will be treated as `0`.
	 *
	 * Examples:
	 * * `v=3`,  `vmax=10` => `0.6`
	 * * `v=0`,  `vmax=10` => `0`
	 * * `v=-1`, `vmax=10` => `0`
	 */
	EXPNORM_ZERO_NULL,
	// XXX: NORMALIZED_ZERO_NULL obsoletes NORMALIZED_IMPLICIT_NULL

	/*
	 * Normalize `v` linearly in the range `(0, vmax)`.
	 * and represent it as `[0, vnorm]`.
	 * If `v=-1` (a.k.a. "NULL"), the result will be `[1, 0]
	 *
	 * Examples:
	 * * `v=3`,  `vmax=10` => `[0, 0.3]`
	 * * `v=0`,  `vmax=10` => `[0, 0]`
	 * * `v=-1`, `vmax=10` => `[1, 0]`
	 */
	LINNORM_EXPLICIT_NULL,

	/*
	 * Normalize `v` linearly in the range `(0, vmax)`.
	 * If `v=-1` (a.k.a. "NULL"), it will not be normalized.
	 *
	 * Examples:
	 * * `v=3`,  `vmax=10` => `0.3`
	 * * `v=0`,  `vmax=10` => `0`
	 * * `v=-1`, `vmax=10` => `-1`
	 */
	LINNORM_MASKING_NULL,

	/*
	 * Normalize `v` linearly in the range `(0, vmax)`.
	 * If `v=-1` (a.k.a. "NULL"), an error will be thrown.
	 *
	 * Examples:
	 * * `v=3`,  `vmax=10` => `0.3`
	 * * `v=0`,  `vmax=10` => `0`
	 * * `v=-1`, `vmax=10` => (error)
	 */
	LINNORM_STRICT_NULL,

	/*
	 * Normalize `v` linearly in the range `(0, vmax)`.
	 * If `v=-1` (a.k.a. "NULL"), it will be treated as `0`.
	 *
	 * Examples:
	 * * `v=3`,  `vmax=10` => `0.6`
	 * * `v=0`,  `vmax=10` => `0`
	 * * `v=-1`, `vmax=10` => `0`
	 */
	LINNORM_ZERO_NULL,
	// XXX: NORMALIZED_ZERO_NULL obsoletes NORMALIZED_IMPLICIT_NULL

	/*
	 * Don't normalize, use as-is.
	 */
	RAW,
};

enum class CombatResult
{
	LEFT_WINS,
	RIGHT_WINS,
	DRAW,
	NONE,

	_count
};

enum class StackActState : int
{
	READY, //   will act this turn, not waited
	WAITING, // will act this turn, already waited
	DONE, //    will not act this turn
	_count
};

enum class HexState : int
{
	// IMPASSABLE, // obstacle/stack/gate(closed,attacker)
	PASSABLE, //      empty/mine/firewall/gate(open)/gate(closed,defender), ...
	STOPPING, //      moat/quicksand
	DAMAGING_L, //    moat/mine/firewall
	DAMAGING_R, //    moat/mine/firewall
	// GATE, //       XXX: redundant? Always set during siege (regardless of gate state)
	_count
};

enum class HexAction : int
{
	AMOVE_TR, //  = Move to (*) + attack at hex 0..11:
	AMOVE_R, //    . . . . . . . . . 5 0 . . . .
	AMOVE_BR, //  . 1-hex:  . . . . 4 * 1 . . .
	AMOVE_BL, //   . . . . . . . . . 3 2 . . . .
	AMOVE_L, //   . . . . . . . . . . . . . . .
	AMOVE_TL, //   . . . . . . . . . 5 0 6 . . .
	AMOVE_2TR, // . 2-hex (R):  . . 4 * # 7 . .
	AMOVE_2R, //   . . . . . . . . . 3 2 8 . . .
	AMOVE_2BR, // . . . . . . . . . . . . . . .
	AMOVE_2BL, //  . . . . . . . .11 5 0 . . . .
	AMOVE_2L, //  . 2-hex (L):  .10 # * 1 . . .
	AMOVE_2TL, //  . . . . . . . . 9 3 2 . . . .
	MOVE, //      = Move to (defend if current hex)
	SHOOT, //     = shoot at
	_count
};

enum class GlobalAction : int
{
	RETREAT,
	WAIT,
	_count
};

enum class GlobalAttribute : int
{
	BATTLE_SIDE, //               0=left, 1=right (does not change during battle)
	BATTLE_SIDE_ACTIVE_PLAYER, // 0=left, 1=right (NA = battle finished)
	BATTLE_WINNER, //             0=left, 1=right (NA = battle not finished)
	BFIELD_VALUE_START_ABS, //    global_value_at_start
	BFIELD_VALUE_NOW_ABS, //      global_value_now
	BFIELD_VALUE_NOW_REL0, //     global_value_now / global_value_at_start
	BFIELD_HP_START_ABS, //       global_hp_at_start
	BFIELD_HP_NOW_ABS, //         global_hp_now
	BFIELD_HP_NOW_REL0, //        global_hp_now / global_hp_at_start
	ACTION_MASK, //               mask for global actions (retreat, wait)

	_count
};

enum class PlayerAttribute : int
{
	BATTLE_SIDE, //           0=left, 1=right
	ARMY_VALUE_NOW_ABS,
	ARMY_VALUE_NOW_REL, //    side_army_value_now / global_value_now
	ARMY_VALUE_NOW_REL0, //   side_army_value_now / global_value_at_start
	ARMY_HP_NOW_ABS,
	ARMY_HP_NOW_REL, //       side_army_hp_now / global_hp_now
	ARMY_HP_NOW_REL0, //      side_army_hp_now / global_hp_at_start
	VALUE_KILLED_NOW_ABS,
	VALUE_KILLED_NOW_REL, //  left_value_killed_this_turn / global_value_last_turn
	VALUE_KILLED_ACC_ABS,
	VALUE_KILLED_ACC_REL0, // left_value_killed_lifetime / global_value_at_start
	VALUE_LOST_NOW_ABS,
	VALUE_LOST_NOW_REL, //    left_value_lost_this_turn / global_value_last_turn
	VALUE_LOST_ACC_ABS,
	VALUE_LOST_ACC_REL0, //   left_value_lost_lifetime / global_value_at_start
	DMG_DEALT_NOW_ABS,
	DMG_DEALT_NOW_REL, //     left_dmg_dealt_this_turn / global_hp_last_turn
	DMG_DEALT_ACC_ABS,
	DMG_DEALT_ACC_REL0, //    left_dmg_dealt_lifetime / global_hp_at_start
	DMG_RECEIVED_NOW_ABS,
	DMG_RECEIVED_NOW_REL, //  left_dmg_taken_this_turn / global_hp_last_turn
	DMG_RECEIVED_ACC_ABS,
	DMG_RECEIVED_ACC_REL0, // left_dmg_taken_lifetime / global_hp_at_start

	_count
};

// For description on each attribute, see the comments for HEX_ENCODING
enum class HexAttribute : int
{
	Y_COORD,
	X_COORD,
	STATE_MASK,
	ACTION_MASK,
	IS_REAR, // is this hex the rear hex of a stack
	STACK_SIDE,
	// STACK_CREATURE_ID,
	STACK_SLOT,
	STACK_QUANTITY,
	STACK_ATTACK,
	STACK_DEFENSE,
	STACK_SHOTS,
	STACK_DMG_MIN,
	STACK_DMG_MAX,
	STACK_HP,
	STACK_HP_LEFT,
	STACK_SPEED,
	STACK_QUEUE,
	// STACK_ESTIMATED_DMG,
	STACK_VALUE_ONE,
	STACK_FLAGS1,
	STACK_FLAGS2,

	STACK_VALUE_REL,
	STACK_VALUE_REL0,
	STACK_VALUE_KILLED_REL,
	STACK_VALUE_KILLED_ACC_REL0,
	STACK_VALUE_LOST_REL,
	STACK_VALUE_LOST_ACC_REL0,
	STACK_DMG_DEALT_REL,
	STACK_DMG_DEALT_ACC_REL0,
	STACK_DMG_RECEIVED_REL,
	STACK_DMG_RECEIVED_ACC_REL0,

	_count
};

enum class StackAttribute : int
{
	SIDE,
	SLOT,
	QUANTITY,
	ATTACK,
	DEFENSE,
	SHOTS,
	DMG_MIN,
	DMG_MAX,
	HP,
	HP_LEFT,
	SPEED,
	QUEUE,
	// ESTIMATED_DMG,
	VALUE_ONE,
	FLAGS1,
	FLAGS2,

	// RELATIVE values
	VALUE_REL, //             stack_value_now / global_value_now
	VALUE_REL0, //            stack_value_now / global_value_at_start
	VALUE_KILLED_REL, //      value_killed_this_turn / global_value_last_turn
	VALUE_KILLED_ACC_REL0, // value_killed_lifetime / global_value_at_start
	VALUE_LOST_REL, //        value_lost_this_turn / global_value_last_turn
	VALUE_LOST_ACC_REL0, //   value_lost_lifetime / global_value_at_start
	DMG_DEALT_REL, //         dmg_dealt_this_turn / global_hp_last_turn
	DMG_DEALT_ACC_REL0, //    dmg_dealt_lifetime / global_hp_at_start
	DMG_RECEIVED_REL, //      dmg_received_this_turn / global_hp_last_turn
	DMG_RECEIVED_ACC_REL0, // dmg_received_lifetime / global_hp_at_start
	_count
};

// flags are split into two, as they can't fit in a single int
enum class StackFlag1 : int
{
	IS_ACTIVE,
	WILL_ACT,
	CAN_WAIT,
	CAN_RETALIATE,
	SLEEPING,
	BLOCKED,
	BLOCKING,
	IS_WIDE,
	FLYING,
	ADDITIONAL_ATTACK,
	NO_MELEE_PENALTY,
	TWO_HEX_ATTACK_BREATH,
	BLOCKS_RETALIATION,
	SHOOTER,
	NON_LIVING,
	WAR_MACHINE,
	FIREBALL,
	DEATH_CLOUD,
	THREE_HEADED_ATTACK,
	ALL_AROUND_ATTACK,
	RETURN_AFTER_STRIKE,
	ENEMY_DEFENCE_REDUCTION,
	LIFE_DRAIN,
	DOUBLE_DAMAGE_CHANCE,
	DEATH_STARE,

	_count
};

enum class StackFlag2 : int
{
	AGE,
	AGE_ATTACK,
	BIND,
	BIND_ATTACK,
	BLIND,
	BLIND_ATTACK,
	CURSE,
	CURSE_ATTACK,
	DISPEL_ATTACK,
	PETRIFY,
	PETRIFY_ATTACK,
	POISON,
	POISON_ATTACK,
	WEAKNESS,
	WEAKNESS_ATTACK,
	_count
};

enum class LinkType : int
{
	// XXX: types are sorted by frequency (desc)

	// ACTION, //          need to link it with v=action (SRC=active stack)
	ADJACENT,
	REACH, //              i.e. "can move to"
	RANGED_MOD, //         v=0.25 / 0.5 / 1
	ACTS_BEFORE, //        v=num of actions (e.g. 2 if waited)
	// BLOCKS, //          adds further attention to units blocking a shooter
	MELEE_DMG_REL, //      v=frac. of DST stack HP
	RETAL_DMG_REL, //      v=frac. of SRC stack HP after hypothetical attack
	RANGED_DMG_REL, //     v=frac. of DST stack HP
	// BLOCKED_BY,
	// REAR_HEX, //        DST=rear hex of wide unit
	_count
};

enum class ErrorCode : int
{
	OK,
	ALREADY_WAITED,
	MOVE_SELF,
	HEX_UNREACHABLE,
	HEX_BLOCKED,
	HEX_MELEE_NA,
	STACK_NA,
	STACK_DEAD,
	STACK_INVALID,
	CANNOT_SHOOT,
	FRIENDLY_FIRE,
	INVALID_DIR,
};

class IGlobalStats
{
public:
	virtual int getAttr(GlobalAttribute) const = 0;
	virtual ~IGlobalStats() = default;
};

class IPlayerStats
{
public:
	virtual int getAttr(PlayerAttribute) const = 0;
	virtual ~IPlayerStats() = default;
};

using GlobalAttrs = std::array<int, static_cast<int>(GlobalAttribute::_count)>;
using PlayerAttrs = std::array<int, static_cast<int>(PlayerAttribute::_count)>;
using HexAttrs = std::array<int, static_cast<int>(HexAttribute::_count)>;
using StackAttrs = std::array<int, static_cast<int>(StackAttribute::_count)>;
using StackFlags1 = std::bitset<EI(StackFlag1::_count)>;
using StackFlags2 = std::bitset<EI(StackFlag2::_count)>;

class IStack
{
public:
	virtual const StackAttrs & getAttrs() const = 0;
	virtual int getAttr(StackAttribute) const = 0;
	virtual int getFlag(StackFlag1) const = 0;
	virtual int getFlag(StackFlag2) const = 0;
	virtual char getAlias() const = 0;
	virtual ~IStack() = default;
};

class IHex
{
public:
	virtual const HexAttrs & getAttrs() const = 0;
	virtual int getID() const = 0;
	virtual int getAttr(HexAttribute) const = 0;
	virtual const IStack * getStack() const = 0;
	virtual ~IHex() = default;
};

class IAttackLog
{
public:
	// NOTE: each of those can be nullptr if cstack was just resurrected/summoned
	virtual IStack * getAttacker() const = 0;
	virtual IStack * getDefender() const = 0;

	virtual int getDamageDealt() const = 0;
	virtual int getDamageDealtPermille() const = 0;
	virtual int getUnitsKilled() const = 0;
	virtual int getValueKilled() const = 0;
	virtual int getValueKilledPermille() const = 0;
	virtual ~IAttackLog() = default;
};

class ILinks
{
public:
	virtual std::vector<int64_t> getSrcIndex() const = 0;
	virtual std::vector<int64_t> getDstIndex() const = 0;
	virtual std::vector<float> getAttributes() const = 0;
	virtual ~ILinks() = default;
};

using AttackLogs = std::vector<IAttackLog *>;
using Stacks = std::vector<IStack *>;
using Hexes = std::array<std::array<IHex *, 15>, 11>;
using AllLinks = std::map<LinkType, ILinks *>;

using StateTransition = std::tuple<Action, ActionMask *, BattlefieldState *>;
using StateTransitions = std::vector<StateTransition>;

// This is returned as std::any by IState
// => MMAI_DLL_LINKAGE is needed to ensure std::any_cast sees the same symbol
class MMAI_DLL_LINKAGE ISupplementaryData
{
public:
	enum class Type : int
	{
		REGULAR,
		ANSI_RENDER
	};

	virtual Type getType() const = 0;
	virtual Side getSide() const = 0;
	virtual std::string getColor() const = 0;
	virtual ErrorCode getErrorCode() const = 0;
	virtual bool getIsBattleEnded() const = 0;
	virtual bool getIsVictorious() const = 0;
	virtual const IGlobalStats * getGlobalStats() const = 0;
	virtual const IPlayerStats * getLeftPlayerStats() const = 0;
	virtual const IPlayerStats * getRightPlayerStats() const = 0;
	virtual Stacks getStacks() const = 0;
	virtual Hexes getHexes() const = 0;
	virtual AllLinks getAllLinks() const = 0;
	virtual AttackLogs getAttackLogs() const = 0;
	virtual std::string getAnsiRender() const = 0;
	virtual StateTransitions getStateTransitions() const = 0;
	virtual ~ISupplementaryData() = default;
};
}
