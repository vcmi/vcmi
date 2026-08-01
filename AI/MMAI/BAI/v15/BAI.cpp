/*
 * BAI.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "AI/BattleAI/BattleEvaluator.h"
#include "BAI/v15/graph/nodes/action.h"
#include "battle/BattleAction.h"
#include "battle/BattleStateInfoForRetreat.h"
#include "battle/CBattleInfoEssentials.h"
#include "callback/CBattleCallback.h"

#include "BAI/v15/BAI.h"
#include "BAI/v15/render.h"
#include "BAI/v15/verify.h"
#include "common.h"
#include "schema/base.h"
#include "schema/v15/types.h"

namespace MMAI::BAI::V15
{

namespace N = Graph::Nodes;

using PA = S15::Graph::NodeAttributes::Player;
using ActionPtr = std::shared_ptr<const N::Action>;
using AT = S15::ActionType;

BAI::BAI(Schema::IModel * model, int version, const std::shared_ptr<Environment> & env, const std::shared_ptr<CBattleCallback> & cb, bool enableSpellsUsage)
	: model(model), version(version), logger(cb->getPlayerID()->toString()), env(env), cb(cb), enableSpellsUsage(enableSpellsUsage)
{
}

Schema::Action BAI::getNonRenderAction()
{
	NestedLogTag _("NN");

	const auto * s = state.get();
	auto action = model->getAction(s);
	while(action == Schema::ACTION_RENDER_ANSI)
	{
		logger.debug("Got a RENDER action");
		if(state->supdata->ansiRender.empty())
		{
			state->supdata->ansiRender = renderANSI();
			state->supdata->type = S15::ISupplementaryData::Type::ANSI_RENDER;
		}

		action = model->getAction(state.get());
	}
	state->supdata->ansiRender.clear();
	state->supdata->type = S15::ISupplementaryData::Type::REGULAR;
	return action;
}

std::unique_ptr<State> BAI::initState(const CPlayerBattleCallback * b)
{
	return std::make_unique<State>(version, cb->getPlayerID()->toString(), *b);
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
	state->onBattleEnd(*br, roundcounter);

	logger.debug("MMAI %s this battle.", (br->winner == battle->battleGetMySide() ? "won" : "lost"));

	if(resetting)
	{
		logger.info("Battle ended due to ACTION_RESET: nothing to do");
		return;
	}
	else if(lastAction == nullptr)
	{
		// no previous action means battle ended without giving us a turn (OK)
		// Happens if the enemy immediately retreats (we won)
		// or if the enemy one-shots us (we lost)
		logger.info("Battle ended without giving us a turn: nothing to do");
	}
	// else if(lastAction->actionType == AT::RETREAT)
	// {
	// 	logger.info("Battle ended due to ACTION_RETREAT: reporting terminal state, expecting ACTION_RESET");
	// 	auto a = getNonRenderAction();
	// 	ASSERT(a == Schema::ACTION_RESET, "expected ACTION_RESET, got: " + std::to_string(EI(a)));
	// }
	else
	{
		logger.debug("Battle ended normally: reporting terminal state, expecting ACTION_RESET");
		auto a = getNonRenderAction();
		ASSERT(a == Schema::ACTION_RESET, "expected ACTION_RESET, got: " + std::to_string(EI(a)));
	}

	if(getActionTotalCalls > 0)
		logger.info("MMAI stats after battle end: %d predictions, %d ms per prediction", getActionTotalCalls, getActionTotalMs / getActionTotalCalls);
	else
		logger.info("MMAI stats after battle end: 0 predictions");

	// BAI is destroyed after this call
	logger.debug("Leaving battleEnd, embracing death");
}

void BAI::battleStacksAttacked(const BattleID & bid, const std::vector<BattleStackAttacked> & bsa, bool ranged)
{
	state->onBattleStacksAttacked(bsa);
}

void BAI::battleTriggerEffect(const BattleID & bid, const BattleTriggerEffect & bte)
{
	state->onBattleTriggerEffect(bte);
}

void BAI::battleNewRound(const BattleID & bid)
{
	++roundcounter;
	logger.debug("rounds: %d", roundcounter);
};

void BAI::yourTacticPhase(const BattleID & bid, int distance)
{
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

	auto lv = state->G->getByExtraIndex<N::Player>(BattleSide::LEFT_SIDE)->attr(PA::ARMY_VALUE_NOW_REL);
	auto rv = state->G->getByExtraIndex<N::Player>(BattleSide::RIGHT_SIDE)->attr(PA::ARMY_VALUE_NOW_REL);
	auto vratio = static_cast<float>(lv) / static_cast<float>(rv);
	if(battle->battleGetMySide() == BattleSide::RIGHT_SIDE)
		vratio = 1 / vratio;

	logger.debug("Attempting a BattleAI spellcast");
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
			maxdmg = static_cast<int>(dmg);
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
				return N::Unit::GetValue(a->unitType()) < N::Unit::GetValue(b->unitType());
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
		logger.debug("Can't flee or surrender.");
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

	logger.info("Will ask for surrender/retreat decision...");
	return cb->makeSurrenderRetreatDecision(bid, bs);
}

namespace
{
	BattleAction ToBattleAction(const CPlayerBattleCallback & battle, const ActionPtr & a, const CStack * acstack)
	{
		switch(a->actionType)
		{
			// case AT::RETREAT:
			// 	assert(battle.battleCanFlee());
			// 	return BattleAction::makeRetreat(battle.battleGetMySide());
			case AT::WAIT:
				assert(a->by && &a->by->cstack == acstack);
				assert(!acstack->waitedThisTurn);
				return BattleAction::makeWait(acstack);
			case AT::DEFEND:
				assert(a->by && &a->by->cstack == acstack);
				return BattleAction::makeDefend(acstack);
			case AT::MOVE:
				assert(a->by && &a->by->cstack == acstack);
				assert(a->endsAt.size() > 0);
				return BattleAction::makeMove(acstack, a->endsAt.at(0)->bhex);
			case AT::AMOVE:
				assert(a->by && &a->by->cstack == acstack);
				assert(a->endsAt.size() > 0);
				assert(a->target && CStack::isMeleeAttackPossible(acstack, &a->target->cstack, a->endsAt.front()->bhex));
				return BattleAction::makeMeleeAttack(acstack, &a->target->cstack, a->endsAt.front()->bhex);
			case AT::SHOOT:
				assert(a->by && &a->by->cstack == acstack);
				assert(a->target && battle.battleCanShoot(acstack, a->target->cstack.getPosition()));
				return BattleAction::makeShotAttack(acstack, &a->target->cstack);
			default:
				throw std::runtime_error("Unexpected action type: " + std::to_string(static_cast<int>(a->actionType)));
		}
	}
}

void BAI::activeStack(const BattleID & bid, const CStack * astack)
{
	try
	{
		_activeStack(bid, astack);
	}
	catch(const std::exception & e)
	{
		logger.error("Falling back to BattleAI due to MMAI error: " + std::string(e.what()));
		auto evaluator = BattleEvaluator(env, cb, astack, *cb->getPlayerID(), bid, battle->battleGetMySide(), 1.0f, 2);
		cb->battleMakeUnitAction(bid, evaluator.selectStackAction(astack));
		return;
	}
}

void BAI::_activeStack(const BattleID & bid, const CStack * astack)
{
	if(const auto ba = maybeBuildAutoAction(astack, bid))
	{
		logger.info("Making automatic action with %s", astack->getDescription());
		cb->battleMakeUnitAction(bid, *ba);
		return;
	}

	// Guard against infinite battles
	// (print warning once, make only fallback actions from there on)
	if(!inFallback && getActionTotalCalls >= 100)
	{
		logger.warn("Reached 100 predictions, will fall back to BattleAI until this combat ends");
		inFallback = true;
	}

	if(inFallback)
	{
		auto evaluator = BattleEvaluator(env, cb, astack, *cb->getPlayerID(), bid, battle->battleGetMySide(), 1.0f, 2);
		cb->battleMakeUnitAction(bid, evaluator.selectStackAction(astack));
		return;
	}

	state->onActiveStack(astack, roundcounter);

	if(maybeCastSpell(astack, bid))
		return;

	auto concede = maybeFleeOrSurrender(bid);
	if(concede)
	{
		logger.info("Making retreat/surrender action.");
		cb->battleMakeUnitAction(bid, *concede);
		return;
	}

	logger.debug("Not conceding.");

	auto t0 = std::chrono::steady_clock::now();
	int a = getNonRenderAction();
	auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
	getActionTotalMs += static_cast<int>(dt);
	getActionTotalCalls += 1;

	allactions.push_back(a);

	// battle should have ended instead
	ASSERT(!resetting, "got activeStack(), but resetting was already true");

	if(a == Schema::ACTION_RESET)
	{
		// XXX: retreat is always allowed for ML, limited by action mask only
		logger.debug("Received ACTION_RESET, will retreat in order to reset battle");
		resetting = true;
		cb->battleMakeUnitAction(bid, BattleAction::makeRetreat(battle->battleGetMySide()));
		return;
	}

	logger.debug("Received action: " + std::to_string(a));
	auto action = state->G->getById<N::Action>(a);
	ASSERT(action->isActive || resetting, "expected active action");

	lastAction = action;

	auto ba = ToBattleAction(*battle, action, astack);
	logger.debug(action->humanName(battle->battleGetMySide()));

	if(isMMAIAutoRender())
		std::cout << "\n" << Render(state.get(), action) << "\n";

	if(isMMAIAutoVerify())
		Verify(state.get());

	cb->battleMakeUnitAction(bid, ba);
}

std::string BAI::renderANSI() const
{
	const auto str = Render(state.get(), lastAction);

	try
	{
		Verify(state.get());
	}
	catch(const std::exception & e)
	{
		try
		{
			std::cout << e.what() << "\n";
			std::cout << "Disaster render:\n";
			std::cout << Render(state.get(), lastAction) << "\n";
		}
		catch(std::exception & e2)
		{
			std::cerr << "(failed: " << e2.what() << ")\n";
		}
		throw;
	}

	return str;
}
}
