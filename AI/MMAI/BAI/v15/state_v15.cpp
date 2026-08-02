/*
 * state.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "BAI/v15/state_v15.h"

#include "BAI/v15/fastbfs_v15.h"
#include "BAI/v15/graph/edges/action_ends_at_hex_v15.h"
#include "BAI/v15/graph/edges/generic_v15.h"
#include "BAI/v15/graph/edges/hex_adjacent_hex_v15.h"
#include "BAI/v15/graph/edges/hex_is_end_of_action_v15.h"
#include "BAI/v15/graph/edges/unit_acts_before_unit_v15.h"
#include "BAI/v15/graph/edges/unit_is_meleed_by_action_v15.h"
#include "BAI/v15/graph/edges/unit_is_shot_by_action_v15.h"
#include "BAI/v15/graph/edges/unit_melee_dmg_unit_v15.h"
#include "BAI/v15/graph/edges/unit_shoot_dmg_unit_v15.h"
#include "BAI/v15/graph/graph_v15.h"
#include "BAI/v15/graph/nodes/hex_v15.h"
#include "BAI/v15/graph/nodes/player_v15.h"
#include "BAI/v15/graph/nodes/unit_v15.h"
#include "battle/BattleHex.h"
#include "battle/BattleSide.h"
#include "battle/CPlayerBattleCallback.h"
#include "battle/CUnitState.h"
#include "battle/DamageCalculator.h"
#include "bonuses/BonusEnum.h"
#include "bonuses/BonusParameters.h" // IWYU pragma: keep (needed for bonus->parameters)
#include "entities/building/TownFortifications.h"
#include "networkPacks/PacksForClientBattle.h"

#include "schema/v15/types.h"
#include "spells/CSpellHandler.h"
#include "spells/ISpellMechanics.h"
#include "spells/ProxyCaster.h"

namespace MMAI::BAI::V15
{

namespace
{
	namespace N = Graph::Nodes;
	namespace E = Graph::Edges;
	using UnitPtr = std::shared_ptr<const N::Unit>;
	using HexPtr = std::shared_ptr<const N::Hex>;
	using ActionPtr = std::shared_ptr<const N::Action>;
	using ActionArgs = N::Action::Args;
	using TowerFlags = N::Global::TowerFlags;
	using CorpseFlags = N::Global::CorpseFlags;
	using ET = S15::Graph::ElementType;
	using AT = S15::ActionType;

	void ReportCounts(const Graph::Graph & G)
	{
		size_t total = 0;
		std::cout << "G:\n";
		for(int i = 0; i < EU(ET::_count); ++i)
		{
			size_t tmp = 0;
			std::cout << "  " << std::setw(5);
			switch(ET(i))
			{
				case ET::NODE_GLOBAL:
					tmp = G.size<N::Global>();
					std::cout << tmp << " NODE_GLOBAL\n";
					break;
				case ET::NODE_PLAYER:
					tmp = G.size<N::Player>();
					std::cout << tmp << " NODE_PLAYER\n";
					break;
				case ET::NODE_UNIT:
					tmp = G.size<N::Unit>();
					std::cout << tmp << " NODE_UNIT\n";
					break;
				case ET::NODE_HEX:
					tmp = G.size<N::Hex>();
					std::cout << tmp << " NODE_HEX\n";
					break;
				case ET::NODE_ACTION:
					tmp = G.size<N::Action>();
					std::cout << tmp << " NODE_ACTION\n";
					break;
				case ET::EDGE_GLOBAL_TO_PLAYER:
					tmp = G.size<E::Global_To_Player>();
					std::cout << tmp << " EDGE_GLOBAL_TO_PLAYER\n";
					break;
				case ET::EDGE_PLAYER_TO_GLOBAL:
					tmp = G.size<E::Player_To_Global>();
					std::cout << tmp << " EDGE_PLAYER_TO_GLOBAL\n";
					break;
				case ET::EDGE_GLOBAL_TO_UNIT:
					tmp = G.size<E::Global_To_Unit>();
					std::cout << tmp << " EDGE_GLOBAL_TO_UNIT\n";
					break;
				case ET::EDGE_UNIT_TO_GLOBAL:
					tmp = G.size<E::Unit_To_Global>();
					std::cout << tmp << " EDGE_UNIT_TO_GLOBAL\n";
					break;
				case ET::EDGE_GLOBAL_TO_HEX:
					tmp = G.size<E::Global_To_Hex>();
					std::cout << tmp << " EDGE_GLOBAL_TO_HEX\n";
					break;
				case ET::EDGE_HEX_TO_GLOBAL:
					tmp = G.size<E::Hex_To_Global>();
					std::cout << tmp << " EDGE_HEX_TO_GLOBAL\n";
					break;
				case ET::EDGE_GLOBAL_TO_ACTION:
					tmp = G.size<E::Global_To_Action>();
					std::cout << tmp << " EDGE_GLOBAL_TO_ACTION\n";
					break;
				case ET::EDGE_PLAYER_OWNS_UNIT:
					tmp = G.size<E::Player_Owns_Unit>();
					std::cout << tmp << " EDGE_PLAYER_OWNS_UNIT\n";
					break;
				case ET::EDGE_UNIT_OWNED_BY_PLAYER:
					tmp = G.size<E::Unit_OwnedBy_Player>();
					std::cout << tmp << " EDGE_UNIT_OWNED_BY_PLAYER\n";
					break;
				case ET::EDGE_UNIT_OCCUPIES_HEX:
					tmp = G.size<E::Unit_Occupies_Hex>();
					std::cout << tmp << " EDGE_UNIT_OCCUPIES_HEX\n";
					break;
				case ET::EDGE_HEX_OCCUPIED_BY_UNIT:
					tmp = G.size<E::Hex_OccupiedBy_Unit>();
					std::cout << tmp << " EDGE_HEX_OCCUPIED_BY_UNIT\n";
					break;
				case ET::EDGE_ACTION_BY_UNIT:
					tmp = G.size<E::Action_By_Unit>();
					std::cout << tmp << " EDGE_ACTION_BY_UNIT\n";
					break;
				case ET::EDGE_UNIT_HAS_ACTION:
					tmp = G.size<E::Unit_Has_Action>();
					std::cout << tmp << " EDGE_UNIT_HAS_ACTION\n";
					break;
				case ET::EDGE_HEX_ADJACENT_HEX:
					tmp = G.size<E::Hex_Adjacent_Hex>();
					std::cout << tmp << " EDGE_HEX_ADJACENT_HEX\n";
					break;
				case ET::EDGE_UNIT_ACTS_BEFORE_UNIT:
					tmp = G.size<E::Unit_ActsBefore_Unit>();
					std::cout << tmp << " EDGE_UNIT_ACTS_BEFORE_UNIT\n";
					break;
				case ET::EDGE_UNIT_MELEE_DMG_UNIT:
					tmp = G.size<E::Unit_MeleeDmg_Unit>();
					std::cout << tmp << " EDGE_UNIT_MELEE_DMG_UNIT\n";
					break;
				case ET::EDGE_UNIT_SHOOT_DMG_UNIT:
					tmp = G.size<E::Unit_ShootDmg_Unit>();
					std::cout << tmp << " EDGE_UNIT_SHOOT_DMG_UNIT\n";
					break;
				case ET::EDGE_UNIT_BLOCKS_UNIT:
					tmp = G.size<E::Unit_Blocks_Unit>();
					std::cout << tmp << " EDGE_UNIT_BLOCKS_UNIT\n";
					break;
				case ET::EDGE_ACTION_ENDS_AT_HEX:
					tmp = G.size<E::Action_EndsAt_Hex>();
					std::cout << tmp << " EDGE_ACTION_ENDS_AT_HEX\n";
					break;
				case ET::EDGE_HEX_IS_END_OF_ACTION:
					tmp = G.size<E::Hex_IsEndOf_Action>();
					std::cout << tmp << " EDGE_HEX_IS_END_OF_ACTION\n";
					break;
				case ET::EDGE_ACTION_BLOCKS_UNIT:
					tmp = G.size<E::Action_Blocks_Unit>();
					std::cout << tmp << " EDGE_ACTION_BLOCKS_UNIT\n";
					break;
				case ET::EDGE_UNIT_BLOCKED_BY_ACTION:
					tmp = G.size<E::Unit_BlockedBy_Action>();
					std::cout << tmp << " EDGE_UNIT_BLOCKED_BY_ACTION\n";
					break;
				case ET::EDGE_UNIT_BECOMES_MELEE_THREAT_AFTER_ACTION:
					tmp = G.size<E::Unit_BecomesMeleeThreatAfter_Action>();
					std::cout << tmp << " EDGE_UNIT_BECOMES_MELEE_THREAT_AFTER_ACTION\n";
					break;
				case ET::EDGE_UNIT_BECOMES_SHOOT_THREAT_AFTER_ACTION:
					tmp = G.size<E::Unit_BecomesShootThreatAfter_Action>();
					std::cout << tmp << " EDGE_UNIT_BECOMES_SHOOT_THREAT_AFTER_ACTION\n";
					break;
				case ET::EDGE_UNIT_IS_MELEED_BY_ACTION:
					tmp = G.size<E::Unit_IsMeleedBy_Action>();
					std::cout << tmp << " EDGE_UNIT_IS_MELEED_BY_ACTION\n";
					break;
				case ET::EDGE_UNIT_IS_SHOT_BY_ACTION:
					tmp = G.size<E::Unit_IsShotBy_Action>();
					std::cout << tmp << " EDGE_UNIT_IS_SHOT_BY_ACTION\n";
					break;
				case ET::EDGE_UNIT_BECOMES_MELEE_TARGET_AFTER_ACTION:
					tmp = G.size<E::Unit_BecomesMeleeTargetAfter_Action>();
					std::cout << tmp << " EDGE_UNIT_BECOMES_MELEE_TARGET_AFTER_ACTION\n";
					break;
				case ET::EDGE_UNIT_BECOMES_SHOOT_TARGET_AFTER_ACTION:
					tmp = G.size<E::Unit_BecomesShootTargetAfter_Action>();
					std::cout << tmp << " EDGE_UNIT_BECOMES_SHOOT_TARGET_AFTER_ACTION\n";
					break;
				case ET::EDGE_HEX_BECOMES_MELEE_TARGET_AFTER_ACTION:
					tmp = G.size<E::Hex_BecomesMeleeTargetAfter_Action>();
					std::cout << tmp << " EDGE_HEX_BECOMES_MELEE_TARGET_AFTER_ACTION\n";
					break;
				case ET::EDGE_HEX_BECOMES_SHOOT_TARGET_AFTER_ACTION:
					tmp = G.size<E::Hex_BecomesShootTargetAfter_Action>();
					std::cout << tmp << " EDGE_HEX_BECOMES_SHOOT_TARGET_AFTER_ACTION\n";
					break;
				default:
					throwf("Unexpected element type: {}", i);
			}
			static_assert(static_cast<int>(ET::_count) == 35);
			total += tmp;
		}
		std::cout << "  ---\n";
		std::cout << "  " << std::setw(5) << total << " TOTAL\n";
		// On gym/generated/4096/4x1024.vmap: total max = 60K, mean = 10K
	}

	TowerFlags GetSiegeTowers(const CPlayerBattleCallback & battle)
	{
		TowerFlags res; // upper, middle, lower

		auto has = [&battle](EWallPart part)
		{
			auto ws = battle.battleGetWallState(part);
			return ws != EWallState::NONE && ws != EWallState::DESTROYED;
		};

		if(has(EWallPart::UPPER_TOWER))
			res.hasUpperTower = true;
		if(has(EWallPart::KEEP))
			res.hasMiddleTower = true;
		if(has(EWallPart::BOTTOM_TOWER))
			res.hasBottomTower = true;

		return res;
	}

	CorpseFlags GetSiegeCorpses(const CPlayerBattleCallback & battle)
	{
		CorpseFlags res; // gate, bridge

		if(battle.battleGetFortifications().wallsHealth == 0)
			return res;

		for(const auto & cstack : battle.battleGetAllStacks(false))
		{
			if(cstack->alive())
				continue;

			if(cstack->coversPos(BattleHex::GATE_INNER) || cstack->coversPos(BattleHex::GATE_OUTER))
				res.hasGateCorpse = true;
			if(cstack->coversPos(BattleHex::GATE_BRIDGE))
				res.hasBridgeCorpse = true;
		};

		return res;
	}

	State::GlobalStats CalcGlobalStats(const CPlayerBattleCallback & battle)
	{
		int lv = 0;
		int lh = 0;
		int rv = 0;
		int rh = 0;

		for(auto & stack : battle.battleGetStacks())
		{
			auto v = stack->getCount() * N::Unit::GetValue(stack->unitType());
			auto h = stack->getAvailableHealth();

			if(stack->unitSide() == BattleSide::ATTACKER)
			{
				lv += v;
				lh += static_cast<int>(h);
			}
			else
			{
				rv += v;
				rh += static_cast<int>(h);
			}
		}

		return State::GlobalStats{.leftValue = lv, .leftHp = lh, .rightValue = rv, .rightHp = rh, .totalValue = lv + rv, .totalHp = lh + rh};
	}

	// Stolen from BattleActionProcessor::handleDeathStare
	// Calculates number of kills
	double CalcDeathStare(const CPlayerBattleCallback & battle, const battle::CUnitState * attacker, const battle::CUnitState * defender, bool ranged)
	{
		/*
		 * Death stare:
		 * - X=10% chance to kill per gorgon
		 * - rolled separately for each gorgon in the stack
		 * - kills capped to (N*X/100), where N=number of gorgons
		 *
		 * Accurate Shot (HotA seadogs):
		 * - same mechanic as death stare, but ranged
		 * - X=3% chance to kill for each seadog (X=2% with range penalty)
		 *
		 * Commander death stare:
		 * - different mechanic: kills depend on level
		 */

		auto subtype = BonusCustomSubtype::deathStareGorgon;

		if(ranged)
		{
			bool distancePenalty = battle.battleHasDistancePenalty(attacker, attacker->getPosition(), defender->getPosition());
			bool obstaclePenalty = battle.battleHasWallPenalty(attacker, attacker->getPosition(), defender->getPosition());

			if(distancePenalty)
				subtype = obstaclePenalty ? BonusCustomSubtype::deathStareRangeObstaclePenalty : BonusCustomSubtype::deathStareRangePenalty;
			else
				subtype = obstaclePenalty ? BonusCustomSubtype::deathStareObstaclePenalty : BonusCustomSubtype::deathStareNoRangePenalty;
		}

		// Non-commander death stare
		int n = attacker->getCount();
		int x = attacker->valOfBonuses(BonusType::DEATH_STARE, subtype);
		double kills = n * x / 100.0;

		// Commander death stare
		int x1 = attacker->valOfBonuses(BonusType::DEATH_STARE, BonusCustomSubtype::deathStareCommander);
		kills += static_cast<double>(x1 * attacker->creatureLevel()) / defender->creatureLevel();

		return kills;
	}

	struct CUnitStateWrapper
	{
		explicit CUnitStateWrapper(const CStack * cstack, const std::shared_ptr<battle::CUnitState> & cstate) : cstack(cstack), cstate(cstate) {}

		// XXX: many methods such as unitType() or getAvailableHealth() throw
		// exceptions when called on a CUnitState => calculate health manually.
		int calcAvailableHealth() const
		{
			// excplicitly cast to int otherwise unsigned int arithmetic may cause UB
			auto n = static_cast<int>(cstate->getCount());
			auto hpOne = static_cast<int>(cstack->getMaxHealth());
			auto hp1st = static_cast<int>(cstate->getFirstHPleft());
			return (std::max(0, (n - 1)) * hpOne) + hp1st;
		}

		const CStack * cstack;
		std::shared_ptr<battle::CUnitState> cstate;
	};

	struct UnitStates
	{
		CUnitStateWrapper a;
		CUnitStateWrapper b;
	};

	// Executes a single attack (without retaliation logic)
	// Applies damage to defender and to attacker (if fire shield)
	// Mutates the given states.
	void ApplyAttack(const UnitStates & states, const CPlayerBattleCallback & battle, bool ranged)
	{
		const auto & A_state = states.a.cstate;
		const auto & B_state = states.b.cstate;

		auto A_bai = BattleAttackInfo(A_state.get(), B_state.get(), 0, ranged);
		auto estimation = std::make_shared<DamageEstimation>(DamageCalculator(battle, A_bai).calculateDmgRange());
		auto A_dmg_min = static_cast<int>(estimation->damage.min);
		auto A_dmg_max = static_cast<int>(estimation->damage.max);
		auto A_dmg_mean = static_cast<int64_t>(0.5 * (A_dmg_min + A_dmg_max));

		int B_qty_old = B_state->getCount();
		// CUnitState->damage() expects a *mutable* ref and can set it to 0 (?!?)
		{
			int64_t dmg = A_dmg_mean;
			B_state->damage(dmg);
		}
		auto A_kills_mean = B_qty_old - B_state->getCount();

		// 1. Handle LIFE_DRAIN and SOUL_STEAL
		// Stolen from BattleActionProcessor::applyBattleEffects
		// (removed permanent / non-permanent logic for SOUL_STEAL here)
		bool B_isLiving = B_state->isLiving();
		if(B_isLiving)
		{
			if(A_state->hasBonusOfType(BonusType::LIFE_DRAIN) && states.a.cstack->getTotalHealth() != states.a.calcAvailableHealth())
			{
				int64_t toHeal = A_dmg_mean * A_state->valOfBonuses(BonusType::LIFE_DRAIN) / 100;
				A_state->heal(toHeal, EHealLevel::RESURRECT, EHealPower::PERMANENT);
			}

			if(int ss = A_state->valOfBonuses(BonusType::SOUL_STEAL))
			{
				int64_t toHeal = static_cast<int64_t>(A_kills_mean) * ss * A_state->getMaxHealth();
				A_state->heal(toHeal, EHealLevel::OVERHEAL, EHealPower::ONE_BATTLE);
			}
		}

		// 2. Handle FIRE_SHIELD (triggers even if B is not alive)
		// Stolen from BattleActionProcessor::applyBattleEffects
		if(!ranged && !B_state->isClone() && B_state->hasBonusOfType(BonusType::FIRE_SHIELD)
		   && !A_state->hasBonusOfType(BonusType::SPELL_SCHOOL_IMMUNITY, BonusSubtypeID(SpellSchool::FIRE))
		   && !A_state->hasBonusOfType(BonusType::NEGATIVE_EFFECTS_IMMUNITY, BonusSubtypeID(SpellSchool::FIRE))
		   && A_state->valOfBonuses(BonusType::SPELL_DAMAGE_REDUCTION, BonusSubtypeID(SpellSchool::FIRE)) < 100 && !B_state->isInvincible())
		{
			auto dmg = (std::min(static_cast<int64_t>(states.b.calcAvailableHealth()), A_dmg_mean) * B_state->valOfBonuses(BonusType::FIRE_SHIELD)) / 100;
			A_state->damage(dmg);
		}

		// 3. Handle DEATH_STARE (must come last; uses attacker qty left after fire shield)
		if(B_state->alive() && B_isLiving && A_state->hasBonusOfType(BonusType::DEATH_STARE))
		{
			auto staredeaths = static_cast<int>(std::round(CalcDeathStare(battle, A_state.get(), B_state.get(), ranged)));

			while(staredeaths > 0 && B_state->alive())
			{
				/*
				 * VCMI's death stare has a bug:
				 * The top "HP Left" of the remaining defender stack is not
				 * reset to full HP after applying the effect
				 * https://discord.com/channels/298106089885401090/1147259775420207256/1506701782011613356
				 * Once that bug is fixed, change the calculation here to use:
				 * int64_t dmg = B_state->getFirstHPleft();
				 */
				int64_t dmg = B_state->getMaxHealth();
				B_state->damage(dmg);
				--staredeaths;
			}
		}
	}

	/*
	 * VCMI's damage estimation helper does not take into account stuff such as:
	 * 	- Base mechanics:
	 * 	 	* HAS_ADDITIONAL_ATTACK - tested
	 * 	 	* DEATH_STARE 			- tested
	 * 	 	* FIRE_SHIELD 			- tested (incl. attacker dying from it)
	 * 	 	* LIFE_DRAIN 			- tested
	 *	- Mod mechanics:
	 * 		* RANGED_RATALIATION 	- tested
	 * 		* FIRST_STRIKE 			- tested
	 * 		* SOUL_STEAL 			- not tested
	 * 		* FEROCITY 				- tested
	 *
	 * This is an attempt to reimplement it here.
	 *
	 */
	// NOLINTNEXTLINE(readability-function-cognitive-complexity)
	UnitStates
	SimulateAttackAction(const CPlayerBattleCallback & battle, const CStack & attacker, const CStack & defender, bool isRangedAttack, bool isDefenderBlocked)
	{
		bool isMeleeAttack = !isRangedAttack;

		// Stolen from BattleActionProcessor::doShootAction
		auto checkRangedRetal = [&attacker, &defender, isDefenderBlocked]()
		{
			return (
				!isDefenderBlocked && defender.hasBonusOfType(BonusType::RANGED_RETALIATION) && !attacker.hasBonusOfType(BonusType::BLOCKS_RANGED_RETALIATION)
			);
		};

		// Stolen from BattleActionProcessor::doAttackAction
		// but using different getBonus functions which use caching strs
		auto checkFirstStrike = [&defender, isRangedAttack](bool canRetal)
		{
			if(defender.isInvincible())
				return false;

			if(!canRetal)
				return false;

			static const auto selRanged = Selector::typeSubtype(BonusType::FIRST_STRIKE, BonusCustomSubtype::damageTypeAll)
											  .Or(Selector::typeSubtype(BonusType::FIRST_STRIKE, BonusCustomSubtype::damageTypeRanged));
			static const auto selMelee = Selector::typeSubtype(BonusType::FIRST_STRIKE, BonusCustomSubtype::damageTypeAll)
											 .Or(Selector::typeSubtype(BonusType::FIRST_STRIKE, BonusCustomSubtype::damageTypeMelee));
			static const std::string strRanged = "firstStrikeSelectorRanged";
			static const std::string strMelee = "firstStrikeSelectorMelee";

			return isRangedAttack ? defender.hasBonus(selRanged, strRanged) : defender.hasBonus(selMelee, strMelee);
		};

		// Stolen from BattleActionProcessor::doAttackAction
		auto getAdditionalAttacks = [&attacker, isRangedAttack]
		{
			int totalAttacks = attacker.getTotalAttacks(isRangedAttack);
			if(const auto * attackingHero = attacker.getMyHero())
				totalAttacks += attackingHero->valOfBonuses(BonusType::HERO_GRANTS_ATTACKS, BonusSubtypeID(attacker.creatureId()));

			return totalAttacks - 1;
		};

		// Stolen from BattleActionProcessor::doAttackAction
		// but using different getBonus functions which use caching strs
		auto getFerocityAttacks = [&attacker](int kills)
		{
			const auto bonuses = attacker.getBonusesOfType(BonusType::FEROCITY);
			const auto bonus = bonuses->getFirst(Selector::all);
			if(!bonus)
				return 0;

			int killThreshold = bonus->parameters ? bonus->parameters->toNumber() : 1;
			return kills >= killThreshold ? bonuses->totalValue(0) : 0;
		};

		bool canRetal = defender.ableToRetaliate() && (isMeleeAttack || checkRangedRetal());
		int ferocityCheckAfter = 0; // when to check for ferocity (depends on first strike)
		auto positions = std::vector<int>{}; //  0=keep, 1=switch

		auto isSwapped = [&positions]()
		{
			// Even number of swaps means not swapped in the end
			return std::ranges::count(positions, 1) % 2 == 1;
		};

		if(checkFirstStrike(canRetal))
		{
			positions.push_back(1); // switch: initial strike is by defender
			positions.push_back(1); // switch: attacker strikes (this is not a retaliation)
			ferocityCheckAfter = 1; // initial attacker strike is actually 2nd
		}
		else
		{
			positions.push_back(0); // no switch: initial strike is by attacker
			if(canRetal && !defender.isInvincible())
				positions.push_back(1); // switch: defender strikes (if able to retalate)
		}

		for(int i = 0; i < getAdditionalAttacks(); ++i)
		{
			positions.push_back(isSwapped());
		}

		const auto states0 = UnitStates{.a = CUnitStateWrapper(&attacker, attacker.acquireState()), .b = CUnitStateWrapper(&defender, defender.acquireState())};
		auto states = states0;

		for(int i = 0; i < positions.size(); ++i)
		{
			bool shouldSwitch = positions.at(i);
			states = shouldSwitch ? UnitStates{.a = states.b, .b = states.a} : UnitStates{.a = states.a, .b = states.b};

			// mutates states
			int b_prevcount = states.b.cstate->getCount();
			ApplyAttack(states, battle, isRangedAttack);

			if(!states.a.cstate->alive() || !states.b.cstate->alive())
				break;

			if(i == ferocityCheckAfter)
			{
				ASSERT(states.b.cstack->unitId() == defender.unitId(), "SimulateAttackAction: ferocity check: expected A=attacker B=defender");
				// ferocity check must always be when a=attacker, b=defender => check if b count changed
				int ferocityAttacks = getFerocityAttacks(b_prevcount - states.b.cstate->getCount());
				for(int j = 0; j < ferocityAttacks; ++j)
					positions.push_back(isSwapped());
			}
		}

		return states.a.cstack->unitId() == states0.a.cstack->unitId() ? states : UnitStates{.a = states.b, .b = states.a};
	}

	namespace Q
	{
		constexpr auto MaxUnits = S15::STACK_QUEUE_SIZE;
		using UnitId = std::uint32_t;
		using Count = std::uint8_t;
		using CountMatrix = std::array<std::array<Count, MaxUnits>, MaxUnits>;

		// stats.count[A][B] = (number of actions A takes before B's first action)
		struct ActsBeforeMatrix
		{
			std::array<UnitPtr, MaxUnits> unique_units{};
			std::size_t unique_count = 0;
			CountMatrix count{};
		};

		std::size_t GetOrAddUnitId(const UnitPtr & unit, ActsBeforeMatrix & result)
		{
			for(std::size_t i = 0; i < result.unique_count; ++i)
				if(result.unique_units[i] == unit)
					return i;
			const std::size_t id = result.unique_count++;
			result.unique_units[id] = unit;
			return id;
		}

		ActsBeforeMatrix BuildActsBeforeMatrix(const Graph::Graph & G, const CPlayerBattleCallback & battle)
		{
			// XXX: there is a bug in VCMI when high morale occurs:
			//      - the stack acts as if it's already the next unit's turn
			//      - as a result, QueuePos for the ACTIVE stack is non-0
			//        while the QueuePos for the next (non-active) stack is 0
			// (this applies only to good morale; bad morale simply skips turn)
			// As a workaround, a "isMorale" flag is passed whenever the astack is
			// acting because of high morale and queue is "shifted" accordingly.

			auto q = std::vector<UnitPtr>{};
			auto tmp = std::vector<battle::Units>{};
			battle.battleGetTurnOrder(tmp, S15::STACK_QUEUE_SIZE, 0);
			for(const auto & cunits : tmp)
			{
				for(const auto & cunit : cunits)
				{
					if(q.size() >= S15::STACK_QUEUE_SIZE)
						break;

					// cunit may not be part of the graph (e.g. arrow towers)
					// XXX: there is an assumption here that unit ID == CStack ID.
					// This must be OK since as of 2026, battle::Unit is a superclass of CStack.
					if(const auto & unit = G.getByExtraIndex<N::Unit>(cunit->unitId(), false))
						q.push_back(unit);
				}
			}

			ActsBeforeMatrix result;
			std::array<Count, MaxUnits> seen_counts{};
			std::array<bool, MaxUnits> first_seen{};

			for(const auto & unit : q)
			{
				const std::size_t b = GetOrAddUnitId(unit, result);

				if(!first_seen[b])
				{
					first_seen[b] = true;

					for(std::size_t a = 0; a < result.unique_count; ++a)
					{
						result.count[a][b] = seen_counts[a];
					}
				}

				++seen_counts[b];
			}

			return result;
		}
	}

	struct AttackLogAggregateData
	{
		int ldd = 0; // left damage dealt
		int ldr = 0; // left damage received
		int lvk = 0; // left value killed
		int lvl = 0; // left value lost
		int rdd = 0; // right damage dealt
		int rdr = 0; // right damage received
		int rvk = 0; // right value killed
		int rvl = 0; // right value lost
	};

	AttackLogAggregateData ProcessAttackLogs(const std::vector<AttackLog> & attackLogs)
	{
		auto res = AttackLogAggregateData{};

		for(const auto & al : attackLogs)
		{
			// attacker is null if the dmg comes from an effect, e.g. acid
			// or if it "appeared" after G was built, e.g. was summoned
			if(al.attacker)
			{
				if(al.attacker->cstack.unitSide() == BattleSide::LEFT_SIDE)
				{
					res.ldd += al.dmg;
					res.lvk += al.value;
				}
				else
				{
					res.rdd += al.dmg;
					res.rvk += al.value;
				}
			}

			// defender is null if it "appeared" after G was built, e.g. was summoned
			if(al.defender)
			{
				if(al.defender->cstack.unitSide() == BattleSide::LEFT_SIDE)
				{
					res.ldr += al.dmg;
					res.lvl += al.value;
				}
				else
				{
					res.rdr += al.dmg;
					res.rvl += al.value;
				}
			}
		}

		return res;
	}

	S15::WallHP GetWallHP(const CPlayerBattleCallback & battle, const BattleHex & bhex)
	{
		auto part = battle.battleHexToWallPart(bhex);
		switch(part)
		{
			case EWallPart::BOTTOM_WALL:
			case EWallPart::BELOW_GATE:
			case EWallPart::OVER_GATE:
			case EWallPart::UPPER_WALL:
			case EWallPart::GATE:
				switch(battle.battleGetWallState(part))
				{
					case EWallState::NONE:
					case EWallState::DESTROYED:
						return S15::WallHP::HP0;
					case EWallState::DAMAGED:
						return S15::WallHP::HP1;
					case EWallState::INTACT:
						return S15::WallHP::HP2;
					case EWallState::REINFORCED:
						return S15::WallHP::HP3;
					default:
						logAi->warn("MMAI: unexpected wall state: %d", EI(battle.battleGetWallState(part)));
						return S15::WallHP::HP0;
				}
			default:
				return S15::WallHP::HP0;
				break;
		}
	}

	// A custom hash function must be provided for the adjmap
	struct PairHash
	{
		std::size_t operator()(const std::pair<si16, si16> & t) const
		{
			std::size_t h0 = std::hash<si16>{}(t.first);
			std::size_t h1 = std::hash<si16>{}(t.second);
			return h0 ^ (h1 << 1);
		}
	};

	std::unordered_map<std::pair<int, int>, int, PairHash> InitAdjMap()
	{
		auto res = std::unordered_map<std::pair<int, int>, int, PairHash>{};

		for(int id1 = 0; id1 < GameConstants::BFIELD_SIZE; id1++)
		{
			auto hex1 = BattleHex(static_cast<int16_t>(id1));
			int i = 0;
			for(const auto dir : hex1.hexagonalDirections())
			{
				static_assert(EU(BattleHex::EDir::TOP_LEFT) == 0);
				static_assert(EU(BattleHex::EDir::TOP_RIGHT) == 1);
				static_assert(EU(BattleHex::EDir::RIGHT) == 2);
				static_assert(EU(BattleHex::EDir::BOTTOM_RIGHT) == 3);
				static_assert(EU(BattleHex::EDir::BOTTOM_LEFT) == 4);
				static_assert(EU(BattleHex::EDir::LEFT) == 5);
				auto hex2 = hex1.cloneInDirection(dir, false);
				res[{hex1.toInt(), hex2.toInt()}] = i;
				++i;
			}
		}

		return res;
	}

	bool checkBerserk(const CStack & cstack)
	{
		return cstack.hasBonusOfType(BonusType::ATTACKS_NEAREST_CREATURE);
	}

	void AddGlobalNode(
		Graph::Graph & G,
		const CPlayerBattleCallback & battle,
		const CStack * acstack,
		const S15::CombatResult result,
		const int round,
		const State::GlobalStats & stats
	)
	{
		G.setFlag(ET::NODE_GLOBAL);

		G.add(
			N::Global::Create(
				{.res = result,
				 .round = round,
				 .value = stats.totalValue,
				 .hp = stats.totalHp,
				 .towers = GetSiegeTowers(battle),
				 .corpses = GetSiegeCorpses(battle)}
			)
		);
	}

	void AddPlayerNodes(
		Graph::Graph & G,
		const CPlayerBattleCallback & battle,
		const State::GlobalStats & startStats,
		const State::GlobalStats & lastStats,
		const State::GlobalStats & stats,
		const AttackLogAggregateData & logdata
	)
	{
		G.setFlag(ET::NODE_PLAYER);

		static_assert(EU(BattleSide::LEFT_SIDE) == 0);
		static_assert(EU(BattleSide::RIGHT_SIDE) == 1);

		G.add(
			N::Player::Create(
				{.side = BattleSide::LEFT_SIDE,
				 .isActive = BattleSide::LEFT_SIDE == battle.battleGetMySide(),
				 .globalValueStart = startStats.totalValue,
				 .globalValuePrevRound = lastStats.totalValue,
				 .globalHpPrevRound = lastStats.totalHp,
				 .value = stats.leftValue,
				 .hp = stats.leftHp,
				 .dmgDealt = logdata.ldd,
				 .dmgReceived = logdata.ldr,
				 .valueKilled = logdata.lvk,
				 .valueLost = logdata.lvl}
			)
		);

		G.add(
			N::Player::Create(
				{.side = BattleSide::RIGHT_SIDE,
				 .isActive = BattleSide::RIGHT_SIDE == battle.battleGetMySide(),
				 .globalValueStart = startStats.totalValue,
				 .globalValuePrevRound = lastStats.totalValue,
				 .globalHpPrevRound = lastStats.totalHp,
				 .value = stats.rightValue,
				 .hp = stats.rightHp,
				 .dmgDealt = logdata.rdd,
				 .dmgReceived = logdata.rdr,
				 .valueKilled = logdata.rvk,
				 .valueLost = logdata.rvl}
			)
		);
	}

	void AddUnitNodes(Graph::Graph & G, const CPlayerBattleCallback & battle, const CStack * acstack, const State::GlobalStats & stats)
	{
		G.getFlags().require(ET::NODE_HEX);
		G.setFlag(ET::NODE_UNIT);

		for(auto & stack : battle.battleGetStacks())
		{
			bool isActive = stack == acstack;
			bool isEnemy = stack->unitSide() != battle.battleGetMySide();
			bool isFlying = stack->hasBonusOfType(BonusType::FLYING);
			auto speed = static_cast<int>(stack->getMovementRange());

			const auto distances = G.getFastBFS().run(stack->getPosition(), stack->getPosition(), stack->unitSide(), isFlying, stack->doubleWide(), speed);

			G.add(
				N::Unit::Create(
					{.cstack = *stack,
					 .isActive = isActive,
					 .isEnemy = isEnemy,
					 .isFlying = isFlying,
					 .speed = speed,
					 .bfieldValue = stats.totalValue,
					 .distances = distances}
				)
			);
		}
	}

	void AddHexNodes(Graph::Graph & G, const CPlayerBattleCallback & battle, const CStack * acstack)
	{
		G.setFlag(ET::NODE_HEX);

		auto hexobstacles = std::array<std::vector<std::shared_ptr<const CObstacleInstance>>, 165>{};

		for(const auto & obstacle : battle.battleGetAllObstacles())
			for(const auto & bh : obstacle->getAffectedTiles())
				if(bh.isAvailable())
					hexobstacles.at(N::Hex::CalcId(bh)).push_back(obstacle);

		auto gatestate = battle.battleGetGateState();
		bool isGateOpen = battle.battleGetFortifications().wallsHealth > 0 && (gatestate == EGateState::OPENED || gatestate == EGateState::DESTROYED);

		for(int y = 0; y < 11; ++y)
		{
			for(int x = 0; x < 15; ++x)
			{
				auto i = (y * 15) + x;
				auto bh = BattleHex(static_cast<int16_t>(x + 1), static_cast<int16_t>(y));
				ASSERT(bh.isAvailable(), "invalid bhex");

				G.add(
					N::Hex::Create({
						.bhex = bh,
						.accessibility = G.getAccessibility().at(bh.toInt()),
						.side = acstack ? acstack->unitSide() : BattleSide::LEFT_SIDE,
						.obstacles = hexobstacles.at(i),
						.wallHP = GetWallHP(battle, bh),
						.isGateOpen = isGateOpen,
						.isSiege = (battle.battleGetFortifications().wallsHealth > 0),
					})
				);
			}
		}
	}

	void AddEdges_Global_To_PlayerUnitHex(Graph::Graph & G, const CPlayerBattleCallback & battle)
	{
		G.getFlags().require(ET::NODE_GLOBAL);
		G.getFlags().require(ET::NODE_PLAYER);
		G.getFlags().require(ET::NODE_UNIT);
		G.getFlags().require(ET::NODE_HEX);
		G.setFlag(ET::EDGE_PLAYER_TO_GLOBAL);
		G.setFlag(ET::EDGE_GLOBAL_TO_PLAYER);
		G.setFlag(ET::EDGE_UNIT_TO_GLOBAL);
		G.setFlag(ET::EDGE_GLOBAL_TO_UNIT);
		G.setFlag(ET::EDGE_HEX_TO_GLOBAL);
		G.setFlag(ET::EDGE_GLOBAL_TO_HEX);

		const auto & global = G.getAll<N::Global>().at(0);

		for(const auto & player : G.getAll<N::Player>())
		{
			G.add(E::Global_To_Player::Create(global, player));
			G.add(E::Player_To_Global::Create(player, global));
		}

		for(const auto & unit : G.getAll<N::Unit>())
		{
			G.add(E::Global_To_Unit::Create(global, unit));
			G.add(E::Unit_To_Global::Create(unit, global));
		}

		for(const auto & hex : G.getAll<N::Hex>())
		{
			G.add(E::Global_To_Hex::Create(global, hex));
			G.add(E::Hex_To_Global::Create(hex, global));
		}
	}

	void AddEdges_Global_To_Action(Graph::Graph & G, EnumFlags<AT> & atFlags, const CPlayerBattleCallback & battle)
	{
		G.getFlags().require(ET::NODE_ACTION);
		for(int i = 0; i < EU(AT::_count); ++i)
			atFlags.require(AT(i));

		G.setFlag(ET::EDGE_GLOBAL_TO_ACTION);

		const auto & global = G.getAll<N::Global>().at(0);

		for(const auto & action : G.getAll<N::Action>())
			G.add(E::Global_To_Action::Create(global, action));
	}

	void AddEdges_Player_Owns_Unit(Graph::Graph & G, const CPlayerBattleCallback & battle)
	{
		G.getFlags().require(ET::NODE_PLAYER);
		G.getFlags().require(ET::NODE_UNIT);
		G.setFlag(ET::EDGE_PLAYER_OWNS_UNIT);
		G.setFlag(ET::EDGE_UNIT_OWNED_BY_PLAYER);

		for(const auto & unit : G.getAll<N::Unit>())
		{
			const auto & player = G.getByExtraIndex<N::Player>(unit->cstack.unitSide());
			G.add(E::Player_Owns_Unit::Create(player, unit));
			G.add(E::Unit_OwnedBy_Player::Create(unit, player));
		}
	}

	void AddEdges_Hex_Adjacent_Hex(Graph::Graph & G)
	{
		G.getFlags().require(ET::NODE_HEX);
		G.setFlag(ET::EDGE_HEX_ADJACENT_HEX);

		static const auto adjmap = InitAdjMap();
		const auto & hexes = G.getAll<N::Hex>();

		for(const auto & src : hexes)
		{
			for(const auto & dst : hexes)
			{
				auto it = adjmap.find({src->bhex.toInt(), dst->bhex.toInt()});
				if(it == adjmap.end())
					continue;
				G.add(E::Hex_Adjacent_Hex::Create(src, dst, it->second));
			}
		}
	}

	void AddEdges_Unit_ActsBefore_Unit(Graph::Graph & G, const CPlayerBattleCallback & battle)
	{
		G.getFlags().require(ET::NODE_UNIT);
		G.setFlag(ET::EDGE_UNIT_ACTS_BEFORE_UNIT);

		const auto matrix = Q::BuildActsBeforeMatrix(G, battle);
		const auto & nodes = G.getAll<N::Unit>();

		ASSERT(matrix.unique_count <= nodes.size(), "unique_count exceeds size of graph nodes");

		for(int i = 0; i < matrix.unique_count; ++i)
		{
			const auto & unit = matrix.unique_units.at(i);
			for(int j = 0; j < matrix.unique_count; ++j)
			{
				auto times = matrix.count[i][j];
				if(times == 0)
					continue;

				const auto & other = matrix.unique_units.at(j);
				G.add(E::Unit_ActsBefore_Unit::Create(unit, other, times));
			}
		}
	}

	void AddEdges_Unit_MeleeDmg_Unit(Graph::Graph & G, const CPlayerBattleCallback & battle, const State::GlobalStats & stats)
	{
		G.getFlags().require(ET::NODE_UNIT);
		G.setFlag(ET::EDGE_UNIT_MELEE_DMG_UNIT);

		auto pairs = std::unordered_set<std::pair<int, int>, PairHash>{};
		const auto & units = G.getAll<N::Unit>();

		for(const auto & unit : units)
		{
			const auto & stack = unit->cstack;
			bool isBerserk = checkBerserk(stack);

			for(const auto & other : units)
			{
				const auto & ostack = other->cstack;

				if(unit == other || ostack.isInvincible())
					continue;

				// XXX: removing MELEE_DMG edge may be problematic, as other edges rely on it
				// (e.g. shooter next to enemy stack will not see it as an enemy without MELEE_DMG edge...)
				// => make sure to explicitly check for nearby war machines when calculating blockers
				if(unit == other || ostack.isInvincible() || unit->attr(N::Unit::A::IS_WAR_MACHINE))
					continue;

				const auto pair = std::pair<int, int>{stack.unitId(), ostack.unitId()};
				auto [_, inserted] = pairs.emplace(pair);

				if(!inserted) // key already existed
					continue;

				if(ostack.unitSide() == stack.unitSide() && !isBerserk && !checkBerserk(ostack))
					continue;

				const auto states = SimulateAttackAction(battle, stack, ostack, false, false);
				const auto & state = states.a;
				const auto & ostate = states.b;

				ASSERT(state.cstack->unitId() == stack.unitId() && ostate.cstack->unitId() == ostack.unitId(), "SimulateAttackAction: fatal error");

				auto hpdiff_attacker = state.calcAvailableHealth() - stack.getAvailableHealth();
				auto hpdiff_defender = ostate.calcAvailableHealth() - ostack.getAvailableHealth();
				auto qtydiff_attacker = state.cstate->getCount() - stack.getCount();
				auto qtydiff_defender = ostate.cstate->getCount() - ostack.getCount();
				auto vdiff_attacker = unit->valueOne * qtydiff_attacker;
				auto vdiff_defender = other->valueOne * qtydiff_defender;

				G.add(
					E::Unit_MeleeDmg_Unit::Create(
						unit,
						other,
						{.vdiffAttacker = vdiff_attacker,
						 .vdiffDefender = vdiff_defender,
						 .hpdiffAttacker = static_cast<int>(hpdiff_attacker),
						 .hpdiffDefender = static_cast<int>(hpdiff_defender),
						 .battlefieldValue = stats.totalValue,
						 .battlefieldHp = stats.totalHp}
					)
				);
			}
		}
	}

	void AddEdges_Unit_ShootDmg_Unit(Graph::Graph & G, const CPlayerBattleCallback & battle, const State::GlobalStats & stats)
	{
		G.getFlags().require(ET::EDGE_UNIT_MELEE_DMG_UNIT);
		G.setFlag(ET::EDGE_UNIT_SHOOT_DMG_UNIT);

		for(const auto & unit : G.getAll<N::Unit>())
		{
			const auto & stack = unit->cstack;
			for(const auto & other : G.getAll<N::Unit>())
			{
				const auto & ostack = other->cstack;

				// RANGED_DMG edges use a subset of the MELEE_DMG edge nodes:
				// all ranged units can also melee, except for ballistas
				// Ballistas are shooters which do NOT have melee dmg edges to enemies
				// => handle separately
				bool isCandidate =
					(G.getEdgeBySrcDst<E::Unit_MeleeDmg_Unit>(unit, other, false) || (stack.isBallista() && stack.unitSide() != ostack.unitSide()));

				if(!isCandidate || !stack.canShoot() || ostack.isInvincible())
					continue;

				bool isOtherBlocked = G.getOneEdgeByDst<E::Unit_Blocks_Unit>(other, false) != nullptr;
				const auto states = SimulateAttackAction(battle, stack, ostack, true, isOtherBlocked);
				const auto & state = states.a;
				const auto & ostate = states.b;

				ASSERT(states.a.cstack->unitId() == stack.unitId() && states.b.cstack->unitId() == ostack.unitId(), "SimulateAttackAction: fatal error");

				auto hpdiff_attacker = state.calcAvailableHealth() - stack.getAvailableHealth();
				auto hpdiff_defender = ostate.calcAvailableHealth() - ostack.getAvailableHealth();
				auto qtydiff_attacker = state.cstate->getCount() - stack.getCount();
				auto qtydiff_defender = ostate.cstate->getCount() - ostack.getCount();
				auto vdiff_attacker = unit->valueOne * qtydiff_attacker;
				auto vdiff_defender = other->valueOne * qtydiff_defender;

				G.add(
					E::Unit_ShootDmg_Unit::Create(
						unit,
						other,
						{.vdiffAttacker = vdiff_attacker,
						 .vdiffDefender = vdiff_defender,
						 .hpdiffAttacker = static_cast<int>(hpdiff_attacker),
						 .hpdiffDefender = static_cast<int>(hpdiff_defender),
						 .battlefieldValue = stats.totalValue,
						 .battlefieldHp = stats.totalHp}
					)
				);
			}
		}
	}

	void AddEdges_Unit_Blocks_Unit(Graph::Graph & G)
	{
		G.getFlags().require(ET::EDGE_UNIT_SHOOT_DMG_UNIT);
		G.setFlag(ET::EDGE_UNIT_BLOCKS_UNIT);

		// BLOCKS edges use a subset of the RANGED_DMG edge nodes
		// (all blocked units must be ranged units)
		for(const auto & edge : G.getAll<E::Unit_ShootDmg_Unit>())
		{
			const auto & unit = edge->srcNode;
			const auto & cstack = unit->cstack;

			if(cstack.canShootBlocked())
				continue;

			const auto & other = edge->dstNode;
			const auto & ostack = other->cstack;

			// XXX: VCMI considers a shooter blocked by an ally only if the shooter (not the ally) is berserk
			// 		It makes sense to become blocked if the ally is berserk, but not sure what original H3 behaviour is.
			// XXX: what about hypnotize?
			if(cstack.unitSide() == ostack.unitSide() && !checkBerserk(cstack))
				continue;

			for(const auto & bhex : cstack.getSurroundingHexes())
			{
				if(ostack.coversPos(bhex))
				{
					G.add(E::Unit_Blocks_Unit::Create(unit, other));
					break;
				}
			}
		}
	}

	void AddEdges_Unit_Occupies_Hex(Graph::Graph & G)
	{
		G.getFlags().require(ET::NODE_UNIT);
		G.getFlags().require(ET::NODE_HEX);
		G.setFlag(ET::EDGE_UNIT_OCCUPIES_HEX);
		G.setFlag(ET::EDGE_HEX_OCCUPIED_BY_UNIT);

		for(const auto & unit : G.getAll<N::Unit>())
		{
			const auto & cstack = unit->cstack;
			for(const auto & bhex : cstack.getHexes())
			{
				if(!bhex.isAvailable())
					continue;
				const auto & hex = G.getByExtraIndex<N::Hex>(bhex.toInt());
				ASSERT(hex != nullptr, "hex not found: " + std::to_string(bhex.toInt()));
				G.add(E::Unit_Occupies_Hex::Create(unit, hex));
				G.add(E::Hex_OccupiedBy_Unit::Create(hex, unit));
			}
		}
	}

	void AddMoveAndDefendActions(Graph::Graph & G, EnumFlags<AT> & atFlags)
	{
		G.getFlags().require(ET::NODE_UNIT);
		G.getFlags().require(ET::NODE_HEX);

		// XXX: these edges will be set only for MOVE actions, however
		// they are required the other 3 action types (AMOVE, SHOOT, WAIT)
		atFlags.set(AT::MOVE);
		atFlags.set(AT::DEFEND);
		G.setFlag(ET::NODE_ACTION);
		G.setFlag(ET::EDGE_ACTION_BY_UNIT);
		G.setFlag(ET::EDGE_UNIT_HAS_ACTION);
		G.setFlag(ET::EDGE_ACTION_ENDS_AT_HEX);
		G.setFlag(ET::EDGE_HEX_IS_END_OF_ACTION);

		for(const auto & unit : G.getAll<N::Unit>())
		{
			const auto & stack = unit->cstack;

			// XXX: disabling this check as blinded/paralyzed/etc. units should
			// 		still have their actions in the graph.
			// 		Whether the unit can actually perform this action can be
			// 		inferred from the IS_SLEEPING attribute or ACTS_BEFORE edge.
			// if (!stack.canMove())
			// 	continue;

			for(const auto & hex : G.getAll<N::Hex>())
			{
				if(unit->distances.at(hex->bhex.toInt()) > unit->speed)
					continue;

				auto stackhexes = std::vector<HexPtr>{};
				for(const auto & stackbhex : stack.getHexes(hex->bhex))
					if(stackbhex.isAvailable())
						stackhexes.emplace_back(G.getByExtraIndex<N::Hex>(stackbhex.toInt()));

				const auto action = N::Action::Create(
					{.actionType = (hex->bhex == stack.getPosition() ? AT::DEFEND : AT::MOVE), .by = unit, .target = nullptr, .endsAt = stackhexes, .flags = {}}
				);

				G.add(action);
				G.add(E::Action_By_Unit::Create(action, unit));
				G.add(E::Unit_Has_Action::Create(unit, action));

				bool isRear = false; // getHexes always returns primary hex first
				for(const auto & stackhex : stackhexes)
				{
					G.add(E::Action_EndsAt_Hex::Create(action, stackhex, isRear));
					G.add(E::Hex_IsEndOf_Action::Create(stackhex, action, isRear));
					isRear = true;
				}
			}
		}
	}

	void AddMoveActionEdges_Action_Blocks_Unit(Graph::Graph & G, EnumFlags<AT> & atFlags, const CPlayerBattleCallback & battle)
	{
		G.getFlags().require(ET::NODE_ACTION);
		atFlags.requireExclusive({AT::DEFEND, AT::MOVE});
		G.getFlags().require(ET::NODE_HEX);
		G.getFlags().require(ET::EDGE_UNIT_OCCUPIES_HEX);
		G.getFlags().require(ET::EDGE_UNIT_SHOOT_DMG_UNIT);

		G.setFlag(ET::EDGE_ACTION_BLOCKS_UNIT);
		G.setFlag(ET::EDGE_UNIT_BLOCKED_BY_ACTION);

		for(const auto & action : G.getAll<N::Action>())
		{
			assert(action->actionType == AT::MOVE || action->actionType == AT::DEFEND);
			const auto & unit = action->by;
			const auto & stack = unit->cstack;
			const auto & hex = action->endsAt.at(0);

			// A wide adjacent unit may have already been inserted
			// The edge is action-blocks-unit (and not action-blocks-hex)
			// => don't add it twice
			auto adjunits = std::unordered_set<UnitPtr>{};

			for(const auto & adjbhex : stack.getSurroundingHexes(hex->bhex))
			{
				if(!adjbhex.isAvailable())
					continue;

				const auto & adjhex = G.getByExtraIndex<N::Hex>(adjbhex.toInt());
				const auto & adjunit = G.getOneEdgeSrcByDst<E::Unit_Occupies_Hex>(adjhex, false);

				if(!adjunit)
					continue;

				if(adjunits.contains(adjunit) || adjunit->cstack.canShootBlocked())
					continue;

				if(!G.getEdgeBySrcDst<E::Unit_ShootDmg_Unit>(adjunit, unit, false))
					continue;

				adjunits.emplace(adjunit);
				G.add(E::Action_Blocks_Unit::Create(action, adjunit));
				G.add(E::Unit_BlockedBy_Action::Create(adjunit, action));
			}
		}
	}

	void AddMoveActionEdges_Unit_BecomesMeleeThreatAfter_Action(Graph::Graph & G, EnumFlags<AT> & atFlags)
	{
		G.getFlags().require(ET::NODE_ACTION);
		atFlags.requireExclusive({AT::DEFEND, AT::MOVE});
		G.getFlags().require(ET::EDGE_UNIT_MELEE_DMG_UNIT);
		G.getFlags().require(ET::EDGE_UNIT_ACTS_BEFORE_UNIT);
		G.getFlags().require(ET::EDGE_ACTION_ENDS_AT_HEX);

		// See note in AddMoveActions()
		G.setFlag(ET::EDGE_UNIT_BECOMES_MELEE_THREAT_AFTER_ACTION);

		// Plan:
		// For each MOVE action, find units which:
		// 	1. are enemies to the actor (i.e. with "MeleeDmg" edge to it)
		//  2. will act after it (i.e. "ActsBefore" actor->them has times==1)
		//  3. can move such that they will end up occupying
		// 		at least of the hexes around the actor's new position
		//  4. the enemy move doesn't overlap with the actor's own move
		for(const auto & action : G.getAll<N::Action>())
		{
			assert(action->actionType == AT::MOVE || action->actionType == AT::DEFEND);
			const auto & unit = action->by;
			const auto & stack = unit->cstack;
			const auto & hex = action->endsAt.at(0);
			const auto & stackhexes = stack.getHexes(hex->bhex);

			for(const auto & ounit : G.getAllEdgesSrcByDst<E::Unit_MeleeDmg_Unit>(unit))
			{
				const auto & actsBefore = G.getEdgeBySrcDst<E::Unit_ActsBefore_Unit>(unit, ounit, false);
				if(actsBefore && actsBefore->times > 1)
					continue;

				const auto & adjbhexes = stack.getSurroundingHexes(hex->bhex);
				for(const auto & oaction : G.getAllEdgesSrcByDst<E::Action_By_Unit>(ounit))
				{
					// must check for overlaps to ensure no exposure is set if
					// the hypothetical endsAt hexes of both stacks overlap
					// e.g. both our and enemy stack can move onto the "x" hexes
					// 		BUT if we move there, the enemy would be unable to
					// 		=> we would NOT be exposed
					// 		(x=endsAt, @=obstacle)
					//
					// . . @ @ @ . . . . .
					//  @ . x x @ . 1 1 .
					// . . @ @ @ . . . . .
					//  . . . . . 2 2 . .
					// . . . . . . . . . .
					bool candidate = false;
					bool overlap = false;
					for(const auto & ohex : oaction->endsAt)
					{
						candidate |= adjbhexes.contains(ohex->bhex);
						overlap |= stackhexes.contains(ohex->bhex);
					}

					if(candidate && !overlap)
					{
						G.add(E::Unit_BecomesMeleeThreatAfter_Action::Create(ounit, action));
						break;
					}
				}
			}
		}
	}

	// This is copied from CBattleInfoCallback::battleHasDistancePenalty
	// but it is changed to accept a hypothetical shooter position
	// +a few optimizations to bonus calls
	bool battleHasDistancePenalty_CUSTOM(const IBonusBearer * shooter, const BattleHex & shooterPosition, const BattleHexArray & targetHexes)
	{
		if(shooter->hasBonusOfType(BonusType::NO_DISTANCE_PENALTY))
			return false;

		int range = GameConstants::BATTLE_SHOOTING_PENALTY_DISTANCE;

		auto bonus = shooter->getBonusesOfType(BonusType::LIMITED_SHOOTING_RANGE)->getFirst(Selector::all);
		if(bonus != nullptr && bonus->parameters)
			range = bonus->parameters->toNumber();

		for(const auto & hex : targetHexes)
			if(BattleHex::getDistance(shooterPosition, hex) <= range)
				//If any hex of target creature is within range, there is no penalty
				return false;

		return true;
	}

	float
	CalcShootDmgMult(const CPlayerBattleCallback & battle, const CStack & attackerStack, const BattleHex & attackerPos, const BattleHexArray & targetHexes)
	{
		float mult = 1;

		// XXX: using custom version of battleHasDistancePenalty where we can specify defender hex
		if(battleHasDistancePenalty_CUSTOM(&attackerStack, attackerPos, targetHexes))
			mult *= 0.5;
		if(battle.battleHasWallPenalty(&attackerStack, attackerPos, targetHexes[0]))
			mult *= 0.5;

		return mult;
	};

	void AddMoveActionEdges_Unit_BecomesShootThreatAfter_Action(Graph::Graph & G, EnumFlags<AT> & atFlags, const CPlayerBattleCallback & battle)
	{
		G.getFlags().require(ET::NODE_ACTION);
		atFlags.requireExclusive({AT::DEFEND, AT::MOVE});
		G.getFlags().require(ET::NODE_ACTION);
		G.getFlags().require(ET::EDGE_UNIT_SHOOT_DMG_UNIT);
		G.getFlags().require(ET::EDGE_UNIT_BLOCKS_UNIT);
		G.getFlags().require(ET::EDGE_ACTION_BLOCKS_UNIT);

		// See note in AddMoveActions()
		G.setFlag(ET::EDGE_UNIT_BECOMES_SHOOT_THREAT_AFTER_ACTION);

		// Plan:
		// For each MOVE action, find units which:
		// 	1. are shooter enemies for the actor (i.e. with "RangedDmg" edge to it)
		// 	2. either:
		//		a. are currently unblocked and will remain unblocked after the move
		// 	    b. are currently blocked *only* by the actor, but will become
		// 			unblocked after the move

		for(const auto & action : G.getAll<N::Action>())
		{
			assert(action->actionType == AT::MOVE || action->actionType == AT::DEFEND);
			const auto & unit = action->by;
			const auto & stack = unit->cstack;
			const auto & hex = action->endsAt.at(0);

			for(const auto & ounit : G.getAllEdgesSrcByDst<E::Unit_ShootDmg_Unit>(unit))
			{
				const auto & blockers = G.getAllEdgesSrcByDst<E::Unit_Blocks_Unit>(ounit);

				auto numBlockers = std::ranges::distance(blockers);
				if(numBlockers > 1)
					continue; // no threat (already blocked by someone else)

				auto actorIsBlocker = std::ranges::find(blockers, unit) != blockers.end();
				if(numBlockers == 1 && !actorIsBlocker)
					continue; // no threat (already blocked by someone else)

				bool willBlock = G.getEdgeBySrcDst<E::Action_Blocks_Unit>(action, ounit, false) != nullptr;
				if(willBlock)
					continue; // no threat (will become blocked after the move)

				const auto & ostack = ounit->cstack;

				float mult = CalcShootDmgMult(battle, ostack, ostack.getPosition(), stack.getHexes(hex->bhex));
				G.add(E::Unit_BecomesShootThreatAfter_Action::Create(ounit, action, mult));
			}
		}
	}

	// NOLINTNEXTLINE(readability-function-cognitive-complexity)
	void AddMoveActionEdges_UnitAndHex_BecomesMeleeTargetAfter_Action(Graph::Graph & G, EnumFlags<AT> & atFlags, const CPlayerBattleCallback & battle)
	{
		G.getFlags().require(ET::EDGE_ACTION_ENDS_AT_HEX);
		G.getFlags().require(ET::EDGE_UNIT_MELEE_DMG_UNIT);
		G.setFlag(ET::EDGE_UNIT_BECOMES_MELEE_TARGET_AFTER_ACTION);
		G.setFlag(ET::EDGE_HEX_BECOMES_MELEE_TARGET_AFTER_ACTION); // active actions only

		// Several actions by the same unit may end on the same hex
		// => can re-use previously calculated reachability
		// Key: {unit_id, hex_id}
		auto distcache = std::unordered_map<std::pair<int, int>, FastBFS::Distances, PairHash>{};

		for(const auto & edge : G.getAll<E::Action_EndsAt_Hex>())
		{
			if(edge->isRear)
				continue;

			const auto & action = edge->srcNode;
			const auto & hex = edge->dstNode;

			assert(action->by);

			const auto & unit = action->by;
			const auto & stack = unit->cstack;
			const auto & bhex = hex->bhex;
			const auto & bhex0 = stack.getPosition();
			const auto & cachekey = std::pair<int, int>(unit->cstack.unitId(), hex->bhex.toInt());

			auto it = distcache.find(cachekey);
			if(it == distcache.end())
				it =
					distcache
						.try_emplace(
							cachekey,
							bhex == bhex0 ? unit->distances : G.getFastBFS().run(bhex0, bhex, stack.unitSide(), unit->isFlying, stack.doubleWide(), unit->speed)
						)
						.first;

			const auto & distances = it->second;

			for(const auto & ounit : G.getAllEdgesDstBySrc<E::Unit_MeleeDmg_Unit>(unit))
			{
				if(G.getEdgeBySrcDst<E::Unit_BecomesMeleeTargetAfter_Action>(ounit, action, false))
					continue;

				const auto & ostack = ounit->cstack;

				for(const auto & adjbhex : ostack.getAttackableHexes(&stack))
				{
					if(distances.at(adjbhex.toInt()) > unit->speed)
						continue;

					G.add(E::Unit_BecomesMeleeTargetAfter_Action::Create(ounit, action));
					break;
				}
			}

			// EDGE_ENABLES_MELEE_AT_HEX is only added for active actions
			// because the number of edges combinatorially explodes
			if(!action->isActive)
				continue;

			for(const auto & ohex : G.getAll<N::Hex>())
			{
				// XXX: this is WRONG in case attacker is wide
				// Will need to use meleeAttackHexes or similar logic
				// auto x1 = bhex.getNeighbouringTilesDoubleWide(BattleSide::LEFT_SIDE);
				// auto x2 = bhex.getNeighbouringTilesDoubleWide(BattleSide::RIGHT_SIDE);
				auto otherside = stack.unitSide() == BattleSide::LEFT_SIDE ? BattleSide::RIGHT_SIDE : BattleSide::LEFT_SIDE;
				for(const auto & adjbhex : G.getNearbyPositions().get(ohex->bhex, stack.unitSide(), otherside, stack.doubleWide(), false))
				{
					if(distances.at(adjbhex.toInt()) > unit->speed)
						continue;

					G.add(E::Hex_BecomesMeleeTargetAfter_Action::Create(ohex, action));
					break;
				}
			}
		}
	}

	void AddMoveActionEdges_UnitAndHex_BecomesShootTargetAfter_Action(Graph::Graph & G, EnumFlags<AT> & atFlags, const CPlayerBattleCallback & battle)
	{
		G.getFlags().require(ET::NODE_ACTION);
		atFlags.requireExclusive({AT::DEFEND, AT::MOVE});
		G.getFlags().require(ET::EDGE_UNIT_OCCUPIES_HEX);
		G.getFlags().require(ET::EDGE_UNIT_MELEE_DMG_UNIT);
		G.getFlags().require(ET::EDGE_UNIT_SHOOT_DMG_UNIT);

		// See note in AddMoveActions()
		G.setFlag(ET::EDGE_UNIT_BECOMES_SHOOT_TARGET_AFTER_ACTION);
		G.setFlag(ET::EDGE_HEX_BECOMES_SHOOT_TARGET_AFTER_ACTION); // active actions only

		// Plan:
		// For each MOVE action:
		//  1. Check if the actor can shoot
		//  1. Check if the actor will be blocked after the move
		// 	2. Find units with "ShootDmg" edge from the actor
		//
		for(const auto & action : G.getAll<N::Action>())
		{
			const auto & unit = action->by;
			const auto & stack = unit->cstack;

			if(!stack.canShoot())
				continue;

			const auto & hex = action->endsAt.at(0);

			auto willBeBlocked = [&G, &unit, &hex]()
			{
				for(const auto & adjbhex : unit->cstack.getSurroundingHexes(hex->bhex))
				{
					if(!adjbhex.isAvailable())
						continue;

					const auto & adjhex = G.getByExtraIndex<N::Hex>(adjbhex.toInt());
					const auto & ounit = G.getOneEdgeSrcByDst<E::Unit_Occupies_Hex>(adjhex, false);

					if(!ounit)
						continue;

					// Some units (e.g. war machines) have no melee dmg edges to enemies
					// but can still block them from shooting
					// => check the reverse melee dmg edge as well
					if(G.getEdgeBySrcDst<E::Unit_MeleeDmg_Unit>(ounit, unit, false) || G.getEdgeBySrcDst<E::Unit_MeleeDmg_Unit>(unit, ounit, false))
						return true;
				}

				return false;
			};

			if(!stack.canShootBlocked() && willBeBlocked())
				continue;

			for(const auto & ounit : G.getAllEdgesDstBySrc<E::Unit_ShootDmg_Unit>(unit))
			{
				const auto & ostack = ounit->cstack;
				float mult = CalcShootDmgMult(battle, stack, hex->bhex, ostack.getHexes());
				G.add(E::Unit_BecomesShootTargetAfter_Action::Create(ounit, action, mult));
			}

			// EDGE_ENABLES_SHOOT_AT_HEX is only added for active actions
			// because the number of edges combinatorially explodes
			if(!action->isActive)
				continue;

			for(const auto & ohex : G.getAll<N::Hex>())
			{
				float mult = CalcShootDmgMult(battle, stack, hex->bhex, {ohex->bhex});
				G.add(E::Hex_BecomesShootTargetAfter_Action::Create(ohex, action, mult));
			}
		}
	}

	template<typename T>
	void WithSnapshot(const auto & range, const auto & func)
	{
		// Iterate from a vector as new nodes will be added to the index
		for(const auto & item : std::vector<std::shared_ptr<const T>>(range.begin(), range.end()))
			func(item);
	};

	void CloneActionEdges(Graph::Graph & G, const ActionPtr & oldAction, const ActionPtr & newAction, const std::unordered_set<ET> & ignore = {})
	{
		// Iterator over a *copy* of the index result
		auto iterateEdgesWithSrcAction = [&G, &oldAction]<typename Edge>(const auto & func)
		{
			WithSnapshot<Edge>(G.getAllEdgesBySrc<Edge>(oldAction), func);
		};

		auto iterateEdgesWithDstAction = [&G, &oldAction]<typename Edge>(const auto & func)
		{
			WithSnapshot<Edge>(G.getAllEdgesByDst<Edge>(oldAction), func);
		};

		// Most edges are simple edges with just a oldAction and newAction
		// => convenience function for cloning those
		auto cloneEdgesWithSrcAction = [&G, &newAction, &iterateEdgesWithSrcAction]<typename Edge>()
		{
			iterateEdgesWithSrcAction.template operator()<Edge>(
				[&G, &newAction](const auto & e)
				{
					G.add(Edge::Create(newAction, e->dstNode));
				}
			);
		};

		// Most edges are simple edges with just a oldAction and newAction
		// => convenience function for cloning those
		auto cloneEdgesWithDstAction = [&G, &newAction, &iterateEdgesWithDstAction]<typename Edge>()
		{
			iterateEdgesWithDstAction.template operator()<Edge>(
				[&G, &newAction](const auto & e)
				{
					G.add(Edge::Create(e->srcNode, newAction));
				}
			);
		};

		for(int i = 0; i < EU(ET::_count); ++i)
		{
			if(ignore.contains(ET(i)))
				continue;

			switch(ET(i))
			{
				case ET::EDGE_ACTION_BY_UNIT:
					cloneEdgesWithSrcAction.template operator()<E::Action_By_Unit>();
					break;
				case ET::EDGE_UNIT_HAS_ACTION:
					cloneEdgesWithDstAction.template operator()<E::Unit_Has_Action>();
					break;
				case ET::EDGE_ACTION_ENDS_AT_HEX:
					iterateEdgesWithSrcAction.template operator()<E::Action_EndsAt_Hex>(
						[&G, &newAction](const auto & e)
						{
							G.add(E::Action_EndsAt_Hex::Create(newAction, e->dstNode, e->isRear));
						}
					);
					break;
				case ET::EDGE_HEX_IS_END_OF_ACTION:
					iterateEdgesWithDstAction.template operator()<E::Hex_IsEndOf_Action>(
						[&G, &newAction](const auto & e)
						{
							G.add(E::Hex_IsEndOf_Action::Create(e->srcNode, newAction, e->isRear));
						}
					);
					break;
				case ET::EDGE_ACTION_BLOCKS_UNIT:
					cloneEdgesWithSrcAction.template operator()<E::Action_Blocks_Unit>();
					break;
				case ET::EDGE_UNIT_BLOCKED_BY_ACTION:
					cloneEdgesWithDstAction.template operator()<E::Unit_BlockedBy_Action>();
					break;
				case ET::EDGE_UNIT_BECOMES_MELEE_THREAT_AFTER_ACTION:
					cloneEdgesWithDstAction.template operator()<E::Unit_BecomesMeleeThreatAfter_Action>();
					break;
				case ET::EDGE_UNIT_BECOMES_SHOOT_THREAT_AFTER_ACTION:
					iterateEdgesWithDstAction.template operator()<E::Unit_BecomesShootThreatAfter_Action>(
						[&G, &newAction](const auto & e)
						{
							G.add(E::Unit_BecomesShootThreatAfter_Action::Create(e->srcNode, newAction, e->mult));
						}
					);
					break;
				case ET::EDGE_UNIT_BECOMES_MELEE_TARGET_AFTER_ACTION:
					cloneEdgesWithDstAction.template operator()<E::Unit_BecomesMeleeTargetAfter_Action>();
					break;
				case ET::EDGE_UNIT_BECOMES_SHOOT_TARGET_AFTER_ACTION:
					iterateEdgesWithDstAction.template operator()<E::Unit_BecomesShootTargetAfter_Action>(
						[&G, &newAction](const auto & e)
						{
							G.add(E::Unit_BecomesShootTargetAfter_Action::Create(e->srcNode, newAction, e->mult));
						}
					);
					break;
				case ET::EDGE_HEX_BECOMES_MELEE_TARGET_AFTER_ACTION:
					cloneEdgesWithDstAction.template operator()<E::Hex_BecomesMeleeTargetAfter_Action>();
					break;
				case ET::EDGE_HEX_BECOMES_SHOOT_TARGET_AFTER_ACTION:
					iterateEdgesWithDstAction.template operator()<E::Hex_BecomesShootTargetAfter_Action>(
						[&G, &newAction](const auto & e)
						{
							G.add(E::Hex_BecomesShootTargetAfter_Action::Create(e->srcNode, newAction, e->mult));
						}
					);
					break;
				case ET::EDGE_GLOBAL_TO_ACTION:
					cloneEdgesWithDstAction.template operator()<E::Global_To_Action>();
					break;
				case ET::EDGE_UNIT_IS_MELEED_BY_ACTION:
					iterateEdgesWithDstAction.template operator()<E::Unit_IsMeleedBy_Action>(
						[&G, &newAction](const auto & e)
						{
							G.add(E::Unit_IsMeleedBy_Action::Create(e->srcNode, newAction, e->isPrimaryTarget));
						}
					);
					break;
				case ET::EDGE_UNIT_IS_SHOT_BY_ACTION:
					iterateEdgesWithDstAction.template operator()<E::Unit_IsShotBy_Action>(
						[&G, &newAction](const auto & e)
						{
							G.add(E::Unit_IsShotBy_Action::Create(e->srcNode, newAction, e->isPrimaryTarget));
						}
					);
					break;

				// Nothing to add for those
				case ET::NODE_GLOBAL:
				case ET::NODE_PLAYER:
				case ET::NODE_UNIT:
				case ET::NODE_HEX:
				case ET::NODE_ACTION:
				case ET::EDGE_GLOBAL_TO_PLAYER:
				case ET::EDGE_PLAYER_TO_GLOBAL:
				case ET::EDGE_GLOBAL_TO_UNIT:
				case ET::EDGE_UNIT_TO_GLOBAL:
				case ET::EDGE_GLOBAL_TO_HEX:
				case ET::EDGE_HEX_TO_GLOBAL:
				case ET::EDGE_PLAYER_OWNS_UNIT:
				case ET::EDGE_UNIT_OWNED_BY_PLAYER:
				case ET::EDGE_HEX_ADJACENT_HEX:
				case ET::EDGE_UNIT_ACTS_BEFORE_UNIT:
				case ET::EDGE_UNIT_MELEE_DMG_UNIT:
				case ET::EDGE_UNIT_SHOOT_DMG_UNIT:
				case ET::EDGE_UNIT_BLOCKS_UNIT:
				case ET::EDGE_UNIT_OCCUPIES_HEX:
				case ET::EDGE_HEX_OCCUPIED_BY_UNIT:
					break;
				default:
					throwf("Unexpected edge type: {}", i);
			}
			static_assert(static_cast<int>(ET::_count) == 35);
		}
	};

	void AddAmoveAction(Graph::Graph & G, const ActionPtr & move, const CPlayerBattleCallback & battle)
	{
		const auto & unit = move->by;
		const auto & hex = move->endsAt.at(0);

		// XXX: Special case when walking into moat/quicksand (see _notes/moats.txt)
		bool willWalkIntoMoat =
			(hex->bhex != unit->cstack.getPosition() && !unit->isFlying
			 && std::ranges::any_of(
				 move->endsAt,
				 [](const HexPtr & hex)
				 {
					 return hex->attr(N::Hex::A::IS_STOPPING);
				 }
			 ));

		if(willWalkIntoMoat)
			return;

		// A wide adjacent unit may have already been inserted
		// The edge is action-IsMeleedBy-unit (and not action-IsMeleedBy-hex)
		// => don't add it twice
		auto ounits = std::unordered_set<UnitPtr>{};

		for(const auto & adjbhex : unit->cstack.getSurroundingHexes(hex->bhex))
		{
			if(!adjbhex.isAvailable())
				continue;

			const auto & adjhex = G.getByExtraIndex<N::Hex>(adjbhex.toInt());
			const auto & ounit = G.getOneEdgeSrcByDst<E::Unit_Occupies_Hex>(adjhex, false);

			if(!ounit)
				continue;

			if(!G.getEdgeBySrcDst<E::Unit_MeleeDmg_Unit>(unit, ounit, false))
				continue;

			auto [it, inserted] = ounits.emplace(ounit);
			if(!inserted)
				continue;

			assert(CStack::isMeleeAttackPossible(&unit->cstack, &ounit->cstack, hex->bhex));

			const auto amove = N::Action::Create({.actionType = AT::AMOVE, .by = unit, .target = ounit, .endsAt = move->endsAt, .flags = {}});

			G.add(amove);
			G.add(E::Unit_IsMeleedBy_Action::Create(ounit, amove, true));

			// Melee AoE attacks, e.g. dragons, hydras
			const auto & stack = unit->cstack;
			const auto & ostack = ounit->cstack;

			// XXX: The code in getAttackedCreatures (for melee attacks) uses
			//      destination hex ONLY for obtaining the defending stack
			// 		(which is great because the attack targets a unit, not a hex)
			const auto & [targets, _] = battle.getAttackedCreatures(&stack, ostack.getPosition(), false, hex->bhex);

			for(const auto & tstack : targets)
			{
				if(tstack == &ostack)
					// XXX: getAttackedCreatures has some stupid logic which does not
					//      return the creature standing on the target hex.
					//      HOWEVER, if the same creature occupies an AoE hex, it is returned
					//      => make sure to not duplicate it.
					continue;

				const auto & tunit = G.getByExtraIndex<N::Unit>(tstack->unitId());
				G.add(E::Unit_IsMeleedBy_Action::Create(tunit, amove, false));
			}

			CloneActionEdges(G, move, amove);

			if(stack.hasBonusOfType(BonusType::RETURN_AFTER_STRIKE))
			{
				// A duplicate action, with different flags and endsAt hexes
				auto flags = N::Action::Flags{};
				flags.set(EU(N::Action::Flag::RETURN_AFTER_STRIKE));

				const auto amove2 =
					N::Action::Create({.actionType = amove->actionType, .by = amove->by, .target = amove->target, .endsAt = amove->endsAt, .flags = flags});

				G.add(amove2);
				CloneActionEdges(G, amove, amove2, {ET::EDGE_ACTION_ENDS_AT_HEX});

				for(const auto & hex : G.getAllEdgesDstBySrc<E::Unit_Occupies_Hex>(amove->by))
				{
					bool isRear = hex->bhex != amove->by->cstack.getPosition();
					G.add(E::Action_EndsAt_Hex::Create(amove2, hex, isRear));
				}
			}
		}
	}

	void AddShootAction(Graph::Graph & G, const ActionPtr & defend, const CPlayerBattleCallback & battle)
	{
		// Iterate from a snapshot as new nodes will be added to the index (via clone)
		const auto & range = G.getAllEdgesSrcByDst<E::Unit_BecomesShootTargetAfter_Action>(defend);
		const auto edges = std::vector(range.begin(), range.end());

		for(const auto & ounit : edges)
		{
			const auto & unit = defend->by;

			const auto shoot = N::Action::Create({.actionType = AT::SHOOT, .by = unit, .target = ounit, .endsAt = defend->endsAt, .flags = {}});

			G.add(shoot);
			G.add(E::Unit_IsShotBy_Action::Create(ounit, shoot, true));

			// AoE attacks - e.g. dragon breath
			const auto & stack = unit->cstack;
			const auto & ostack = ounit->cstack;
			const auto & ohex = G.getByExtraIndex<N::Hex>(ounit->cstack.getPosition().toInt());

			// XXX: The code in getAttackedCreatures (for ranged attacks)
			// 		does NOT consider AoE from SPELL_LIKE_ATTACK
			//      (which is all the AoE in vanilla H3/SoD: Fireball, Death Cloud)
			// 		Call it, in case some mod adds a regular non-spell like ranged AoE,
			// 		but make sure to handle the spell-like attacks separately.
			auto [targets, _] = battle.getAttackedCreatures(&stack, ohex->bhex, true, stack.getPosition());

			// Handle spell-like attacks
			for(const auto & bonus : *stack.getBonusesOfType(BonusType::SPELL_LIKE_ATTACK))
			{
				// Stolen from CBattleInfoCallback::estimateSpellLikeAttackDamage
				const auto * spell = bonus->subtype.as<SpellID>().toSpell();
				const auto & proxy = spells::ProxyCaster(&stack);
				const auto & params = spells::BattleCast(&battle, &proxy, spells::Mode::PASSIVE, spell);
				const auto mech = std::unique_ptr<spells::Mechanics>(spell->battleMechanics(&params));
				if(!mech)
					break;
				auto aim = spells::Target{};
				aim.emplace_back(ohex->bhex);
				for(const auto & tstack : mech->getAffectedStacks(aim))
					targets.emplace(tstack);
			}

			for(const auto & tstack : targets)
			{
				if(tstack == &ostack)
					// see note in AddAmoveAction
					continue;

				const auto & tunit = G.getByExtraIndex<N::Unit>(tstack->unitId());
				G.add(E::Unit_IsShotBy_Action::Create(tunit, shoot, false));
			}

			CloneActionEdges(G, defend, shoot);
		}
	}

	void AddWaitAction(Graph::Graph & G, const ActionPtr & defend)
	{
		if(defend->by->cstack.waitedThisTurn)
			return;

		const auto wait = N::Action::Create({.actionType = AT::WAIT, .by = defend->by, .target = nullptr, .endsAt = defend->endsAt, .flags = {}});

		G.add(wait);
		CloneActionEdges(G, defend, wait);
	}

	void AddOtherActions(Graph::Graph & G, EnumFlags<AT> & atFlags, const CPlayerBattleCallback & battle)
	{
		// All MOVE actions with all their edges must be available here
		// except for IsMeleedBy and IsShotBy edges which are for AMOVE only
		atFlags.requireExclusive({AT::DEFEND, AT::MOVE});
		G.getFlags().require(ET::NODE_ACTION);
		G.getFlags().require(ET::EDGE_UNIT_OCCUPIES_HEX);
		G.getFlags().require(ET::EDGE_ACTION_BY_UNIT);
		G.getFlags().require(ET::EDGE_ACTION_BLOCKS_UNIT);
		G.getFlags().require(ET::EDGE_ACTION_ENDS_AT_HEX);
		G.getFlags().require(ET::EDGE_UNIT_BECOMES_MELEE_THREAT_AFTER_ACTION);
		G.getFlags().require(ET::EDGE_UNIT_BECOMES_SHOOT_THREAT_AFTER_ACTION);
		G.getFlags().require(ET::EDGE_UNIT_BECOMES_MELEE_TARGET_AFTER_ACTION);
		G.getFlags().require(ET::EDGE_UNIT_BECOMES_SHOOT_TARGET_AFTER_ACTION);

		G.setFlag(ET::EDGE_UNIT_IS_MELEED_BY_ACTION);
		G.setFlag(ET::EDGE_UNIT_IS_SHOT_BY_ACTION);
		atFlags.set(AT::AMOVE);
		atFlags.set(AT::SHOOT);
		atFlags.set(AT::WAIT);

		// These are needed for SHOOT actions
		auto defendmoves = std::unordered_map<UnitPtr, const ActionPtr>{};

		// Iterate from a snapshot as new actions will be added to the index
		const auto & range = G.getAll<N::Action>();
		const auto actions = std::vector(range.begin(), range.end());
		for(const auto & move : actions)
		{
			assert(move->actionType == AT::MOVE || move->actionType == AT::DEFEND);
			AddAmoveAction(G, move, battle);
			if(move->actionType == AT::DEFEND)
			{
				assert(move->by->cstack.getPosition() == move->endsAt.at(0)->bhex);
				defendmoves.try_emplace(move->by, move);
			}
		};

		for(const auto & unit : G.getAll<N::Unit>())
		{
			const auto & defendhex = G.getByExtraIndex<N::Hex>(unit->cstack.getPosition().toInt());
			const auto & defend = defendmoves.at(unit);
			AddShootAction(G, defend, battle);
			AddWaitAction(G, defend);
		}
	}
}

