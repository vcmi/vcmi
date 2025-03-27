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

#include "../CGameHandler.h"
#include "../TurnTimerHandler.h"
#include "../processors/HeroPoolProcessor.h"
#include "../queries/QueriesProcessor.h"
#include "../queries/BattleQueries.h"

#include "../../lib/ArtifactUtils.h"
#include "../../lib/CStack.h"
#include "../../lib/CPlayerState.h"
#include "../../lib/IGameSettings.h"
#include "../../lib/battle/CBattleInfoCallback.h"
#include "../../lib/battle/IBattleState.h"
#include "../../lib/battle/SideInBattle.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"
#include "../../lib/networkPacks/PacksForClient.h"
#include "../../lib/spells/CSpellHandler.h"

#include <vstd/RNG.h>

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
					if(h && h->commander == c && (st->getCount() == 0 || !st->alive()))
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
	if (result.winner == BattleSide::ATTACKER)
	{
		winnerHero = info.getBattle()->getSideHero(BattleSide::ATTACKER);
		loserHero = info.getBattle()->getSideHero(BattleSide::DEFENDER);
		victor = info.getBattle()->getSidePlayer(BattleSide::ATTACKER);
		loser = info.getBattle()->getSidePlayer(BattleSide::DEFENDER);
	}
	else
	{
		winnerHero = info.getBattle()->getSideHero(BattleSide::DEFENDER);
		loserHero = info.getBattle()->getSideHero(BattleSide::ATTACKER);
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
			r.exp[r.winner] += LIBRARY->creh->objects.at(i->first)->valOfBonuses(BonusType::STACK_HEALTH) * i->second;
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

	const EBattleResult result = battleResult->result;

	//calculate casualties before deleting battle
	CasualtiesAfterBattle cab1(battle, BattleSide::ATTACKER);
	CasualtiesAfterBattle cab2(battle, BattleSide::DEFENDER);

	cab1.updateArmy(gameHandler);
	cab2.updateArmy(gameHandler); //take casualties after battle is deleted

	if(battleResult->winner == BattleSide::DEFENDER
	   && finishingBattle->winnerHero
	   && finishingBattle->winnerHero->visitedTown
	   && !finishingBattle->winnerHero->inTownGarrison
	   && finishingBattle->winnerHero->visitedTown->garrisonHero == finishingBattle->winnerHero)
	{
		gameHandler->swapGarrisonOnSiege(finishingBattle->winnerHero->visitedTown->id); //return defending visitor from garrison to its rightful place
	}
	//give exp
	if(!finishingBattle->isDraw() && battleResult->exp[finishingBattle->winnerSide] && finishingBattle->winnerHero)
		gameHandler->giveExperience(finishingBattle->winnerHero, battleResult->exp[finishingBattle->winnerSide]);

	// Eagle Eye handling
	if(!finishingBattle->isDraw() && finishingBattle->winnerHero)
	{
		ChangeSpells spells;

		if(auto eagleEyeLevel = finishingBattle->winnerHero->valOfBonuses(BonusType::LEARN_BATTLE_SPELL_LEVEL_LIMIT))
		{
			auto eagleEyeChance = finishingBattle->winnerHero->valOfBonuses(BonusType::LEARN_BATTLE_SPELL_CHANCE);
			for(auto & spellId : battle.getBattle()->getUsedSpells(battle.otherSide(battleResult->winner)))
			{
				auto spell = spellId.toEntity(LIBRARY->spells());
				if(spell
					&& spell->getLevel() <= eagleEyeLevel
					&& !finishingBattle->winnerHero->spellbookContainsSpell(spell->getId())
					&& gameHandler->getRandomGenerator().nextInt(99) < eagleEyeChance)
				{
					spells.spells.insert(spell->getId());
				}
			}
		}

		if(!spells.spells.empty())
		{
			spells.learn = 1;
			spells.hid = finishingBattle->winnerHero->id;

			InfoWindow iw;
			iw.player = finishingBattle->winnerHero->tempOwner;
			iw.text.appendLocalString(EMetaText::GENERAL_TXT, 221); //Through eagle-eyed observation, %s is able to learn %s
			iw.text.replaceRawString(finishingBattle->winnerHero->getNameTranslated());

			std::ostringstream names;
			for(int i = 0; i < spells.spells.size(); i++)
			{
				names << "%s";
				if(i < spells.spells.size() - 2)
					names << ", ";
				else if(i < spells.spells.size() - 1)
					names << "%s";
			}
			names << ".";

			iw.text.replaceRawString(names.str());

			auto it = spells.spells.begin();
			for(int i = 0; i < spells.spells.size(); i++, it++)
			{
				iw.text.replaceName(*it);
				if(i == spells.spells.size() - 2) //we just added pre-last name
					iw.text.replaceLocalString(EMetaText::GENERAL_TXT, 141); // " and "
				iw.components.emplace_back(ComponentType::SPELL, *it);
			}
			gameHandler->sendAndApply(iw);
			gameHandler->sendAndApply(spells);
		}
	}
	// Artifacts handling
	if(result == EBattleResult::NORMAL && !finishingBattle->isDraw() && finishingBattle->winnerHero)
	{
		std::vector<const CArtifactInstance*> arts; // display them in window
		CArtifactFittingSet artFittingSet(*finishingBattle->winnerHero);

		const auto addArtifactToTransfer = [&artFittingSet, &arts](BulkMoveArtifacts & pack, const ArtifactPosition & srcSlot, const CArtifactInstance * art)
		{
			assert(art);
			const auto dstSlot = ArtifactUtils::getArtAnyPosition(&artFittingSet, art->getTypeId());
			if(dstSlot != ArtifactPosition::PRE_FIRST)
			{
				pack.artsPack0.emplace_back(BulkMoveArtifacts::LinkedSlots(srcSlot, dstSlot));
				if(ArtifactUtils::isSlotEquipment(dstSlot))
					pack.artsPack0.back().askAssemble = true;
				arts.emplace_back(art);
				artFittingSet.putArtifact(dstSlot, const_cast<CArtifactInstance*>(art));
			}
		};
		const auto sendArtifacts = [this](BulkMoveArtifacts & bma)
		{
			if(!bma.artsPack0.empty())
				gameHandler->sendAndApply(bma);
		};

		BulkMoveArtifacts packHero(finishingBattle->winnerHero->getOwner(), ObjectInstanceID::NONE, finishingBattle->winnerHero->id, false);
		if(finishingBattle->loserHero)
		{
			packHero.srcArtHolder = finishingBattle->loserHero->id;
			for(const auto & slot : ArtifactUtils::commonWornSlots())
			{
				if(const auto artSlot = finishingBattle->loserHero->artifactsWorn.find(slot);
					artSlot != finishingBattle->loserHero->artifactsWorn.end() && ArtifactUtils::isArtRemovable(*artSlot))
				{
					addArtifactToTransfer(packHero, artSlot->first, artSlot->second.getArt());
				}
			}
			for(const auto & artSlot : finishingBattle->loserHero->artifactsInBackpack)
			{
				if(const auto art = artSlot.getArt(); art->getTypeId() != ArtifactID::GRAIL)
					addArtifactToTransfer(packHero, finishingBattle->loserHero->getArtPos(art), art);
			}

			if(finishingBattle->loserHero->commander)
			{
				BulkMoveArtifacts packCommander(finishingBattle->winnerHero->getOwner(), finishingBattle->loserHero->id, finishingBattle->winnerHero->id, false);
				packCommander.srcCreature = finishingBattle->loserHero->findStack(finishingBattle->loserHero->commander);
				for(const auto & artSlot : finishingBattle->loserHero->commander->artifactsWorn)
					addArtifactToTransfer(packCommander, artSlot.first, artSlot.second.getArt());
				sendArtifacts(packCommander);
			}
			auto armyObj = battle.battleGetArmyObject(battle.otherSide(battleResult->winner));
			for(const auto & armySlot : armyObj->stacks)
			{
				BulkMoveArtifacts packsArmy(finishingBattle->winnerHero->getOwner(), finishingBattle->loserHero->id, finishingBattle->winnerHero->id, false);
				packsArmy.srcArtHolder = armyObj->id;
				packsArmy.srcCreature = armySlot.first;
				for(const auto & artSlot : armySlot.second->artifactsWorn)
					addArtifactToTransfer(packsArmy, artSlot.first, armySlot.second->getArt(artSlot.first));
				sendArtifacts(packsArmy);
			}
		}
		// Display loot
		if(!arts.empty())
		{
			InfoWindow iw;
			iw.player = finishingBattle->winnerHero->tempOwner;
			iw.text.appendLocalString(EMetaText::GENERAL_TXT, 30); //You have captured enemy artifact

			for(const auto art : arts) //TODO; separate function to display loot for various objects?
			{
				if(art->isScroll())
					iw.components.emplace_back(ComponentType::SPELL_SCROLL, art->getScrollSpellID());
				else
					iw.components.emplace_back(ComponentType::ARTIFACT, art->getTypeId());

				if(iw.components.size() >= GameConstants::INFO_WINDOW_ARTIFACTS_MAX_ITEMS)
				{
					gameHandler->sendAndApply(iw);
					iw.components.clear();
				}
			}
			gameHandler->sendAndApply(iw);
		}
		if(!packHero.artsPack0.empty())
			sendArtifacts(packHero);
	}

	// Remove beaten hero
	if(finishingBattle->loserHero)
	{
		//add statistics
		if(!finishingBattle->isDraw())
		{
			ConstTransitivePtr<CGHeroInstance> strongestHero = nullptr;
			for(auto & hero : gameHandler->gameState()->getPlayerState(finishingBattle->loser)->getHeroes())
				if(!strongestHero || hero->exp > strongestHero->exp)
					strongestHero = hero;
			if(strongestHero->id == finishingBattle->loserHero->id && strongestHero->level > 5)
				gameHandler->gameState()->statistic.accumulatedValues[finishingBattle->victor].lastDefeatedStrongestHeroDay = gameHandler->gameState()->getDate(Date::DAY);
		}

		RemoveObject ro(finishingBattle->loserHero->id, finishingBattle->victor);
		gameHandler->sendAndApply(ro);
	}
	// For draw case both heroes should be removed
	if(finishingBattle->isDraw() && finishingBattle->winnerHero)
	{
		RemoveObject ro(finishingBattle->winnerHero->id, finishingBattle->loser);
		gameHandler->sendAndApply(ro);
	}

	// add statistic
	if(battle.sideToPlayer(BattleSide::ATTACKER) == PlayerColor::NEUTRAL || battle.sideToPlayer(BattleSide::DEFENDER) == PlayerColor::NEUTRAL)
	{
		gameHandler->gameState()->statistic.accumulatedValues[battle.sideToPlayer(BattleSide::ATTACKER)].numBattlesNeutral++;
		gameHandler->gameState()->statistic.accumulatedValues[battle.sideToPlayer(BattleSide::DEFENDER)].numBattlesNeutral++;
		if(!finishingBattle->isDraw())
			gameHandler->gameState()->statistic.accumulatedValues[battle.sideToPlayer(finishingBattle->winnerSide)].numWinBattlesNeutral++;
	}
	else
	{
		gameHandler->gameState()->statistic.accumulatedValues[battle.sideToPlayer(BattleSide::ATTACKER)].numBattlesPlayer++;
		gameHandler->gameState()->statistic.accumulatedValues[battle.sideToPlayer(BattleSide::DEFENDER)].numBattlesPlayer++;
		if(!finishingBattle->isDraw())
			gameHandler->gameState()->statistic.accumulatedValues[battle.sideToPlayer(finishingBattle->winnerSide)].numWinBattlesPlayer++;
	}

	BattleResultAccepted raccepted;
	raccepted.battleID = battle.getBattle()->getBattleID();
	raccepted.heroResult[BattleSide::ATTACKER].army = const_cast<CArmedInstance*>(battle.battleGetArmyObject(BattleSide::ATTACKER));
	raccepted.heroResult[BattleSide::DEFENDER].army = const_cast<CArmedInstance*>(battle.battleGetArmyObject(BattleSide::DEFENDER));
	raccepted.heroResult[BattleSide::ATTACKER].hero = const_cast<CGHeroInstance*>(battle.battleGetFightingHero(BattleSide::ATTACKER));
	raccepted.heroResult[BattleSide::DEFENDER].hero = const_cast<CGHeroInstance*>(battle.battleGetFightingHero(BattleSide::DEFENDER));
	raccepted.heroResult[BattleSide::ATTACKER].exp = battleResult->exp[BattleSide::ATTACKER];
	raccepted.heroResult[BattleSide::DEFENDER].exp = battleResult->exp[BattleSide::DEFENDER];
	raccepted.winnerSide = finishingBattle->winnerSide;
	gameHandler->sendAndApply(raccepted);

	gameHandler->queries->popIfTop(battleQuery);
	//--> continuation (battleAfterLevelUp) occurs after level-up gameHandler->queries are handled or on removing query
}

