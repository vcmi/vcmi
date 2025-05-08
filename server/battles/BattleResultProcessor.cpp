/*
 * BattleResultProcessor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleResultProcessor.h"
#include "battle/BattleInfo.h"

#include "../CGameHandler.h"
#include "../TurnTimerHandler.h"
#include "../processors/HeroPoolProcessor.h"
#include "../queries/QueriesProcessor.h"
#include "../queries/BattleQueries.h"

#include "../../lib/CStack.h"
#include "../../lib/CPlayerState.h"
#include "../../lib/IGameSettings.h"
#include "../../lib/battle/SideInBattle.h"
#include "../../lib/entities/artifact/ArtifactUtils.h"
#include "../../lib/entities/artifact/CArtifactFittingSet.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"
#include "../../lib/spells/CSpellHandler.h"

#include <boost/lexical_cast.hpp>

BattleResultProcessor::BattleResultProcessor(BattleProcessor * owner, CGameHandler * newGameHandler)
//	: owner(owner)
	: gameHandler(newGameHandler)
{
}

CasualtiesAfterBattle::CasualtiesAfterBattle(const CBattleInfoCallback & battle, BattleSide sideInBattle):
	army(battle.battleGetArmyObject(sideInBattle))
{
	heroWithDeadCommander = ObjectInstanceID();

	PlayerColor color = battle.sideToPlayer(sideInBattle);

	auto allStacks = battle.battleGetStacksIf([color](const CStack * stack){

		if (stack->summoned)//don't take into account temporary summoned stacks
			return false;

		if(stack->unitOwner() != color) //remove only our stacks
			return false;

		if (stack->isTurret())
			return false;

		return true;
	});

	for(const CStack * stConst : allStacks)
	{
		// Use const cast - in order to call non-const "takeResurrected" for proper calculation of casualties
		// TODO: better solution
		CStack * st = const_cast<CStack*>(stConst);

		logGlobal->debug("Calculating casualties for %s", st->nodeName());

		st->health.takeResurrected();

		if(st->unitSlot() == SlotID::WAR_MACHINES_SLOT)
		{
			auto warMachine = st->unitType()->warMachine;

			if(warMachine == ArtifactID::NONE)
			{
				logGlobal->error("Invalid creature in war machine virtual slot. Stack: %s", st->nodeName());
			}
			//catapult artifact remain even if "creature" killed in siege
			else if(warMachine != ArtifactID::CATAPULT && st->getCount() <= 0)
			{
				logGlobal->debug("War machine has been destroyed");
				auto hero = dynamic_cast<const CGHeroInstance*> (army);
				if (hero)
					removedWarMachines.push_back (ArtifactLocation(hero->id, hero->getArtPos(warMachine, true)));
				else
					logGlobal->error("War machine in army without hero");
			}
		}
		else if(st->unitSlot() == SlotID::SUMMONED_SLOT_PLACEHOLDER)
		{
			if(st->alive() && st->getCount() > 0)
			{
				logGlobal->debug("Permanently summoned %d units.", st->getCount());
				const CreatureID summonedType = st->creatureId();
				summoned[summonedType] += st->getCount();
			}
		}
		else if(st->unitSlot() == SlotID::COMMANDER_SLOT_PLACEHOLDER)
		{
			if (nullptr == st->base)
			{
				logGlobal->error("Stack with no base in commander slot. Stack: %s", st->nodeName());
			}
			else
			{
				auto c = dynamic_cast <const CCommanderInstance *>(st->base);
				if(c)
				{
					auto h = dynamic_cast <const CGHeroInstance *>(army);
					if(h && h->getCommander() == c && (st->getCount() == 0 || !st->alive()))
					{
						logGlobal->debug("Commander is dead.");
						heroWithDeadCommander = army->id; //TODO: unify commander handling
					}
				}
				else
					logGlobal->error("Stack with invalid instance in commander slot. Stack: %s", st->nodeName());
			}
		}
		else if(st->base && !army->slotEmpty(st->unitSlot()))
		{
			logGlobal->debug("Count: %d; base count: %d", st->getCount(), army->getStackCount(st->unitSlot()));
			if(st->getCount() == 0 || !st->alive())
			{
				logGlobal->debug("Stack has been destroyed.");
				StackLocation sl(army->id, st->unitSlot());
				newStackCounts.push_back(TStackAndItsNewCount(sl, 0));
			}
			else if(st->getCount() != army->getStackCount(st->unitSlot()))
			{
				logGlobal->debug("Stack size changed: %d -> %d units.", army->getStackCount(st->unitSlot()), st->getCount());
				StackLocation sl(army->id, st->unitSlot());
				newStackCounts.push_back(TStackAndItsNewCount(sl, st->getCount()));
			}
		}
		else
		{
			logGlobal->warn("Unable to process stack: %s", st->nodeName());
		}
	}
}

void CasualtiesAfterBattle::updateArmy(CGameHandler *gh)
{
	if (gh->getObjInstance(army->id) == nullptr)
		throw std::runtime_error("Object " + army->getObjectName() + " is not on the map!");

	for (TStackAndItsNewCount &ncount : newStackCounts)
	{
		if (ncount.second > 0)
			gh->changeStackCount(ncount.first, ncount.second, true);
		else
			gh->eraseStack(ncount.first, true);
	}
	for (auto summoned_iter : summoned)
	{
		SlotID slot = army->getSlotFor(summoned_iter.first);
		if (slot.validSlot())
		{
			StackLocation location(army->id, slot);
			gh->addToSlot(location, summoned_iter.first.toCreature(), summoned_iter.second);
		}
		else
		{
			//even if it will be possible to summon anything permanently it should be checked for free slot
			//necromancy is handled separately
			gh->complain("No free slot to put summoned creature");
		}
	}
	for (auto al : removedWarMachines)
	{
		gh->removeArtifact(al);
	}
	if (heroWithDeadCommander != ObjectInstanceID())
	{
		SetCommanderProperty scp;
		scp.heroid = heroWithDeadCommander;
		scp.which = SetCommanderProperty::ALIVE;
		scp.amount = 0;
		gh->sendAndApply(scp);
	}
}

FinishingBattleHelper::FinishingBattleHelper(const CBattleInfoCallback & info, const BattleResult & result, int remainingBattleQueriesCount)
{
	const auto attackerHero = info.getBattle()->getSideHero(BattleSide::ATTACKER);
	const auto defenderHero = info.getBattle()->getSideHero(BattleSide::DEFENDER);
	if (result.winner == BattleSide::ATTACKER)
	{
		winnerId = attackerHero ? attackerHero->id : ObjectInstanceID::NONE;
		loserId = defenderHero ? defenderHero->id : ObjectInstanceID::NONE;
		victor = info.getBattle()->getSidePlayer(BattleSide::ATTACKER);
		loser = info.getBattle()->getSidePlayer(BattleSide::DEFENDER);
	}
	else
	{
		winnerId = defenderHero ? defenderHero->id : ObjectInstanceID::NONE;
		loserId = attackerHero ? attackerHero->id : ObjectInstanceID::NONE;
		victor = info.getBattle()->getSidePlayer(BattleSide::DEFENDER);
		loser = info.getBattle()->getSidePlayer(BattleSide::ATTACKER);
	}

	winnerSide = result.winner;

	this->remainingBattleQueriesCount = remainingBattleQueriesCount;
}

void BattleResultProcessor::endBattle(const CBattleInfoCallback & battle)
{
	auto const & giveExp = [&battle](BattleResult &r)
	{
		if (r.winner == BattleSide::NONE)
		{
			// draw
			return;
		}
		r.exp[BattleSide::ATTACKER] = 0;
		r.exp[BattleSide::DEFENDER] = 0;
		for (auto i = r.casualties[battle.otherSide(r.winner)].begin(); i!=r.casualties[battle.otherSide(r.winner)].end(); i++)
		{
			r.exp[r.winner] += i->first.toCreature()->valOfBonuses(BonusType::STACK_HEALTH) * i->second;
		}
	};

	LOG_TRACE(logGlobal);

	auto * battleResult = battleResults.at(battle.getBattle()->getBattleID()).get();
	const auto * heroAttacker = battle.battleGetFightingHero(BattleSide::ATTACKER);
	const auto * heroDefender = battle.battleGetFightingHero(BattleSide::DEFENDER);

	//Fill BattleResult structure with exp info
	giveExp(*battleResult);

	if (battleResult->result == EBattleResult::NORMAL) // give 500 exp for defeating hero, unless he escaped
	{
		if(heroAttacker)
			battleResult->exp[BattleSide::DEFENDER] += 500;
		if(heroDefender)
			battleResult->exp[BattleSide::ATTACKER] += 500;
	}

	// Give 500 exp to winner if a town was conquered during the battle
	const auto * defendedTown = battle.battleGetDefendedTown();
	if (defendedTown && battleResult->winner == BattleSide::ATTACKER)
		battleResult->exp[BattleSide::ATTACKER] += 500;

	if(heroAttacker)
		battleResult->exp[BattleSide::ATTACKER] = heroAttacker->calculateXp(battleResult->exp[BattleSide::ATTACKER]);//scholar skill
	if(heroDefender)
		battleResult->exp[BattleSide::DEFENDER] = heroDefender->calculateXp(battleResult->exp[BattleSide::DEFENDER]);

	auto battleQuery = std::dynamic_pointer_cast<CBattleQuery>(gameHandler->queries->topQuery(battle.sideToPlayer(BattleSide::ATTACKER)));
	if(!battleQuery)
		battleQuery = std::dynamic_pointer_cast<CBattleQuery>(gameHandler->queries->topQuery(battle.sideToPlayer(BattleSide::DEFENDER)));
	if (!battleQuery)
	{
		logGlobal->error("Cannot find battle query!");
		gameHandler->complain("Player " + boost::lexical_cast<std::string>(battle.sideToPlayer(BattleSide::ATTACKER)) + " has no battle query at the top!");
		return;
	}

	battleQuery->result = std::make_optional(*battleResult);

	//Check how many battle gameHandler->queries were created (number of players blocked by battle)
	const int queriedPlayers = battleQuery ? (int)boost::count(gameHandler->queries->allQueries(), battleQuery) : 0;

	assert(finishingBattles.count(battle.getBattle()->getBattleID()) == 0);
	finishingBattles[battle.getBattle()->getBattleID()] = std::make_unique<FinishingBattleHelper>(battle, *battleResult, queriedPlayers);

	// in battles against neutrals, 1st player can ask to replay battle manually
	const auto * attackerPlayer = gameHandler->getPlayerState(battle.getBattle()->getSidePlayer(BattleSide::ATTACKER));
	const auto * defenderPlayer = gameHandler->getPlayerState(battle.getBattle()->getSidePlayer(BattleSide::DEFENDER));
	bool isAttackerHuman = attackerPlayer && attackerPlayer->isHuman();
	bool isDefenderHuman = defenderPlayer && defenderPlayer->isHuman();
	bool onlyOnePlayerHuman = isAttackerHuman != isDefenderHuman;
	// in battles against neutrals attacker can ask to replay battle manually, additionally in battles against AI player human side can also ask for replay
	if(onlyOnePlayerHuman)
	{
		auto battleDialogQuery = std::make_shared<CBattleDialogQuery>(gameHandler, battle.getBattle(), battleQuery->result);
		battleResult->queryID = battleDialogQuery->queryID;
		gameHandler->queries->addQuery(battleDialogQuery);
	}
	else
		battleResult->queryID = QueryID::NONE;

	//set same battle result for all gameHandler->queries
	for(auto q : gameHandler->queries->allQueries())
	{
		auto otherBattleQuery = std::dynamic_pointer_cast<CBattleQuery>(q);
		if(otherBattleQuery && otherBattleQuery->battleID == battle.getBattle()->getBattleID())
			otherBattleQuery->result = battleQuery->result;
	}

	gameHandler->turnTimerHandler->onBattleEnd(battle.getBattle()->getBattleID());
	gameHandler->sendAndApply(*battleResult);

	if (battleResult->queryID == QueryID::NONE)
		endBattleConfirm(battle);
}

void BattleResultProcessor::endBattleConfirm(const CBattleInfoCallback & battle)
{
	auto battleQuery = std::dynamic_pointer_cast<CBattleQuery>(gameHandler->queries->topQuery(battle.sideToPlayer(BattleSide::ATTACKER)));
	if(!battleQuery)
		battleQuery = std::dynamic_pointer_cast<CBattleQuery>(gameHandler->queries->topQuery(battle.sideToPlayer(BattleSide::DEFENDER)));
	if(!battleQuery)
	{
		logGlobal->trace("No battle query, battle end was confirmed by another player");
		return;
	}

	auto * battleResult = battleResults.at(battle.getBattle()->getBattleID()).get();
	auto * finishingBattle = finishingBattles.at(battle.getBattle()->getBattleID()).get();

	//calculate casualties before deleting battle
	CasualtiesAfterBattle cab1(battle, BattleSide::ATTACKER);
	CasualtiesAfterBattle cab2(battle, BattleSide::DEFENDER);

	cab1.updateArmy(gameHandler);
	cab2.updateArmy(gameHandler); //take casualties after battle is deleted

	const auto winnerHero = battle.battleGetFightingHero(finishingBattle->winnerSide);
	const auto loserHero = battle.battleGetFightingHero(CBattleInfoEssentials::otherSide(finishingBattle->winnerSide));

	if(battleResult->winner == BattleSide::DEFENDER
	   && winnerHero
	   && winnerHero->getVisitedTown()
	   && !winnerHero->isGarrisoned()
	   && winnerHero->getVisitedTown()->getGarrisonHero() == winnerHero)
	{
		gameHandler->swapGarrisonOnSiege(winnerHero->getVisitedTown()->id); //return defending visitor from garrison to its rightful place
	}
	//give exp
	if(!finishingBattle->isDraw() && battleResult->exp[finishingBattle->winnerSide] && winnerHero)
		gameHandler->giveExperience(winnerHero, battleResult->exp[finishingBattle->winnerSide]);

	// Add statistics
	if(loserHero && !finishingBattle->isDraw())
	{
		const CGHeroInstance * strongestHero = nullptr;
		for(auto & hero : gameHandler->gameState().getPlayerState(finishingBattle->loser)->getHeroes())
			if(!strongestHero || hero->exp > strongestHero->exp)
				strongestHero = hero;
		if(strongestHero->id == finishingBattle->loserId && strongestHero->level > 5)
			gameHandler->gameState().statistic.accumulatedValues[finishingBattle->victor].lastDefeatedStrongestHeroDay = gameHandler->gameState().getDate(Date::DAY);
	}
	if(battle.sideToPlayer(BattleSide::ATTACKER) == PlayerColor::NEUTRAL || battle.sideToPlayer(BattleSide::DEFENDER) == PlayerColor::NEUTRAL)
	{
		gameHandler->gameState().statistic.accumulatedValues[battle.sideToPlayer(BattleSide::ATTACKER)].numBattlesNeutral++;
		gameHandler->gameState().statistic.accumulatedValues[battle.sideToPlayer(BattleSide::DEFENDER)].numBattlesNeutral++;
		if(!finishingBattle->isDraw())
			gameHandler->gameState().statistic.accumulatedValues[battle.sideToPlayer(finishingBattle->winnerSide)].numWinBattlesNeutral++;
	}
	else
	{
		gameHandler->gameState().statistic.accumulatedValues[battle.sideToPlayer(BattleSide::ATTACKER)].numBattlesPlayer++;
		gameHandler->gameState().statistic.accumulatedValues[battle.sideToPlayer(BattleSide::DEFENDER)].numBattlesPlayer++;
		if(!finishingBattle->isDraw())
			gameHandler->gameState().statistic.accumulatedValues[battle.sideToPlayer(finishingBattle->winnerSide)].numWinBattlesPlayer++;
	}

	BattleResultAccepted raccepted;
	raccepted.battleID = battle.getBattle()->getBattleID();
	raccepted.heroResult[finishingBattle->winnerSide].heroID = winnerHero ? winnerHero->id : ObjectInstanceID::NONE;
	raccepted.heroResult[CBattleInfoEssentials::otherSide(finishingBattle->winnerSide)].heroID = loserHero ? loserHero->id : ObjectInstanceID::NONE;
	raccepted.heroResult[BattleSide::ATTACKER].armyID = battle.battleGetArmyObject(BattleSide::ATTACKER)->id;
	raccepted.heroResult[BattleSide::DEFENDER].armyID = battle.battleGetArmyObject(BattleSide::DEFENDER)->id;
	raccepted.heroResult[BattleSide::ATTACKER].exp = battleResult->exp[BattleSide::ATTACKER];
	raccepted.heroResult[BattleSide::DEFENDER].exp = battleResult->exp[BattleSide::DEFENDER];
	raccepted.winnerSide = finishingBattle->winnerSide;
	gameHandler->sendAndApply(raccepted);

	gameHandler->queries->popIfTop(battleQuery); // Workaround to remove battle query for AI case. TODO Think of a cleaner solution.
	//--> continuation (battleFinalize) occurs after level-up gameHandler->queries are handled or on removing query
}

void BattleResultProcessor::battleFinalize(const BattleID & battleID, const BattleResult & result)
{
	LOG_TRACE(logGlobal);

	assert(finishingBattles.count(battleID) != 0);
	if(finishingBattles.count(battleID) == 0)
		return;

	auto & finishingBattle = finishingBattles[battleID];

	finishingBattle->remainingBattleQueriesCount--;
	logGlobal->trace("Decremented gameHandler->queries count to %d", finishingBattle->remainingBattleQueriesCount);

	if (finishingBattle->remainingBattleQueriesCount > 0)
		//Battle results will be handled when all battle gameHandler->queries are closed
		return;

	//TODO consider if we really want it to work like above. ATM each player as unblocked as soon as possible
	// but the battle consequences are applied after final player is unblocked. Hard to abuse...
	// Still, it looks like a hole.

	const auto battle = std::find_if(gameHandler->gameState().currentBattles.begin(), gameHandler->gameState().currentBattles.end(),
		[battleID](const auto & desiredBattle)
		{
			return desiredBattle->battleID == battleID;
		});
	assert(battle != gameHandler->gameState().currentBattles.end());

	const auto winnerHero = (*battle)->battleGetFightingHero(finishingBattle->winnerSide);
	const auto loserHero = (*battle)->battleGetFightingHero(CBattleInfoEssentials::otherSide(finishingBattle->winnerSide));
	BattleResultsApplied resultsApplied;

	// Eagle Eye handling
	if(!finishingBattle->isDraw() && winnerHero)
	{
		if(auto eagleEyeLevel = winnerHero->valOfBonuses(BonusType::LEARN_BATTLE_SPELL_LEVEL_LIMIT))
		{
			resultsApplied.learnedSpells.learn = 1;
			resultsApplied.learnedSpells.hid = finishingBattle->winnerId;
			for(const auto & spellId : (*battle)->getUsedSpells(CBattleInfoEssentials::otherSide(result.winner)))
			{
				const auto spell = spellId.toEntity(LIBRARY->spells());
				if(spell
					&& spell->getLevel() <= eagleEyeLevel
					&& !winnerHero->spellbookContainsSpell(spell->getId())
					&& gameHandler->getRandomGenerator().nextInt(99) < winnerHero->valOfBonuses(BonusType::LEARN_BATTLE_SPELL_CHANCE))
				{
					resultsApplied.learnedSpells.spells.insert(spell->getId());
				}
			}
		}
	}

	// Artifacts handling
	if(result.result == EBattleResult::NORMAL && !finishingBattle->isDraw() && winnerHero)
	{
		CArtifactFittingSet artFittingSet(*winnerHero);
		const auto addArtifactToTransfer = [&artFittingSet](BulkMoveArtifacts & pack, const ArtifactPosition & srcSlot, const CArtifactInstance * art)
		{
			assert(art);
			const auto dstSlot = ArtifactUtils::getArtAnyPosition(&artFittingSet, art->getTypeId());
			if(dstSlot != ArtifactPosition::PRE_FIRST)
			{
				pack.artsPack0.emplace_back(MoveArtifactInfo(srcSlot, dstSlot));
				if(ArtifactUtils::isSlotEquipment(dstSlot))
					pack.artsPack0.back().askAssemble = true;
				artFittingSet.putArtifact(dstSlot, const_cast<CArtifactInstance*>(art));
			}
		};

		if(loserHero)
		{
			auto & packHero = resultsApplied.artifacts.emplace_back(finishingBattle->victor, finishingBattle->loserId, finishingBattle->winnerId, false);
			packHero.srcArtHolder = finishingBattle->loserId;
			for(const auto & slot : ArtifactUtils::commonWornSlots())
			{
				if(const auto artSlot = loserHero->artifactsWorn.find(slot); artSlot != loserHero->artifactsWorn.end() && ArtifactUtils::isArtRemovable(*artSlot))
				{
					addArtifactToTransfer(packHero, artSlot->first, artSlot->second.getArt());
				}
			}
			for(const auto & artSlot : loserHero->artifactsInBackpack)
			{
				if(const auto art = artSlot.getArt(); art->getTypeId() != ArtifactID::GRAIL)
					addArtifactToTransfer(packHero, loserHero->getArtPos(art), art);
			}

			if(loserHero->getCommander())
			{
				auto & packCommander = resultsApplied.artifacts.emplace_back(finishingBattle->victor, finishingBattle->loserId, finishingBattle->winnerId, false);
				packCommander.srcCreature = loserHero->findStack(loserHero->getCommander());
				for(const auto & artSlot : loserHero->getCommander()->artifactsWorn)
					addArtifactToTransfer(packCommander, artSlot.first, artSlot.second.getArt());
			}
			auto armyObj = dynamic_cast<const CArmedInstance*>(gameHandler->getObj(finishingBattle->loserId));
			for(const auto & armySlot : armyObj->stacks)
			{
				auto & packsArmy = resultsApplied.artifacts.emplace_back(finishingBattle->victor, finishingBattle->loserId, finishingBattle->winnerId, false);
				packsArmy.srcArtHolder = armyObj->id;
				packsArmy.srcCreature = armySlot.first;
				for(const auto & artSlot : armySlot.second->artifactsWorn)
					addArtifactToTransfer(packsArmy, artSlot.first, armySlot.second->getArt(artSlot.first));
			}
		}
	}

	// Necromancy handling
	// Give raised units to winner, if any were raised, units will be given after casualties are taken
	if(winnerHero)
	{
		resultsApplied.raisedStack = winnerHero->calculateNecromancy(result);
		const SlotID necroSlot = resultsApplied.raisedStack.getCreature() ? winnerHero->getSlotFor(resultsApplied.raisedStack.getCreature()) : SlotID();
		if(necroSlot != SlotID() && !finishingBattle->isDraw())
			gameHandler->addToSlot(StackLocation(finishingBattle->winnerId, necroSlot), resultsApplied.raisedStack.getCreature(), resultsApplied.raisedStack.getCount());
	}

	resultsApplied.battleID = battleID;
	resultsApplied.victor = finishingBattle->victor;
	resultsApplied.loser = finishingBattle->loser;
	gameHandler->sendAndApply(resultsApplied);

	//handle victory/loss of engaged players
	gameHandler->checkVictoryLossConditions({finishingBattle->loser, finishingBattle->victor});

	// Remove beaten hero
	if(loserHero)
	{
		RemoveObject ro(loserHero->id, finishingBattle->victor);
		gameHandler->sendAndApply(ro);
	}
	// For draw case both heroes should be removed
	if(finishingBattle->isDraw() && winnerHero)
	{
		RemoveObject ro(winnerHero->id, finishingBattle->loser);
		gameHandler->sendAndApply(ro);
		if(gameHandler->getSettings().getBoolean(EGameSettings::HEROES_RETREAT_ON_WIN_WITHOUT_TROOPS))
			gameHandler->heroPool->onHeroEscaped(finishingBattle->victor, winnerHero);
	}

	if (result.result == EBattleResult::SURRENDER)
	{
		gameHandler->gameState().statistic.accumulatedValues[finishingBattle->loser].numHeroSurrendered++;
		gameHandler->heroPool->onHeroSurrendered(finishingBattle->loser, loserHero);
	}

	if (result.result == EBattleResult::ESCAPE)
	{
		gameHandler->gameState().statistic.accumulatedValues[finishingBattle->loser].numHeroEscaped++;
		gameHandler->heroPool->onHeroEscaped(finishingBattle->loser, loserHero);
	}

	finishingBattles.erase(battleID);
	battleResults.erase(battleID);
}

void BattleResultProcessor::setBattleResult(const CBattleInfoCallback & battle, EBattleResult resultType, BattleSide victoriusSide)
{
	assert(battleResults.count(battle.getBattle()->getBattleID()) == 0);

	battleResults[battle.getBattle()->getBattleID()] = std::make_unique<BattleResult>();

	auto & battleResult = battleResults[battle.getBattle()->getBattleID()];
	battleResult->battleID = battle.getBattle()->getBattleID();
	battleResult->result = resultType;
	battleResult->winner = victoriusSide; //surrendering side loses

	auto allStacks = battle.battleGetStacksIf([](const CStack * stack){

		if (stack->summoned)//don't take into account temporary summoned stacks
			return false;

		if (stack->isTurret())
			return false;

		return true;
	});

	for(const auto & st : allStacks) //setting casualties
	{
		si32 killed = st->getKilled();
		if(killed > 0)
			battleResult->casualties[st->unitSide()][st->creatureId()] += killed;
	}
}

bool BattleResultProcessor::battleIsEnding(const CBattleInfoCallback & battle) const
{
	return battleResults.count(battle.getBattle()->getBattleID()) != 0;
}
