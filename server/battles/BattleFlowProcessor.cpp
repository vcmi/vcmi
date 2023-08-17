/*
 * BattleFlowProcessor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleFlowProcessor.h"

#include "BattleProcessor.h"

#include "../CGameHandler.h"
#include "../CVCMIServer.h"
#include "../processors/HeroPoolProcessor.h"
#include "../queries/QueriesProcessor.h"
#include "../queries/BattleQueries.h"

#include "../../lib/ArtifactUtils.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CStack.h"
#include "../../lib/CondSh.h"
#include "../../lib/GameSettings.h"
#include "../../lib/ScopeGuard.h"
#include "../../lib/TerrainHandler.h"
#include "../../lib/UnlockGuard.h"
#include "../../lib/battle/BattleInfo.h"
#include "../../lib/battle/CUnitState.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/serializer/Cast.h"
#include "../../lib/spells/AbilityCaster.h"
#include "../../lib/spells/BonusCaster.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/spells/ISpellMechanics.h"
#include "../../lib/spells/ObstacleCasterProxy.h"
#include "../../lib/spells/Problem.h"

BattleFlowProcessor::BattleFlowProcessor(BattleProcessor * owner)
	: owner(owner)
	, gameHandler(nullptr)
{
}

void BattleFlowProcessor::setGameHandler(CGameHandler * newGameHandler)
{
	gameHandler = newGameHandler;
}

void BattleFlowProcessor::summonGuardiansHelper(std::vector<BattleHex> & output, const BattleHex & targetPosition, ui8 side, bool targetIsTwoHex) //return hexes for summoning two hex monsters in output, target = unit to guard
{
	int x = targetPosition.getX();
	int y = targetPosition.getY();

	const bool targetIsAttacker = side == BattleSide::ATTACKER;

	if (targetIsAttacker) //handle front guardians, TODO: should we handle situation when units start battle near opposite side of the battlefield? Cannot happen in normal H3...
		BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::RIGHT, false).cloneInDirection(BattleHex::EDir::RIGHT, false), output);
	else
		BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::LEFT, false).cloneInDirection(BattleHex::EDir::LEFT, false), output);

	//guardian spawn locations for four default position cases for attacker and defender, non-default starting location for att and def is handled in first two if's
	if (targetIsAttacker && ((y % 2 == 0) || (x > 1)))
	{
		if (targetIsTwoHex && (y % 2 == 1) && (x == 2)) //handle exceptional case
		{
			BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::TOP_RIGHT, false), output);
			BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::BOTTOM_RIGHT, false), output);
		}
		else
		{	//add back-side guardians for two-hex target, side guardians for one-hex
			BattleHex::checkAndPush(targetPosition.cloneInDirection(targetIsTwoHex ? BattleHex::EDir::TOP_LEFT : BattleHex::EDir::TOP_RIGHT, false), output);
			BattleHex::checkAndPush(targetPosition.cloneInDirection(targetIsTwoHex ? BattleHex::EDir::BOTTOM_LEFT : BattleHex::EDir::BOTTOM_RIGHT, false), output);

			if (!targetIsTwoHex && x > 2) //back guard for one-hex
				BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::LEFT, false), output);
			else if (targetIsTwoHex)//front-side guardians for two-hex target
			{
				BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::RIGHT, false).cloneInDirection(BattleHex::EDir::TOP_RIGHT, false), output);
				BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::RIGHT, false).cloneInDirection(BattleHex::EDir::BOTTOM_RIGHT, false), output);
				if (x > 3) //back guard for two-hex
					BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::LEFT, false).cloneInDirection(BattleHex::EDir::LEFT, false), output);
			}
		}

	}

	else if (!targetIsAttacker && ((y % 2 == 1) || (x < GameConstants::BFIELD_WIDTH - 2)))
	{
		if (targetIsTwoHex && (y % 2 == 0) && (x == GameConstants::BFIELD_WIDTH - 3)) //handle exceptional case... equivalent for above for defender side
		{
			BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::TOP_LEFT, false), output);
			BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::BOTTOM_LEFT, false), output);
		}
		else
		{
			BattleHex::checkAndPush(targetPosition.cloneInDirection(targetIsTwoHex ? BattleHex::EDir::TOP_RIGHT : BattleHex::EDir::TOP_LEFT, false), output);
			BattleHex::checkAndPush(targetPosition.cloneInDirection(targetIsTwoHex ? BattleHex::EDir::BOTTOM_RIGHT : BattleHex::EDir::BOTTOM_LEFT, false), output);

			if (!targetIsTwoHex && x < GameConstants::BFIELD_WIDTH - 3)
				BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::RIGHT, false), output);
			else if (targetIsTwoHex)
			{
				BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::LEFT, false).cloneInDirection(BattleHex::EDir::TOP_LEFT, false), output);
				BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::LEFT, false).cloneInDirection(BattleHex::EDir::BOTTOM_LEFT, false), output);
				if (x < GameConstants::BFIELD_WIDTH - 4)
					BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::RIGHT, false).cloneInDirection(BattleHex::EDir::RIGHT, false), output);
			}
		}
	}

	else if (!targetIsAttacker && y % 2 == 0)
	{
		BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::LEFT, false).cloneInDirection(BattleHex::EDir::TOP_LEFT, false), output);
		BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::LEFT, false).cloneInDirection(BattleHex::EDir::BOTTOM_LEFT, false), output);
	}

	else if (targetIsAttacker && y % 2 == 1)
	{
		BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::RIGHT, false).cloneInDirection(BattleHex::EDir::TOP_RIGHT, false), output);
		BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::RIGHT, false).cloneInDirection(BattleHex::EDir::BOTTOM_RIGHT, false), output);
	}
}

void BattleFlowProcessor::onBattleStarted()
{
	gameHandler->setBattle(gameHandler->gameState()->curB);
	assert(gameHandler->gameState()->curB);
	//TODO: pre-tactic stuff, call scripts etc.

	//Moat should be initialized here, because only here we can use spellcasting
	if (gameHandler->gameState()->curB->town && gameHandler->gameState()->curB->town->fortLevel() >= CGTownInstance::CITADEL)
	{
		const auto * h = gameHandler->gameState()->curB->battleGetFightingHero(BattleSide::DEFENDER);
		const auto * actualCaster = h ? static_cast<const spells::Caster*>(h) : nullptr;
		auto moatCaster = spells::SilentCaster(gameHandler->gameState()->curB->getSidePlayer(BattleSide::DEFENDER), actualCaster);
		auto cast = spells::BattleCast(gameHandler->gameState()->curB, &moatCaster, spells::Mode::PASSIVE, gameHandler->gameState()->curB->town->town->moatAbility.toSpell());
		auto target = spells::Target();
		cast.cast(gameHandler->spellEnv, target);
	}

	if (gameHandler->gameState()->curB->tacticDistance == 0)
		onTacticsEnded();
}

void BattleFlowProcessor::trySummonGuardians(const CStack * stack)
{
	if (!stack->hasBonusOfType(BonusType::SUMMON_GUARDIANS))
		return;

	std::shared_ptr<const Bonus> summonInfo = stack->getBonus(Selector::type()(BonusType::SUMMON_GUARDIANS));
	auto accessibility = gameHandler->getAccesibility();
	CreatureID creatureData = CreatureID(summonInfo->subtype);
	std::vector<BattleHex> targetHexes;
	const bool targetIsBig = stack->unitType()->isDoubleWide(); //target = creature to guard
	const bool guardianIsBig = creatureData.toCreature()->isDoubleWide();

	/*Chosen idea for two hex units was to cover all possible surrounding hexes of target unit with as small number of stacks as possible.
		For one-hex targets there are four guardians - front, back and one per side (up + down).
		Two-hex targets are wider and the difference is there are two guardians per side to cover 3 hexes + extra hex in the front
		Additionally, there are special cases for starting positions etc., where guardians would be outside of battlefield if spawned normally*/
	if (!guardianIsBig)
		targetHexes = stack->getSurroundingHexes();
	else
		summonGuardiansHelper(targetHexes, stack->getPosition(), stack->unitSide(), targetIsBig);

	for(auto hex : targetHexes)
	{
		if(accessibility.accessible(hex, guardianIsBig, stack->unitSide())) //without this multiple creatures can occupy one hex
		{
			battle::UnitInfo info;
			info.id = gameHandler->gameState()->curB->battleNextUnitId();
			info.count =  std::max(1, (int)(stack->getCount() * 0.01 * summonInfo->val));
			info.type = creatureData;
			info.side = stack->unitSide();
			info.position = hex;
			info.summoned = true;

			BattleUnitsChanged pack;
			pack.changedStacks.emplace_back(info.id, UnitChanges::EOperation::ADD);
			info.save(pack.changedStacks.back().data);
			gameHandler->sendAndApply(&pack);
		}
	}
}