void BattleResultProcessor::battleAfterLevelUp(const BattleID & battleID, const BattleResult & result)
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

	// Necromancy if applicable.
	const CStackBasicDescriptor raisedStack = finishingBattle->winnerHero ? finishingBattle->winnerHero->calculateNecromancy(result) : CStackBasicDescriptor();
	// Give raised units to winner and show dialog, if any were raised,
	// units will be given after casualties are taken
	const SlotID necroSlot = raisedStack.getCreature() ? finishingBattle->winnerHero->getSlotFor(raisedStack.getCreature()) : SlotID();

	if (necroSlot != SlotID() && !finishingBattle->isDraw())
	{
		finishingBattle->winnerHero->showNecromancyDialog(raisedStack, gameHandler->getRandomGenerator());
		gameHandler->addToSlot(StackLocation(finishingBattle->winnerHero->id, necroSlot), raisedStack.getCreature(), raisedStack.count);
	}

	BattleResultsApplied resultsApplied;
	resultsApplied.battleID = battleID;
	resultsApplied.player1 = finishingBattle->victor;
	resultsApplied.player2 = finishingBattle->loser;
	gameHandler->sendAndApply(resultsApplied);

	//handle victory/loss of engaged players
	std::set<PlayerColor> playerColors = {finishingBattle->loser, finishingBattle->victor};
	gameHandler->checkVictoryLossConditions(playerColors);

	if (result.result == EBattleResult::SURRENDER)
	{
		gameHandler->gameState()->statistic.accumulatedValues[finishingBattle->loser].numHeroSurrendered++;
		gameHandler->heroPool->onHeroSurrendered(finishingBattle->loser, finishingBattle->loserHero);
	}

	if (result.result == EBattleResult::ESCAPE)
	{
		gameHandler->gameState()->statistic.accumulatedValues[finishingBattle->loser].numHeroEscaped++;
		gameHandler->heroPool->onHeroEscaped(finishingBattle->loser, finishingBattle->loserHero);
	}

	if (result.winner != BattleSide::NONE && finishingBattle->winnerHero && finishingBattle->winnerHero->stacks.empty()
		&& (!finishingBattle->winnerHero->commander || !finishingBattle->winnerHero->commander->alive))
	{
		RemoveObject ro(finishingBattle->winnerHero->id, finishingBattle->winnerHero->getOwner());
		gameHandler->sendAndApply(ro);

		if (gameHandler->getSettings().getBoolean(EGameSettings::HEROES_RETREAT_ON_WIN_WITHOUT_TROOPS))
			gameHandler->heroPool->onHeroEscaped(finishingBattle->victor, finishingBattle->winnerHero);
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