State::State(int version, const std::string & colorname, const CPlayerBattleCallback & battle)
	: version_(version), battle(battle), colorname(colorname), side(battle.battleGetMySide()), startStats(CalcGlobalStats(battle)), lastStats(startStats)
{
}

void State::onBattleStacksAttacked(const std::vector<BattleStackAttacked> & bsa)
{
	if(!G)
		// Ignore logs until our first turn starts
		return;

	auto cstacks = battle.battleGetStacks();

	for(const auto & elem : bsa)
	{
		const auto * defender = battle.battleGetStackByID(static_cast<int>(elem.stackAttacked), false);
		const auto * attacker = battle.battleGetStackByID(static_cast<int>(elem.attackerID), false);

		if(!defender)
		{
			logAi->error("MMAI: received BattleStackAttacked with invalid stackAttacked: " + std::to_string(elem.stackAttacked));
			continue;
		}

		auto bf_valueNow = lastStats.leftValue + lastStats.rightValue;
		auto bf_hpNow = lastStats.leftHp + lastStats.rightHp;
		auto value = elem.killedAmount * N::Unit::GetValue(defender->unitType());

		attackLogs.emplace_back(
			// attacker and/or defender CStack may be missing in G
			// (e.g. resurrected after G was constructed)
			elem,
			attacker ? G->getByExtraIndex<N::Unit>(attacker->unitId(), false) : nullptr,
			G->getByExtraIndex<N::Unit>(defender->unitId(), false),
			static_cast<int>(elem.damageAmount),
			static_cast<int>(1000 * elem.damageAmount / bf_hpNow),
			static_cast<int>(elem.killedAmount),
			static_cast<int>(value),
			static_cast<int>(1000 * value / bf_valueNow)
		);
	}
}