void BattleFlowProcessor::castOpeningSpells()
{
	for (int i = 0; i < 2; ++i)
	{
		auto h = gameHandler->gameState()->curB->battleGetFightingHero(i);
		if (!h)
			continue;

		TConstBonusListPtr bl = h->getBonuses(Selector::type()(BonusType::OPENING_BATTLE_SPELL));

		for (auto b : *bl)
		{
			spells::BonusCaster caster(h, b);

			const CSpell * spell = SpellID(b->subtype).toSpell();

			spells::BattleCast parameters(gameHandler->gameState()->curB, &caster, spells::Mode::PASSIVE, spell);
			parameters.setSpellLevel(3);
			parameters.setEffectDuration(b->val);
			parameters.massive = true;
			parameters.castIfPossible(gameHandler->spellEnv, spells::Target());
		}
	}
}

void BattleFlowProcessor::onTacticsEnded()
{
	//initial stacks appearance triggers, e.g. built-in bonus spells
	auto initialStacks = gameHandler->gameState()->curB->stacks; //use temporary variable to outclude summoned stacks added to gameHandler->gameState()->curB->stacks from processing

	for (CStack * stack : initialStacks)
	{
		trySummonGuardians(stack);
		stackEnchantedTrigger(stack);
	}

	castOpeningSpells();

	// it is possible that due to opening spells one side was eliminated -> check for end of battle
	owner->checkBattleStateChanges();

	startNextRound(true);
	activateNextStack();
}

