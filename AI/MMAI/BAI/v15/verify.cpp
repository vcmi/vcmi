/*
 * verify.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "BAI/v15/graph/edges/generic.h"
#include "BAI/v15/graph/edges/hex_adjacent_hex.h"
#include "BAI/v15/graph/edges/unit_acts_before_unit.h"
#include "BAI/v15/graph/edges/unit_is_meleed_by_action.h"
#include "BAI/v15/graph/nodes/action.h"
#include "CStack.h"
#include "battle/AccessibilityInfo.h"
#include "battle/BattleAttackInfo.h"
#include "battle/BattleHex.h"
#include "battle/BattleSide.h"
#include "battle/CObstacleInstance.h"
#include "battle/CPlayerBattleCallback.h"
#include "battle/IBattleInfoCallback.h"
#include "battle/ReachabilityInfo.h"
#include "battle/Unit.h"
#include "constants/Enumerations.h"
#include "entities/building/TownFortifications.h"
#include "schema/base.h"
#include "vcmi/spells/Caster.h"

#include "common.h"
#include "schema/v15/types.h"
#include "verify.h"
#include <algorithm>
#include <string>
#include <vector>

namespace MMAI::BAI::V15
{
namespace S = Schema;
namespace S15 = S::V15;

namespace N = Graph::Nodes;
namespace E = Graph::Edges;

// using UnitPtr = std::shared_ptr<const N::Unit>;
// using HexPtr = std::shared_ptr<const N::Hex>;
// using ActionPtr = std::shared_ptr<const N::Action>;
// using TowerFlags = N::Global::TowerFlags;
// using CorpseFlags = N::Global::CorpseFlags;
// using ET = S15::Graph::ElementType;

namespace
{
	struct Context
	{
		const Graph::Graph & G;
		const CPlayerBattleCallback & battle;
		const CStack * astack;
		const std::unordered_set<const CStack *> allstacks;
		const std::unordered_map<int, const CStack *> bhexstacks;
		const std::bitset<GameConstants::BFIELD_SIZE> bhexcorpses;
		const std::map<const CStack *, ReachabilityInfo::TDistances> distances;
		const std::vector<const battle::Unit *> queue;
		const bool ended;
		const bool hasFort;
	};

	Context BuildContext(const State * state)
	{
		const auto & battle = state->battle;
		const auto & G = state->G;
		const CStack * astack = nullptr;
		const auto ended = state->supdata->ended;

		auto allstacks = std::unordered_set<const CStack *>{};
		auto bhexstacks = std::unordered_map<int, const CStack *>{};
		auto bhexcorpses = std::bitset<GameConstants::BFIELD_SIZE>{};
		auto distances = std::map<const CStack *, ReachabilityInfo::TDistances>{};

		for(const auto & cstack : battle.battleGetStacks(CBattleInfoEssentials::EStackOwnership::MINE_AND_ENEMY, false))
		{
			if(cstack->creatureId() == CreatureID::ARROW_TOWERS)
				continue;

			if(!cstack->alive())
			{
				for(const auto & bh : cstack->getHexes())
					bhexcorpses.set(bh.toInt());
				continue;
			}

			allstacks.emplace(cstack);
			distances.emplace(cstack, battle.getReachability(cstack).distances);

			for(const auto & bh : cstack->getHexes())
				bhexstacks.emplace(bh.toInt(), cstack);

			if(cstack->unitId() == battle.battleActiveUnit()->unitId())
				astack = cstack;
		}

		// XXX: good morale is NOT handled here for simplicity
		//      See comments in Battlefield::GetQueue how to handle it.
		auto tmp = std::vector<battle::Units>{};
		battle.battleGetTurnOrder(tmp, S15::STACK_QUEUE_SIZE, 0);
		auto queue = std::vector<const battle::Unit *>{};
		for(const auto & units : tmp)
		{
			for(const auto & unit : units)
			{
				if(queue.size() < S15::STACK_QUEUE_SIZE)
					queue.push_back(unit);
				else
					break;
			}
		}

		return Context{
			.G = *G,
			.battle = battle,
			.astack = astack,
			.allstacks = allstacks,
			.bhexstacks = bhexstacks,
			.bhexcorpses = bhexcorpses,
			.distances = distances,
			.queue = queue,
			.ended = ended,
			.hasFort = (battle.battleGetFortifications().wallsHealth > 0),
		};
	}

	// Plain message overload: expect(cond, "expectation failed");
	inline void expect(bool exp, std::string_view message)
	{
		if(exp)
			return;

		throw std::runtime_error(std::string(message));
	}

	template<typename... Args>
	inline void expect(bool exp, std::string_view format, const Args &... args)
	requires(sizeof...(Args) > 0)
	{
		if(exp)
			return;

		boost::format f{std::string(format)};

		// Fold expression: expands to (f % arg1, f % arg2, ...)
		((f % args), ...);

		throw std::runtime_error(f.str());
	}

	void vassert(int have, int want, const std::string_view attrname, const std::string & desc = "")
	{
		desc.empty() ? expect(have == want, "%s: have: %d, want: %d", attrname, have, want)
					 : expect(have == want, "%s: have: %d, want: %d (%s)", attrname, have, want, desc.c_str());
	};

	void Verify_NODE_GLOBAL(const Context & ctx)
	{
		const auto & nodes = ctx.G.getAll<N::Global>();

		expect(nodes.size() == 1, "GLOBAL: nodes.size() != 1");
		const auto & global = nodes.front();

		auto haswall = [&ctx](EWallPart wp)
		{
			auto wstate = ctx.battle.battleGetWallState(wp);
			return wstate == EWallState::DAMAGED || wstate == EWallState::INTACT || wstate == EWallState::REINFORCED;
		};

		using A = N::Global::A;
		for(int i = 0; i < EU(A::_count); ++i)
		{
			auto a = A(i);
			auto v = global->attr(a);

			switch(a)
			{
				case A::BATTLE_WINNER:
					switch(S15::CombatResult(v))
					{

						case S15::CombatResult::LEFT_WINS:
						case S15::CombatResult::RIGHT_WINS:
						case S15::CombatResult::DRAW:
							// XXX: The logic in battleIsFinished is flawed and returns no value
							//      (i.e. "not finished") when both sides have alive units.
							//      This is incorrect in case of a retreat => don't use it
							// vassert(v, battle.battleIsFinished(), "A::BATTLE_WINNER");
							expect(ctx.ended, "A::BATTLE_WINNER is " + std::to_string(v) + ", but ended is false");
							break;
						case S15::CombatResult::NONE:
							break;
						default:
							throw std::runtime_error("Unexpected CombatResult: " + std::to_string(v));
							break;
					}
					break;
				case A::BATTLE_ROUND:
					// XXX: technically, the first round for MMAI may not be the 1st VCMI round
					// With GUI, when "auto-play" is pressed, MMAI perceives starts from round 0
					// In headless mode it's unlikely: all MMAI units must be blinded for the 1st round
					vassert(v, ctx.battle.battleGetRound(), "GLOBAL.BATTLE_ROUND");
					break;
				case A::HAS_UPPER_TOWER:
					vassert(v, haswall(EWallPart::UPPER_TOWER), "GLOBAL.HAS_UPPER_TOWER");
					break;
				case A::HAS_MIDDLE_TOWER:
					vassert(v, haswall(EWallPart::KEEP), "GLOBAL.HAS_MIDDLE_TOWER");
					break;
				case A::HAS_BOTTOM_TOWER:
					vassert(v, haswall(EWallPart::BOTTOM_TOWER), "GLOBAL.HAS_BOTTOM_TOWER");
					break;
				case A::HAS_GATE_CORPSE:
					ctx.hasFort
						? vassert(v, ctx.bhexcorpses.test(BattleHex::GATE_INNER) || ctx.bhexcorpses.test(BattleHex::GATE_OUTER), "GLOBAL.HAS_GATE_CORPSE")
						: vassert(v, 0, "GLOBAL.HAS_GATE_CORPSE: no fort");
					break;
				case A::HAS_BRIDGE_CORPSE:
					ctx.hasFort ? vassert(v, ctx.bhexcorpses.test(BattleHex::GATE_BRIDGE), "GLOBAL.HAS_BRIDGE_CORPSE")
								: vassert(v, 0, "GLOBAL.HAS_BRIDGE_CORPSE: no fort");
					break;
				default:
					throw std::runtime_error("Unexpected GLOBAL attr: " + std::to_string(EU(a)));
			}
		}
	}

	void Verify_NODE_PLAYER(const Context & ctx)
	{
		const auto & nodes = ctx.G.getAll<N::Player>();

		expect(nodes.size() == 2, "PLAYER: nodes.size() != 2");

		using A = N::Player::A;
		for(const auto & player : nodes)
		{
			for(int i = 0; i < EU(A::_count); ++i)
			{
				auto a = A(i);
				auto v = player->attr(a);

				static_assert(EU(S::Side::LEFT) == EU(BattleSide::LEFT_SIDE));
				static_assert(EU(S::Side::RIGHT) == EU(BattleSide::RIGHT_SIDE));
				const auto cplayer = ctx.battle.sideToPlayer(BattleSide(player->attr(A::BATTLE_SIDE)));

				switch(a)
				{
					case A::BATTLE_SIDE:
						break;
					case A::IS_ACTIVE:
						vassert(v, cplayer == ctx.astack->getOwner(), "PLAYER.IS_ACTIVE");
						break;
					case A::ARMY_VALUE_NOW_REL0:
					case A::ARMY_VALUE_NOW_REL:
					case A::ARMY_HP_NOW_REL:
					case A::VALUE_KILLED_NOW_REL:
					case A::VALUE_LOST_NOW_REL:
					case A::DMG_DEALT_NOW_REL:
					case A::DMG_RECEIVED_NOW_REL:
						// Not verifying those.
						break;
					default:
						throw std::runtime_error("Unexpected PLAYER attr: " + std::to_string(EU(a)));
				}
			}
		}
	}

	void Verify_NODE_UNIT(const Context & ctx)
	{
		const auto & nodes = ctx.G.getAll<N::Unit>();
		expect(nodes.size() == ctx.allstacks.size(), "UNIT: nodes.size() != " + std::to_string(ctx.allstacks.size()));

		auto duration = [](const CStack & cstack, BonusSource source, BonusSourceID sourceID)
		{
			auto cachingStr = "source_" + std::to_string(static_cast<int>(source)) + sourceID.toString();
			auto bonuses = cstack.getBonuses(Selector::source(source, sourceID), cachingStr);
			return bonuses->empty() ? 0 : bonuses->front()->turnsRemain;
		};

		using A = N::Unit::A;
		for(const auto & unit : nodes)
		{
			for(int i = 0; i < EU(A::_count); ++i)
			{
				auto a = A(i);
				auto v = unit->attr(a);

				const auto & cstack = unit->cstack;

				switch(a)
				{
					case A::VALUE_REL:
						// not verifying
						break;
					case A::SHOTS:
						vassert(v, cstack.shots.available(), "UNIT.SHOTS", cstack.getDescription());
						break;
					case A::DMG_UNCERTAINTY:
						// not verifying
						break;
					case A::IS_ACTIVE:
						vassert(v, cstack.unitId() == ctx.battle.battleActiveUnit()->unitId(), "UNIT.IS_ACTIVE", cstack.getDescription());
						break;
					case A::IS_ENEMY:
						vassert(v, cstack.unitSide() != ctx.battle.battleGetMySide(), "UNIT.IS_ENEMY", cstack.getDescription());
						break;
					case A::IS_SLEEPING:
						cstack.creatureId() == CreatureID::AMMO_CART ? vassert(v, false, "UNIT.IS_SLEEPING")
																	 : vassert(v, cstack.hasBonusOfType(BonusType::NOT_ACTIVE), "UNIT.IS_SLEEPING");
						break;
					case A::IS_WAR_MACHINE:
						vassert(v, cstack.hasBonusOfType(BonusType::SIEGE_WEAPON), "UNIT.IS_WAR_MACHINE", cstack.getDescription());
						break;
					case A::HAS_ADDITIONAL_ATTACK:
						vassert(v, cstack.hasBonusOfType(BonusType::ADDITIONAL_ATTACK), "UNIT.HAS_ADDITIONAL_ATTACK", cstack.getDescription());
						break;
					case A::HAS_ALL_AROUND_ATTACK:
						vassert(v, cstack.hasBonusOfType(BonusType::ATTACKS_ALL_ADJACENT), "UNIT.HAS_ALL_AROUND_ATTACK", cstack.getDescription());
						break;
					case A::HAS_BLOCKS_RETALIATION:
						vassert(v, cstack.hasBonusOfType(BonusType::BLOCKS_RETALIATION), "UNIT.HAS_BLOCKS_RETALIATION", cstack.getDescription());
						break;
					case A::HAS_DEATH_CLOUD:
						vassert(
							v,
							cstack.hasBonusOfType(BonusType::SPELL_LIKE_ATTACK, SpellID(SpellID::DEATH_CLOUD)),
							"UNIT.HAS_DEATH_CLOUD",
							cstack.getDescription()
						);
						break;
					case A::HAS_DOUBLE_DAMAGE_CHANCE:
						vassert(v, 10 * cstack.valOfBonuses(BonusType::DOUBLE_DAMAGE_CHANCE), "UNIT.HAS_DOUBLE_DAMAGE_CHANCE", cstack.getDescription());
						break;
					case A::HAS_FIREBALL:
						vassert(
							v, cstack.hasBonusOfType(BonusType::SPELL_LIKE_ATTACK, SpellID(SpellID::FIREBALL)), "UNIT.HAS_FIREBALL", cstack.getDescription()
						);
						break;
					case A::HAS_FLYING:
						vassert(v, cstack.hasBonusOfType(BonusType::FLYING), "UNIT.HAS_FLYING", cstack.getDescription());
						break;
					case A::HAS_LIFE_DRAIN:
						vassert(v, 10 * cstack.valOfBonuses(BonusType::LIFE_DRAIN), "UNIT.HAS_LIFE_DRAIN", cstack.getDescription());
						break;
					case A::HAS_NON_LIVING:
					{
						auto undead = cstack.hasBonusOfType(BonusType::UNDEAD);
						auto nonliving = cstack.hasBonusOfType(BonusType::NON_LIVING);
						vassert(v, undead || nonliving, "UNIT.HAS_NON_LIVING", cstack.getDescription());
						break;
					}
					case A::HAS_NO_MELEE_PENALTY:
						vassert(v, cstack.hasBonusOfType(BonusType::NO_MELEE_PENALTY), "UNIT.HAS_NO_MELEE_PENALTY", cstack.getDescription());
						break;
					case A::HAS_RETURN_AFTER_STRIKE:
						vassert(v, cstack.hasBonusOfType(BonusType::RETURN_AFTER_STRIKE), "UNIT.HAS_RETURN_AFTER_STRIKE", cstack.getDescription());
						break;
					case A::HAS_THREE_HEADED_ATTACK:
						vassert(v, cstack.hasBonusOfType(BonusType::THREE_HEADED_ATTACK), "UNIT.HAS_THREE_HEADED_ATTACK", cstack.getDescription());
						break;
					case A::HAS_TWO_HEX_ATTACK_BREATH:
						vassert(v, cstack.hasBonusOfType(BonusType::TWO_HEX_ATTACK_BREATH), "UNIT.HAS_TWO_HEX_ATTACK_BREATH", cstack.getDescription());
						break;
					case A::HAS_AGE:
						vassert(v, duration(cstack, BonusSource::SPELL_EFFECT, SpellID(SpellID::AGE)), "UNIT.HAS_AGE", cstack.getDescription());
						break;
					case A::HAS_AGE_ATTACK:
						vassert(
							v, 10 * cstack.valOfBonuses(BonusType::SPELL_AFTER_ATTACK, SpellID(SpellID::AGE)), "UNIT.HAS_AGE_ATTACK", cstack.getDescription()
						);
						break;
					case A::HAS_BIND:
						vassert(v, duration(cstack, BonusSource::SPELL_EFFECT, SpellID(SpellID::BIND)), "UNIT.HAS_BIND", cstack.getDescription());
						break;
					case A::HAS_BIND_ATTACK:
						vassert(
							v, 10 * cstack.valOfBonuses(BonusType::SPELL_AFTER_ATTACK, SpellID(SpellID::BIND)), "UNIT.HAS_BIND_ATTACK", cstack.getDescription()
						);
						break;
					case A::HAS_BLIND:
					{
						auto blind = duration(cstack, BonusSource::SPELL_EFFECT, SpellID(SpellID::BLIND));
						auto paralyze = duration(cstack, BonusSource::SPELL_EFFECT, SpellID(SpellID::PARALYZE));
						vassert(v, std::max(blind, paralyze), "UNIT.HAS_BLIND", cstack.getDescription());
						break;
					}
					case A::HAS_BLIND_ATTACK:
					{
						auto blind = cstack.valOfBonuses(BonusType::SPELL_AFTER_ATTACK, SpellID(SpellID::BLIND));
						auto paralyze = cstack.valOfBonuses(BonusType::SPELL_AFTER_ATTACK, SpellID(SpellID::PARALYZE));
						vassert(v, 10 * (blind + paralyze), "UNIT.HAS_BLIND_ATTACK", cstack.getDescription());
						break;
					}
					case A::HAS_CURSE:
						vassert(v, duration(cstack, BonusSource::SPELL_EFFECT, SpellID(SpellID::CURSE)), "UNIT.HAS_CURSE", cstack.getDescription());
						break;
					case A::HAS_CURSE_ATTACK:
						vassert(
							v,
							10 * cstack.valOfBonuses(BonusType::SPELL_AFTER_ATTACK, SpellID(SpellID::CURSE)),
							"UNIT.HAS_CURSE_ATTACK",
							cstack.getDescription()
						);
						break;
					case A::HAS_DISPEL_ATTACK:
						vassert(
							v,
							10 * cstack.valOfBonuses(BonusType::SPELL_AFTER_ATTACK, SpellID(SpellID::DISPEL_HELPFUL_SPELLS)),
							"UNIT.HAS_DISPEL_ATTACK",
							cstack.getDescription()
						);
						break;
					case A::HAS_PETRIFY:
						vassert(v, duration(cstack, BonusSource::SPELL_EFFECT, SpellID(SpellID::STONE_GAZE)), "UNIT.HAS_PETRIFY", cstack.getDescription());
						break;
					case A::HAS_PETRIFY_ATTACK:
						vassert(
							v,
							10 * cstack.valOfBonuses(BonusType::SPELL_AFTER_ATTACK, SpellID(SpellID::STONE_GAZE)),
							"UNIT.HAS_PETRIFY_ATTACK",
							cstack.getDescription()
						);
						break;
					case A::HAS_POISON:
						vassert(v, duration(cstack, BonusSource::SPELL_EFFECT, SpellID(SpellID::POISON)), "UNIT.HAS_POISON", cstack.getDescription());
						break;
					case A::HAS_POISON_ATTACK:
						vassert(
							v,
							10 * cstack.valOfBonuses(BonusType::SPELL_AFTER_ATTACK, SpellID(SpellID::POISON)),
							"UNIT.HAS_POISON_ATTACK",
							cstack.getDescription()
						);
						break;
					case A::HAS_WEAKNESS:
						vassert(v, duration(cstack, BonusSource::SPELL_EFFECT, SpellID(SpellID::WEAKNESS)), "UNIT.HAS_WEAKNESS", cstack.getDescription());
						break;
					case A::HAS_WEAKNESS_ATTACK:
						vassert(
							v,
							10 * cstack.valOfBonuses(BonusType::SPELL_AFTER_ATTACK, SpellID(SpellID::WEAKNESS)),
							"UNIT.HAS_WEAKNESS_ATTACK",
							cstack.getDescription()
						);
						break;
					default:
						throw std::runtime_error("Unexpected UNIT attr: " + std::to_string(EU(a)));
				}
			}
		}
	}

	void Verify_NODE_HEX(const Context & ctx) // NOLINT(readability-function-cognitive-complexity)
	{
		const auto & nodes = ctx.G.getAll<N::Hex>();

		expect(nodes.size() == 165, "HEX: nodes.size() != 165");

		const auto obstacles = ctx.battle.battleGetAllObstacles();

		using A = N::Hex::A;
		for(const auto & hex : nodes)
		{
			auto hexobstacle = [&hex, &obstacles](auto fn)
			{
				return std::any_of(
					obstacles.begin(),
					obstacles.end(),
					[&hex, &fn](const std::shared_ptr<const CObstacleInstance> & o)
					{
						return o->getAffectedTiles().contains(hex->bhex) && fn(o);
					}
				);
			};

			for(int i = 0; i < EU(A::_count); ++i)
			{
				auto a = A(i);
				auto v = hex->attr(a);

				const auto & bhex = hex->bhex;
				expect(bhex.isAvailable(), "HEX: unavailable");

				const auto access = ctx.battle.getAccessibility().at(bhex.toInt());

				switch(a)
				{
					case A::Y_COORD:
						vassert(v, N::Hex::CalcXY(bhex).second, "HEX.Y_COORD", hex->name());
						break;
					case A::X_COORD:
						vassert(v, N::Hex::CalcXY(bhex).first, "HEX.X_COORD", hex->name());
						break;
					case A::IS_PASSABLE:
					{
						// TODO: handle gate and bridge (complicated llogic)
						if(bhex == BattleHex::GATE_OUTER || bhex == BattleHex::GATE_INNER || bhex == BattleHex::GATE_BRIDGE)
							break;
						vassert(v, access == EAccessibility::ACCESSIBLE, "HEX.IS_PASSABLE", hex->name());
						break;
					}
					case A::IS_STOPPING:
					{
						// TODO: handle gate and bridge (complicated llogic)
						if(bhex == BattleHex::GATE_OUTER || bhex == BattleHex::GATE_INNER || bhex == BattleHex::GATE_BRIDGE)
							break;

						auto stopping = hexobstacle(std::mem_fn(&CObstacleInstance::stopsMovement));
						vassert(v, stopping, "HEX.IS_STOPPING: no obstacle stops movement", hex->name());
						break;
					}
					case A::IS_DAMAGING_L:
					{
						// auto ldmg = hexobstacle([&ctx, &bhex](const std::shared_ptr<const CObstacleInstance> & o)
						// {
						// 	if(o->obstacleType == CObstacleInstance::MOAT)
						// 	{
						// 		if (bhex == BattleHex::GATE_BRIDGE)
						// 		{
						// 			const auto gs = ctx.battle.battleGetGateState();
						// 			return gs == EGateState::CLOSED || gs == EGateState::BLOCKED;
						// 		}
						// 		return true;
						// 	}
						// 	if(!o->triggersEffects())
						// 		return false;
						// 	auto s = SpellID(o->ID);
						// 	if(s == SpellID::FIRE_WALL)
						// 		return true;
						// 	if(s != SpellID::LAND_MINE)
						// 		return false;
						// 	ASSERT(o->obstacleType == CObstacleInstance::EObstacleType::SPELL_CREATED, "expected spell created obstacle");
						// 	const auto * so = dynamic_cast<const SpellCreatedObstacle *>(o.get());
						// 	return so->casterSide != BattleSide::LEFT_SIDE;
						// });
						// vassert(v, ldmg, "HEX.IS_DAMAGING_L: no obstacle triggers a damaging effect", hex->name());
						// XXX: too complicated to check
						break;
					}
					case A::IS_DAMAGING_R:
					{
						// auto rdmg = hexobstacle([&ctx, &bhex](const std::shared_ptr<const CObstacleInstance> & o)
						// {
						// 	if(o->obstacleType == CObstacleInstance::MOAT)
						// 	{
						// 		// for defenders, a blocked gate means no bridge will lower
						// 		// However, it seems it's not implemented this way in graph/hex.cpp
						// 		if (bhex == BattleHex::GATE_BRIDGE)
						// 		{
						// 			const auto gs = ctx.battle.battleGetGateState();
						// 			return gs == EGateState::CLOSED || gs == EGateState::BLOCKED;
						// 		}
						// 		return true;
						// 	}
						// 	if(!o->triggersEffects())
						// 		return false;
						// 	auto s = SpellID(o->ID);
						// 	if(s == SpellID::FIRE_WALL)
						// 		return true;
						// 	if(s != SpellID::LAND_MINE)
						// 		return false;
						// 	ASSERT(o->obstacleType == CObstacleInstance::EObstacleType::SPELL_CREATED, "expected spell created obstacle");
						// 	const auto * so = dynamic_cast<const SpellCreatedObstacle *>(o.get());
						// 	return so->casterSide != BattleSide::RIGHT_SIDE;
						// });
						// vassert(v, rdmg, "HEX.IS_DAMAGING_R: no obstacle triggers a damaging effect", hex->name());
						break;
					}
					case A::IS_SIEGE_GATE:
						ctx.hasFort
							? vassert(v, bhex == BattleHex::GATE_OUTER || bhex == BattleHex::GATE_INNER, "HEX.IS_SIEGE_GATE: not a gate hex", hex->name())
							: vassert(v, 0, "HEX.IS_SIEGE_GATE: no fort on this battlefield", hex->name());
						break;
					case A::IS_SIEGE_BRIDGE:
						ctx.hasFort ? vassert(v, bhex == BattleHex::GATE_BRIDGE, "HEX.IS_SIEGE_BRIDGE: no a bridge hex", hex->name())
									: vassert(v, 0, "HEX.IS_SIEGE_BRIDGE: no fort on this battlefield", hex->name());
						break;
					case A::IS_OBSTACLE:
					{
						bool want = hexobstacle(
							[](const std::shared_ptr<const CObstacleInstance> & o)
							{
								return o->blocksTiles();
							}
						);
						// indestructible walls (and maybe regular?), space between ships, etc. are not obstacles
						want = want || access == EAccessibility::UNAVAILABLE;
						vassert(v, want, "HEX.IS_OBSTACLE: no obstacle blocks this hex", hex->name());
						break;
					}
					case A::WALL_HEALTH:
					{
						using WP = EWallPart;
						const auto wp = ctx.battle.battleHexToWallPart(bhex);
						static_assert(0 == EU(EWallState::DESTROYED));
						static_assert(1 == EU(EWallState::DAMAGED));
						static_assert(2 == EU(EWallState::INTACT));
						static_assert(3 == EU(EWallState::REINFORCED));
						if(wp == WP::INVALID || wp == WP::BOTTOM_TOWER)
							vassert(v, 0, "Hex.WALL_HEALTH: not a wall hex", hex->name());
						else if(ctx.battle.battleGetFortifications().wallsHealth == 0)
							vassert(v, 0, "HEX.WALL_HEALTH: no fort", hex->name());
						else if(wp == WP::UPPER_TOWER || wp == WP::INDESTRUCTIBLE_PART || wp == WP::INDESTRUCTIBLE_PART_OF_GATE)
							vassert(v, 0, "HEX.WALL_HEALTH: indestructible walls and top tower should have 0 HP", hex->name());
						else
							vassert(v, EU(ctx.battle.battleGetWallState(wp)), "HEX.WALL_HEALTH: state mismatch", hex->name());
						break;
					}
					default:
						throw std::runtime_error("Unexpected HEX attr: " + std::to_string(EU(a)));
				}
			}
		}
	}

	void Verify_NODE_ACTION(const Context & ctx)
	{
		for(const auto & id : ctx.G.getActiveActionIds())
			expect(ctx.G.getById<N::Action>(id, false) != nullptr, "active action not active: " + std::to_string(id));

		auto isReachable = [&ctx](const CStack & stack, const BattleHex & bh)
		{
			auto distance = ctx.distances.at(&stack).at(bh.toInt());
			return distance <= stack.getMovementRange();
		};

		using A = N::Action::A;
		for(const auto & action : ctx.G.getAll<N::Action>())
		{
			using AT = S15::ActionType;

			const CStack & actor = action->by->cstack;
			const auto endBhex = action->endsAt.at(0)->bhex;

			// NOTE: the method name can *move* is misleading.
			// It is actually checks if stack can *act*.
			// XXX: disabling this check as blinded/paralyzed/etc. units still
			// 		have their actions in the graph.
			// 		Whether the unit can actually perform this action can be
			// 		inferred from the IS_SLEEPING attribute or ACTS_BEFORE edge.
			// expect(actor.canMove(), "ACTION: cannot act");

			for(int i = 0; i < EU(A::_count); ++i)
			{
				auto a = A(i);
				auto v = action->attr(a);

				switch(a)
				{
					case A::ACTION_TYPE:
						switch(AT(v))
						{
							// case AT::RETREAT:
							//     expect(actor == nullptr, "ACTION.ACTION_TYPE[RETREAT]: actor must be nullptr");
							//     break;
							case AT::WAIT:
								expect(!actor.waited(), "ACTION.ACTION_TYPE[WAIT]: already waited");
								break;
							case AT::DEFEND:
								// nothing to check (always possible action)
								break;
							case AT::MOVE:
								expect(
									isReachable(actor, endBhex),
									"ACTION.ACTION_TYPE[MOVE]: endBhex unreachable: " + action->endsAt.at(0)->name() + " by " + action->by->name()
								);
								break;
							case AT::AMOVE:
								expect(isReachable(actor, endBhex), "ACTION.ACTION_TYPE[AMOVE]: endBhex unreachable");
								expect(action->target != nullptr, "ACTION.ACTION_TYPE[AMOVE]: target is nullptr");
								expect(
									CStack::isMeleeAttackPossible(&actor, &action->target->cstack, endBhex),
									"ACTION.ACTION_TYPE[AMOVE]: melee attack is impossible"
								);
								break;
							case AT::SHOOT:
								expect(actor.canShoot(), "ACTION.ACTION_TYPE[SHOOT]: can't shoot");
								expect(
									!ctx.battle.battleIsUnitBlocked(&actor) || actor.hasBonusOfType(BonusType::FREE_SHOOTING) || actor.isBallista(),
									"ACTION.ACTION_TYPE[SHOOT]: blocked"
								);
								break;
							default:
								throw std::runtime_error("Unexpected Action type: " + std::to_string(v));
								break;
						}
						break;
					case A::IS_ACTIVE:
						vassert(v, &actor == ctx.astack, "ACTION.ACTION_TYPE[IS_ACTIVE]: not active");
						break;
					default:
						throw std::runtime_error("Unexpected Action attr: " + std::to_string(EU(a)));
						break;
				}
			}
		}
	}

	void Verify_EDGE_UNIT_ACTS_BEFORE_UNIT(const Context & ctx)
	{
		auto countActsBefore = [&ctx](const battle::Unit * src, const battle::Unit * dst)
		{
			const auto dstIt = std::ranges::find(ctx.queue, dst);
			return std::ranges::count_if(
				ctx.queue.begin(),
				dstIt,
				[src](const battle::Unit * unit)
				{
					return unit == src;
				}
			);
		};

		for(const auto & edge : ctx.G.getAll<E::Unit_ActsBefore_Unit>())
		{
			const auto * src = &edge->srcNode->cstack;
			const auto * dst = &edge->dstNode->cstack;
			expect(edge->attr(E::Unit_ActsBefore_Unit::A::TIMES) == countActsBefore(src, dst), "EDGE_UNIT_ACTS_BEFORE_UNIT.TIMES mismatch");
		}
	}

	void Verify_EDGE_UNIT_BLOCKS_UNIT(const Context & ctx)
	{
		for(const auto & edge : ctx.G.getAll<E::Unit_Blocks_Unit>())
		{
			const auto & blocked = edge->srcNode->cstack;
			const auto & blocker = edge->dstNode->cstack;

			expect(blocked.canShoot(), "EDGE_UNIT_BLOCKS_UNIT: blocked unit cannot shoot");
			expect(!blocked.canShootBlocked(), "EDGE_UNIT_BLOCKS_UNIT: blocked unit can shoot while blocked");
			expect(!blocked.hasBonusOfType(BonusType::SIEGE_WEAPON), "EDGE_UNIT_BLOCKS_UNIT: war machine cannot be blocked");
			expect(
				blocker.unitSide() != blocked.unitSide() || blocked.hasBonusOfType(BonusType::ATTACKS_NEAREST_CREATURE),
				"EDGE_UNIT_BLOCKS_UNIT: friendly unit blocks non-berserk shooter"
			);

			const auto surroundingHexes = blocked.getSurroundingHexes();
			expect(
				std::ranges::any_of(
					surroundingHexes,
					[&blocker](const BattleHex & bhex)
					{
						return blocker.coversPos(bhex);
					}
				),
				"EDGE_UNIT_BLOCKS_UNIT: blocker is not adjacent to blocked unit"
			);
		}
	}

	void Verify_EDGE_ACTION_BLOCKS_UNIT(const Context & ctx)
	{
		for(const auto & edge : ctx.G.getAll<E::Action_Blocks_Unit>())
		{
			const auto & blocker = edge->srcNode->by->cstack;
			const auto & blocked = edge->dstNode->cstack;

			expect(blocked.canShoot(), "ACTION_BLOCKS_UNIT: blocked unit cannot shoot");
			expect(!blocked.canShootBlocked(), "ACTION_BLOCKS_UNIT: blocked unit can shoot while blocked");
			expect(!blocked.hasBonusOfType(BonusType::SIEGE_WEAPON), "ACTION_BLOCKS_UNIT: war machine cannot be blocked");
			expect(
				blocker.unitSide() != blocked.unitSide() || blocked.hasBonusOfType(BonusType::ATTACKS_NEAREST_CREATURE),
				"ACTION_BLOCKS_UNIT: friendly unit blocks non-berserk shooter"
			);

			const auto surroundingHexes = blocked.getSurroundingHexes();
			expect(
				std::ranges::any_of(
					surroundingHexes,
					[&blocker](const BattleHex & bhex)
					{
						return CStack::getHexes(bhex, blocker.doubleWide(), blocker.unitSide()).contains(bhex);
					}
				),
				"ACTION_BLOCKS_UNIT: blocker is not adjacent to blocked unit after the action"
			);

			expect(
				ctx.G.getEdgeBySrcDst<E::Unit_BlockedBy_Action>(edge->dstNode, edge->srcNode, false) != nullptr,
				"ACTION_BLOCKS_UNIT: no corresponding reverse edge"
			);
		}
	}

	void Verify_EDGE_UNIT_BLOCKED_BY_ACTION(const Context & ctx)
	{
		for(const auto & edge : ctx.G.getAll<E::Unit_BlockedBy_Action>())
		{
			const auto & action = edge->dstNode;
			const auto & blocked = edge->srcNode->cstack;
			const auto & actor = action->by->cstack;

			expect(blocked.canShoot(), "EDGE_UNIT_BLOCKED_BY_ACTION: blocked unit cannot shoot");

			const bool unblockable = blocked.canShootBlocked() || blocked.hasBonusOfType(BonusType::SIEGE_WEAPON);
			expect(!unblockable, "EDGE_UNIT_BLOCKED_BY_ACTION: blocked unit is unblockable");

			const auto & attackHexes = CStack::meleeAttackHexes(&actor, &blocked, action->endsAt.at(0)->bhex, blocked.getPosition());

			// NOTE: this will be incorrect if either stack is berserk
			expect(
				!attackHexes.empty(),
				"EDGE_UNIT_BLOCKED_BY_ACTION: actor will not block the unit: " + action->name() + " :: " + action->by->name() + "::" + edge->srcNode->name()
			);
		}
	}

	void Verify_EDGE_UNIT_BECOMES_MELEE_THREAT_AFTER_ACTION(const Context & ctx)
	{
		for(const auto & edge : ctx.G.getAll<E::Unit_BecomesMeleeThreatAfter_Action>())
		{
			const auto & threat = edge->srcNode->cstack;
			const auto & action = edge->dstNode;
			const auto & actor = action->by->cstack;
			// NOTE: this will fail if either stack is berserk
			expect(threat.unitSide() != actor.unitSide(), "EDGE_UNIT_BECOMES_MELEE_THREAT_AFTER_ACTION: same side");

			const auto speed = threat.getMovementRange();

			// From the threat's POV, accessible hexes are:
			// 1. its own hexes
			auto knownAccessible = threat.getHexes();

			// 2. plus the actor's old position (as actor will move out of there)/
			for(const auto & bhex : actor.getHexes())
				knownAccessible.checkAndPush(bhex);

			// 3. minus the actor's new position (after it moves)
			knownAccessible.eraseIf(
				[&action](const auto & bhex)
				{
					for(const auto & hex : action->endsAt)
						if(hex->bhex == bhex)
							return true;
					return false;
				}
			);

			const auto & params = ReachabilityInfo::Parameters(ctx.battle.battleGetMySide(), &threat, threat.getPosition(), knownAccessible);

			auto reachability = ctx.battle.getReachability(params);

			// "Move" the actor to its new position
			const auto & actorstate = actor.acquireState();
			actorstate->setPosition(action->endsAt.at(0)->bhex);

			const auto & attackableHexes = actorstate->getAttackableHexes(&threat);
			bool isThreat = std::ranges::any_of(
				attackableHexes,
				[&speed, &reachability](const BattleHex & bh)
				{
					return reachability.distances.at(bh.toInt()) <= speed;
				}
			);

			// TODO: not a threat if it's sleeping
			// bool willActsBefore = ...

			expect(isThreat, "EDGE_UNIT_BECOMES_MELEE_THREAT_AFTER_ACTION: unit cannot reach any hex adjacent to action destination");
		}
	}

	void Verify_EDGE_UNIT_BECOMES_SHOOT_THREAT_AFTER_ACTION(const Context & ctx)
	{
		for(const auto & edge : ctx.G.getAll<E::Unit_BecomesShootThreatAfter_Action>())
		{
			const auto & shooter = edge->srcNode->cstack;
			const auto & action = edge->dstNode;
			const auto & actor = action->by;
			const auto & actorStack = actor->cstack;
			expect(shooter.canShoot(), "EDGE_UNIT_BECOMES_SHOOT_THREAT_AFTER_ACTION: unit cannot shoot");

			const auto blockers = ctx.G.getAllEdgesSrcByDst<E::Unit_Blocks_Unit>(edge->srcNode);
			const auto numBlockers = std::ranges::distance(blockers);
			const bool canShootWhileBlocked = shooter.canShootBlocked() || shooter.hasBonusOfType(BonusType::SIEGE_WEAPON);

			// NOTE: this will be incorrect if either stack is berserk
			expect(shooter.unitSide() != actorStack.unitSide(), "EDGE_UNIT_BECOMES_SHOOT_THREAT_AFTER_ACTION: shooter and actor are on the same side");

			if(canShootWhileBlocked)
				continue;

			expect(numBlockers <= 1, "EDGE_UNIT_BECOMES_SHOOT_THREAT_AFTER_ACTION: shooter is blocked by multiple units");

			if(numBlockers == 1)
				expect(actor == *blockers.begin(), "EDGE_UNIT_BECOMES_SHOOT_THREAT_AFTER_ACTION: shooter is blocked by another unit");

			// TODO: not a threat if it's sleeping
			// bool willActsBefore = ...

			const auto attackableHexes = shooter.getAttackableHexes(&actor->cstack);
			const bool actorWillBlockShooter = attackableHexes.contains(action->endsAt.at(0)->bhex);
			expect(!actorWillBlockShooter, "EDGE_UNIT_BECOMES_SHOOT_THREAT_AFTER_ACTION: actor will block shooter");
		}
	}

	void Verify_EDGE_UNIT_BECOMES_MELEE_TARGET_AFTER_ACTION(const Context & ctx)
	{
		for(const auto & edge : ctx.G.getAll<E::Unit_BecomesMeleeTargetAfter_Action>())
		{
			const auto & target = edge->srcNode->cstack;
			const auto & action = edge->dstNode;
			const auto & actor = action->by->cstack;

			// NOTE: this will be incorrect if either stack is berserk
			expect(target.unitSide() != actor.unitSide(), "EDGE_UNIT_BECOMES_MELEE_THREAT_AFTER_ACTION: same side");

			const auto dstHex = action->endsAt.at(0)->bhex;
			const auto speed = actor.getMovementRange();

			// From the actor's POV, accessible hexes are:
			// 1. its old position (as actor will move out of there)
			auto knownAccessible = actor.getHexes();

			// 2. plus its new position (after it moves)
			for(const auto & bhex : actor.getHexes(dstHex))
				knownAccessible.checkAndPush(bhex);

			const auto & params = ReachabilityInfo::Parameters(ctx.battle.battleGetMySide(), &actor, dstHex, knownAccessible);
			const auto & reachability = ctx.battle.getReachability(params);
			const auto & attackableHexes = target.getAttackableHexes(&actor);
			bool threat = std::ranges::any_of(
				attackableHexes,
				[&speed, &reachability](const BattleHex & bh)
				{
					return reachability.distances.at(bh.toInt()) <= speed;
				}
			);

			expect(threat, "EDGE_UNIT_BECOMES_MELEE_TARGET_AFTER_ACTION: unit cannot reach any hex adjacent to action destination");
		}
	}

	void Verify_EDGE_UNIT_BECOMES_SHOOT_TARGET_AFTER_ACTION(const Context & ctx)
	{
		for(const auto & edge : ctx.G.getAll<E::Unit_BecomesShootTargetAfter_Action>())
		{
			const auto & target = edge->srcNode->cstack;
			const auto & actor = edge->dstNode->by->cstack;

			expect(actor.canShoot(), "EDGE_UNIT_BECOMES_SHOOT_TARGET_AFTER_ACTION: actor cannot shoot");

			// NOTE: this will be incorrect if either stack is berserk
			expect(target.unitSide() != actor.unitSide(), "EDGE_UNIT_BECOMES_SHOOT_TARGET_AFTER_ACTION: actor and target are on the same side");

			if(actor.canShootBlocked() || actor.hasBonusOfType(BonusType::SIEGE_WEAPON))
				continue;

			const auto & dstHex = edge->dstNode->endsAt.at(0)->bhex;
			const auto & surroundingHexes = actor.getSurroundingHexes(dstHex);

			for(const auto & nbhex : surroundingHexes)
			{
				const auto & blockers = ctx.battle.battleGetUnitsIf(
					[&nbhex, &actor](const battle::Unit * u)
					{
						// NOTE: this will be incorrect if either stack is berserk
						return u->unitSide() != actor.unitSide() && u->alive() && u->coversPos(nbhex);
					}
				);

				expect(blockers.empty(), "EDGE_UNIT_BECOMES_SHOOT_TARGET_AFTER_ACTION: actor will be blocked");
			}
		}
	}

	void Verify_EDGE_HEX_BECOMES_MELEE_TARGET_AFTER_ACTION(const Context & ctx)
	{
		for(const auto & edge : ctx.G.getAll<E::Hex_BecomesMeleeTargetAfter_Action>())
		{
			const auto & targetHex = edge->srcNode->bhex;
			const auto & action = edge->dstNode;
			const auto & moveDest = action->endsAt.at(0)->bhex;
			const auto & actor = action->by->cstack;

			/*
			 * Returned hexes for x.getNeighbouringTilesDoubleWide(LEFT_SIDE):
			 *  . . . . . . . . . . . .
			 * . . . . o o o . . . . . .  Defender=wide LEFT defender
			 *  . . . o ~ x o . . . . .   Attacker=small attacker
			 * . . . . o o o . . . . . .
			 *  . . . . . . . . . . . .
			 * 		^^^^^
			 * this is the same as the attack positions for wide *RIGHT attacker*.
			 */

			const auto otherside = actor.unitSide() == BattleSide::LEFT_SIDE ? BattleSide::RIGHT_SIDE : BattleSide::LEFT_SIDE;

			const auto attackPositions_ = actor.doubleWide() ? targetHex.getNeighbouringTilesDoubleWide(otherside) : targetHex.getNeighbouringTiles();

			const auto attackPositions = std::vector<BattleHex>(attackPositions_.begin(), attackPositions_.end());

			// From the actor's POV, accessible hexes are:
			// 1. its own/current hexes
			// 2. its new/hypothetical hexes (where the actor will move to)
			auto knownAccessible = actor.getHexes();
			for(const auto & bhex : actor.getHexes(moveDest))
				knownAccessible.checkAndPush(bhex);

			const auto & params = ReachabilityInfo::Parameters(ctx.battle.battleGetMySide(), &actor, moveDest, knownAccessible);
			const auto & reachability = ctx.battle.getReachability(params);

			const bool attackable = std::ranges::any_of(
				attackPositions,
				[&reachability, &actor](const BattleHex & bh)
				{
					return reachability.distances.at(bh.toInt()) <= actor.getMovementRange();
				}
			);

			if(!attackable)
				// NOTE: this will be incorrect if either stack is berserk
				expect(attackable, "EDGE_HEX_BECOMES_MELEE_TARGET_AFTER_ACTION: cannot reach any neighbouring hex: " + edge->name());
		}
	}

	void Verify_EDGE_HEX_BECOMES_SHOOT_TARGET_AFTER_ACTION(const Context & ctx)
	{
		for(const auto & edge : ctx.G.getAll<E::Hex_BecomesShootTargetAfter_Action>())
		{
			const auto & action = edge->dstNode;
			const auto & actor = action->by->cstack;

			expect(actor.canShoot(), "EDGE_HEX_BECOMES_SHOOT_TARGET_AFTER_ACTION: actor unit cannot shoot");

			const bool unblockable = actor.canShootBlocked() || actor.hasBonusOfType(BonusType::SIEGE_WEAPON);

			// Nothing more to check
			if(unblockable)
				continue;

			bool willBeBlocked = std::ranges::any_of(
				actor.getSurroundingHexes(action->endsAt.at(0)->bhex),
				[&ctx, &actor](const BattleHex & bh)
				{
					const auto * unit = ctx.battle.battleGetUnitByPos(bh);
					// XXX: this will not work correctly if either stack is berserk
					return unit && unit->unitSide() != actor.unitSide();
				}
			);

			// NOTE: this will be incorrect if either stack is berserk
			expect(!willBeBlocked, "EDGE_HEX_BECOMES_SHOOT_TARGET_AFTER_ACTION: actor will be blocked");
		}
	}
}