void State::onBattleTriggerEffect(const BattleTriggerEffect & bte)
{
	if(bte.effect != BonusType::MORALE)
		return;

	isMorale = true;
}

void State::onBattleEnd(const BattleResult & br, int round)
{
	switch(br.winner)
	{
		case BattleSide::LEFT_SIDE:
			onActiveStack(nullptr, round, S15::CombatResult::LEFT_WINS);
			break;
		case BattleSide::RIGHT_SIDE:
			onActiveStack(nullptr, round, S15::CombatResult::RIGHT_WINS);
			break;
		default:
			onActiveStack(nullptr, round, S15::CombatResult::DRAW);
	}
}

void State::onActiveStack(const CStack * acstack, int round, S15::CombatResult result)
{
	logAi->debug("onActiveStack: round=%d, result=%d", round, EI(result));
	G = std::make_shared<Graph::Graph>(battle);

	const auto stats = CalcGlobalStats(battle);
	const auto logdata = ProcessAttackLogs(attackLogs);

	AddGlobalNode(*G, battle, acstack, result, round, stats);
	AddPlayerNodes(*G, battle, startStats, lastStats, stats, logdata);
	AddHexNodes(*G, battle, acstack);
	AddUnitNodes(*G, battle, acstack, stats);

	AddEdges_Global_To_PlayerUnitHex(*G, battle);
	AddEdges_Player_Owns_Unit(*G, battle);
	AddEdges_Hex_Adjacent_Hex(*G);
	AddEdges_Unit_ActsBefore_Unit(*G, battle);
	AddEdges_Unit_MeleeDmg_Unit(*G, battle, stats);
	AddEdges_Unit_ShootDmg_Unit(*G, battle, stats);
	AddEdges_Unit_Blocks_Unit(*G);
	AddEdges_Unit_Occupies_Hex(*G);

	auto atFlags = EnumFlags<S15::ActionType>();
	AddMoveAndDefendActions(*G, atFlags); // + edges: ActionByUnit, ActionEndsAtHex
	// AddMoveActionEdges_Action_By_Unit() // already added
	AddMoveActionEdges_Action_Blocks_Unit(*G, atFlags, battle);
	// AddMoveActionEdges_Action_EndsAt_Hex() // already added
	AddMoveActionEdges_Unit_BecomesMeleeThreatAfter_Action(*G, atFlags);
	AddMoveActionEdges_Unit_BecomesShootThreatAfter_Action(*G, atFlags, battle);

	AddMoveActionEdges_UnitAndHex_BecomesMeleeTargetAfter_Action(*G, atFlags, battle);
	AddMoveActionEdges_UnitAndHex_BecomesShootTargetAfter_Action(*G, atFlags, battle);

	AddOtherActions(*G, atFlags, battle);
	// AddRetreatAction(*G, atFlags); // XXX: retreats intentionally disabled

	AddEdges_Global_To_Action(*G, atFlags, battle);

	for(int i = 0; i < G->getFlags().flags.size(); ++i)
		ASSERT(G->getFlags().flags.test(i), "etFlags check: " + std::to_string(i) + ": " + G->getFlags().flags.to_string());

	ASSERT(G->getFlags().flags.all(), "etFlags check: " + G->getFlags().flags.to_string());
	ASSERT(atFlags.flags.all(), "atFlags check: " + atFlags.flags.to_string());
	G->verify();

	supdata = std::make_unique<SupplementaryData>(
		colorname,
		static_cast<Side>(side),
		G,
		attackLogs, // store the logs since OUR last turn
		result
	);

	attackLogs.clear(); // accumulate new logs until next turn
	isMorale = false;
	lastStats = stats;

	if(isMMAIVerbose())
		ReportCounts(*G);
}
};