void BattleFlowProcessor::startNextRound(bool isFirstRound)
{
	BattleNextRound bnr;
	bnr.round = gameHandler->gameState()->curB->round + 1;
	logGlobal->debug("Round %d", bnr.round);
	gameHandler->sendAndApply(&bnr);

	auto obstacles = gameHandler->gameState()->curB->obstacles; //we copy container, because we're going to modify it
	for (auto &obstPtr : obstacles)
	{
		if (const SpellCreatedObstacle *sco = dynamic_cast<const SpellCreatedObstacle *>(obstPtr.get()))
			if (sco->turnsRemaining == 0)
				removeObstacle(*obstPtr);
	}

	const BattleInfo & curB = *gameHandler->gameState()->curB;

	for(auto stack : curB.stacks)
	{
		if(stack->alive() && !isFirstRound)
			stackEnchantedTrigger(stack);
	}
}

const CStack * BattleFlowProcessor::getNextStack()
{
	std::vector<battle::Units> q;
	gameHandler->gameState()->curB->battleGetTurnOrder(q, 1, 0, -1); //todo: get rid of "turn -1"

	if(q.empty())
		return nullptr;

	if(q.front().empty())
		return nullptr;

	auto next = q.front().front();
	const auto stack = dynamic_cast<const CStack *>(next);

	// regeneration takes place before everything else but only during first turn attempt in each round
	// also works under blind and similar effects
	if(stack && stack->alive() && !stack->waiting)
	{
		BattleTriggerEffect bte;
		bte.stackID = stack->unitId();
		bte.effect = vstd::to_underlying(BonusType::HP_REGENERATION);

		const int32_t lostHealth = stack->getMaxHealth() - stack->getFirstHPleft();
		if(stack->hasBonusOfType(BonusType::HP_REGENERATION))
			bte.val = std::min(lostHealth, stack->valOfBonuses(BonusType::HP_REGENERATION));

		if(bte.val) // anything to heal
			gameHandler->sendAndApply(&bte);
	}

	if(!next->willMove())
		return nullptr;

	return stack;
}

