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
#include "../../lib/battle/BattleInfo.h"
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

CasualtiesAfterBattle::CasualtiesAfterBattle(const SideInBattle & battleSide, const BattleInfo * bat):
	army(battleSide.armyObject)
{
	heroWithDeadCommander = ObjectInstanceID();

	PlayerColor color = battleSide.color;

	for(CStack * st : bat->stacks)
	{
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

FinishingBattleHelper::FinishingBattleHelper(std::shared_ptr<const CBattleQuery> Query, int remainingBattleQueriesCount)
{
	assert(Query->result);
	assert(Query->bi);
	auto &result = *Query->result;
	auto &info = *Query->bi;

	winnerHero = result.winner != 0 ? info.sides[1].hero : info.sides[0].hero;
	loserHero = result.winner != 0 ? info.sides[0].hero : info.sides[1].hero;
	victor = info.sides[result.winner].color;
	loser = info.sides[!result.winner].color;
	winnerSide = result.winner;
	this->remainingBattleQueriesCount = remainingBattleQueriesCount;
}

FinishingBattleHelper::FinishingBattleHelper()
{
	winnerHero = loserHero = nullptr;
	winnerSide = 0;
	remainingBattleQueriesCount = 0;
}

void BattleResultProcessor::endBattle(int3 tile, const CGHeroInstance * heroAttacker, const CGHeroInstance * heroDefender)
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

	auto battleQuery = std::dynamic_pointer_cast<CBattleQuery>(gameHandler->queries->topQuery(gameHandler->gameState()->curB->sides[0].color));
	if (!battleQuery)
	{
		logGlobal->error("Cannot find battle query!");
		gameHandler->complain("Player " + boost::lexical_cast<std::string>(gameHandler->gameState()->curB->sides[0].color) + " has no battle query at the top!");
		return;
	}

	battleQuery->result = std::make_optional(*battleResult);

	//Check how many battle gameHandler->queries were created (number of players blocked by battle)
	const int queriedPlayers = battleQuery ? (int)boost::count(gameHandler->queries->allQueries(), battleQuery) : 0;
	finishingBattle = std::make_unique<FinishingBattleHelper>(battleQuery, queriedPlayers);

	// in battles against neutrals, 1st player can ask to replay battle manually
	if (!gameHandler->gameState()->curB->sides[1].color.isValidPlayer())
	{
		auto battleDialogQuery = std::make_shared<CBattleDialogQuery>(gameHandler, gameHandler->gameState()->curB);
		battleResult->queryID = battleDialogQuery->queryID;
		gameHandler->queries->addQuery(battleDialogQuery);
	}
	else
		battleResult->queryID = -1;

	//set same battle result for all gameHandler->queries
	for(auto q : gameHandler->queries->allQueries())
	{
		auto otherBattleQuery = std::dynamic_pointer_cast<CBattleQuery>(q);
		if(otherBattleQuery)
			otherBattleQuery->result = battleQuery->result;
	}

	gameHandler->sendAndApply(battleResult.get()); //after this point casualties objects are destroyed

	if (battleResult->queryID == -1)
		endBattleConfirm(gameHandler->gameState()->curB);
}

void BattleResultProcessor::endBattleConfirm(const BattleInfo * battleInfo)
{
	auto battleQuery = std::dynamic_pointer_cast<CBattleQuery>(gameHandler->queries->topQuery(battleInfo->sides.at(0).color));
	if(!battleQuery)
	{
		logGlobal->trace("No battle query, battle end was confirmed by another player");
		return;
	}

	const EBattleResult result = battleResult.get()->result;

	CasualtiesAfterBattle cab1(battleInfo->sides.at(0), battleInfo), cab2(battleInfo->sides.at(1), battleInfo); //calculate casualties before deleting battle
	ChangeSpells cs; //for Eagle Eye

	if(!finishingBattle->isDraw() && finishingBattle->winnerHero)
	{
		if (int eagleEyeLevel = finishingBattle->winnerHero->valOfBonuses(BonusType::LEARN_BATTLE_SPELL_LEVEL_LIMIT, -1))
		{
			double eagleEyeChance = finishingBattle->winnerHero->valOfBonuses(BonusType::LEARN_BATTLE_SPELL_CHANCE, 0);
			for(auto & spellId : battleInfo->sides.at(!battleResult->winner).usedSpellsHistory)
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
					ArtifactPosition(GameConstants::BACKPACK_START + slotNumber)); //backpack automatically shifts arts to beginning
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
		for (auto armySlot : battleInfo->sides.at(!battleResult->winner).armyObject->stacks)
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
				art->artType->getId() == ArtifactID::SPELL_SCROLL? art->getScrollSpellID() : 0, 0);
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
	raccepted.heroResult[0].army = const_cast<CArmedInstance*>(battleInfo->sides.at(0).armyObject);
	raccepted.heroResult[1].army = const_cast<CArmedInstance*>(battleInfo->sides.at(1).armyObject);
	raccepted.heroResult[0].hero = const_cast<CGHeroInstance*>(battleInfo->sides.at(0).hero);
	raccepted.heroResult[1].hero = const_cast<CGHeroInstance*>(battleInfo->sides.at(1).hero);
	raccepted.heroResult[0].exp = battleResult->exp[0];
	raccepted.heroResult[1].exp = battleResult->exp[1];
	raccepted.winnerSide = finishingBattle->winnerSide;
	gameHandler->sendAndApply(&raccepted);

	gameHandler->queries->popIfTop(battleQuery);
	//--> continuation (battleAfterLevelUp) occurs after level-up gameHandler->queries are handled or on removing query
}

void BattleResultProcessor::battleAfterLevelUp(const BattleResult &result)
{
	LOG_TRACE(logGlobal);

	if(!finishingBattle)
		return;

	finishingBattle->remainingBattleQueriesCount--;
	logGlobal->trace("Decremented gameHandler->queries count to %d", finishingBattle->remainingBattleQueriesCount);

	if (finishingBattle->remainingBattleQueriesCount > 0)
		//Battle results will be handled when all battle gameHandler->queries are closed
		return;

	//TODO consider if we really want it to work like above. ATM each player as unblocked as soon as possible
	// but the battle consequences are applied after final player is unblocked. Hard to abuse...
	// Still, it looks like a hole.

	// Necromancy if applicable.
	const CStackBasicDescriptor raisedStack = finishingBattle->winnerHero ? finishingBattle->winnerHero->calculateNecromancy(*battleResult) : CStackBasicDescriptor();
	// Give raised units to winner and show dialog, if any were raised,
	// units will be given after casualties are taken
	const SlotID necroSlot = raisedStack.type ? finishingBattle->winnerHero->getSlotFor(raisedStack.type) : SlotID();

	if (necroSlot != SlotID())
	{
		finishingBattle->winnerHero->showNecromancyDialog(raisedStack, gameHandler->getRandomGenerator());
		gameHandler->addToSlot(StackLocation(finishingBattle->winnerHero, necroSlot), raisedStack.type, raisedStack.count);
	}

	BattleResultsApplied resultsApplied;
	resultsApplied.player1 = finishingBattle->victor;
	resultsApplied.player2 = finishingBattle->loser;
	gameHandler->sendAndApply(&resultsApplied);

	gameHandler->setBattle(nullptr);

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

	finishingBattle.reset();
	battleResult.reset();
}

void BattleResultProcessor::setBattleResult(EBattleResult resultType, int victoriusSide)
{
	battleResult = std::make_unique<BattleResult>();
	battleResult->result = resultType;
	battleResult->winner = victoriusSide; //surrendering side loses
	gameHandler->gameState()->curB->calculateCasualties(battleResult->casualties);
}

void BattleResultProcessor::setupBattle()
{
	finishingBattle.reset();
	battleResult.reset();
}

bool BattleResultProcessor::battleIsEnding() const
{
	return battleResult != nullptr;
}
