/*
 * BattleActionProcessor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleActionProcessor.h"

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

BattleActionProcessor::BattleActionProcessor(BattleProcessor * owner)
	: owner(owner)
	, gameHandler(nullptr)
{
}

void BattleActionProcessor::setGameHandler(CGameHandler * newGameHandler)
{
	gameHandler = newGameHandler;
}

bool BattleActionProcessor::makeBattleAction(BattleAction &ba)
{
	bool ok = true;

	battle::Target target = ba.getTarget(gameHandler->gameState()->curB);

	const CStack * stack = gameHandler->battleGetStackByID(ba.stackNumber); //may be nullptr if action is not about stack

	const bool isAboutActiveStack = stack && (ba.stackNumber == gameHandler->gameState()->curB->getActiveStackID());

	logGlobal->trace("Making action: %s", ba.toString());

	switch(ba.actionType)
	{
	case EActionType::WALK: //walk
	case EActionType::DEFEND: //defend
	case EActionType::WAIT: //wait
	case EActionType::WALK_AND_ATTACK: //walk or attack
	case EActionType::SHOOT: //shoot
	case EActionType::CATAPULT: //catapult
	case EActionType::STACK_HEAL: //healing with First Aid Tent
	case EActionType::MONSTER_SPELL:

		if (!stack)
		{
			gameHandler->complain("No such stack!");
			return false;
		}
		if (!stack->alive())
		{
			gameHandler->complain("This stack is dead: " + stack->nodeName());
			return false;
		}

		if (gameHandler->battleTacticDist())
		{
			if (stack && stack->unitSide() != gameHandler->battleGetTacticsSide())
			{
				gameHandler->complain("This is not a stack of side that has tactics!");
				return false;
			}
		}
		else if (!isAboutActiveStack)
		{
			gameHandler->complain("Action has to be about active stack!");
			return false;
		}
	}

	static EndAction end_action;
	auto wrapAction = [this](BattleAction &ba)
	{
		StartAction startAction(ba);
		gameHandler->sendAndApply(&startAction);

		return vstd::makeScopeGuard([&]()
		{
			gameHandler->sendAndApply(&end_action);
		});
	};

	switch(ba.actionType)
	{
	case EActionType::END_TACTIC_PHASE: //wait
	case EActionType::BAD_MORALE:
	case EActionType::NO_ACTION:
		{
			auto wrapper = wrapAction(ba);
			break;
		}
	case EActionType::WALK:
		{
			auto wrapper = wrapAction(ba);
			if(target.size() < 1)
			{
				gameHandler->complain("Destination required for move action.");
				ok = false;
				break;
			}
			int walkedTiles = moveStack(ba.stackNumber, target.at(0).hexValue); //move
			if (!walkedTiles)
				gameHandler->complain("Stack failed movement!");
			break;
		}
	case EActionType::DEFEND:
		{
			//defensive stance, TODO: filter out spell boosts from bonus (stone skin etc.)
			SetStackEffect sse;
			Bonus defenseBonusToAdd(BonusDuration::STACK_GETS_TURN, BonusType::PRIMARY_SKILL, BonusSource::OTHER, 20, -1, PrimarySkill::DEFENSE, BonusValueType::PERCENT_TO_ALL);
			Bonus bonus2(BonusDuration::STACK_GETS_TURN, BonusType::PRIMARY_SKILL, BonusSource::OTHER, stack->valOfBonuses(BonusType::DEFENSIVE_STANCE),
				 -1, PrimarySkill::DEFENSE, BonusValueType::ADDITIVE_VALUE);
			Bonus alternativeWeakCreatureBonus(BonusDuration::STACK_GETS_TURN, BonusType::PRIMARY_SKILL, BonusSource::OTHER, 1, -1, PrimarySkill::DEFENSE, BonusValueType::ADDITIVE_VALUE);

			BonusList defence = *stack->getBonuses(Selector::typeSubtype(BonusType::PRIMARY_SKILL, PrimarySkill::DEFENSE));
			int oldDefenceValue = defence.totalValue();

			defence.push_back(std::make_shared<Bonus>(defenseBonusToAdd));
			defence.push_back(std::make_shared<Bonus>(bonus2));

			int difference = defence.totalValue() - oldDefenceValue;
			std::vector<Bonus> buffer;
			if(difference == 0) //give replacement bonus for creatures not reaching 5 defense points (20% of def becomes 0)
			{
				difference = 1;
				buffer.push_back(alternativeWeakCreatureBonus);
			}
			else
			{
				buffer.push_back(defenseBonusToAdd);
			}

			buffer.push_back(bonus2);

			sse.toUpdate.push_back(std::make_pair(ba.stackNumber, buffer));
			gameHandler->sendAndApply(&sse);

			BattleLogMessage message;

			MetaString text;
			stack->addText(text, EMetaText::GENERAL_TXT, 120);
			stack->addNameReplacement(text);
			text.replaceNumber(difference);

			message.lines.push_back(text);

			gameHandler->sendAndApply(&message);
			//don't break - we share code with next case
		}
		[[fallthrough]];
	case EActionType::WAIT:
		{
			auto wrapper = wrapAction(ba);
			break;
		}
	case EActionType::RETREAT: //retreat/flee
		{
			if (!gameHandler->gameState()->curB->battleCanFlee(gameHandler->gameState()->curB->sides.at(ba.side).color))
				gameHandler->complain("Cannot retreat!");
			else
				owner->setBattleResult(EBattleResult::ESCAPE, !ba.side); //surrendering side loses
			break;
		}
	case EActionType::SURRENDER:
		{
			PlayerColor player = gameHandler->gameState()->curB->sides.at(ba.side).color;
			int cost = gameHandler->gameState()->curB->battleGetSurrenderCost(player);
			if (cost < 0)
				gameHandler->complain("Cannot surrender!");
			else if (gameHandler->getResource(player, EGameResID::GOLD) < cost)
				gameHandler->complain("Not enough gold to surrender!");
			else
			{
				gameHandler->giveResource(player, EGameResID::GOLD, -cost);
				owner->setBattleResult(EBattleResult::SURRENDER, !ba.side); //surrendering side loses
			}
			break;
		}
	case EActionType::WALK_AND_ATTACK: //walk or attack
		{
			auto wrapper = wrapAction(ba);

			if(!stack)
			{
				gameHandler->complain("No attacker");
				ok = false;
				break;
			}

			if(target.size() < 2)
			{
				gameHandler->complain("Two destinations required for attack action.");
				ok = false;
				break;
			}

			BattleHex attackPos = target.at(0).hexValue;
			BattleHex destinationTile = target.at(1).hexValue;
			const CStack * destinationStack = gameHandler->gameState()->curB->battleGetStackByPos(destinationTile, true);

			if(!destinationStack)
			{
				gameHandler->complain("Invalid target to attack");
				ok = false;
				break;
			}

			BattleHex startingPos = stack->getPosition();
			int distance = moveStack(ba.stackNumber, attackPos);

			logGlobal->trace("%s will attack %s", stack->nodeName(), destinationStack->nodeName());

			if(stack->getPosition() != attackPos
				&& !(stack->doubleWide() && (stack->getPosition() == attackPos.cloneInDirection(stack->destShiftDir(), false)))
				)
			{
				// we were not able to reach destination tile, nor occupy specified hex
				// abort attack attempt, but treat this case as legal - we may have stepped onto a quicksands/mine
				break;
			}

			if(destinationStack && stack->unitId() == destinationStack->unitId()) //we should just move, it will be handled by following check
			{
				destinationStack = nullptr;
			}

			if(!destinationStack)
			{
				gameHandler->complain("Unit can not attack itself");
				ok = false;
				break;
			}

			if(!CStack::isMeleeAttackPossible(stack, destinationStack))
			{
				gameHandler->complain("Attack cannot be performed!");
				ok = false;
				break;
			}

			//attack
			int totalAttacks = stack->totalAttacks.getMeleeValue();

			//TODO: move to CUnitState
			const auto * attackingHero = gameHandler->gameState()->curB->battleGetFightingHero(ba.side);
			if(attackingHero)
			{
				totalAttacks += attackingHero->valOfBonuses(BonusType::HERO_GRANTS_ATTACKS, stack->creatureIndex());
			}


			const bool firstStrike = destinationStack->hasBonusOfType(BonusType::FIRST_STRIKE);
			const bool retaliation = destinationStack->ableToRetaliate();
			for (int i = 0; i < totalAttacks; ++i)
			{
				//first strike
				if(i == 0 && firstStrike && retaliation)
				{
					makeAttack(destinationStack, stack, 0, stack->getPosition(), true, false, true);
				}

				//move can cause death, eg. by walking into the moat, first strike can cause death or paralysis/petrification
				if(stack->alive() && !stack->hasBonusOfType(BonusType::NOT_ACTIVE) && destinationStack->alive())
				{
					makeAttack(stack, destinationStack, (i ? 0 : distance), destinationTile, i==0, false, false);//no distance travelled on second attack
				}

				//counterattack
				//we check retaliation twice, so if it unblocked during attack it will work only on next attack
				if(stack->alive()
					&& !stack->hasBonusOfType(BonusType::BLOCKS_RETALIATION)
					&& (i == 0 && !firstStrike)
					&& retaliation && destinationStack->ableToRetaliate())
				{
					makeAttack(destinationStack, stack, 0, stack->getPosition(), true, false, true);
				}
			}

			//return
			if(stack->hasBonusOfType(BonusType::RETURN_AFTER_STRIKE)
				&& target.size() == 3
				&& startingPos != stack->getPosition()
				&& startingPos == target.at(2).hexValue
				&& stack->alive())
			{
				moveStack(ba.stackNumber, startingPos);
				//NOTE: curStack->unitId() == ba.stackNumber (rev 1431)
			}
			break;
		}
	case EActionType::SHOOT:
		{
			if(target.size() < 1)
			{
				gameHandler->complain("Destination required for shot action.");
				ok = false;
				break;
			}

			auto destination = target.at(0).hexValue;

			const CStack * destinationStack = gameHandler->gameState()->curB->battleGetStackByPos(destination);

			if (!gameHandler->gameState()->curB->battleCanShoot(stack, destination))
			{
				gameHandler->complain("Cannot shoot!");
				break;
			}
			if (!destinationStack)
			{
				gameHandler->complain("No target to shoot!");
				break;
			}

			auto wrapper = wrapAction(ba);

			makeAttack(stack, destinationStack, 0, destination, true, true, false);

			//ranged counterattack
			if (destinationStack->hasBonusOfType(BonusType::RANGED_RETALIATION)
				&& !stack->hasBonusOfType(BonusType::BLOCKS_RANGED_RETALIATION)
				&& destinationStack->ableToRetaliate()
				&& gameHandler->gameState()->curB->battleCanShoot(destinationStack, stack->getPosition())
				&& stack->alive()) //attacker may have died (fire shield)
			{
				makeAttack(destinationStack, stack, 0, stack->getPosition(), true, true, true);
			}
			//allow more than one additional attack

			int totalRangedAttacks = stack->totalAttacks.getRangedValue();

			//TODO: move to CUnitState
			const auto * attackingHero = gameHandler->gameState()->curB->battleGetFightingHero(ba.side);
			if(attackingHero)
			{
				totalRangedAttacks += attackingHero->valOfBonuses(BonusType::HERO_GRANTS_ATTACKS, stack->creatureIndex());
			}


			for(int i = 1; i < totalRangedAttacks; ++i)
			{
				if(
					stack->alive()
					&& destinationStack->alive()
					&& stack->shots.canUse()
					)
				{
					makeAttack(stack, destinationStack, 0, destination, false, true, false);
				}
			}
			break;
		}
	case EActionType::CATAPULT:
		{
			auto wrapper = wrapAction(ba);
			const CStack * shooter = gameHandler->gameState()->curB->battleGetStackByID(ba.stackNumber);
			std::shared_ptr<const Bonus> catapultAbility = stack->getBonusLocalFirst(Selector::type()(BonusType::CATAPULT));
			if(!catapultAbility || catapultAbility->subtype < 0)
			{
				gameHandler->complain("We do not know how to shoot :P");
			}
			else
			{
				const CSpell * spell = SpellID(catapultAbility->subtype).toSpell();
				spells::BattleCast parameters(gameHandler->gameState()->curB, shooter, spells::Mode::SPELL_LIKE_ATTACK, spell); //We can shot infinitely by catapult
				auto shotLevel = stack->valOfBonuses(Selector::typeSubtype(BonusType::CATAPULT_EXTRA_SHOTS, catapultAbility->subtype));
				parameters.setSpellLevel(shotLevel);
				parameters.cast(gameHandler->spellEnv, target);
			}
			//finish by scope guard
			break;
		}
		case EActionType::STACK_HEAL: //healing with First Aid Tent
		{
			auto wrapper = wrapAction(ba);
			const CStack * healer = gameHandler->gameState()->curB->battleGetStackByID(ba.stackNumber);

			if(target.size() < 1)
			{
				gameHandler->complain("Destination required for heal action.");
				ok = false;
				break;
			}

			const battle::Unit * destStack = nullptr;
			std::shared_ptr<const Bonus> healerAbility = stack->getBonusLocalFirst(Selector::type()(BonusType::HEALER));

			if(target.at(0).unitValue)
				destStack = target.at(0).unitValue;
			else
				destStack = gameHandler->gameState()->curB->battleGetUnitByPos(target.at(0).hexValue);

			if(healer == nullptr || destStack == nullptr || !healerAbility || healerAbility->subtype < 0)
			{
				gameHandler->complain("There is either no healer, no destination, or healer cannot heal :P");
			}
			else
			{
				const CSpell * spell = SpellID(healerAbility->subtype).toSpell();
				spells::BattleCast parameters(gameHandler->gameState()->curB, healer, spells::Mode::SPELL_LIKE_ATTACK, spell); //We can heal infinitely by first aid tent
				auto dest = battle::Destination(destStack, target.at(0).hexValue);
				parameters.setSpellLevel(0);
				parameters.cast(gameHandler->spellEnv, {dest});
			}
			break;
		}
		case EActionType::MONSTER_SPELL:
		{
			auto wrapper = wrapAction(ba);

			const CStack * stack = gameHandler->gameState()->curB->battleGetStackByID(ba.stackNumber);
			SpellID spellID = SpellID(ba.actionSubtype);

			std::shared_ptr<const Bonus> randSpellcaster = stack->getBonus(Selector::type()(BonusType::RANDOM_SPELLCASTER));
			std::shared_ptr<const Bonus> spellcaster = stack->getBonus(Selector::typeSubtype(BonusType::SPELLCASTER, spellID));

			//TODO special bonus for genies ability
			if (randSpellcaster && gameHandler->battleGetRandomStackSpell(gameHandler->getRandomGenerator(), stack, CBattleInfoCallback::RANDOM_AIMED) < 0)
				spellID = gameHandler->battleGetRandomStackSpell(gameHandler->getRandomGenerator(), stack, CBattleInfoCallback::RANDOM_GENIE);

			if (spellID < 0)
				gameHandler->complain("That stack can't cast spells!");
			else
			{
				const CSpell * spell = SpellID(spellID).toSpell();
				spells::BattleCast parameters(gameHandler->gameState()->curB, stack, spells::Mode::CREATURE_ACTIVE, spell);
				int32_t spellLvl = 0;
				if(spellcaster)
					vstd::amax(spellLvl, spellcaster->val);
				if(randSpellcaster)
					vstd::amax(spellLvl, randSpellcaster->val);
				parameters.setSpellLevel(spellLvl);
				parameters.cast(gameHandler->spellEnv, target);
			}
			break;
		}
	}

	if(ba.actionType == EActionType::WAIT || ba.actionType == EActionType::DEFEND || ba.actionType == EActionType::SHOOT || ba.actionType == EActionType::MONSTER_SPELL)
		gameHandler->handleObstacleTriggersForUnit(*gameHandler->spellEnv, *stack);

	return ok;
}

bool BattleActionProcessor::makeCustomAction(BattleAction & ba)
{
	switch(ba.actionType)
	{
	case EActionType::HERO_SPELL:
		{
			const CGHeroInstance *h = gameHandler->gameState()->curB->battleGetFightingHero(ba.side);
			if (!h)
			{
				logGlobal->error("Wrong caster!");
				return false;
			}

			const CSpell * s = SpellID(ba.actionSubtype).toSpell();
			if (!s)
			{
				logGlobal->error("Wrong spell id (%d)!", ba.actionSubtype);
				return false;
			}

			spells::BattleCast parameters(gameHandler->gameState()->curB, h, spells::Mode::HERO, s);

			spells::detail::ProblemImpl problem;

			auto m = s->battleMechanics(&parameters);

			if(!m->canBeCast(problem))//todo: should we check aimed cast?
			{
				logGlobal->warn("Spell cannot be cast!");
				std::vector<std::string> texts;
				problem.getAll(texts);
				for(auto s : texts)
					logGlobal->warn(s);
				return false;
			}

			StartAction start_action(ba);
			gameHandler->sendAndApply(&start_action); //start spell casting

			parameters.cast(gameHandler->spellEnv, ba.getTarget(gameHandler->gameState()->curB));

			EndAction end_action;
			gameHandler->sendAndApply(&end_action);
			return true;
		}
	}
	return false;
}

int BattleActionProcessor::moveStack(int stack, BattleHex dest)
{
	int ret = 0;

	const CStack *curStack = gameHandler->battleGetStackByID(stack),
		*stackAtEnd = gameHandler->gameState()->curB->battleGetStackByPos(dest);

	assert(curStack);
	assert(dest < GameConstants::BFIELD_SIZE);

	if (gameHandler->gameState()->curB->tacticDistance)
	{
		assert(gameHandler->gameState()->curB->isInTacticRange(dest));
	}

	auto start = curStack->getPosition();
	if (start == dest)
		return 0;

	//initing necessary tables
	auto accessibility = gameHandler->getAccesibility(curStack);
	std::set<BattleHex> passed;
	//Ignore obstacles on starting position
	passed.insert(curStack->getPosition());
	if(curStack->doubleWide())
		passed.insert(curStack->occupiedHex());

	//shifting destination (if we have double wide stack and we can occupy dest but not be exactly there)
	if(!stackAtEnd && curStack->doubleWide() && !accessibility.accessible(dest, curStack))
	{
		BattleHex shifted = dest.cloneInDirection(curStack->destShiftDir(), false);

		if(accessibility.accessible(shifted, curStack))
			dest = shifted;
	}

	if((stackAtEnd && stackAtEnd!=curStack && stackAtEnd->alive()) || !accessibility.accessible(dest, curStack))
	{
		gameHandler->complain("Given destination is not accessible!");
		return 0;
	}

	bool canUseGate = false;
	auto dbState = gameHandler->gameState()->curB->si.gateState;
	if(gameHandler->battleGetSiegeLevel() > 0 && curStack->unitSide() == BattleSide::DEFENDER &&
		dbState != EGateState::DESTROYED &&
		dbState != EGateState::BLOCKED)
	{
		canUseGate = true;
	}

	std::pair< std::vector<BattleHex>, int > path = gameHandler->gameState()->curB->getPath(start, dest, curStack);

	ret = path.second;

	int creSpeed = curStack->speed(0, true);

	if (gameHandler->gameState()->curB->tacticDistance > 0 && creSpeed > 0)
		creSpeed = GameConstants::BFIELD_SIZE;

	bool hasWideMoat = vstd::contains_if(gameHandler->battleGetAllObstaclesOnPos(BattleHex(ESiegeHex::GATE_BRIDGE), false), [](const std::shared_ptr<const CObstacleInstance> & obst)
	{
		return obst->obstacleType == CObstacleInstance::MOAT;
	});

	auto isGateDrawbridgeHex = [&](BattleHex hex) -> bool
	{
		if (hasWideMoat && hex == ESiegeHex::GATE_BRIDGE)
			return true;
		if (hex == ESiegeHex::GATE_OUTER)
			return true;
		if (hex == ESiegeHex::GATE_INNER)
			return true;

		return false;
	};

	auto occupyGateDrawbridgeHex = [&](BattleHex hex) -> bool
	{
		if (isGateDrawbridgeHex(hex))
			return true;

		if (curStack->doubleWide())
		{
			BattleHex otherHex = curStack->occupiedHex(hex);
			if (otherHex.isValid() && isGateDrawbridgeHex(otherHex))
				return true;
		}

		return false;
	};

	if (curStack->hasBonusOfType(BonusType::FLYING))
	{
		if (path.second <= creSpeed && path.first.size() > 0)
		{
			if (canUseGate && dbState != EGateState::OPENED &&
				occupyGateDrawbridgeHex(dest))
			{
				BattleUpdateGateState db;
				db.state = EGateState::OPENED;
				gameHandler->sendAndApply(&db);
			}

			//inform clients about move
			BattleStackMoved sm;
			sm.stack = curStack->unitId();
			std::vector<BattleHex> tiles;
			tiles.push_back(path.first[0]);
			sm.tilesToMove = tiles;
			sm.distance = path.second;
			sm.teleporting = false;
			gameHandler->sendAndApply(&sm);
		}
	}
	else //for non-flying creatures
	{
		std::vector<BattleHex> tiles;
		const int tilesToMove = std::max((int)(path.first.size() - creSpeed), 0);
		int v = (int)path.first.size()-1;
		path.first.push_back(start);

		// check if gate need to be open or closed at some point
		BattleHex openGateAtHex, gateMayCloseAtHex;
		if (canUseGate)
		{
			for (int i = (int)path.first.size()-1; i >= 0; i--)
			{
				auto needOpenGates = [&](BattleHex hex) -> bool
				{
					if (hasWideMoat && hex == ESiegeHex::GATE_BRIDGE)
						return true;
					if (hex == ESiegeHex::GATE_BRIDGE && i-1 >= 0 && path.first[i-1] == ESiegeHex::GATE_OUTER)
						return true;
					else if (hex == ESiegeHex::GATE_OUTER || hex == ESiegeHex::GATE_INNER)
						return true;

					return false;
				};

				auto hex = path.first[i];
				if (!openGateAtHex.isValid() && dbState != EGateState::OPENED)
				{
					if (needOpenGates(hex))
						openGateAtHex = path.first[i+1];

					//TODO we need find batter way to handle double-wide stacks
					//currently if only second occupied stack part is standing on gate / bridge hex then stack will start to wait for bridge to lower before it's needed. Though this is just a visual bug.
					if (curStack->doubleWide())
					{
						BattleHex otherHex = curStack->occupiedHex(hex);
						if (otherHex.isValid() && needOpenGates(otherHex))
							openGateAtHex = path.first[i+2];
					}

					//gate may be opened and then closed during stack movement, but not other way around
					if (openGateAtHex.isValid())
						dbState = EGateState::OPENED;
				}

				if (!gateMayCloseAtHex.isValid() && dbState != EGateState::CLOSED)
				{
					if (hex == ESiegeHex::GATE_INNER && i-1 >= 0 && path.first[i-1] != ESiegeHex::GATE_OUTER)
					{
						gateMayCloseAtHex = path.first[i-1];
					}
					if (hasWideMoat)
					{
						if (hex == ESiegeHex::GATE_BRIDGE && i-1 >= 0 && path.first[i-1] != ESiegeHex::GATE_OUTER)
						{
							gateMayCloseAtHex = path.first[i-1];
						}
						else if (hex == ESiegeHex::GATE_OUTER && i-1 >= 0 &&
							path.first[i-1] != ESiegeHex::GATE_INNER &&
							path.first[i-1] != ESiegeHex::GATE_BRIDGE)
						{
							gateMayCloseAtHex = path.first[i-1];
						}
					}
					else if (hex == ESiegeHex::GATE_OUTER && i-1 >= 0 && path.first[i-1] != ESiegeHex::GATE_INNER)
					{
						gateMayCloseAtHex = path.first[i-1];
					}
				}
			}
		}

		bool stackIsMoving = true;

		while(stackIsMoving)
		{
			if (v<tilesToMove)
			{
				logGlobal->error("Movement terminated abnormally");
				break;
			}

			bool gateStateChanging = false;
			//special handling for opening gate on from starting hex
			if (openGateAtHex.isValid() && openGateAtHex == start)
				gateStateChanging = true;
			else
			{
				for (bool obstacleHit = false; (!obstacleHit) && (!gateStateChanging) && (v >= tilesToMove); --v)
				{
					BattleHex hex = path.first[v];
					tiles.push_back(hex);

					if ((openGateAtHex.isValid() && openGateAtHex == hex) ||
						(gateMayCloseAtHex.isValid() && gateMayCloseAtHex == hex))
					{
						gateStateChanging = true;
					}

					//if we walked onto something, finalize this portion of stack movement check into obstacle
					if(!gameHandler->battleGetAllObstaclesOnPos(hex, false).empty())
						obstacleHit = true;

					if (curStack->doubleWide())
					{
						BattleHex otherHex = curStack->occupiedHex(hex);
						//two hex creature hit obstacle by backside
						auto obstacle2 = gameHandler->battleGetAllObstaclesOnPos(otherHex, false);
						if(otherHex.isValid() && !obstacle2.empty())
							obstacleHit = true;
					}
					if(!obstacleHit)
						passed.insert(hex);
				}
			}

			if (!tiles.empty())
			{
				//commit movement
				BattleStackMoved sm;
				sm.stack = curStack->unitId();
				sm.distance = path.second;
				sm.teleporting = false;
				sm.tilesToMove = tiles;
				gameHandler->sendAndApply(&sm);
				tiles.clear();
			}

			//we don't handle obstacle at the destination tile -> it's handled separately in the if at the end
			if (curStack->getPosition() != dest)
			{
				if(stackIsMoving && start != curStack->getPosition())
				{
					stackIsMoving = gameHandler->handleObstacleTriggersForUnit(*gameHandler->spellEnv, *curStack, passed);
					passed.insert(curStack->getPosition());
					if(curStack->doubleWide())
						passed.insert(curStack->occupiedHex());
				}
				if (gateStateChanging)
				{
					if (curStack->getPosition() == openGateAtHex)
					{
						openGateAtHex = BattleHex();
						//only open gate if stack is still alive
						if (curStack->alive())
						{
							BattleUpdateGateState db;
							db.state = EGateState::OPENED;
							gameHandler->sendAndApply(&db);
						}
					}
					else if (curStack->getPosition() == gateMayCloseAtHex)
					{
						gateMayCloseAtHex = BattleHex();
						owner->updateGateState();
					}
				}
			}
			else
				//movement finished normally: we reached destination
				stackIsMoving = false;
		}
	}
	//handle last hex separately for deviation
	if (VLC->settings()->getBoolean(EGameSettings::COMBAT_ONE_HEX_TRIGGERS_OBSTACLES))
	{
		if (dest == battle::Unit::occupiedHex(start, curStack->doubleWide(), curStack->unitSide())
			|| start == battle::Unit::occupiedHex(dest, curStack->doubleWide(), curStack->unitSide()))
			passed.clear(); //Just empty passed, obstacles will handled automatically
	}
	//handling obstacle on the final field (separate, because it affects both flying and walking stacks)
	gameHandler->handleObstacleTriggersForUnit(*gameHandler->spellEnv, *curStack, passed);

	return ret;
}

void BattleActionProcessor::makeAttack(const CStack * attacker, const CStack * defender, int distance, BattleHex targetHex, bool first, bool ranged, bool counter)
{
	if(first && !counter)
		handleAttackBeforeCasting(ranged, attacker, defender);

	FireShieldInfo fireShield;
	BattleAttack bat;
	BattleLogMessage blm;
	bat.stackAttacking = attacker->unitId();
	bat.tile = targetHex;

	std::shared_ptr<battle::CUnitState> attackerState = attacker->acquireState();

	if(ranged)
		bat.flags |= BattleAttack::SHOT;
	if(counter)
		bat.flags |= BattleAttack::COUNTER;

	const int attackerLuck = attacker->luckVal();

	if(attackerLuck > 0)
	{
		auto diceSize = VLC->settings()->getVector(EGameSettings::COMBAT_GOOD_LUCK_DICE);
		size_t diceIndex = std::min<size_t>(diceSize.size() - 1, attackerLuck);

		if(diceSize.size() > 0 && gameHandler->getRandomGenerator().nextInt(1, diceSize[diceIndex]) == 1)
			bat.flags |= BattleAttack::LUCKY;
	}

	if(attackerLuck < 0)
	{
		auto diceSize = VLC->settings()->getVector(EGameSettings::COMBAT_BAD_LUCK_DICE);
		size_t diceIndex = std::min<size_t>(diceSize.size() - 1, -attackerLuck);

		if(diceSize.size() > 0 && gameHandler->getRandomGenerator().nextInt(1, diceSize[diceIndex]) == 1)
			bat.flags |= BattleAttack::UNLUCKY;
	}

	if (gameHandler->getRandomGenerator().nextInt(99) < attacker->valOfBonuses(BonusType::DOUBLE_DAMAGE_CHANCE))
	{
		bat.flags |= BattleAttack::DEATH_BLOW;
	}

	const auto * owner = gameHandler->gameState()->curB->getHero(attacker->unitOwner());
	if(owner)
	{
		int chance = owner->valOfBonuses(BonusType::BONUS_DAMAGE_CHANCE, attacker->creatureIndex());
		if (chance > gameHandler->getRandomGenerator().nextInt(99))
			bat.flags |= BattleAttack::BALLISTA_DOUBLE_DMG;
	}

	int64_t drainedLife = 0;

	// only primary target
	if(defender->alive())
		drainedLife += applyBattleEffects(bat, attackerState, fireShield, defender, distance, false);

	//multiple-hex normal attack
	std::set<const CStack*> attackedCreatures = gameHandler->gameState()->curB->getAttackedCreatures(attacker, targetHex, bat.shot()); //creatures other than primary target
	for(const CStack * stack : attackedCreatures)
	{
		if(stack != defender && stack->alive()) //do not hit same stack twice
			drainedLife += applyBattleEffects(bat, attackerState, fireShield, stack, distance, true);
	}

	std::shared_ptr<const Bonus> bonus = attacker->getBonusLocalFirst(Selector::type()(BonusType::SPELL_LIKE_ATTACK));
	if(bonus && ranged) //TODO: make it work in melee?
	{
		//this is need for displaying hit animation
		bat.flags |= BattleAttack::SPELL_LIKE;
		bat.spellID = SpellID(bonus->subtype);

		//TODO: should spell override creature`s projectile?

		auto spell = bat.spellID.toSpell();

		battle::Target target;
		target.emplace_back(defender, targetHex);

		spells::BattleCast event(gameHandler->gameState()->curB, attacker, spells::Mode::SPELL_LIKE_ATTACK, spell);
		event.setSpellLevel(bonus->val);

		auto attackedCreatures = spell->battleMechanics(&event)->getAffectedStacks(target);

		//TODO: get exact attacked hex for defender

		for(const CStack * stack : attackedCreatures)
		{
			if(stack != defender && stack->alive()) //do not hit same stack twice
			{
				drainedLife += applyBattleEffects(bat, attackerState, fireShield, stack, distance, true);
			}
		}

		//now add effect info for all attacked stacks
		for (BattleStackAttacked & bsa : bat.bsa)
		{
			if (bsa.attackerID == attacker->unitId()) //this is our attack and not f.e. fire shield
			{
				//this is need for displaying affect animation
				bsa.flags |= BattleStackAttacked::SPELL_EFFECT;
				bsa.spellID = SpellID(bonus->subtype);
			}
		}
	}

	attackerState->afterAttack(ranged, counter);

	{
		UnitChanges info(attackerState->unitId(), UnitChanges::EOperation::RESET_STATE);
		attackerState->save(info.data);
		bat.attackerChanges.changedStacks.push_back(info);
	}

	if (drainedLife > 0)
		bat.flags |= BattleAttack::LIFE_DRAIN;

	gameHandler->sendAndApply(&bat);

	{
		const bool multipleTargets = bat.bsa.size() > 1;

		int64_t totalDamage = 0;
		int32_t totalKills = 0;

		for(const BattleStackAttacked & bsa : bat.bsa)
		{
			totalDamage += bsa.damageAmount;
			totalKills += bsa.killedAmount;
		}

		{
			MetaString text;
			attacker->addText(text, EMetaText::GENERAL_TXT, 376);
			attacker->addNameReplacement(text);
			text.replaceNumber(totalDamage);
			blm.lines.push_back(text);
		}

		addGenericKilledLog(blm, defender, totalKills, multipleTargets);
	}

	// drain life effect (as well as log entry) must be applied after the attack
	if(drainedLife > 0)
	{
		MetaString text;
		attackerState->addText(text, EMetaText::GENERAL_TXT, 361);
		attackerState->addNameReplacement(text, false);
		text.replaceNumber(drainedLife);
		defender->addNameReplacement(text, true);
		blm.lines.push_back(std::move(text));
	}

	if(!fireShield.empty())
	{
		//todo: this should be "virtual" spell instead, we only need fire spell school bonus here
		const CSpell * fireShieldSpell = SpellID(SpellID::FIRE_SHIELD).toSpell();
		int64_t totalDamage = 0;

		for(const auto & item : fireShield)
		{
			const CStack * actor = item.first;
			int64_t rawDamage = item.second;

			const CGHeroInstance * actorOwner = gameHandler->gameState()->curB->getHero(actor->unitOwner());

			if(actorOwner)
			{
				rawDamage = fireShieldSpell->adjustRawDamage(actorOwner, attacker, rawDamage);
			}
			else
			{
				rawDamage = fireShieldSpell->adjustRawDamage(actor, attacker, rawDamage);
			}

			totalDamage+=rawDamage;
			//FIXME: add custom effect on actor
		}

		if (totalDamage > 0)
		{
			BattleStackAttacked bsa;

			bsa.flags |= BattleStackAttacked::FIRE_SHIELD;
			bsa.stackAttacked = attacker->unitId(); //invert
			bsa.attackerID = defender->unitId();
			bsa.damageAmount = totalDamage;
			attacker->prepareAttacked(bsa, gameHandler->getRandomGenerator());

			StacksInjured pack;
			pack.stacks.push_back(bsa);
			gameHandler->sendAndApply(&pack);

			// TODO: this is already implemented in Damage::describeEffect()
			{
				MetaString text;
				text.appendLocalString(EMetaText::GENERAL_TXT, 376);
				text.replaceLocalString(EMetaText::SPELL_NAME, SpellID::FIRE_SHIELD);
				text.replaceNumber(totalDamage);
				blm.lines.push_back(std::move(text));
			}
			addGenericKilledLog(blm, attacker, bsa.killedAmount, false);
		}
	}

	gameHandler->sendAndApply(&blm);

	handleAfterAttackCasting(ranged, attacker, defender);
}

void BattleActionProcessor::attackCasting(bool ranged, BonusType attackMode, const battle::Unit * attacker, const battle::Unit * defender)
{
	if(attacker->hasBonusOfType(attackMode))
	{
		std::set<SpellID> spellsToCast;
		TConstBonusListPtr spells = attacker->getBonuses(Selector::type()(attackMode));
		for(const auto & sf : *spells)
		{
			spellsToCast.insert(SpellID(sf->subtype));
		}
		for(SpellID spellID : spellsToCast)
		{
			bool castMe = false;
			if(!defender->alive())
			{
				logGlobal->debug("attackCasting: all attacked creatures have been killed");
				return;
			}
			int32_t spellLevel = 0;
			TConstBonusListPtr spellsByType = attacker->getBonuses(Selector::typeSubtype(attackMode, spellID));
			for(const auto & sf : *spellsByType)
			{
				int meleeRanged;
				if(sf->additionalInfo.size() < 2)
				{
					// legacy format
					vstd::amax(spellLevel, sf->additionalInfo[0] % 1000);
					meleeRanged = sf->additionalInfo[0] / 1000;
				}
				else
				{
					vstd::amax(spellLevel, sf->additionalInfo[0]);
					meleeRanged = sf->additionalInfo[1];
				}
				if (meleeRanged == 0 || (meleeRanged == 1 && ranged) || (meleeRanged == 2 && !ranged))
					castMe = true;
			}
			int chance = attacker->valOfBonuses((Selector::typeSubtype(attackMode, spellID)));
			vstd::amin(chance, 100);

			const CSpell * spell = SpellID(spellID).toSpell();
			spells::AbilityCaster caster(attacker, spellLevel);

			spells::Target target;
			target.emplace_back(defender);

			spells::BattleCast parameters(gameHandler->gameState()->curB, &caster, spells::Mode::PASSIVE, spell);

			auto m = spell->battleMechanics(&parameters);

			spells::detail::ProblemImpl ignored;

			if(!m->canBeCastAt(target, ignored))
				continue;

			//check if spell should be cast (probability handling)
			if(gameHandler->getRandomGenerator().nextInt(99) >= chance)
				continue;

			//casting
			if(castMe)
			{
				parameters.cast(gameHandler->spellEnv, target);
			}
		}
	}
}

void BattleActionProcessor::handleAttackBeforeCasting(bool ranged, const CStack * attacker, const CStack * defender)
{
	attackCasting(ranged, BonusType::SPELL_BEFORE_ATTACK, attacker, defender); //no death stare / acid breath needed?
}

void BattleActionProcessor::handleAfterAttackCasting(bool ranged, const CStack * attacker, const CStack * defender)
{
	if(!attacker->alive() || !defender->alive()) // can be already dead
		return;

	attackCasting(ranged, BonusType::SPELL_AFTER_ATTACK, attacker, defender);

	if(!defender->alive())
	{
		//don't try death stare or acid breath on dead stack (crash!)
		return;
	}

	if(attacker->hasBonusOfType(BonusType::DEATH_STARE))
	{
		// mechanics of Death Stare as in H3:
		// each gorgon have 10% chance to kill (counted separately in H3) -> binomial distribution
		//original formula x = min(x, (gorgons_count + 9)/10);

		double chanceToKill = attacker->valOfBonuses(BonusType::DEATH_STARE, 0) / 100.0f;
		vstd::amin(chanceToKill, 1); //cap at 100%

		std::binomial_distribution<> distribution(attacker->getCount(), chanceToKill);

		int staredCreatures = distribution(gameHandler->getRandomGenerator().getStdGenerator());

		double cap = 1 / std::max(chanceToKill, (double)(0.01));//don't divide by 0
		int maxToKill = static_cast<int>((attacker->getCount() + cap - 1) / cap); //not much more than chance * count
		vstd::amin(staredCreatures, maxToKill);

		staredCreatures += (attacker->level() * attacker->valOfBonuses(BonusType::DEATH_STARE, 1)) / defender->level();
		if(staredCreatures)
		{
			//TODO: death stare was not originally available for multiple-hex attacks, but...
			const CSpell * spell = SpellID(SpellID::DEATH_STARE).toSpell();

			spells::AbilityCaster caster(attacker, 0);

			spells::BattleCast parameters(gameHandler->gameState()->curB, &caster, spells::Mode::PASSIVE, spell);
			spells::Target target;
			target.emplace_back(defender);
			parameters.setEffectValue(staredCreatures);
			parameters.cast(gameHandler->spellEnv, target);
		}
	}

	if(!defender->alive())
		return;

	int64_t acidDamage = 0;
	TConstBonusListPtr acidBreath = attacker->getBonuses(Selector::type()(BonusType::ACID_BREATH));
	for(const auto & b : *acidBreath)
	{
		if(b->additionalInfo[0] > gameHandler->getRandomGenerator().nextInt(99))
			acidDamage += b->val;
	}

	if(acidDamage > 0)
	{
		const CSpell * spell = SpellID(SpellID::ACID_BREATH_DAMAGE).toSpell();

		spells::AbilityCaster caster(attacker, 0);

		spells::BattleCast parameters(gameHandler->gameState()->curB, &caster, spells::Mode::PASSIVE, spell);
		spells::Target target;
		target.emplace_back(defender);

		parameters.setEffectValue(acidDamage * attacker->getCount());
		parameters.cast(gameHandler->spellEnv, target);
	}


	if(!defender->alive())
		return;

	if(attacker->hasBonusOfType(BonusType::TRANSMUTATION) && defender->isLiving()) //transmutation mechanics, similar to WoG werewolf ability
	{
		double chanceToTrigger = attacker->valOfBonuses(BonusType::TRANSMUTATION) / 100.0f;
		vstd::amin(chanceToTrigger, 1); //cap at 100%

		if(gameHandler->getRandomGenerator().getDoubleRange(0, 1)() > chanceToTrigger)
			return;

		int bonusAdditionalInfo = attacker->getBonus(Selector::type()(BonusType::TRANSMUTATION))->additionalInfo[0];

		if(defender->unitType()->getId() == bonusAdditionalInfo ||
			(bonusAdditionalInfo == CAddInfo::NONE && defender->unitType()->getId() == attacker->unitType()->getId()))
			return;

		battle::UnitInfo resurrectInfo;
		resurrectInfo.id = gameHandler->gameState()->curB->battleNextUnitId();
		resurrectInfo.summoned = false;
		resurrectInfo.position = defender->getPosition();
		resurrectInfo.side = defender->unitSide();

		if(bonusAdditionalInfo != CAddInfo::NONE)
			resurrectInfo.type = CreatureID(bonusAdditionalInfo);
		else
			resurrectInfo.type = attacker->creatureId();

		if(attacker->hasBonusOfType((BonusType::TRANSMUTATION), 0))
			resurrectInfo.count = std::max((defender->getCount() * defender->getMaxHealth()) / resurrectInfo.type.toCreature()->getMaxHealth(), 1u);
		else if (attacker->hasBonusOfType((BonusType::TRANSMUTATION), 1))
			resurrectInfo.count = defender->getCount();
		else
			return; //wrong subtype

		BattleUnitsChanged addUnits;
		addUnits.changedStacks.emplace_back(resurrectInfo.id, UnitChanges::EOperation::ADD);
		resurrectInfo.save(addUnits.changedStacks.back().data);

		BattleUnitsChanged removeUnits;
		removeUnits.changedStacks.emplace_back(defender->unitId(), UnitChanges::EOperation::REMOVE);
		gameHandler->sendAndApply(&removeUnits);
		gameHandler->sendAndApply(&addUnits);
	}

	if(attacker->hasBonusOfType(BonusType::DESTRUCTION, 0) || attacker->hasBonusOfType(BonusType::DESTRUCTION, 1))
	{
		double chanceToTrigger = 0;
		int amountToDie = 0;

		if(attacker->hasBonusOfType(BonusType::DESTRUCTION, 0)) //killing by percentage
		{
			chanceToTrigger = attacker->valOfBonuses(BonusType::DESTRUCTION, 0) / 100.0f;
			int percentageToDie = attacker->getBonus(Selector::type()(BonusType::DESTRUCTION).And(Selector::subtype()(0)))->additionalInfo[0];
			amountToDie = static_cast<int>(defender->getCount() * percentageToDie * 0.01f);
		}
		else if(attacker->hasBonusOfType(BonusType::DESTRUCTION, 1)) //killing by count
		{
			chanceToTrigger = attacker->valOfBonuses(BonusType::DESTRUCTION, 1) / 100.0f;
			amountToDie = attacker->getBonus(Selector::type()(BonusType::DESTRUCTION).And(Selector::subtype()(1)))->additionalInfo[0];
		}

		vstd::amin(chanceToTrigger, 1); //cap trigger chance at 100%

		if(gameHandler->getRandomGenerator().getDoubleRange(0, 1)() > chanceToTrigger)
			return;

		BattleStackAttacked bsa;
		bsa.attackerID = -1;
		bsa.stackAttacked = defender->unitId();
		bsa.damageAmount = amountToDie * defender->getMaxHealth();
		bsa.flags = BattleStackAttacked::SPELL_EFFECT;
		bsa.spellID = SpellID::SLAYER;
		defender->prepareAttacked(bsa, gameHandler->getRandomGenerator());

		StacksInjured si;
		si.stacks.push_back(bsa);

		gameHandler->sendAndApply(&si);
		sendGenericKilledLog(defender, bsa.killedAmount, false);
	}
}

int64_t BattleActionProcessor::applyBattleEffects(BattleAttack & bat, std::shared_ptr<battle::CUnitState> attackerState, FireShieldInfo & fireShield, const CStack * def, int distance, bool secondary)
{
	BattleStackAttacked bsa;
	if(secondary)
		bsa.flags |= BattleStackAttacked::SECONDARY; //all other targets do not suffer from spells & spell-like abilities

	bsa.attackerID = attackerState->unitId();
	bsa.stackAttacked = def->unitId();
	{
		BattleAttackInfo bai(attackerState.get(), def, distance, bat.shot());

		bai.deathBlow = bat.deathBlow();
		bai.doubleDamage = bat.ballistaDoubleDmg();
		bai.luckyStrike  = bat.lucky();
		bai.unluckyStrike  = bat.unlucky();

		auto range = gameHandler->gameState()->curB->calculateDmgRange(bai);
		bsa.damageAmount = gameHandler->gameState()->curB->getActualDamage(range.damage, attackerState->getCount(), gameHandler->getRandomGenerator());
		CStack::prepareAttacked(bsa, gameHandler->getRandomGenerator(), bai.defender->acquireState()); //calculate casualties
	}

	int64_t drainedLife = 0;

	//life drain handling
	if(attackerState->hasBonusOfType(BonusType::LIFE_DRAIN) && def->isLiving())
	{
		int64_t toHeal = bsa.damageAmount * attackerState->valOfBonuses(BonusType::LIFE_DRAIN) / 100;
		attackerState->heal(toHeal, EHealLevel::RESURRECT, EHealPower::PERMANENT);
		drainedLife += toHeal;
	}

	//soul steal handling
	if(attackerState->hasBonusOfType(BonusType::SOUL_STEAL) && def->isLiving())
	{
		//we can have two bonuses - one with subtype 0 and another with subtype 1
		//try to use permanent first, use only one of two
		for(si32 subtype = 1; subtype >= 0; subtype--)
		{
			if(attackerState->hasBonusOfType(BonusType::SOUL_STEAL, subtype))
			{
				int64_t toHeal = bsa.killedAmount * attackerState->valOfBonuses(BonusType::SOUL_STEAL, subtype) * attackerState->getMaxHealth();
				attackerState->heal(toHeal, EHealLevel::OVERHEAL, ((subtype == 0) ? EHealPower::ONE_BATTLE : EHealPower::PERMANENT));
				drainedLife += toHeal;
				break;
			}
		}
	}
	bat.bsa.push_back(bsa); //add this stack to the list of victims after drain life has been calculated

	//fire shield handling
	if(!bat.shot() &&
		!def->isClone() &&
		def->hasBonusOfType(BonusType::FIRE_SHIELD) &&
		!attackerState->hasBonusOfType(BonusType::FIRE_IMMUNITY) &&
		CStack::isMeleeAttackPossible(attackerState.get(), def) // attacked needs to be adjacent to defender for fire shield to trigger (e.g. Dragon Breath attack)
			)
	{
		//TODO: use damage with bonus but without penalties
		auto fireShieldDamage = (std::min<int64_t>(def->getAvailableHealth(), bsa.damageAmount) * def->valOfBonuses(BonusType::FIRE_SHIELD)) / 100;
		fireShield.push_back(std::make_pair(def, fireShieldDamage));
	}

	return drainedLife;
}

void BattleActionProcessor::sendGenericKilledLog(const CStack * defender, int32_t killed, bool multiple)
{
	if(killed > 0)
	{
		BattleLogMessage blm;
		addGenericKilledLog(blm, defender, killed, multiple);
		gameHandler->sendAndApply(&blm);
	}
}

void BattleActionProcessor::addGenericKilledLog(BattleLogMessage & blm, const CStack * defender, int32_t killed, bool multiple)
{
	if(killed > 0)
	{
		const int32_t txtIndex = (killed > 1) ? 379 : 378;
		std::string formatString = VLC->generaltexth->allTexts[txtIndex];

		// these default h3 texts have unnecessary new lines, so get rid of them before displaying (and trim just in case, trimming newlines does not works for some reason)
		formatString.erase(std::remove(formatString.begin(), formatString.end(), '\n'), formatString.end());
		formatString.erase(std::remove(formatString.begin(), formatString.end(), '\r'), formatString.end());
		boost::algorithm::trim(formatString);

		boost::format txt(formatString);
		if(killed > 1)
		{
			txt % killed % (multiple ? VLC->generaltexth->allTexts[43] : defender->unitType()->getNamePluralTranslated()); // creatures perish
		}
		else //killed == 1
		{
			txt % (multiple ? VLC->generaltexth->allTexts[42] : defender->unitType()->getNameSingularTranslated()); // creature perishes
		}
		MetaString line;
		line.appendRawString(txt.str());
		blm.lines.push_back(std::move(line));
	}
}