void BattleFlowProcessor::activateNextStack()
{
	//TODO: activate next round if next == nullptr
	const auto & curB = *gameHandler->gameState()->curB;

	// Find next stack that requires manual control
	for (;;)
	{
		const CStack * next = getNextStack();

		if (!next)
		{
			// No stacks to move - start next round
			startNextRound(false);
			next = getNextStack();
			if (!next)
				throw std::runtime_error("Failed to find valid stack to act!");
		}

		BattleUnitsChanged removeGhosts;

		for(auto stack : curB.stacks)
		{
			if(stack->ghostPending)
				removeGhosts.changedStacks.emplace_back(stack->unitId(), UnitChanges::EOperation::REMOVE);
		}

		if(!removeGhosts.changedStacks.empty())
			gameHandler->sendAndApply(&removeGhosts);

		if (!tryMakeAutomaticAction(next))
		{
			logGlobal->trace("Activating %s", next->nodeName());
			auto nextId = next->unitId();
			BattleSetActiveStack sas;
			sas.stack = nextId;
			gameHandler->sendAndApply(&sas);
			break;
		}
	}
}

bool BattleFlowProcessor::tryMakeAutomaticAction(const CStack * next)
{
	const auto & curB = *gameHandler->gameState()->curB;

	// check for bad morale => freeze
	int nextStackMorale = next->moraleVal();
	if(!next->hadMorale && !next->waited() && nextStackMorale < 0)
	{
		auto diceSize = VLC->settings()->getVector(EGameSettings::COMBAT_BAD_MORALE_DICE);
		size_t diceIndex = std::min<size_t>(diceSize.size()-1, -nextStackMorale);

		if(diceSize.size() > 0 && gameHandler->getRandomGenerator().nextInt(1, diceSize[diceIndex]) == 1)
		{
			//unit loses its turn - empty freeze action
			BattleAction ba;
			ba.actionType = EActionType::BAD_MORALE;
			ba.side = next->unitSide();
			ba.stackNumber = next->unitId();

			makeAutomaticAction(next, ba);
			return true;
		}
	}

	if (next->hasBonusOfType(BonusType::ATTACKS_NEAREST_CREATURE)) //while in berserk
	{
		logGlobal->trace("Handle Berserk effect");
		std::pair<const battle::Unit *, BattleHex> attackInfo = curB.getNearestStack(next);
		if (attackInfo.first != nullptr)
		{
			BattleAction attack;
			attack.actionType = EActionType::WALK_AND_ATTACK;
			attack.side = next->unitSide();
			attack.stackNumber = next->unitId();
			attack.aimToHex(attackInfo.second);
			attack.aimToUnit(attackInfo.first);

			makeAutomaticAction(next, attack);
			logGlobal->trace("Attacked nearest target %s", attackInfo.first->getDescription());
		}
		else
		{
			makeStackDoNothing(next);
			logGlobal->trace("No target found");
		}
		return true;
	}

	const CGHeroInstance * curOwner = gameHandler->battleGetOwnerHero(next);
	const int stackCreatureId = next->unitType()->getId();

	if ((stackCreatureId == CreatureID::ARROW_TOWERS || stackCreatureId == CreatureID::BALLISTA)
		&& (!curOwner || gameHandler->getRandomGenerator().nextInt(99) >= curOwner->valOfBonuses(BonusType::MANUAL_CONTROL, stackCreatureId)))
	{
		BattleAction attack;
		attack.actionType = EActionType::SHOOT;
		attack.side = next->unitSide();
		attack.stackNumber = next->unitId();

		//TODO: select target by priority

		const battle::Unit * target = nullptr;

		for(auto & elem : gameHandler->gameState()->curB->stacks)
		{
			if(elem->unitType()->getId() != CreatureID::CATAPULT
			   && elem->unitOwner() != next->unitOwner()
			   && elem->isValidTarget()
			   && gameHandler->gameState()->curB->battleCanShoot(next, elem->getPosition()))
			{
				target = elem;
				break;
			}
		}

		if(target == nullptr)
		{
			makeStackDoNothing(next);
		}
		else
		{
			attack.aimToUnit(target);
			makeAutomaticAction(next, attack);
		}
		return true;
	}

	if (next->unitType()->getId() == CreatureID::CATAPULT)
	{
		const auto & attackableBattleHexes = curB.getAttackableBattleHexes();

		if (attackableBattleHexes.empty())
		{
			makeStackDoNothing(next);
			return true;
		}

		if (!curOwner || gameHandler->getRandomGenerator().nextInt(99) >= curOwner->valOfBonuses(BonusType::MANUAL_CONTROL, CreatureID::CATAPULT))
		{
			BattleAction attack;
			attack.actionType = EActionType::CATAPULT;
			attack.side = next->unitSide();
			attack.stackNumber = next->unitId();

			makeAutomaticAction(next, attack);
			return true;
		}
	}

	if (next->unitType()->getId() == CreatureID::FIRST_AID_TENT)
	{
		TStacks possibleStacks = gameHandler->battleGetStacksIf([=](const CStack * s)
		{
			return s->unitOwner() == next->unitOwner() && s->canBeHealed();
		});

		if (possibleStacks.empty())
		{
			makeStackDoNothing(next);
			return true;
		}

		if (!curOwner || gameHandler->getRandomGenerator().nextInt(99) >= curOwner->valOfBonuses(BonusType::MANUAL_CONTROL, CreatureID::FIRST_AID_TENT))
		{
			RandomGeneratorUtil::randomShuffle(possibleStacks, gameHandler->getRandomGenerator());
			const CStack * toBeHealed = possibleStacks.front();

			BattleAction heal;
			heal.actionType = EActionType::STACK_HEAL;
			heal.aimToUnit(toBeHealed);
			heal.side = next->unitSide();
			heal.stackNumber = next->unitId();

			makeAutomaticAction(next, heal);
			return true;
		}
	}

	stackTurnTrigger(next); //various effects

	if(next->fear)
	{
		makeStackDoNothing(next); //end immediately if stack was affected by fear
		return true;
	}
	return false;
}

