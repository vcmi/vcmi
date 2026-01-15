/*
 * hex.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "vcmi/spells/Service.h"
#include "vcmi/spells/Spell.h"

#include "BAI/v13/hex.h"
#include "common.h"
#include "schema/v13/constants.h"

namespace MMAI::BAI::V13
{
namespace S13 = Schema::V13;

using HA = Schema::V13::HexAttribute;
using HS = Schema::V13::HexState;
using SA = Schema::V13::StackAttribute;

constexpr HexStateMask S_PASSABLE = 1 << EI(HexState::PASSABLE);
constexpr HexStateMask S_STOPPING = 1 << EI(HexState::STOPPING);
constexpr HexStateMask S_DAMAGING_L = 1 << EI(HexState::DAMAGING_L);
constexpr HexStateMask S_DAMAGING_R = 1 << EI(HexState::DAMAGING_R);
constexpr HexStateMask S_DAMAGING_ALL = 1 << EI(HexState::DAMAGING_L) | 1 << EI(HexState::DAMAGING_R);

// static
int Hex::CalcId(const BattleHex & bh)
{
	ASSERT(bh.isAvailable(), "Hex unavailable: " + std::to_string(bh.toInt()));
	return bh.getX() - 1 + (bh.getY() * 15);
}

// static
std::pair<int, int> Hex::CalcXY(const BattleHex & bh)
{
	return {bh.getX() - 1, bh.getY()};
}

//
// Return bh's neighbouring hexes for setting action mask
//
// return nearby hexes for "X":
//
//  . . . . . . . . . .
// . . .11 5 0 6 . . .
//  . .10 4 X 1 7 . . .
// . . . 9 3 2 8 . . .
//  . . . . . . . . . .
//
// NOTE:
// The index of each hex in the returned array corresponds to a
// the respective AMOVE_* HexAction w.r.t. "X" (see hexaction.h)
//
// static
HexActionHex Hex::NearbyBattleHexes(const BattleHex & bh)
{
	static_assert(EI(HexAction::AMOVE_TR) == 0);
	static_assert(EI(HexAction::AMOVE_R) == 1);
	static_assert(EI(HexAction::AMOVE_BR) == 2);
	static_assert(EI(HexAction::AMOVE_BL) == 3);
	static_assert(EI(HexAction::AMOVE_L) == 4);
	static_assert(EI(HexAction::AMOVE_TL) == 5);
	static_assert(EI(HexAction::AMOVE_2TR) == 6);
	static_assert(EI(HexAction::AMOVE_2R) == 7);
	static_assert(EI(HexAction::AMOVE_2BR) == 8);
	static_assert(EI(HexAction::AMOVE_2BL) == 9);
	static_assert(EI(HexAction::AMOVE_2L) == 10);
	static_assert(EI(HexAction::AMOVE_2TL) == 11);

	auto nbhR = bh.cloneInDirection(BattleHex::EDir::RIGHT, false);
	auto nbhL = bh.cloneInDirection(BattleHex::EDir::LEFT, false);

	return HexActionHex{
		// The 6 basic directions
		bh.cloneInDirection(BattleHex::EDir::TOP_RIGHT, false),
		nbhR,
		bh.cloneInDirection(BattleHex::EDir::BOTTOM_RIGHT, false),
		bh.cloneInDirection(BattleHex::EDir::BOTTOM_LEFT, false),
		nbhL,
		bh.cloneInDirection(BattleHex::EDir::TOP_LEFT, false),

		// Extended directions for R-side wide creatures
		nbhR.cloneInDirection(BattleHex::EDir::TOP_RIGHT, false),
		nbhR.cloneInDirection(BattleHex::EDir::RIGHT, false),
		nbhR.cloneInDirection(BattleHex::EDir::BOTTOM_RIGHT, false),

		// Extended directions for L-side wide creatures
		nbhL.cloneInDirection(BattleHex::EDir::BOTTOM_LEFT, false),
		nbhL.cloneInDirection(BattleHex::EDir::LEFT, false),
		nbhL.cloneInDirection(BattleHex::EDir::TOP_LEFT, false)
	};
}

Hex::Hex(
	const BattleHex & bhex_,
	const EAccessibility accessibility,
	const EGateState gatestate,
	const std::vector<std::shared_ptr<const CObstacleInstance>> & obstacles,
	const std::map<BattleHex, std::shared_ptr<Stack>> & hexstacks,
	const std::shared_ptr<ActiveStackInfo> & astackinfo
)
	: bhex(bhex_), id(CalcId(bhex_))
{
	attrs.fill(S13::NULL_VALUE_UNENCODED);

	auto [x, y] = CalcXY(bhex);
	auto it = hexstacks.find(bhex);
	stack = it == hexstacks.end() ? nullptr : it->second;

	setattr(HA::Y_COORD, y);
	setattr(HA::X_COORD, x);

	// This is never N/A => set separately (not within the if below)
	setattr(HA::IS_REAR, stack && bhex == stack->cstack->occupiedHex());

	static_assert(EI(SA::_count) == 25, "whistleblower in case attributes change");

	auto attrmap = std::map<HA, SA>{
		{HA::STACK_SIDE,                  SA::SIDE                 },
		{HA::STACK_SLOT,                  SA::SLOT                 },
		{HA::STACK_QUANTITY,              SA::QUANTITY             },
		{HA::STACK_ATTACK,                SA::ATTACK               },
		{HA::STACK_DEFENSE,               SA::DEFENSE              },
		{HA::STACK_SHOTS,                 SA::SHOTS                },
		{HA::STACK_DMG_MIN,               SA::DMG_MIN              },
		{HA::STACK_DMG_MAX,               SA::DMG_MAX              },
		{HA::STACK_HP,                    SA::HP                   },
		{HA::STACK_HP_LEFT,               SA::HP_LEFT              },
		{HA::STACK_SPEED,                 SA::SPEED                },
		{HA::STACK_QUEUE,                 SA::QUEUE                },
		{HA::STACK_VALUE_ONE,             SA::VALUE_ONE            },
		{HA::STACK_FLAGS1,                SA::FLAGS1               },
		{HA::STACK_FLAGS2,                SA::FLAGS2               },

		{HA::STACK_VALUE_REL,             SA::VALUE_REL            },
		{HA::STACK_VALUE_REL0,            SA::VALUE_REL0           },
		{HA::STACK_VALUE_KILLED_REL,      SA::VALUE_KILLED_REL     },
		{HA::STACK_VALUE_KILLED_ACC_REL0, SA::VALUE_KILLED_ACC_REL0},
		{HA::STACK_VALUE_LOST_REL,        SA::VALUE_LOST_REL       },
		{HA::STACK_VALUE_LOST_ACC_REL0,   SA::VALUE_LOST_ACC_REL0  },
		{HA::STACK_DMG_DEALT_REL,         SA::DMG_DEALT_REL        },
		{HA::STACK_DMG_DEALT_ACC_REL0,    SA::DMG_DEALT_ACC_REL0   },
		{HA::STACK_DMG_RECEIVED_REL,      SA::DMG_RECEIVED_REL     },
		{HA::STACK_DMG_RECEIVED_ACC_REL0, SA::DMG_RECEIVED_ACC_REL0},
	};

	if(stack)
	{
		int i = 0;
		for(const auto & [a, sa] : attrmap)
		{
			setattr(a, stack->attr(sa));
			++i;
		}

		ASSERT(i == EI(SA::_count), "not all stack attributes encoded: i=" + std::to_string(i));
	}

	if(astackinfo)
	{
		setStateMask(accessibility, obstacles, astackinfo->stack->cstack->unitSide());
		setActionMask(astackinfo, hexstacks);
	}
	else
	{
		setStateMask(accessibility, obstacles, BattleSide::ATTACKER);
	}

	finalize();
}

const HexAttrs & Hex::getAttrs() const
{
	return attrs;
}

int Hex::getID() const
{
	return id;
}

int Hex::getAttr(HexAttribute a) const
{
	return attr(a);
}

int Hex::attr(HexAttribute a) const
{
	return attrs.at(EI(a));
};
void Hex::setattr(HexAttribute a, int value)
{
	attrs.at(EI(a)) = value;
};

std::string Hex::name() const
{
	return "(" + std::to_string(attr(HA::Y_COORD)) + "," + std::to_string(attr(HA::X_COORD)) + ")";
}

void Hex::finalize()
{
	attrs.at(EI(HA::ACTION_MASK)) = actmask.to_ulong();
	attrs.at(EI(HA::STATE_MASK)) = statemask.to_ulong();
}

const Stack * Hex::getStack() const
{
	return stack.get();
}

// private

void Hex::setStateMask(const EAccessibility accessibility, const std::vector<std::shared_ptr<const CObstacleInstance>> & obstacles, BattleSide side)
{
	// First process obstacles
	// XXX: set only non-PASSABLE flags
	// (e.g. there may be a stack standing on the obstacle (firewall, moat))
	// so the PASSABLE mask bit will set later
	// XXX: moats are a weird obstacle:
	//      * if dispellable (Tower mines?) => type=SPELL_CREATED
	//      * otherwise => type=MOAT
	//      * their trigger ability is a spell, as it seems
	//        (which is not found in spells.json, neither is available as a SpellID constant)
	//
	//      Ref: Moat::placeObstacles()
	//           BattleEvaluator::goTowardsNearest() // var triggerAbility
	//

	for(const auto & obstacle : obstacles)
	{
		switch(obstacle->obstacleType)
		{
			case CObstacleInstance::USUAL:
			case CObstacleInstance::ABSOLUTE_OBSTACLE:
				statemask &= ~S_PASSABLE;
				break;
			case CObstacleInstance::MOAT:
				statemask |= (S_STOPPING | S_DAMAGING_ALL);
				break;
			case CObstacleInstance::SPELL_CREATED:
				// XXX: the public Obstacle / Spell API does not seem to expose
				//      any useful methods for checking if friendly creatures
				//      would get damaged by an obstacle.
				switch(SpellID(obstacle->ID))
				{
					case SpellID::QUICKSAND:
						statemask |= S_STOPPING;
						break;
					case SpellID::LAND_MINE:
						auto casterside = dynamic_cast<const SpellCreatedObstacle *>(obstacle.get())->casterSide;
						// XXX: in practice, there is no situation where enemy
						//      mines are visible (e.g. when our army has a stack
						// 		which is native to the battlefield terrain),
						// 		as the UI simply does not allow to cast the spell
						// 		in this case .
						if(side == casterside)
							statemask |= (side == BattleSide::DEFENDER ? S_DAMAGING_L : S_DAMAGING_R);
						else
							statemask |= (side == BattleSide::DEFENDER ? S_DAMAGING_R : S_DAMAGING_L);
				}
				break;
			default:
				THROW_FORMAT("Unexpected obstacle type: %d", EI(obstacle->obstacleType));
		}
	}

	switch(accessibility)
	{
		case EAccessibility::ACCESSIBLE:
			ASSERT(!stack, "accessibility is ACCESSIBLE, but a stack was found on hex");
			statemask |= S_PASSABLE;
			break;
		case EAccessibility::OBSTACLE:
			ASSERT(!stack, "accessibility is OBSTACLE, but a stack was found on hex");
			statemask &= ~S_PASSABLE;
			break;
		case EAccessibility::ALIVE_STACK:
			// XXX: stack can be NULL if it was left out of the observation
			// ASSERT(stack, "accessibility is ALIVE_STACK, but no stack was found on hex");
			statemask &= ~S_PASSABLE;
			break;
		case EAccessibility::DESTRUCTIBLE_WALL:
			// XXX: Destroyed walls become ACCESSIBLE.
			ASSERT(!stack, "accessibility is DESTRUCTIBLE_WALL, but a stack was found on hex");
			statemask &= ~S_PASSABLE;
			break;
		case EAccessibility::GATE:
			// See BattleProcessor::updateGateState() for gate states
			// See CBattleInfoCallback::getAccessibility() for accessibility on gate
			//
			// TL; DR:
			// -> GATE means closed, non-blocked gate
			// -> UNAVAILABLE means blocked
			// -> ACCESSIBLE otherwise (open, destroyed)
			//
			// Regardless of the gate state, we always set the GATE flag
			// purely based on the hex coordinates and not on the accessibility
			// => not setting GATE flag here
			//
			// However, in case of GATE accessibility, we still need
			// to set the PASSABLE flag accordingly.
			side == BattleSide::DEFENDER ? statemask.set(EI(HS::PASSABLE)) : statemask.reset(EI(HS::PASSABLE));
			break;
		case EAccessibility::UNAVAILABLE:
			statemask &= ~S_PASSABLE;
			break;
		default:
			THROW_FORMAT("Unexpected hex accessibility for bhex %d: %d", bhex.toInt() % EI(accessibility));
	}
}

void Hex::setActionMask(const std::shared_ptr<ActiveStackInfo> & astackinfo, const std::map<BattleHex, std::shared_ptr<Stack>> & hexstacks)
{
	const auto * astack = astackinfo->stack;

	// XXX: for statehist, astack may be enemy stack
	// in this case building the actmask is redundant

	if(astackinfo->canshoot && stack && stack->cstack->unitSide() != astack->cstack->unitSide())
		actmask.set(EI(HexAction::SHOOT));

	// XXX: ReachabilityInfo::isReachable() must not be used as it
	//      returns true even if speed is insufficient => use distances.
	// NOTE: distances is 0 for the stack's main hex and 1 for its rear hex
	//       (100000 if it can't fit there)
	if(astackinfo->rinfo->distances.at(bhex.toInt()) <= astack->attr(SA::SPEED))
		actmask.set(EI(HexAction::MOVE));
	else
		// astack can't MOVE here => AMOVE_* will never be possible
		return;

	const auto & nbhexes = NearbyBattleHexes(bhex);
	const auto * const a_cstack = astack->cstack;

	for(int i = 0; i < nbhexes.size(); ++i)
	{
		const auto & n_bhex = nbhexes.at(i);
		if(!n_bhex.isAvailable())
			continue;

		auto it = hexstacks.find(n_bhex);
		if(it == hexstacks.end())
			continue;

		const auto & n_cstack = it->second->cstack;
		auto hexaction = static_cast<HexAction>(i);

		if(n_cstack->unitSide() == a_cstack->unitSide())
			continue;

		if(hexaction <= HexAction::AMOVE_TL)
		{
			ASSERT(CStack::isMeleeAttackPossible(a_cstack, n_cstack, bhex), "vcmi says melee attack is IMPOSSIBLE [1]");
			actmask.set(i);
		}
		else if(hexaction <= HexAction::AMOVE_2BR)
		{
			// only wide R stacks can perform 2TR/2R/2BR attacks
			if(a_cstack->unitSide() == BattleSide::DEFENDER && a_cstack->doubleWide())
			{
				ASSERT(CStack::isMeleeAttackPossible(a_cstack, n_cstack, bhex), "vcmi says melee attack is IMPOSSIBLE [2]");
				actmask.set(i);
			}
		}
		else
		{
			// only wide L stacks can perform 2TL/2L/2BL attacks
			if(a_cstack->unitSide() == BattleSide::ATTACKER && a_cstack->doubleWide())
			{
				ASSERT(CStack::isMeleeAttackPossible(a_cstack, n_cstack, bhex), "vcmi says melee attack is IMPOSSIBLE");
				actmask.set(i);
			}
		}
	}
}
}
