/*
 * BAI.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "battle/BattleAction.h"
#include "battle/BattleStateInfoForRetreat.h"
#include "battle/CBattleInfoEssentials.h"

#include "BAI/base.h"
#include "BAI/v13/BAI.h"
#include "BAI/v13/action.h"
#include "BAI/v13/hexaction.h"
#include "BAI/v13/hexactmask.h"
#include "BAI/v13/render.h"
#include "BAI/v13/supplementary_data.h"
#include "common.h"
#include "schema/v13/types.h"

#include "AI/BattleAI/BattleEvaluator.h"
#include <algorithm>
#include <optional>

namespace MMAI::BAI::V13
{

using ErrorCode = Schema::V13::ErrorCode;
using PA = Schema::V13::PlayerAttribute;

Schema::Action BAI::getNonRenderAction()
{
	// info("getNonRenderAciton called with result type: " + std::to_string(result->type));
	const auto * s = state.get();
	auto action = model->getAction(s);
	debug("Got action: " + std::to_string(action));
	while(action == Schema::ACTION_RENDER_ANSI)
	{
		if(state->supdata->ansiRender.empty())
		{
			state->supdata->ansiRender = renderANSI();
			state->supdata->type = Schema::V13::ISupplementaryData::Type::ANSI_RENDER;
		}

		// info("getNonRenderAciton (loop) called with result type: " + std::to_string(res.type));
		action = model->getAction(state.get());
	}
	state->supdata->ansiRender.clear();
	state->supdata->type = Schema::V13::ISupplementaryData::Type::REGULAR;
	return action;
}

std::unique_ptr<State> BAI::initState(const CPlayerBattleCallback * b)
{
	return std::make_unique<State>(version, colorname, b);
}

void BAI::battleStart(
	const BattleID & bid,
	const CCreatureSet * army1,
	const CCreatureSet * army2,
	int3 tile,
	const CGHeroInstance * hero1,
	const CGHeroInstance * hero2,
	BattleSide side,
	bool replayAllowed
)
{
	Base::battleStart(bid, army1, army2, tile, hero1, hero2, side, replayAllowed);
	battle = cb->getBattle(bid);
	state = initState(battle.get());
	getActionTotalMs = 0;
	getActionTotalCalls = 0;
}

// XXX: battleEnd() is NOT called by CPlayerInterface (i.e. GUI)
//      However, it's called by AAI (i.e. headless) and that's all we want
//      since the terminal result is needed only during training.
void BAI::battleEnd(const BattleID & bid, const BattleResult * br, QueryID queryID)
{
	Base::battleEnd(bid, br, queryID);
	state->onBattleEnd(br);

	debug("MMAI %s this battle.", (br->winner == battle->battleGetMySide() ? "won" : "lost"));

	// Check if battle ended normally or was forced via a RETREAT action
	if(state->action == nullptr)
	{
		// no previous action means battle ended without giving us a turn (OK)
		// Happens if the enemy immediately retreats (we won)
		// or if the enemy one-shots us (we lost)
		info("Battle ended without giving us a turn: nothing to do");
	}
	else if(state->action->action == Schema::ACTION_RETREAT)
	{
		if(resetting)
		{
			// this is an intended restart (i.e. converted ACTION_RESTART)
			info("Battle ended due to ACTION_RESET: nothing to do");
		}
		else
		{
			// this is real retreat
			info("Battle ended due to ACTION_RETREAT: reporting terminal state, expecting ACTION_RESET");
			auto a = getNonRenderAction();
			ASSERT(a == Schema::ACTION_RESET, "expected ACTION_RESET, got: " + std::to_string(EI(a)));
		}
	}
	else
	{
		debug("Battle ended normally: reporting terminal state, expecting ACTION_RESET");
		auto a = getNonRenderAction();
		ASSERT(a == Schema::ACTION_RESET, "expected ACTION_RESET, got: " + std::to_string(EI(a)));
	}

	if(getActionTotalCalls > 0)
		info("MMAI stats after battle end: %d predictions, %d ms per prediction", getActionTotalCalls, getActionTotalMs / getActionTotalCalls);
	else
		info("MMAI stats after battle end: 0 predictions");

	// BAI is destroyed after this call
	debug("Leaving battleEnd, embracing death");
}

void BAI::battleStacksAttacked(const BattleID & bid, const std::vector<BattleStackAttacked> & bsa, bool ranged)
{
	Base::battleStacksAttacked(bid, bsa, ranged);
	state->onBattleStacksAttacked(bsa);
}

void BAI::battleTriggerEffect(const BattleID & bid, const BattleTriggerEffect & bte)
{
	Base::battleTriggerEffect(bid, bte);
	state->onBattleTriggerEffect(bte);
}

void BAI::yourTacticPhase(const BattleID & bid, int distance)
{
	Base::yourTacticPhase(bid, distance);
	cb->battleMakeTacticAction(bid, BattleAction::makeEndOFTacticPhase(battle->battleGetTacticsSide()));
}

bool BAI::maybeCastSpell(const CStack * astack, const BattleID & bid)
{
	if(!enableSpellsUsage)
		return false;

	const auto * hero = battle->battleGetMyHero();

	if(!hero)
		return false;

	if(battle->battleCanCastSpell(hero, spells::Mode::HERO) != ESpellCastProblem::OK)
		return false;

	auto lv = state->lpstats->getAttr(PA::ARMY_VALUE_NOW_ABS);
	auto rv = state->rpstats->getAttr(PA::ARMY_VALUE_NOW_ABS);
	auto vratio = static_cast<float>(lv) / rv;
	if(battle->battleGetMySide() == BattleSide::RIGHT_SIDE)
		vratio = 1 / vratio;

	logAi->debug("Attempting a BattleAI spellcast");
	auto evaluator = BattleEvaluator(env, cb, astack, *cb->getPlayerID(), bid, battle->battleGetMySide(), vratio, 2);
	return evaluator.attemptCastingSpell(astack);
}

std::shared_ptr<BattleAction> BAI::maybeBuildAutoAction(const CStack * astack, const BattleID & bid) const
{
	if(astack->creatureId() == CreatureID::FIRST_AID_TENT)
	{
		const CStack * target = nullptr;
		auto maxdmg = 0;
		for(const auto * stack : battle->battleGetStacks(CBattleInfoEssentials::ONLY_MINE))
		{
			auto dmg = stack->getMaxHealth() - stack->getFirstHPleft();
			if(dmg <= maxdmg)
				continue;
			maxdmg = dmg;
			target = stack;
		}

		if(target)
		{
			return std::make_shared<BattleAction>(BattleAction::makeHeal(astack, target));
		}
	}
	else if(astack->creatureId() == CreatureID::CATAPULT)
	{
		if(!astack->canShoot())
			// out of ammo
			return std::make_shared<BattleAction>(BattleAction::makeDefend(astack)); // out of ammo (arrow towers have 99 shots)

		auto ba = std::make_shared<BattleAction>();
		ba->side = astack->unitSide();
		ba->stackNumber = astack->unitId();
		ba->actionType = EActionType::CATAPULT;

		if(battle->battleGetGateState() == EGateState::CLOSED)
		{
			ba->aimToHex(battle->wallPartToBattleHex(EWallPart::GATE));
			return ba;
		}

		using WP = EWallPart;
		auto wallparts = {WP::KEEP, WP::BOTTOM_TOWER, WP::UPPER_TOWER, WP::BELOW_GATE, WP::OVER_GATE, WP::BOTTOM_WALL, WP::UPPER_WALL};

		for(auto wp : wallparts)
		{
			using WS = EWallState;
			auto ws = battle->battleGetWallState(wp);
			if(ws == WS::REINFORCED || ws == WS::INTACT || ws == WS::DAMAGED)
			{
				ba->aimToHex(battle->wallPartToBattleHex(wp));
				return ba;
			}
		}

		// no walls left
		return std::make_shared<BattleAction>(BattleAction::makeDefend(astack)); // out of ammo (arrow towers have 99 shots)
	}
	else if(astack->creatureId() == CreatureID::ARROW_TOWERS)
	{
		if(!astack->canShoot())
			// out of ammo (arrow towers have 99 shots)
			return std::make_shared<BattleAction>(BattleAction::makeDefend(astack));

		auto allstacks = battle->battleGetStacks(CBattleInfoEssentials::ONLY_ENEMY);
		auto target = std::ranges::max_element(
			allstacks,
			[](const CStack * a, const CStack * b)
			{
				return Stack::GetValue(a->unitType()) < Stack::GetValue(b->unitType());
			}
		);

		ASSERT(target != allstacks.end(), "Could not find an enemy stack to attack. Falling back to a defend.");
		return std::make_shared<BattleAction>(BattleAction::makeShotAttack(astack, *target));
	}

	return nullptr;
}

std::optional<BattleAction> BAI::maybeFleeOrSurrender(const BattleID & bid)
{
	BattleStateInfoForRetreat bs;

	bs.canFlee = battle->battleCanFlee();
	bs.canSurrender = battle->battleCanSurrender(*cb->getPlayerID());
	if(!bs.canFlee && !bs.canSurrender)
	{
		logAi->debug("Can't flee or surrender.");
		return std::nullopt;
	}

	bs.ourSide = battle->battleGetMySide();
	bs.ourHero = battle->battleGetMyHero();
	bs.enemyHero = nullptr;

	for(const auto * stack : battle->battleGetAllStacks(false))
	{
		if(stack->alive())
		{
			if(stack->unitSide() == bs.ourSide)
			{
				bs.ourStacks.push_back(stack);
			}
			else
			{
				bs.enemyStacks.push_back(stack);
				bs.enemyHero = battle->battleGetOwnerHero(stack);
			}
		}
	}

	logAi->info("Making surrender/retreat decision...");
	return cb->makeSurrenderRetreatDecision(bid, bs);
}

void BAI::activeStack(const BattleID & bid, const CStack * astack)
{
	Base::activeStack(bid, astack);

	try
	{
		_activeStack(bid, astack);
	}
	catch(const std::exception & e)
	{
		error("Falling back to BattleAI due to MMAI error: " + std::string(e.what()));
		auto evaluator = BattleEvaluator(env, cb, astack, *cb->getPlayerID(), bid, battle->battleGetMySide(), 1.0f, 2);
		cb->battleMakeUnitAction(bid, evaluator.selectStackAction(astack));
		return;
	}
}

void BAI::_activeStack(const BattleID & bid, const CStack * astack)
{
	auto ba = maybeBuildAutoAction(astack, bid);

	if(ba)
	{
		info("Making automatic action with %s", astack->getDescription());
		cb->battleMakeUnitAction(bid, *ba);
		return;
	}

	// Guard against infinite battles
	// (print warning once, make only fallback actions from there on)
	if(!inFallback && getActionTotalCalls >= 100)
	{
		warn("Reached 100 predictions, will fall back to BattleAI until this combat ends");
		inFallback = true;
	}

	if(inFallback)
	{
		auto evaluator = BattleEvaluator(env, cb, astack, *cb->getPlayerID(), bid, battle->battleGetMySide(), 1.0f, 2);
		cb->battleMakeUnitAction(bid, evaluator.selectStackAction(astack));
		return;
	}

	state->onActiveStack(astack);

	if(maybeCastSpell(astack, bid))
		return;

	if(state->battlefield->astack == nullptr)
	{
		error(
			"The current stack is not part of the state. "
			"This should NOT happen. "
			"Falling back to a wait/defend action."
		);
		auto fa = astack->waitedThisTurn ? BattleAction::makeDefend(astack) : BattleAction::makeWait(astack);
		cb->battleMakeUnitAction(bid, fa);
		return;
	}

	auto concede = maybeFleeOrSurrender(bid);
	if(concede)
	{
		info("Making retreat/surrender action.");
		cb->battleMakeUnitAction(bid, *concede);
		return;
	}

	logAi->debug("Not conceding.");

	while(true)
	{
		auto t0 = std::chrono::steady_clock::now();
		auto a = getNonRenderAction();
		auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
		getActionTotalMs += dt;
		getActionTotalCalls += 1;

		allactions.push_back(a);

		if(a == Schema::ACTION_RESET)
		{
			// XXX: retreat is always allowed for ML, limited by action mask only
			debug("Received ACTION_RESET, converting to ACTION_RETREAT in order to reset battle");
			a = Schema::ACTION_RETREAT;
			resetting = true;
		}

		state->action = std::make_unique<Action>(a, state->battlefield.get(), colorname);
		debug("(%lld ms) Got action: %d: %s ", dt, a, state->action->name());

		ba = buildBattleAction();

		if(ba && (a != Schema::ACTION_RETREAT || resetting))
		{
			debug("Action is VALID: " + state->action->name());
			errcounter = 0;
			cb->battleMakeUnitAction(bid, *ba);
			break;
		}
		else
		{
			++errcounter;
			error("Action is INVALID: " + state->action->name());
			if(errcounter > 10)
			{
				warn("Got 10 consecutive errors, will fall back to BattleAI until this combat ends");
				auto evaluator = BattleEvaluator(env, cb, astack, *cb->getPlayerID(), bid, battle->battleGetMySide(), 1.0f, 2);
				cb->battleMakeUnitAction(bid, evaluator.selectStackAction(astack));
				inFallback = true;
				break;
			}
		}
	}
}

std::shared_ptr<BattleAction> BAI::buildBattleAction()
{
	ASSERT(state->battlefield, "Cannot build battle action if state->battlefield is missing");
	auto * action = state->action.get();
	const auto * bf = state->battlefield.get();
	const auto * acstack = bf->astack->cstack;

	auto [x, y] = Hex::CalcXY(acstack->getPosition());
	auto & hex = bf->hexes->at(y).at(x);
	std::shared_ptr<BattleAction> res = nullptr;

	if(!state->action->hex)
	{
		switch(static_cast<GlobalAction>(state->action->action))
		{
			case GlobalAction::RETREAT:
				res = std::make_shared<BattleAction>(BattleAction::makeRetreat(battle->battleGetMySide()));
				break;
			case GlobalAction::WAIT:
				if(acstack->waitedThisTurn)
				{
					ASSERT(!state->actmask.at(EI(GlobalAction::WAIT)), "mask allowed wait when stack has already waited");
					state->supdata->errcode = ErrorCode::ALREADY_WAITED;
					error("Action error: %s (%d): ALREADY_WAITED", action->name(), EI(action->action));
					return res;
				}
				res = std::make_shared<BattleAction>(BattleAction::makeWait(acstack));
				break;
			default:
				THROW_FORMAT("Unexpected non-hex action: %d", state->action->action);
		}

		return res;
	}

	// With action masking, invalid actions should never occur
	// However, for manual playing/testing, it's bad to raise exceptions
	// => return errcode (Gym env will raise an exception if errcode > 0)
	const auto & bhex = action->hex->bhex;
	auto & stack = action->hex->stack; // may be null
	auto mask = HexActMask(action->hex->attr(HexAttribute::ACTION_MASK));
	if(mask.test(EI(action->hexaction)))
	{
		// Action is VALID
		// XXX: Do minimal asserts to prevent bugs with nullptr deref
		//      Server will log any attempted invalid actions otherwise
		switch(action->hexaction)
		{
			case HexAction::MOVE:
			{
				auto ba = (bhex == acstack->getPosition()) ? BattleAction::makeDefend(acstack) : BattleAction::makeMove(acstack, bhex);
				res = std::make_shared<BattleAction>(ba);
			}
			break;
			case HexAction::SHOOT:
				ASSERT(stack, "no target to shoot");
				res = std::make_shared<BattleAction>(BattleAction::makeShotAttack(acstack, stack->cstack));
				break;
			case HexAction::AMOVE_TR:
			case HexAction::AMOVE_R:
			case HexAction::AMOVE_BR:
			case HexAction::AMOVE_BL:
			case HexAction::AMOVE_L:
			case HexAction::AMOVE_TL:
			{
				const auto & edir = AMOVE_TO_EDIR.at(EI(action->hexaction));
				auto nbh = bhex.cloneInDirection(edir, false); // neighbouring bhex
				ASSERT(nbh.isAvailable(), "mask allowed attack to an unavailable hex #" + std::to_string(nbh.toInt()));
				const auto * estack = battle->battleGetStackByPos(nbh);
				ASSERT(estack, "no enemy stack for melee attack");
				res = std::make_shared<BattleAction>(BattleAction::makeMeleeAttack(acstack, nbh, bhex));
			}
			break;
			case HexAction::AMOVE_2TR:
			case HexAction::AMOVE_2R:
			case HexAction::AMOVE_2BR:
			case HexAction::AMOVE_2BL:
			case HexAction::AMOVE_2L:
			case HexAction::AMOVE_2TL:
			{
				ASSERT(acstack->doubleWide(), "got AMOVE_2 action for a single-hex stack");
				const auto & edir = AMOVE_TO_EDIR.at(EI(action->hexaction));
				auto obh = acstack->occupiedHex(bhex);
				auto nbh = obh.cloneInDirection(edir, false); // neighbouring bhex
				ASSERT(nbh.isAvailable(), "mask allowed attack to an unavailable hex #" + std::to_string(nbh.toInt()));
				const auto * estack = battle->battleGetStackByPos(nbh);
				ASSERT(estack, "no enemy stack for melee attack");
				res = std::make_shared<BattleAction>(BattleAction::makeMeleeAttack(acstack, nbh, bhex));
			}
			break;
			default:
				THROW_FORMAT("Unexpected hexaction: %d", EI(action->hexaction));
		}

		return res;
	}

	// Action is INVALID
	// XXX:
	// mask prevents certain actions, but during TESTING
	// those actions may be taken anyway.
	//
	// IF we are here, it means the mask disallows that action
	//
	// => *throw* errors here only if the mask SHOULD HAVE ALLOWED it
	//    and *set* regular, non-throw errors otherwise
	//
	handleUnexpectedAction(acstack, hex, action);
	ASSERT(state->supdata->errcode != ErrorCode::OK, "Could not identify why the action is invalid" + debugInfo(action, acstack, nullptr));

	return res;
}

void BAI::handleUnexpectedAction(const CStack * acstack, std::unique_ptr<Hex> & hex, Action * action)
{
	const auto & bhex = action->hex->bhex;
	auto & stack = action->hex->stack; // may be null
	auto rinfo = battle->getReachability(acstack);
	auto ainfo = battle->getAccessibility();

	switch(state->action->hexaction)
	{
		case HexAction::AMOVE_TR:
		case HexAction::AMOVE_R:
		case HexAction::AMOVE_BR:
		case HexAction::AMOVE_BL:
		case HexAction::AMOVE_L:
		case HexAction::AMOVE_TL:
		case HexAction::AMOVE_2TR:
		case HexAction::AMOVE_2R:
		case HexAction::AMOVE_2BR:
		case HexAction::AMOVE_2BL:
		case HexAction::AMOVE_2L:
		case HexAction::AMOVE_2TL:
		case HexAction::MOVE:
		{
			auto a = ainfo.at(action->hex->bhex.toInt());
			if(a == EAccessibility::OBSTACLE)
			{
				auto statemask = HexStateMask(hex->attr(HexAttribute::STATE_MASK));
				ASSERT(
					!statemask.test(EI(HexState::PASSABLE)),
					"accessibility is OBSTACLE, but hex state mask has PASSABLE set: " + statemask.to_string() + debugInfo(action, acstack, nullptr)
				);
				state->supdata->errcode = ErrorCode::HEX_BLOCKED;
				error("Action error: %s (%d): HEX_BLOCKED", action->name(), EI(action->action));
				break;
			}
			else if(a == EAccessibility::ALIVE_STACK)
			{
				auto bh = action->hex->bhex;
				if(bh.toInt() == acstack->getPosition().toInt())
				{
					// means we want to defend (moving to self)
					// or attack from same hex we're currently at
					// this should always be allowed
					ASSERT(false, "mask prevented (A)MOVE to own hex" + debugInfo(action, acstack, nullptr));
				}
				else if(bh.toInt() == acstack->occupiedHex().toInt())
				{
					ASSERT(
						rinfo.distances.at(bh.toInt()) == ReachabilityInfo::INFINITE_DIST,
						"mask prevented (A)MOVE to self-occupied hex" + debugInfo(action, acstack, nullptr)
					);
					// means we can't fit on our own back hex
				}

				// means we try to move onto another stack
				state->supdata->errcode = ErrorCode::HEX_BLOCKED;
				error("Action error: %s (%d): HEX_BLOCKED", action->name(), EI(action->action));
				break;
			}

			// only remaining is ACCESSIBLE
			ASSERT(a == EAccessibility::ACCESSIBLE, "accessibility should've been ACCESSIBLE, was: " = std::to_string(EI(a)));

			auto nbh = BattleHex{};

			if(action->hexaction < HexAction::AMOVE_2TR)
			{
				auto edir = AMOVE_TO_EDIR.at(EI(action->hexaction));
				nbh = bhex.cloneInDirection(edir, false);
			}
			else
			{
				if(!acstack->doubleWide())
				{
					state->supdata->errcode = ErrorCode::INVALID_DIR;
					error("Action error: %s (%d): INVALID_DIR", action->name(), EI(action->action));
					break;
				}

				auto edir = AMOVE_TO_EDIR.at(EI(action->hexaction));
				nbh = acstack->occupiedHex().cloneInDirection(edir, false);
			}

			if(!nbh.isAvailable())
			{
				state->supdata->errcode = ErrorCode::HEX_MELEE_NA;
				error("Action error: %s (%d): HEX_MELEE_NA", action->name(), EI(action->action));
				break;
			}

			const auto * estack = battle->battleGetStackByPos(nbh);

			if(!estack)
			{
				state->supdata->errcode = ErrorCode::STACK_NA;
				error("Action error: %s (%d): STACK_NA", action->name(), EI(action->action));
				break;
			}

			if(estack->unitSide() == acstack->unitSide())
			{
				state->supdata->errcode = ErrorCode::FRIENDLY_FIRE;
				error("Action error: %s (%d): FRIENDLY_FIRE", action->name(), EI(action->action));
				break;
			}
		}
		break;
		case HexAction::SHOOT:
			if(!stack)
			{
				state->supdata->errcode = ErrorCode::STACK_NA;
				error("Action error: %s (%d): STACK_NA", action->name(), EI(action->action));
				break;
			}
			else if(stack->cstack->unitSide() == acstack->unitSide())
			{
				state->supdata->errcode = ErrorCode::FRIENDLY_FIRE;
				error("Action error: %s (%d): FRIENDLY_FIRE", action->name(), EI(action->action));
				break;
			}
			else
			{
				ASSERT(!battle->battleCanShoot(acstack, bhex), "mask prevented SHOOT at a shootable bhex " + action->hex->name());
				state->supdata->errcode = ErrorCode::CANNOT_SHOOT;
				error("Action error: %s (%d): CANNOT_SHOOT", action->name(), EI(action->action));
				break;
			}
			break;
		default:
			THROW_FORMAT("Unexpected hexaction: %d", EI(action->hexaction));
	}
}

std::string BAI::debugInfo(Action * action, const CStack * astack, const BattleHex * const nbh)
{
	auto info = std::stringstream();
	info << "\n*** DEBUG INFO ***\n";
	info << "action: " << action->name() << " [" << action->action << "]\n";
	info << "action->hex->bhex.toInt() = " << action->hex->bhex.toInt() << "\n";

	auto ainfo = battle->getAccessibility();
	auto rinfo = battle->getReachability(astack);

	info << "ainfo[bhex]=" << EI(ainfo.at(action->hex->bhex.toInt())) << "\n";
	info << "rinfo.distances[bhex] <= astack->getMovementRange(): " << (rinfo.distances[action->hex->bhex.toInt()] <= astack->getMovementRange()) << "\n";

	info << "action->hex->name = " << action->hex->name() << "\n";

	for(int i = 0; i < action->hex->attrs.size(); i++)
		info << "action->hex->attrs[" << i << "] = " << EI(action->hex->attrs[i]) << "\n";

	info << "action->hex->hexactmask = ";
	info << HexActMask(action->hex->attr(HexAttribute::ACTION_MASK)).to_string();
	info << "\n";

	auto stack = action->hex->stack;
	if(stack)
	{
		info << "stack->cstack->getPosition().toInt()=" << stack->cstack->getPosition().toInt() << "\n";
		info << "stack->cstack->slot=" << stack->cstack->unitSlot() << "\n";
		info << "stack->cstack->doubleWide=" << stack->cstack->doubleWide() << "\n";
		info << "cb->battleCanShoot(stack->cstack)=" << battle->battleCanShoot(stack->cstack) << "\n";
	}
	else
	{
		info << "cstack: (nullptr)\n";
	}

	info << "astack->getPosition().toInt()=" << astack->getPosition().toInt() << "\n";
	info << "astack->slot=" << astack->unitSlot() << "\n";
	info << "astack->doubleWide=" << astack->doubleWide() << "\n";
	info << "cb->battleCanShoot(astack)=" << battle->battleCanShoot(astack) << "\n";

	if(nbh)
	{
		info << "nbh->toInt()=" << nbh->toInt() << "\n";
		info << "ainfo[nbh]=" << EI(ainfo.at(nbh->toInt())) << "\n";
		info << "rinfo.distances[nbh] <= astack->getMovementRange(): " << (rinfo.distances[nbh->toInt()] <= astack->getMovementRange()) << "\n";

		if(stack)
			info << "astack->isMeleeAttackPossible(...)=" << astack->isMeleeAttackPossible(astack, stack->cstack, *nbh) << "\n";
	}

	info << "\nACTION TRACE:\n";
	for(const auto & a : allactions)
		info << a << ",";

	info << "\nRENDER:\n";
	info << renderANSI();

	return info.str();
}

std::string BAI::renderANSI() const
{
	try
	{
		Verify(state.get());
	}
	catch(const std::exception & e)
	{
		try
		{
			std::cout << e.what();
			std::cout << "Disaster render:\n";
			std::cout << Render(state.get(), state->action.get()) << "\n";
		}
		catch(std::exception & e2)
		{
			std::cerr << "(failed: " << e2.what() << ")\n";
		}
		throw;
	}

	return Render(state.get(), state->action.get());
}

void BAI::actionStarted(const BattleID & bid, const BattleAction & action)
{
	Base::actionStarted(bid, action);
	state->onActionStarted(action);
};

void BAI::actionFinished(const BattleID & bid, const BattleAction & action)
{
	Base::actionFinished(bid, action);
	state->onActionFinished(action);
};
}