bool BattleFlowProcessor::rollGoodMorale(const CStack * next)
{
	//check for good morale
	auto nextStackMorale = next->moraleVal();
	if(    !next->hadMorale
		&& !next->defending
		&& !next->waited()
		&& !next->fear
		&& next->alive()
		&& nextStackMorale > 0)
	{
		auto diceSize = VLC->settings()->getVector(EGameSettings::COMBAT_GOOD_MORALE_DICE);
		size_t diceIndex = std::min<size_t>(diceSize.size()-1, nextStackMorale);

		if(diceSize.size() > 0 && gameHandler->getRandomGenerator().nextInt(1, diceSize[diceIndex]) == 1)
		{
			BattleTriggerEffect bte;
			bte.stackID = next->unitId();
			bte.effect = vstd::to_underlying(BonusType::MORALE);
			bte.val = 1;
			bte.additionalInfo = 0;
			gameHandler->sendAndApply(&bte); //play animation
			return true;
		}
	}
	return false;
}

void BattleFlowProcessor::onActionMade(const BattleAction &ba)
{
	const auto & battle = gameHandler->gameState()->curB;

	const CStack * actedStack = battle->battleGetStackByID(ba.stackNumber);
	const CStack * activeStack = battle->battleGetStackByID(battle->getActiveStackID());
	assert(activeStack != nullptr);

	//we're after action, all results applied
	owner->checkBattleStateChanges(); //check if this action ended the battle

	bool heroAction = ba.actionType == EActionType::HERO_SPELL || ba.actionType ==EActionType::SURRENDER || ba.actionType ==EActionType::RETREAT || ba.actionType ==EActionType::END_TACTIC_PHASE;

	if (heroAction)
	{
		if (activeStack->alive())
		{
			// this is action made by hero AND unit is alive (e.g. not killed by casted spell)
			// keep current active stack for next action
			BattleSetActiveStack sas;
			sas.stack = activeStack->unitId();
			gameHandler->sendAndApply(&sas);
			return;
		}
	}
	else
	{
		assert(actedStack != nullptr);

		if (rollGoodMorale(actedStack))
		{
			// Good morale - same stack makes 2nd turn
			BattleSetActiveStack sas;
			sas.stack = actedStack->unitId();
			gameHandler->sendAndApply(&sas);
			return;
		}
	}

	activateNextStack();
}