// This function used during model development and is never called otherwise
void Verify(const State * state) // NOLINT(readability-function-cognitive-complexity)
{

	const auto ctx = BuildContext(state);

	expect(ctx.astack || ctx.ended, "astack is NULL, but ended is not true");

	for(int i = 0; i < EU(S15::Graph::ElementType::_count); ++i)
	{
		using ET = S15::Graph::ElementType;
		const auto et = ET(i);

		switch(et)
		{
			case ET::NODE_GLOBAL:
				Verify_NODE_GLOBAL(ctx);
				break;
			case ET::NODE_PLAYER:
				Verify_NODE_PLAYER(ctx);
				break;
			case ET::NODE_UNIT:
				Verify_NODE_UNIT(ctx);
				break;
			case ET::NODE_HEX:
				Verify_NODE_HEX(ctx);
				break;
			case ET::NODE_ACTION:
				Verify_NODE_ACTION(ctx);
				break;
			case ET::EDGE_GLOBAL_TO_PLAYER:
			case ET::EDGE_PLAYER_TO_GLOBAL:
			case ET::EDGE_GLOBAL_TO_UNIT:
			case ET::EDGE_UNIT_TO_GLOBAL:
			case ET::EDGE_GLOBAL_TO_HEX:
			case ET::EDGE_HEX_TO_GLOBAL:
			case ET::EDGE_GLOBAL_TO_ACTION:
				// nothing special about these
				break;
			case ET::EDGE_PLAYER_OWNS_UNIT:
				for(const auto & edge : ctx.G.getAll<E::Player_Owns_Unit>())
				{
					expect(ctx.battle.sideToPlayer(edge->srcNode->side) == edge->dstNode->cstack.getOwner(), "EDGE_PLAYER_OWNS_UNIT: owner mismatch");
					expect(
						ctx.G.getEdgeBySrcDst<E::Unit_OwnedBy_Player>(edge->dstNode, edge->srcNode) != nullptr,
						"EDGE_PLAYER_OWNS_UNIT: no corresponding reverse edge"
					);
				}

				break;
			case ET::EDGE_UNIT_OWNED_BY_PLAYER:
				for(const auto & edge : ctx.G.getAll<E::Unit_OwnedBy_Player>())
				{
					expect(edge->srcNode->cstack.getOwner() == ctx.battle.sideToPlayer(edge->dstNode->side), "EDGE_UNIT_OWNED_BY_PLAYER: owner mismatch");
					expect(
						ctx.G.getEdgeBySrcDst<E::Player_Owns_Unit>(edge->dstNode, edge->srcNode) != nullptr,
						"EDGE_UNIT_OWNED_BY_PLAYER: no corresponding reverse edge"
					);
				}
				break;
			case ET::EDGE_UNIT_OCCUPIES_HEX:
				for(const auto & edge : ctx.G.getAll<E::Unit_Occupies_Hex>())
					expect(edge->srcNode->cstack.getHexes().contains(edge->dstNode->bhex), "stack does not occupy bhex");
				break;
			case ET::EDGE_HEX_OCCUPIED_BY_UNIT:
				for(const auto & edge : ctx.G.getAll<E::Hex_OccupiedBy_Unit>())
					expect(edge->dstNode->cstack.getHexes().contains(edge->srcNode->bhex), "stack does not occupy bhex");
				break;
			case ET::EDGE_ACTION_BY_UNIT:
				// Already checked that action->actor can perform the specific action
				// Here, check only that action->actor corresponds to this edge
				for(const auto & edge : ctx.G.getAll<E::Action_By_Unit>())
				{
					expect(edge->dstNode == edge->srcNode->by, "EDGE_ACTION_BY_UNIT: actor mismatch");
					expect(
						ctx.G.getEdgeBySrcDst<E::Unit_Has_Action>(edge->dstNode, edge->srcNode) != nullptr, "EDGE_ACTION_BY_UNIT: no corresponding reverse edge"
					);
				}
				break;
			case ET::EDGE_UNIT_HAS_ACTION:
				for(const auto & edge : ctx.G.getAll<E::Unit_Has_Action>())
					expect(edge->srcNode == edge->dstNode->by, "EDGE_UNIT_HAS_ACTION: actor mismatch");
				break;
			case ET::EDGE_HEX_ADJACENT_HEX:
				for(const auto & edge : ctx.G.getAll<E::Hex_Adjacent_Hex>())
					expect(
						edge->attr(E::Hex_Adjacent_Hex::A::DIRECTION) == EU(BattleHex::mutualPosition(edge->srcNode->bhex, edge->dstNode->bhex)),
						"EDGE_HEX_ADJACENT_HEX: direction mismatch"
					);
				break;
			case ET::EDGE_UNIT_ACTS_BEFORE_UNIT:
				Verify_EDGE_UNIT_ACTS_BEFORE_UNIT(ctx);
				break;
			case ET::EDGE_UNIT_MELEE_DMG_UNIT:
			case ET::EDGE_UNIT_SHOOT_DMG_UNIT:
				// too complex
				break;
			case ET::EDGE_UNIT_BLOCKS_UNIT:
				Verify_EDGE_UNIT_BLOCKS_UNIT(ctx);
				break;
			case ET::EDGE_ACTION_ENDS_AT_HEX:
			case ET::EDGE_HEX_IS_END_OF_ACTION:
				// Nothing to check
			case ET::EDGE_ACTION_BLOCKS_UNIT:
				Verify_EDGE_ACTION_BLOCKS_UNIT(ctx);
				break;
			case ET::EDGE_UNIT_BLOCKED_BY_ACTION:
				Verify_EDGE_UNIT_BLOCKED_BY_ACTION(ctx);
				break;
			case ET::EDGE_UNIT_BECOMES_MELEE_THREAT_AFTER_ACTION:
				Verify_EDGE_UNIT_BECOMES_MELEE_THREAT_AFTER_ACTION(ctx);
				break;
			case ET::EDGE_UNIT_BECOMES_SHOOT_THREAT_AFTER_ACTION:
				Verify_EDGE_UNIT_BECOMES_SHOOT_THREAT_AFTER_ACTION(ctx);
				break;
			case ET::EDGE_UNIT_IS_MELEED_BY_ACTION:
				for(const auto & edge : ctx.G.getAll<E::Unit_IsMeleedBy_Action>())
				{
					if(!edge->isPrimaryTarget)
						// TODO: verify non-primary targets (e.g. dragon breath 2nd hex)
						continue;

					expect(edge->srcNode == edge->dstNode->target, "EDGE_UNIT_IS_MELEED_BY_ACTION: target mismatch: " + edge->name());
					expect(
						CStack::isMeleeAttackPossible(&edge->dstNode->by->cstack, &edge->srcNode->cstack, edge->dstNode->endsAt.at(0)->bhex),
						"EDGE_UNIT_IS_MELEED_BY_ACTION: attack not possible: " + edge->name()
					);
				}
				break;
			case ET::EDGE_UNIT_IS_SHOT_BY_ACTION:
				for(const auto & edge : ctx.G.getAll<E::Unit_IsShotBy_Action>())
				{
					const auto & shooter = edge->dstNode->by->cstack;
					expect(shooter.canShoot(), "EDGE_UNIT_IS_SHOT_BY_ACTION: unit cannot shoot");
					expect(
						shooter.canShootBlocked() || shooter.hasBonusOfType(BonusType::SIEGE_WEAPON) || !ctx.battle.battleIsUnitBlocked(&shooter),
						"EDGE_UNIT_IS_SHOT_BY_ACTION: shooter is blocked"
					);

					if(!edge->isPrimaryTarget)
						// TODO: verify non-primary targets (e.g. magog fireball)
						continue;

					expect(edge->srcNode == edge->dstNode->target, "EDGE_UNIT_IS_SHOT_BY_ACTION: target mismatch: " + edge->name());
				}
				break;
			case ET::EDGE_UNIT_BECOMES_MELEE_TARGET_AFTER_ACTION:
				Verify_EDGE_UNIT_BECOMES_MELEE_TARGET_AFTER_ACTION(ctx);
				break;
			case ET::EDGE_UNIT_BECOMES_SHOOT_TARGET_AFTER_ACTION:
				Verify_EDGE_UNIT_BECOMES_SHOOT_TARGET_AFTER_ACTION(ctx);
				break;
			case ET::EDGE_HEX_BECOMES_MELEE_TARGET_AFTER_ACTION:
				Verify_EDGE_HEX_BECOMES_MELEE_TARGET_AFTER_ACTION(ctx);
				break;
			case ET::EDGE_HEX_BECOMES_SHOOT_TARGET_AFTER_ACTION:
				Verify_EDGE_HEX_BECOMES_SHOOT_TARGET_AFTER_ACTION(ctx);
				break;
			default:
				throw std::runtime_error("Unexpected ElementType: " + std::to_string(i));
				break;
		}
		static_assert(static_cast<int>(S15::Graph::ElementType::_count) == 35);
	}
}
}
