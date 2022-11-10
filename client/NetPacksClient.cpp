/*
 * NetPacksClient.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "../lib/NetPacks.h"

#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/FileInfo.h"
#include "../CCallback.h"
#include "Client.h"
#include "CPlayerInterface.h"
#include "CGameInfo.h"
#include "../lib/serializer/Connection.h"
#include "../lib/serializer/BinarySerializer.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/mapping/CMap.h"
#include "../lib/VCMIDirs.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/CSoundBase.h"
#include "../lib/StartInfo.h"
#include "mapHandler.h"
#include "windows/GUIClasses.h"
#include "../lib/CConfigHandler.h"
#include "gui/SDL_Extensions.h"
#include "battle/CBattleInterface.h"
#include "../lib/mapping/CCampaignHandler.h"
#include "../lib/CGameState.h"
#include "../lib/CStack.h"
#include "../lib/battle/BattleInfo.h"
#include "../lib/GameConstants.h"
#include "../lib/CPlayerState.h"
#include "gui/CGuiHandler.h"
#include "widgets/MiscWidgets.h"
#include "widgets/AdventureMapClasses.h"
#include "CMT.h"
#include "CServerHandler.h"

// TODO: as Tow suggested these template should all be part of CClient
// This will require rework spectator interface properly though

template<typename T, typename ... Args, typename ... Args2>
bool callOnlyThatInterface(CClient * cl, PlayerColor player, void (T::*ptr)(Args...), Args2 && ...args)
{
	if(vstd::contains(cl->playerint, player))
	{
		((*cl->playerint[player]).*ptr)(std::forward<Args2>(args)...);
		return true;
	}
	return false;
}

template<typename T, typename ... Args, typename ... Args2>
bool callInterfaceIfPresent(CClient * cl, PlayerColor player, void (T::*ptr)(Args...), Args2 && ...args)
{
	bool called = callOnlyThatInterface(cl, player, ptr, std::forward<Args2>(args)...);
	return called;
}

template<typename T, typename ... Args, typename ... Args2>
void callOnlyThatBattleInterface(CClient * cl, PlayerColor player, void (T::*ptr)(Args...), Args2 && ...args)
{
	if(vstd::contains(cl->battleints,player))
		((*cl->battleints[player]).*ptr)(std::forward<Args2>(args)...);

	if(cl->additionalBattleInts.count(player))
	{
		for(auto bInt : cl->additionalBattleInts[player])
			((*bInt).*ptr)(std::forward<Args2>(args)...);
	}
}

template<typename T, typename ... Args, typename ... Args2>
void callBattleInterfaceIfPresent(CClient * cl, PlayerColor player, void (T::*ptr)(Args...), Args2 && ...args)
{
	callOnlyThatInterface(cl, player, ptr, std::forward<Args2>(args)...);
}

//calls all normal interfaces and privileged ones, playerints may be updated when iterating over it, so we need a copy
template<typename T, typename ... Args, typename ... Args2>
void callAllInterfaces(CClient * cl, void (T::*ptr)(Args...), Args2 && ...args)
{
	for(auto pInt : cl->playerint)
		((*pInt.second).*ptr)(std::forward<Args2>(args)...);
}

//calls all normal interfaces and privileged ones, playerints may be updated when iterating over it, so we need a copy
template<typename T, typename ... Args, typename ... Args2>
void callBattleInterfaceIfPresentForBothSides(CClient * cl, void (T::*ptr)(Args...), Args2 && ...args)
{
	callOnlyThatBattleInterface(cl, cl->gameState()->curB->sides[0].color, ptr, std::forward<Args2>(args)...);
	callOnlyThatBattleInterface(cl, cl->gameState()->curB->sides[1].color, ptr, std::forward<Args2>(args)...);
	if(settings["session"]["spectate"].Bool() && !settings["session"]["spectate-skip-battle"].Bool() && LOCPLINT->battleInt)
	{
		callOnlyThatBattleInterface(cl, PlayerColor::SPECTATOR, ptr, std::forward<Args2>(args)...);
	}
}

void SetResources::applyCl(CClient *cl)
{
	//todo: inform on actual resource set transfered
	callInterfaceIfPresent(cl, player, &IGameEventsReceiver::receivedResource);
}

void SetPrimSkill::applyCl(CClient *cl)
{
	const CGHeroInstance *h = cl->getHero(id);
	if(!h)
	{
		logNetwork->error("Cannot find hero with ID %d", id.getNum());
		return;
	}
	callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroPrimarySkillChanged, h, which, val);
}

void SetSecSkill::applyCl(CClient *cl)
{
	const CGHeroInstance *h = cl->getHero(id);
	if(!h)
	{
		logNetwork->error("Cannot find hero with ID %d", id.getNum());
		return;
	}
	callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroSecondarySkillChanged, h, which, val);
}

void HeroVisitCastle::applyCl(CClient *cl)
{
	const CGHeroInstance *h = cl->getHero(hid);

	if(start())
	{
		callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroVisitsTown, h, GS(cl)->getTown(tid));
	}
}

void ChangeSpells::applyCl(CClient *cl)
{
	//TODO: inform interface?
}

void SetMana::applyCl(CClient *cl)
{
	const CGHeroInstance *h = cl->getHero(hid);
	callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroManaPointsChanged, h);
}

void SetMovePoints::applyCl(CClient *cl)
{
	const CGHeroInstance *h = cl->getHero(hid);
	cl->invalidatePaths();
	callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroMovePointsChanged, h);
}

void FoWChange::applyCl(CClient *cl)
{
	for(auto &i : cl->playerint)
	{
		if(cl->getPlayerRelations(i.first, player) == PlayerRelations::SAME_PLAYER && waitForDialogs && LOCPLINT == i.second.get())
		{
			LOCPLINT->waitWhileDialog();
		}
		if(cl->getPlayerRelations(i.first, player) != PlayerRelations::ENEMIES)
		{
			if(mode)
				i.second->tileRevealed(tiles);
			else
				i.second->tileHidden(tiles);
		}
	}
	cl->invalidatePaths();
}

void SetAvailableHeroes::applyCl(CClient *cl)
{
	//TODO: inform interface?
}

static void dispatchGarrisonChange(CClient * cl, ObjectInstanceID army1, ObjectInstanceID army2)
{
	auto obj1 = cl->getObj(army1);
	if(!obj1)
	{
		logNetwork->error("Cannot find army with ID %d", army1.getNum());
		return;
	}

	callInterfaceIfPresent(cl, obj1->tempOwner, &IGameEventsReceiver::garrisonsChanged, army1, army2);

	if(army2 != ObjectInstanceID() && army2 != army1)
	{
		auto obj2 = cl->getObj(army2);
		if(!obj2)
		{
			logNetwork->error("Cannot find army with ID %d", army2.getNum());
			return;
		}

		if(obj1->tempOwner != obj2->tempOwner)
			callInterfaceIfPresent(cl, obj2->tempOwner, &IGameEventsReceiver::garrisonsChanged, army1, army2);
	}
}

void ChangeStackCount::applyCl(CClient * cl)
{
	dispatchGarrisonChange(cl, army, ObjectInstanceID());
}

void SetStackType::applyCl(CClient * cl)
{
	dispatchGarrisonChange(cl, army, ObjectInstanceID());
}

void EraseStack::applyCl(CClient * cl)
{
	dispatchGarrisonChange(cl, army, ObjectInstanceID());
}

void SwapStacks::applyCl(CClient * cl)
{
	dispatchGarrisonChange(cl, srcArmy, dstArmy);
}

void InsertNewStack::applyCl(CClient * cl)
{
	dispatchGarrisonChange(cl, army, ObjectInstanceID());
}

void RebalanceStacks::applyCl(CClient * cl)
{
	dispatchGarrisonChange(cl, srcArmy, dstArmy);
}

void BulkRebalanceStacks::applyCl(CClient * cl)
{
	if(!moves.empty())
	{
		auto destArmy = moves[0].srcArmy == moves[0].dstArmy
			? ObjectInstanceID()
			: moves[0].dstArmy;
		dispatchGarrisonChange(cl, moves[0].srcArmy, destArmy);
	}
}

void BulkSmartRebalanceStacks::applyCl(CClient * cl)
{
	if(!moves.empty())
	{
		assert(moves[0].srcArmy == moves[0].dstArmy);
		dispatchGarrisonChange(cl, moves[0].srcArmy, ObjectInstanceID());
	}
	else if(!changes.empty())
	{
		dispatchGarrisonChange(cl, changes[0].army, ObjectInstanceID());
	}
}

void PutArtifact::applyCl(CClient *cl)
{
	callInterfaceIfPresent(cl, al.owningPlayer(), &IGameEventsReceiver::artifactPut, al);
}

void EraseArtifact::applyCl(CClient *cl)
{
	callInterfaceIfPresent(cl, al.owningPlayer(), &IGameEventsReceiver::artifactRemoved, al);
}

void MoveArtifact::applyCl(CClient *cl)
{
	callInterfaceIfPresent(cl, src.owningPlayer(), &IGameEventsReceiver::artifactMoved, src, dst);
	callInterfaceIfPresent(cl, src.owningPlayer(), &IGameEventsReceiver::artifactPossibleAssembling, dst);
	if(src.owningPlayer() != dst.owningPlayer())
	{
		callInterfaceIfPresent(cl, dst.owningPlayer(), &IGameEventsReceiver::artifactMoved, src, dst);
		callInterfaceIfPresent(cl, dst.owningPlayer(), &IGameEventsReceiver::artifactPossibleAssembling, dst);
	}
}

void BulkMoveArtifacts::applyCl(CClient * cl)
{
	auto & movingArts = artsPack0;
	for(auto & slotToMove : movingArts)
	{
		auto srcLoc = ArtifactLocation(srcArtHolder, slotToMove.srcPos);
		auto dstLoc = ArtifactLocation(dstArtHolder, slotToMove.dstPos);
		callInterfaceIfPresent(cl, srcLoc.owningPlayer(), &IGameEventsReceiver::artifactMoved, srcLoc, dstLoc);
		if (srcLoc.owningPlayer() != dstLoc.owningPlayer())
			callInterfaceIfPresent(cl, dstLoc.owningPlayer(), &IGameEventsReceiver::artifactMoved, srcLoc, dstLoc);
	}

	if(swap)
	{
		movingArts = artsPack1;
		for(auto & slotToMove : movingArts)
		{
			auto srcLoc = ArtifactLocation(srcArtHolder, slotToMove.srcPos);
			auto dstLoc = ArtifactLocation(dstArtHolder, slotToMove.dstPos);
			callInterfaceIfPresent(cl, srcLoc.owningPlayer(), &IGameEventsReceiver::artifactMoved, srcLoc, dstLoc);
			if(srcLoc.owningPlayer() != dstLoc.owningPlayer())
				callInterfaceIfPresent(cl, dstLoc.owningPlayer(), &IGameEventsReceiver::artifactMoved, srcLoc, dstLoc);
		}
	}
}

void AssembledArtifact::applyCl(CClient *cl)
{
	callInterfaceIfPresent(cl, al.owningPlayer(), &IGameEventsReceiver::artifactAssembled, al);
}

void DisassembledArtifact::applyCl(CClient *cl)
{
	callInterfaceIfPresent(cl, al.owningPlayer(), &IGameEventsReceiver::artifactDisassembled, al);
}

void HeroVisit::applyCl(CClient * cl)
{
	auto hero = cl->getHero(heroId);
	auto obj = cl->getObj(objId, false);
	callInterfaceIfPresent(cl, player, &IGameEventsReceiver::heroVisit, hero, obj, starting);
}

void NewTurn::applyCl(CClient *cl)
{
	cl->invalidatePaths();
}

void GiveBonus::applyCl(CClient *cl)
{
	cl->invalidatePaths();
	switch(who)
	{
	case HERO:
		{
			const CGHeroInstance *h = GS(cl)->getHero(ObjectInstanceID(id));
			callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroBonusChanged, h, *h->getBonusList().back(), true);
		}
		break;
	case PLAYER:
		{
			const PlayerState *p = GS(cl)->getPlayerState(PlayerColor(id));
			callInterfaceIfPresent(cl, PlayerColor(id), &IGameEventsReceiver::playerBonusChanged, *p->getBonusList().back(), true);
		}
		break;
	}
}

void ChangeObjPos::applyFirstCl(CClient *cl)
{
	CGObjectInstance *obj = GS(cl)->getObjInstance(objid);
	if(flags & 1 && CGI->mh)
		CGI->mh->hideObject(obj);
}
void ChangeObjPos::applyCl(CClient *cl)
{
	CGObjectInstance *obj = GS(cl)->getObjInstance(objid);
	if(flags & 1 && CGI->mh)
		CGI->mh->printObject(obj);

	cl->invalidatePaths();
}

void PlayerEndsGame::applyCl(CClient *cl)
{
	callAllInterfaces(cl, &IGameEventsReceiver::gameOver, player, victoryLossCheckResult);

	// In auto testing mode we always close client if red player won or lose
	if(!settings["session"]["testmap"].isNull() && player == PlayerColor(0))
		handleQuit(settings["session"]["spectate"].Bool()); // if spectator is active ask to close client or not
}

void PlayerReinitInterface::applyCl(CClient * cl)
{
	auto initInterfaces = [cl]()
	{
		cl->initPlayerInterfaces();
		auto currentPlayer = cl->gameState()->currentPlayer;
		callAllInterfaces(cl, &IGameEventsReceiver::playerStartsTurn, currentPlayer);
		callOnlyThatInterface(cl, currentPlayer, &CGameInterface::yourTurn);
	};
	
	for(auto player : players)
	{
		auto & plSettings = CSH->si->getIthPlayersSettings(player);
		if(playerConnectionId == PlayerSettings::PLAYER_AI)
		{
			plSettings.connectedPlayerIDs.clear();
			cl->initPlayerEnvironments();
			initInterfaces();
		}
		else if(playerConnectionId == CSH->c->connectionID)
		{
			plSettings.connectedPlayerIDs.insert(playerConnectionId);
			cl->playerint.clear();
			initInterfaces();
		}
	}
}

void RemoveBonus::applyCl(CClient *cl)
{
	cl->invalidatePaths();
	switch(who)
	{
	case HERO:
		{
			const CGHeroInstance *h = GS(cl)->getHero(ObjectInstanceID(id));
			callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroBonusChanged, h, bonus, false);
		}
		break;
	case PLAYER:
		{
			//const PlayerState *p = GS(cl)->getPlayerState(id);
			callInterfaceIfPresent(cl, PlayerColor(id), &IGameEventsReceiver::playerBonusChanged, bonus, false);
		}
		break;
	}
}

void RemoveObject::applyFirstCl(CClient *cl)
{
	const CGObjectInstance *o = cl->getObj(id);

	if(CGI->mh)
		CGI->mh->hideObject(o, true);

	//notify interfaces about removal
	for(auto i=cl->playerint.begin(); i!=cl->playerint.end(); i++)
	{
		//below line contains little cheat for AI so it will be aware of deletion of enemy heroes that moved or got re-covered by FoW
		//TODO: loose requirements as next AI related crashes appear, for example another player collects object that got re-covered by FoW, unsure if AI code workarounds this
		if(GS(cl)->isVisible(o, i->first) || (!cl->getPlayerState(i->first)->human && o->ID == Obj::HERO && o->tempOwner != i->first))
			i->second->objectRemoved(o);
	}
}

void RemoveObject::applyCl(CClient *cl)
{
	cl->invalidatePaths();
}

void TryMoveHero::applyFirstCl(CClient *cl)
{
	CGHeroInstance *h = GS(cl)->getHero(id);

	//check if playerint will have the knowledge about movement - if not, directly update maphandler
	for(auto i=cl->playerint.begin(); i!=cl->playerint.end(); i++)
	{
		auto ps = GS(cl)->getPlayerState(i->first);
		if(ps && (GS(cl)->isVisible(start - int3(1, 0, 0), i->first) || GS(cl)->isVisible(end - int3(1, 0, 0), i->first)))
		{
			if(ps->human)
				humanKnows = true;
		}
	}

	if(!CGI->mh)
		return;

	if(result == TELEPORTATION  ||  result == EMBARK  ||  result == DISEMBARK  ||  !humanKnows)
		CGI->mh->hideObject(h, result == EMBARK && humanKnows);

	if(result == DISEMBARK)
		CGI->mh->printObject(h->boat);
}

void TryMoveHero::applyCl(CClient *cl)
{
	const CGHeroInstance *h = cl->getHero(id);
	cl->invalidatePaths();

	if(CGI->mh)
	{
		if(result == TELEPORTATION  ||  result == EMBARK  ||  result == DISEMBARK)
			CGI->mh->printObject(h, result == DISEMBARK);

		if(result == EMBARK)
			CGI->mh->hideObject(h->boat);
	}

	PlayerColor player = h->tempOwner;

	for(auto &i : cl->playerint)
		if(cl->getPlayerRelations(i.first, player) != PlayerRelations::ENEMIES)
			i.second->tileRevealed(fowRevealed);

	//notify interfaces about move
	auto gs = cl->gameState();

	for(auto i=cl->playerint.begin(); i!=cl->playerint.end(); i++)
	{
		if(i->first != PlayerColor::SPECTATOR && gs->checkForStandardLoss(i->first)) // Do not notify vanquished player's interface
			continue;

		if(GS(cl)->isVisible(start - int3(1, 0, 0), i->first)
			|| GS(cl)->isVisible(end - int3(1, 0, 0), i->first))
		{
			// src and dst of enemy hero move may be not visible => 'verbose' should be false
			const bool verbose = cl->getPlayerRelations(i->first, player) != PlayerRelations::ENEMIES;
			i->second->heroMoved(*this, verbose);
		}
	}

	//maphandler didn't get update from playerint, do it now
	//TODO: restructure nicely
	if(!humanKnows && CGI->mh)
		CGI->mh->printObject(h);
}

bool TryMoveHero::stopMovement() const
{
	return result != SUCCESS && result != EMBARK && result != DISEMBARK && result != TELEPORTATION;
}

void NewStructures::applyCl(CClient *cl)
{
	CGTownInstance *town = GS(cl)->getTown(tid);
	for(const auto & id : bid)
	{
		callInterfaceIfPresent(cl, town->tempOwner, &IGameEventsReceiver::buildChanged, town, id, 1);
	}
}
void RazeStructures::applyCl (CClient *cl)
{
	CGTownInstance *town = GS(cl)->getTown(tid);
	for(const auto & id : bid)
	{
		callInterfaceIfPresent(cl, town->tempOwner, &IGameEventsReceiver::buildChanged, town, id, 2);
	}
}

void SetAvailableCreatures::applyCl(CClient *cl)
{
	const CGDwelling *dw = static_cast<const CGDwelling*>(cl->getObj(tid));

	//inform order about the change

	PlayerColor p;
	if(dw->ID == Obj::WAR_MACHINE_FACTORY) //War Machines Factory is not flaggable, it's "owned" by visitor
		p = cl->getTile(dw->visitablePos())->visitableObjects.back()->tempOwner;
	else
		p = dw->tempOwner;

	callInterfaceIfPresent(cl, p, &IGameEventsReceiver::availableCreaturesChanged, dw);
}

void SetHeroesInTown::applyCl(CClient *cl)
{
	CGTownInstance *t = GS(cl)->getTown(tid);
	CGHeroInstance *hGarr  = GS(cl)->getHero(this->garrison);
	CGHeroInstance *hVisit = GS(cl)->getHero(this->visiting);

	//inform all players that see this object
	for(auto i = cl->playerint.cbegin(); i != cl->playerint.cend(); ++i)
	{
		if(i->first >= PlayerColor::PLAYER_LIMIT)
			continue;

		if(GS(cl)->isVisible(t, i->first) ||
			(hGarr && GS(cl)->isVisible(hGarr, i->first)) ||
			(hVisit && GS(cl)->isVisible(hVisit, i->first)))
		{
			cl->playerint[i->first]->heroInGarrisonChange(t);
		}
	}
}

void HeroRecruited::applyCl(CClient *cl)
{
	CGHeroInstance *h = GS(cl)->map->heroesOnMap.back();
	if(h->subID != hid)
	{
		logNetwork->error("Something wrong with hero recruited!");
	}

	bool needsPrinting = true;
	if(callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroCreated, h))
	{
		if(const CGTownInstance *t = GS(cl)->getTown(tid))
		{
			callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroInGarrisonChange, t);
			needsPrinting = false;
		}
	}
	if(needsPrinting && CGI->mh)
		CGI->mh->printObject(h);
}

void GiveHero::applyCl(CClient *cl)
{
	CGHeroInstance *h = GS(cl)->getHero(id);
	if(CGI->mh)
		CGI->mh->printObject(h);
	callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroCreated, h);
}

void GiveHero::applyFirstCl(CClient *cl)
{
	if(CGI->mh)
		CGI->mh->hideObject(GS(cl)->getHero(id));
}

void InfoWindow::applyCl(CClient *cl)
{
	std::string str;
	text.toString(str);

	if(!callInterfaceIfPresent(cl, player, &CGameInterface::showInfoDialog, str,components,(soundBase::soundID)soundID))
		logNetwork->warn("We received InfoWindow for not our player...");
}

void SetObjectProperty::applyCl(CClient * cl)
{
	//inform all players that see this object
	for(auto it = cl->playerint.cbegin(); it != cl->playerint.cend(); ++it)
	{
		if(GS(cl)->isVisible(GS(cl)->getObjInstance(id), it->first))
			callInterfaceIfPresent(cl, it->first, &IGameEventsReceiver::objectPropertyChanged, this);
	}
}

void HeroLevelUp::applyCl(CClient *cl)
{
	const CGHeroInstance * hero = cl->getHero(heroId);
	assert(hero);
	callOnlyThatInterface(cl, player, &CGameInterface::heroGotLevel, hero, primskill, skills, queryID);
}

void CommanderLevelUp::applyCl(CClient *cl)
{
	const CGHeroInstance * hero = cl->getHero(heroId);
	assert(hero);
	const CCommanderInstance * commander = hero->commander;
	assert(commander);
	assert(commander->armyObj); //is it possible for Commander to exist beyond armed instance?
	callOnlyThatInterface(cl, player, &CGameInterface::commanderGotLevel, commander, skills, queryID);
}

void BlockingDialog::applyCl(CClient *cl)
{
	std::string str;
	text.toString(str);

	if(!callOnlyThatInterface(cl, player, &CGameInterface::showBlockingDialog, str, components, queryID, (soundBase::soundID)soundID, selection(), cancel()))
		logNetwork->warn("We received YesNoDialog for not our player...");
}

void GarrisonDialog::applyCl(CClient *cl)
{
	const CGHeroInstance *h = cl->getHero(hid);
	const CArmedInstance *obj = static_cast<const CArmedInstance*>(cl->getObj(objid));

	callOnlyThatInterface(cl, h->getOwner(), &CGameInterface::showGarrisonDialog, obj, h, removableUnits, queryID);
}

void ExchangeDialog::applyCl(CClient *cl)
{
	callInterfaceIfPresent(cl, player, &IGameEventsReceiver::heroExchangeStarted, hero1, hero2, queryID);
}

void TeleportDialog::applyCl(CClient *cl)
{
	callOnlyThatInterface(cl, player, &CGameInterface::showTeleportDialog, channel, exits, impassable, queryID);
}

void MapObjectSelectDialog::applyCl(CClient * cl)
{
	callOnlyThatInterface(cl, player, &CGameInterface::showMapObjectSelectDialog, queryID, icon, title, description, objects);
}

void BattleStart::applyFirstCl(CClient *cl)
{
	// Cannot use the usual code because curB is not set yet
	callOnlyThatBattleInterface(cl, info->sides[0].color, &IBattleEventsReceiver::battleStartBefore, info->sides[0].armyObject, info->sides[1].armyObject,
		info->tile, info->sides[0].hero, info->sides[1].hero);
	callOnlyThatBattleInterface(cl, info->sides[1].color, &IBattleEventsReceiver::battleStartBefore, info->sides[0].armyObject, info->sides[1].armyObject,
		info->tile, info->sides[0].hero, info->sides[1].hero);
	callOnlyThatBattleInterface(cl, PlayerColor::SPECTATOR, &IBattleEventsReceiver::battleStartBefore, info->sides[0].armyObject, info->sides[1].armyObject,
		info->tile, info->sides[0].hero, info->sides[1].hero);
}

void BattleStart::applyCl(CClient *cl)
{
	cl->battleStarted(info);
}

void BattleNextRound::applyFirstCl(CClient *cl)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleNewRoundFirst, round);
}

void BattleNextRound::applyCl(CClient *cl)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleNewRound, round);
}

void BattleSetActiveStack::applyCl(CClient *cl)
{
	if(!askPlayerInterface)
		return;

	const CStack *activated = GS(cl)->curB->battleGetStackByID(stack);
	PlayerColor playerToCall; //player that will move activated stack
	if (activated->hasBonusOfType(Bonus::HYPNOTIZED))
	{
		playerToCall = (GS(cl)->curB->sides[0].color == activated->owner
			? GS(cl)->curB->sides[1].color
			: GS(cl)->curB->sides[0].color);
	}
	else
	{
		playerToCall = activated->owner;
	}

	cl->startPlayerBattleAction(playerToCall);
}

void BattleLogMessage::applyCl(CClient * cl)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleLogMessage, lines);
}

void BattleTriggerEffect::applyCl(CClient * cl)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleTriggerEffect, *this);
}

void BattleUpdateGateState::applyFirstCl(CClient * cl)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleGateStateChanged, state);
}

void BattleResult::applyFirstCl(CClient *cl)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleEnd, this);
	cl->battleFinished();
}

void BattleStackMoved::applyFirstCl(CClient *cl)
{
	const CStack * movedStack = GS(cl)->curB->battleGetStackByID(stack);
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleStackMoved, movedStack, tilesToMove, distance);
}

void BattleAttack::applyFirstCl(CClient *cl)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleAttack, this);
}

void BattleAttack::applyCl(CClient *cl)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleStacksAttacked, bsa);
}

void StartAction::applyFirstCl(CClient *cl)
{
	cl->curbaction = boost::make_optional(ba);
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::actionStarted, ba);
}

void BattleSpellCast::applyCl(CClient *cl)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleSpellCast, this);
}

void SetStackEffect::applyCl(CClient *cl)
{
	//informing about effects
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleStacksEffectsSet, *this);
}

void StacksInjured::applyCl(CClient *cl)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleStacksAttacked, stacks);
}

void BattleResultsApplied::applyCl(CClient *cl)
{
	callInterfaceIfPresent(cl, player1, &IGameEventsReceiver::battleResultsApplied);
	callInterfaceIfPresent(cl, player2, &IGameEventsReceiver::battleResultsApplied);
	callInterfaceIfPresent(cl, PlayerColor::SPECTATOR, &IGameEventsReceiver::battleResultsApplied);
}

void BattleUnitsChanged::applyCl(CClient * cl)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleUnitsChanged, changedStacks, customEffects);
}

void BattleObstaclesChanged::applyCl(CClient *cl)
{
	//inform interfaces about removed obstacles
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleObstaclesChanged, changes);
}

void CatapultAttack::applyCl(CClient *cl)
{
	//inform interfaces about catapult attack
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleCatapultAttacked, *this);
}

CGameState* CPackForClient::GS(CClient *cl)
{
	return cl->gs;
}

void EndAction::applyCl(CClient *cl)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::actionFinished, *cl->curbaction);
	cl->curbaction.reset();
}

void PackageApplied::applyCl(CClient *cl)
{
	callInterfaceIfPresent(cl, player, &IGameEventsReceiver::requestRealized, this);
	if(!CClient::waitingRequest.tryRemovingElement(requestID))
		logNetwork->warn("Surprising server message! PackageApplied for unknown requestID!");
}

void SystemMessage::applyCl(CClient *cl)
{
	std::ostringstream str;
	str << "System message: " << text;

	logNetwork->error(str.str()); // usually used to receive error messages from server
	if(LOCPLINT && !settings["session"]["hideSystemMessages"].Bool())
		LOCPLINT->cingconsole->print(str.str());
}

void PlayerBlocked::applyCl(CClient *cl)
{
	callInterfaceIfPresent(cl, player, &IGameEventsReceiver::playerBlocked, reason, startOrEnd == BLOCKADE_STARTED);
}

void YourTurn::applyCl(CClient *cl)
{
	logNetwork->debug("Server gives turn to %s", player.getStr());

	callAllInterfaces(cl, &IGameEventsReceiver::playerStartsTurn, player);
	callOnlyThatInterface(cl, player, &CGameInterface::yourTurn);
}

void SaveGameClient::applyCl(CClient *cl)
{
	const auto stem = FileInfo::GetPathStem(fname);
	if(!CResourceHandler::get("local")->createResource(stem.to_string() + ".vcgm1"))
	{
		logNetwork->error("Failed to create resource %s", stem.to_string() + ".vcgm1");
		return;
	}

	try
	{
		CSaveFile save(*CResourceHandler::get()->getResourceName(ResourceID(stem.to_string(), EResType::CLIENT_SAVEGAME)));
		cl->saveCommonState(save);
		save << *cl;
	}
	catch(std::exception &e)
	{
		logNetwork->error("Failed to save game:%s", e.what());
	}
}

void PlayerMessageClient::applyCl(CClient *cl)
{
	logNetwork->debug("Player %s sends a message: %s", player.getStr(), text);

	std::ostringstream str;
	if(player.isSpectator())
		str << "Spectator: " << text;
	else
		str << cl->getPlayerState(player)->nodeName() <<": " << text;
	if(LOCPLINT)
		LOCPLINT->cingconsole->print(str.str());
}

void ShowInInfobox::applyCl(CClient *cl)
{
	callInterfaceIfPresent(cl, player, &IGameEventsReceiver::showComp, c, text.toString());
}

void AdvmapSpellCast::applyCl(CClient *cl)
{
	cl->invalidatePaths();
	auto caster = cl->getHero(casterID);
	if(caster)
		//consider notifying other interfaces that see hero?
		callInterfaceIfPresent(cl, caster->getOwner(), &IGameEventsReceiver::advmapSpellCast, caster, spellID);
	else
		logNetwork->error("Invalid hero instance");
}

void ShowWorldViewEx::applyCl(CClient * cl)
{
	callOnlyThatInterface(cl, player, &CGameInterface::showWorldViewEx, objectPositions);
}

void OpenWindow::applyCl(CClient *cl)
{
	switch(window)
	{
	case RECRUITMENT_FIRST:
	case RECRUITMENT_ALL:
		{
			const CGDwelling *dw = dynamic_cast<const CGDwelling*>(cl->getObj(ObjectInstanceID(id1)));
			const CArmedInstance *dst = dynamic_cast<const CArmedInstance*>(cl->getObj(ObjectInstanceID(id2)));
			callInterfaceIfPresent(cl, dst->tempOwner, &IGameEventsReceiver::showRecruitmentDialog, dw, dst, window == RECRUITMENT_FIRST ? 0 : -1);
		}
		break;
	case SHIPYARD_WINDOW:
		{
			const IShipyard *sy = IShipyard::castFrom(cl->getObj(ObjectInstanceID(id1)));
			callInterfaceIfPresent(cl, sy->o->tempOwner, &IGameEventsReceiver::showShipyardDialog, sy);
		}
		break;
	case THIEVES_GUILD:
		{
			//displays Thieves' Guild window (when hero enters Den of Thieves)
			const CGObjectInstance *obj = cl->getObj(ObjectInstanceID(id2));
			callInterfaceIfPresent(cl, PlayerColor(id1), &IGameEventsReceiver::showThievesGuildWindow, obj);
		}
		break;
	case UNIVERSITY_WINDOW:
		{
			//displays University window (when hero enters University on adventure map)
			const IMarket *market = IMarket::castFrom(cl->getObj(ObjectInstanceID(id1)));
			const CGHeroInstance *hero = cl->getHero(ObjectInstanceID(id2));
			callInterfaceIfPresent(cl, hero->tempOwner, &IGameEventsReceiver::showUniversityWindow, market, hero);
		}
		break;
	case MARKET_WINDOW:
		{
			//displays Thieves' Guild window (when hero enters Den of Thieves)
			const CGObjectInstance *obj = cl->getObj(ObjectInstanceID(id1));
			const CGHeroInstance *hero = cl->getHero(ObjectInstanceID(id2));
			const IMarket *market = IMarket::castFrom(obj);
			callInterfaceIfPresent(cl, cl->getTile(obj->visitablePos())->visitableObjects.back()->tempOwner, &IGameEventsReceiver::showMarketWindow, market, hero);
		}
		break;
	case HILL_FORT_WINDOW:
		{
			//displays Hill fort window
			const CGObjectInstance *obj = cl->getObj(ObjectInstanceID(id1));
			const CGHeroInstance *hero = cl->getHero(ObjectInstanceID(id2));
			callInterfaceIfPresent(cl, cl->getTile(obj->visitablePos())->visitableObjects.back()->tempOwner, &IGameEventsReceiver::showHillFortWindow, obj, hero);
		}
		break;
	case PUZZLE_MAP:
		{
			callInterfaceIfPresent(cl, PlayerColor(id1), &IGameEventsReceiver::showPuzzleMap);
		}
		break;
	case TAVERN_WINDOW:
		const CGObjectInstance *obj1 = cl->getObj(ObjectInstanceID(id1)),
								*obj2 = cl->getObj(ObjectInstanceID(id2));
		callInterfaceIfPresent(cl, obj1->tempOwner, &IGameEventsReceiver::showTavernWindow, obj2);
		break;
	}

}

void CenterView::applyCl(CClient *cl)
{
	callInterfaceIfPresent(cl, player, &IGameEventsReceiver::centerView, pos, focusTime);
}

void NewObject::applyCl(CClient *cl)
{
	cl->invalidatePaths();

	const CGObjectInstance *obj = cl->getObj(id);
	if(CGI->mh)
		CGI->mh->printObject(obj, true);

	for(auto i=cl->playerint.begin(); i!=cl->playerint.end(); i++)
	{
		if(GS(cl)->isVisible(obj, i->first))
			i->second->newObject(obj);
	}
}

void SetAvailableArtifacts::applyCl(CClient *cl)
{
	if(id < 0) //artifact merchants globally
	{
		callAllInterfaces(cl, &IGameEventsReceiver::availableArtifactsChanged, nullptr);
	}
	else
	{
		const CGBlackMarket *bm = dynamic_cast<const CGBlackMarket *>(cl->getObj(ObjectInstanceID(id)));
		assert(bm);
		callInterfaceIfPresent(cl, cl->getTile(bm->visitablePos())->visitableObjects.back()->tempOwner, &IGameEventsReceiver::availableArtifactsChanged, bm);
	}
}


void EntitiesChanged::applyCl(CClient *cl)
{
	cl->invalidatePaths();
}