void BattleFlowProcessor::makeStackDoNothing(const CStack * next)
{
	BattleAction doNothing;
	doNothing.actionType = EActionType::NO_ACTION;
	doNothing.side = next->unitSide();
	doNothing.stackNumber = next->unitId();

	makeAutomaticAction(next, doNothing);
}

bool BattleFlowProcessor::makeAutomaticAction(const CStack *stack, BattleAction &ba)
{
	BattleSetActiveStack bsa;
	bsa.stack = stack->unitId();
	bsa.askPlayerInterface = false;
	gameHandler->sendAndApply(&bsa);

	bool ret = owner->makeBattleAction(ba);
	owner->checkBattleStateChanges();
	return ret;
}

void BattleFlowProcessor::stackEnchantedTrigger(const CStack * st)
{
	auto bl = *(st->getBonuses(Selector::type()(BonusType::ENCHANTED)));
	for(auto b : bl)
	{
		const CSpell * sp = SpellID(b->subtype).toSpell();
		if(!sp)
			continue;

		const int32_t val = bl.valOfBonuses(Selector::typeSubtype(b->type, b->subtype));
		const int32_t level = ((val > 3) ? (val - 3) : val);

		spells::BattleCast battleCast(gameHandler->gameState()->curB, st, spells::Mode::PASSIVE, sp);
		//this makes effect accumulate for at most 50 turns by default, but effect may be permanent and last till the end of battle
		battleCast.setEffectDuration(50);
		battleCast.setSpellLevel(level);
		spells::Target target;

		if(val > 3)
		{
			for(auto s : gameHandler->gameState()->curB->battleGetAllStacks())
				if(gameHandler->battleMatchOwner(st, s, true) && s->isValidTarget()) //all allied
					target.emplace_back(s);
		}
		else
		{
			target.emplace_back(st);
		}
		battleCast.applyEffects(gameHandler->spellEnv, target, false, true);
	}
}

void BattleFlowProcessor::removeObstacle(const CObstacleInstance & obstacle)
{
	BattleObstaclesChanged obsRem;
	obsRem.changes.emplace_back(obstacle.uniqueID, ObstacleChanges::EOperation::REMOVE);
	gameHandler->sendAndApply(&obsRem);
}

