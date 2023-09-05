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
#include "../processors/HeroPoolProcessor.h"
#include "../queries/QueriesProcessor.h"
#include "../queries/BattleQueries.h"

#include "../../lib/ArtifactUtils.h"
#include "../../lib/CStack.h"
#include "../../lib/GameSettings.h"
#include "../../lib/battle/CBattleInfoCallback.h"
#include "../../lib/battle/IBattleState.h"
#include "../../lib/battle/SideInBattle.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/serializer/Cast.h"
#include "../../lib/spells/CSpellHandler.h"

BattleResultProcessor::BattleResultProcessor(BattleProcessor * owner)
//	: owner(owner)
	: gameHandler(nullptr)
{
}

void BattleResultProcessor::setGameHandler(CGameHandler * newGameHandler)
{
	gameHandler = newGameHandler;
}

CasualtiesAfterBattle::CasualtiesAfterBattle(const CBattleInfoCallback & battle, uint8_t sideInBattle):
	army(battle.battleGetArmyObject(sideInBattle))
{
	heroWithDeadCommander = ObjectInstanceID();

	PlayerColor color = battle.sideToPlayer(sideInBattle);

	for(const CStack * stConst : battle.battleGetAllStacks(true))
	{
		// Use const cast - in order to call non-const "takeResurrected" for proper calculation of casualties
		// TODO: better solution
		CStack * st = const_cast<CStack*>(stConst);

		if(st->summoned) //don't take into account temporary summoned stacks
			continue;
		if(st->unitOwner() != color) //remove only our stacks
			continue;

		logGlobal->debug("Calculating casualties for %s", st->nodeName());

		st->health.takeResurrected();

		if(st->unitSlot() == SlotID::ARROW_TOWERS_SLOT)
		{
			logGlobal->debug("Ignored arrow towers stack.");
		}
		else if(st->unitSlot() == SlotID::WAR_MACHINES_SLOT)
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
				auto hero = dynamic_ptr_cast<CGHeroInstance> (army);
				if (hero)
					removedWarMachines.push_back (ArtifactLocation(hero, hero->getArtPos(warMachine, true)));
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
				StackLocation sl(army, st->unitSlot());
				newStackCounts.push_back(TStackAndItsNewCount(sl, 0));
			}
			else if(st->getCount() < army->getStackCount(st->unitSlot()))
			{
				logGlobal->debug("Stack lost %d units.", army->getStackCount(st->unitSlot()) - st->getCount());
				StackLocation sl(army, st->unitSlot());
				newStackCounts.push_back(TStackAndItsNewCount(sl, st->getCount()));
			}
			else if(st->getCount() > army->getStackCount(st->unitSlot()))
			{
				logGlobal->debug("Stack gained %d units.", st->getCount() - army->getStackCount(st->unitSlot()));
				StackLocation sl(army, st->unitSlot());
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
			StackLocation location(army, slot);
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
		gh->sendAndApply(&scp);
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

//FinishingBattleHelper::FinishingBattleHelper()
//{
//	winnerHero = loserHero = nullptr;
//	winnerSide = 0;
//	remainingBattleQueriesCount = 0;
//}

void BattleResultProcessor::endBattle(const CBattleInfoCallback & battle)
{
	auto const & giveExp = [](BattleResult &r)
	{
		if (r.winner > 1)
		{
			// draw
			return;
		}
		r.exp[0] = 0;
		r.exp[1] = 0;
		for (auto i = r.casualties[!r.winner].begin(); i!=r.casualties[!r.winner].end(); i++)
		{
			r.exp[r.winner] += VLC->creh->objects.at(i->first)->valOfBonuses(BonusType::STACK_HEALTH) * i->second;
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
			battleResult->exp[1] += 500;
		if(heroDefender)
			battleResult->exp[0] += 500;
	}

	if(heroAttacker)
		battleResult->exp[0] = heroAttacker->calculateXp(battleResult->exp[0]);//scholar skill
	if(heroDefender)
		battleResult->exp[1] = heroDefender->calculateXp(battleResult->exp[1]);

	auto battleQuery = std::dynamic_pointer_cast<CBattleQuery>(gameHandler->queries->topQuery(battle.sideToPlayer(0)));
	if (!battleQuery)
	{
		logGlobal->error("Cannot find battle query!");
		gameHandler->complain("Player " + boost::lexical_cast<std::string>(battle.sideToPlayer(0)) + " has no battle query at the top!");
		return;
	}

	battleQuery->result = std::make_optional(*battleResult);

	//Check how many battle gameHandler->queries were created (number of players blocked by battle)
	const int queriedPlayers = battleQuery ? (int)boost::count(gameHandler->queries->allQueries(), battleQuery) : 0;

	assert(finishingBattles.count(battle.getBattle()->getBattleID()) == 0);
	finishingBattles[battle.getBattle()->getBattleID()] = std::make_unique<FinishingBattleHelper>(battle, *battleResult, queriedPlayers);

	// in battles against neutrals, 1st player can ask to replay battle manually
	if (!battle.sideToPlayer(1).isValidPlayer())
	{
		auto battleDialogQuery = std::make_shared<CBattleDialogQuery>(gameHandler, battle.getBattle());
		battleResult->queryID = battleDialogQuery->queryID;
		gameHandler->queries->addQuery(battleDialogQuery);
	}
	else
		battleResult->queryID = -1;

	//set same battle result for all gameHandler->queries
	for(auto q : gameHandler->queries->allQueries())
	{
		auto otherBattleQuery = std::dynamic_pointer_cast<CBattleQuery>(q);
		if(otherBattleQuery && otherBattleQuery->battleID == battle.getBattle()->getBattleID())
			otherBattleQuery->result = battleQuery->result;
	}

	gameHandler->turnTimerHandler.onBattleEnd(battle.getBattle()->getBattleID());
	gameHandler->sendAndApply(battleResult);

	if (battleResult->queryID == QueryID::NONE)
		endBattleConfirm(battle);
}

void BattleResultProcessor::endBattleConfirm(const CBattleInfoCallback & battle)
{
	auto battleQuery = std::dynamic_pointer_cast<CBattleQuery>(gameHandler->queries->topQuery(battle.sideToPlayer(0)));
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

	ChangeSpells cs; //for Eagle Eye

	if(!finishingBattle->isDraw() && finishingBattle->winnerHero)
	{
		if (int eagleEyeLevel = finishingBattle->winnerHero->valOfBonuses(BonusType::LEARN_BATTLE_SPELL_LEVEL_LIMIT, -1))
		{
			double eagleEyeChance = finishingBattle->winnerHero->valOfBonuses(BonusType::LEARN_BATTLE_SPELL_CHANCE, 0);
			for(auto & spellId : battle.getBattle()->getUsedSpells(battle.otherSide(battleResult->winner)))
			{
				auto spell = spellId.toSpell(VLC->spells());
				if(spell && spell->getLevel() <= eagleEyeLevel && !finishingBattle->winnerHero->spellbookContainsSpell(spell->getId()) && gameHandler->getRandomGenerator().nextInt(99) < eagleEyeChance)
					cs.spells.insert(spell->getId());
			}
		}
	}
	std::vector<const CArtifactInstance *> arts; //display them in window

	if(result == EBattleResult::NORMAL && !finishingBattle->isDraw() && finishingBattle->winnerHero)
	{
		auto sendMoveArtifact = [&](const CArtifactInstance *art, MoveArtifact *ma)
		{
			const auto slot = ArtifactUtils::getArtAnyPosition(finishingBattle->winnerHero, art->getTypeId());
			if(slot != ArtifactPosition::PRE_FIRST)
			{
				arts.push_back(art);
				ma->dst = ArtifactLocation(finishingBattle->winnerHero, slot);
				if(ArtifactUtils::isSlotBackpack(slot))
					ma->askAssemble = false;
				gameHandler->sendAndApply(ma);
			}
		};

		if (finishingBattle->loserHero)
		{
			//TODO: wrap it into a function, somehow (std::variant -_-)
			auto artifactsWorn = finishingBattle->loserHero->artifactsWorn;
			for (auto artSlot : artifactsWorn)
			{
				MoveArtifact ma;
				ma.src = ArtifactLocation(finishingBattle->loserHero, artSlot.first);
				const CArtifactInstance * art =  ma.src.getArt();
				if (art && !art->artType->isBig() &&
					art->artType->getId() != ArtifactID::SPELLBOOK)
						// don't move war machines or locked arts (spellbook)
				{
					sendMoveArtifact(art, &ma);
				}
			}
			for(int slotNumber = finishingBattle->loserHero->artifactsInBackpack.size() - 1; slotNumber >= 0; slotNumber--)
			{
				//we assume that no big artifacts can be found
				MoveArtifact ma;
				ma.src = ArtifactLocation(finishingBattle->loserHero,
					ArtifactPosition(ArtifactPosition::BACKPACK_START + slotNumber)); //backpack automatically shifts arts to beginning
				const CArtifactInstance * art =  ma.src.getArt();
				if (art->artType->getId() != ArtifactID::GRAIL) //grail may not be won
				{
					sendMoveArtifact(art, &ma);
				}
			}
			if (finishingBattle->loserHero->commander) //TODO: what if commanders belong to no hero?
			{
				artifactsWorn = finishingBattle->loserHero->commander->artifactsWorn;
				for (auto artSlot : artifactsWorn)
				{
					MoveArtifact ma;
					ma.src = ArtifactLocation(finishingBattle->loserHero->commander.get(), artSlot.first);
					const CArtifactInstance * art =  ma.src.getArt();
					if (art && !art->artType->isBig())
					{
						sendMoveArtifact(art, &ma);
					}
				}
			}
		}

		auto loser = battle.otherSide(battleResult->winner);

		for (auto armySlot : battle.battleGetArmyObject(loser)->stacks)
		{
			auto artifactsWorn = armySlot.second->artifactsWorn;
			for (auto artSlot : artifactsWorn)
			{
				MoveArtifact ma;
				ma.src = ArtifactLocation(armySlot.second, artSlot.first);
				const CArtifactInstance * art =  ma.src.getArt();
				if (art && !art->artType->isBig())
				{
					sendMoveArtifact(art, &ma);
				}
			}
		}
	}

	if (arts.size()) //display loot
	{
		InfoWindow iw;
		iw.player = finishingBattle->winnerHero->tempOwner;

		iw.text.appendLocalString (EMetaText::GENERAL_TXT, 30); //You have captured enemy artifact

		for (auto art : arts) //TODO; separate function to display loot for various ojects?
		{
			iw.components.emplace_back(
				Component::EComponentType::ARTIFACT, art->artType->getId(),
				art->artType->getId() == ArtifactID::SPELL_SCROLL? art->getScrollSpellID() : SpellID(0), 0);
			if (iw.components.size() >= 14)
			{
				gameHandler->sendAndApply(&iw);
				iw.components.clear();
			}
		}
		if (iw.components.size())
		{
			gameHandler->sendAndApply(&iw);
		}
	}
	//Eagle Eye secondary skill handling
	if (!cs.spells.empty())
	{
		cs.learn = 1;
		cs.hid = finishingBattle->winnerHero->id;

		InfoWindow iw;
		iw.player = finishingBattle->winnerHero->tempOwner;
		iw.text.appendLocalString(EMetaText::GENERAL_TXT, 221); //Through eagle-eyed observation, %s is able to learn %s
		iw.text.replaceRawString(finishingBattle->winnerHero->getNameTranslated());

		std::ostringstream names;
		for (int i = 0; i < cs.spells.size(); i++)
		{
			names << "%s";
			if (i < cs.spells.size() - 2)
				names << ", ";
			else if (i < cs.spells.size() - 1)
				names << "%s";
		}
		names << ".";

		iw.text.replaceRawString(names.str());

		auto it = cs.spells.begin();
		for (int i = 0; i < cs.spells.size(); i++, it++)
		{
			iw.text.replaceLocalString(EMetaText::SPELL_NAME, it->toEnum());
			if (i == cs.spells.size() - 2) //we just added pre-last name
				iw.text.replaceLocalString(EMetaText::GENERAL_TXT, 141); // " and "
			iw.components.emplace_back(Component::EComponentType::SPELL, *it, 0, 0);
		}
		gameHandler->sendAndApply(&iw);
		gameHandler->sendAndApply(&cs);
	}
	cab1.updateArmy(gameHandler);
	cab2.updateArmy(gameHandler); //take casualties after battle is deleted

	if(finishingBattle->loserHero) //remove beaten hero
	{
		RemoveObject ro(finishingBattle->loserHero->id);
		gameHandler->sendAndApply(&ro);
	}
	if(finishingBattle->isDraw() && finishingBattle->winnerHero) //for draw case both heroes should be removed
	{
		RemoveObject ro(finishingBattle->winnerHero->id);
		gameHandler->sendAndApply(&ro);
	}

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
		gameHandler->changePrimSkill(finishingBattle->winnerHero, PrimarySkill::EXPERIENCE, battleResult->exp[finishingBattle->winnerSide]);

	BattleResultAccepted raccepted;
	raccepted.battleID = battle.getBattle()->getBattleID();
	raccepted.heroResult[0].army = const_cast<CArmedInstance*>(battle.battleGetArmyObject(0));
	raccepted.heroResult[1].army = const_cast<CArmedInstance*>(battle.battleGetArmyObject(1));
	raccepted.heroResult[0].hero = const_cast<CGHeroInstance*>(battle.battleGetFightingHero(0));
	raccepted.heroResult[1].hero = const_cast<CGHeroInstance*>(battle.battleGetFightingHero(1));
	raccepted.heroResult[0].exp = battleResult->exp[0];
	raccepted.heroResult[1].exp = battleResult->exp[1];
	raccepted.winnerSide = finishingBattle->winnerSide; 
	gameHandler->sendAndApply(&raccepted);

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
	const SlotID necroSlot = raisedStack.type ? finishingBattle->winnerHero->getSlotFor(raisedStack.type) : SlotID();

	if (necroSlot != SlotID())
	{
		finishingBattle->winnerHero->showNecromancyDialog(raisedStack, gameHandler->getRandomGenerator());
		gameHandler->addToSlot(StackLocation(finishingBattle->winnerHero, necroSlot), raisedStack.type, raisedStack.count);
	}

	BattleResultsApplied resultsApplied;
	resultsApplied.battleID = battleID;
	resultsApplied.player1 = finishingBattle->victor;
	resultsApplied.player2 = finishingBattle->loser;
	gameHandler->sendAndApply(&resultsApplied);

	//handle victory/loss of engaged players
	std::set<PlayerColor> playerColors = {finishingBattle->loser, finishingBattle->victor};
	gameHandler->checkVictoryLossConditions(playerColors);

	if (result.result == EBattleResult::SURRENDER)
		gameHandler->heroPool->onHeroSurrendered(finishingBattle->loser, finishingBattle->loserHero);

	if (result.result == EBattleResult::ESCAPE)
		gameHandler->heroPool->onHeroEscaped(finishingBattle->loser, finishingBattle->loserHero);

	if (result.winner != 2 && finishingBattle->winnerHero && finishingBattle->winnerHero->stacks.empty()
		&& (!finishingBattle->winnerHero->commander || !finishingBattle->winnerHero->commander->alive))
	{
		RemoveObject ro(finishingBattle->winnerHero->id);
		gameHandler->sendAndApply(&ro);

		if (VLC->settings()->getBoolean(EGameSettings::HEROES_RETREAT_ON_WIN_WITHOUT_TROOPS))
			gameHandler->heroPool->onHeroEscaped(finishingBattle->victor, finishingBattle->winnerHero);
	}

	finishingBattles.erase(battleID);
	battleResults.erase(battleID);
}

void BattleResultProcessor::setBattleResult(const CBattleInfoCallback & battle, EBattleResult resultType, int victoriusSide)
{
	assert(battleResults.count(battle.getBattle()->getBattleID()) == 0);

	battleResults[battle.getBattle()->getBattleID()] = std::make_unique<BattleResult>();

	auto & battleResult = battleResults[battle.getBattle()->getBattleID()];
	battleResult->battleID = battle.getBattle()->getBattleID();
	battleResult->result = resultType;
	battleResult->winner = victoriusSide; //surrendering side loses

	for(const auto & st : battle.battleGetAllStacks(true)) //setting casualties
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