void BattleFlowProcessor::stackTurnTrigger(const CStack *st)
{
	BattleTriggerEffect bte;
	bte.stackID = st->unitId();
	bte.effect = -1;
	bte.val = 0;
	bte.additionalInfo = 0;
	if (st->alive())
	{
		//unbind
		if (st->hasBonus(Selector::type()(BonusType::BIND_EFFECT)))
		{
			bool unbind = true;
			BonusList bl = *(st->getBonuses(Selector::type()(BonusType::BIND_EFFECT)));
			auto adjacent = gameHandler->gameState()->curB->battleAdjacentUnits(st);

			for (auto b : bl)
			{
				if(b->additionalInfo != CAddInfo::NONE)
				{
					const CStack * stack = gameHandler->gameState()->curB->battleGetStackByID(b->additionalInfo[0]); //binding stack must be alive and adjacent
					if(stack)
					{
						if(vstd::contains(adjacent, stack)) //binding stack is still present
							unbind = false;
					}
				}
				else
				{
					unbind = false;
				}
			}
			if (unbind)
			{
				BattleSetStackProperty ssp;
				ssp.which = BattleSetStackProperty::UNBIND;
				ssp.stackID = st->unitId();
				gameHandler->sendAndApply(&ssp);
			}
		}

		if (st->hasBonusOfType(BonusType::POISON))
		{
			std::shared_ptr<const Bonus> b = st->getBonusLocalFirst(Selector::source(BonusSource::SPELL_EFFECT, SpellID::POISON).And(Selector::type()(BonusType::STACK_HEALTH)));
			if (b) //TODO: what if not?...
			{
				bte.val = std::max (b->val - 10, -(st->valOfBonuses(BonusType::POISON)));
				if (bte.val < b->val) //(negative) poison effect increases - update it
				{
					bte.effect = vstd::to_underlying(BonusType::POISON);
					gameHandler->sendAndApply(&bte);
				}
			}
		}
		if(st->hasBonusOfType(BonusType::MANA_DRAIN) && !st->drainedMana)
		{
			const PlayerColor opponent = gameHandler->gameState()->curB->otherPlayer(gameHandler->gameState()->curB->battleGetOwner(st));
			const CGHeroInstance * opponentHero = gameHandler->gameState()->curB->getHero(opponent);
			if(opponentHero)
			{
				ui32 manaDrained = st->valOfBonuses(BonusType::MANA_DRAIN);
				vstd::amin(manaDrained, opponentHero->mana);
				if(manaDrained)
				{
					bte.effect = vstd::to_underlying(BonusType::MANA_DRAIN);
					bte.val = manaDrained;
					bte.additionalInfo = opponentHero->id.getNum(); //for sanity
					gameHandler->sendAndApply(&bte);
				}
			}
		}
		if (st->isLiving() && !st->hasBonusOfType(BonusType::FEARLESS))
		{
			bool fearsomeCreature = false;
			for (CStack * stack : gameHandler->gameState()->curB->stacks)
			{
				if (gameHandler->battleMatchOwner(st, stack) && stack->alive() && stack->hasBonusOfType(BonusType::FEAR))
				{
					fearsomeCreature = true;
					break;
				}
			}
			if (fearsomeCreature)
			{
				if (gameHandler->getRandomGenerator().nextInt(99) < 10) //fixed 10%
				{
					bte.effect = vstd::to_underlying(BonusType::FEAR);
					gameHandler->sendAndApply(&bte);
				}
			}
		}
		BonusList bl = *(st->getBonuses(Selector::type()(BonusType::ENCHANTER)));
		int side = gameHandler->gameState()->curB->whatSide(st->unitOwner());
		if(st->canCast() && gameHandler->gameState()->curB->battleGetEnchanterCounter(side) == 0)
		{
			bool cast = false;
			while(!bl.empty() && !cast)
			{
				auto bonus = *RandomGeneratorUtil::nextItem(bl, gameHandler->getRandomGenerator());
				auto spellID = SpellID(bonus->subtype);
				const CSpell * spell = SpellID(spellID).toSpell();
				bl.remove_if([&bonus](const Bonus * b)
				{
					return b == bonus.get();
				});
				spells::BattleCast parameters(gameHandler->gameState()->curB, st, spells::Mode::ENCHANTER, spell);
				parameters.setSpellLevel(bonus->val);
				parameters.massive = true;
				parameters.smart = true;
				//todo: recheck effect level
				if(parameters.castIfPossible(gameHandler->spellEnv, spells::Target(1, spells::Destination())))
				{
					cast = true;

					int cooldown = bonus->additionalInfo[0];
					BattleSetStackProperty ssp;
					ssp.which = BattleSetStackProperty::ENCHANTER_COUNTER;
					ssp.absolute = false;
					ssp.val = cooldown;
					ssp.stackID = st->unitId();
					gameHandler->sendAndApply(&ssp);
				}
			}
		}
	}
}
