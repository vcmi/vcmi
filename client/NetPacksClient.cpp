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
#include "ClientNetPackVisitors.h"

#include "Client.h"
#include "CPlayerInterface.h"
#include "CGameInfo.h"
#include "windows/GUIClasses.h"
#include "mapRenderer/mapHandler.h"
#include "adventureMap/CInGameConsole.h"
#include "battle/BattleInterface.h"
#include "gui/CGuiHandler.h"
#include "widgets/MiscWidgets.h"
#include "CMT.h"
#include "CServerHandler.h"

#include "../CCallback.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/FileInfo.h"
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
#include "../lib/CConfigHandler.h"
#include "../lib/mapping/CCampaignHandler.h"
#include "../lib/CGameState.h"
#include "../lib/CStack.h"
#include "../lib/battle/BattleInfo.h"
#include "../lib/GameConstants.h"
#include "../lib/CPlayerState.h"

// TODO: as Tow suggested these template should all be part of CClient
// This will require rework spectator interface properly though

template<typename T, typename ... Args, typename ... Args2>
bool callOnlyThatInterface(CClient & cl, PlayerColor player, void (T::*ptr)(Args...), Args2 && ...args)
{
	if(vstd::contains(cl.playerint, player))
	{
		((*cl.playerint[player]).*ptr)(std::forward<Args2>(args)...);
		return true;
	}
	return false;
}

template<typename T, typename ... Args, typename ... Args2>
bool callInterfaceIfPresent(CClient & cl, PlayerColor player, void (T::*ptr)(Args...), Args2 && ...args)
{
	bool called = callOnlyThatInterface(cl, player, ptr, std::forward<Args2>(args)...);
	return called;
}

template<typename T, typename ... Args, typename ... Args2>
void callOnlyThatBattleInterface(CClient & cl, PlayerColor player, void (T::*ptr)(Args...), Args2 && ...args)
{
	if(vstd::contains(cl.battleints,player))
		((*cl.battleints[player]).*ptr)(std::forward<Args2>(args)...);

	if(cl.additionalBattleInts.count(player))
	{
		for(auto bInt : cl.additionalBattleInts[player])
			((*bInt).*ptr)(std::forward<Args2>(args)...);
	}
}

template<typename T, typename ... Args, typename ... Args2>
void callBattleInterfaceIfPresent(CClient & cl, PlayerColor player, void (T::*ptr)(Args...), Args2 && ...args)
{
	callOnlyThatInterface(cl, player, ptr, std::forward<Args2>(args)...);
}

//calls all normal interfaces and privileged ones, playerints may be updated when iterating over it, so we need a copy
template<typename T, typename ... Args, typename ... Args2>
void callAllInterfaces(CClient & cl, void (T::*ptr)(Args...), Args2 && ...args)
{
	for(auto pInt : cl.playerint)
		((*pInt.second).*ptr)(std::forward<Args2>(args)...);
}

//calls all normal interfaces and privileged ones, playerints may be updated when iterating over it, so we need a copy
template<typename T, typename ... Args, typename ... Args2>
void callBattleInterfaceIfPresentForBothSides(CClient & cl, void (T::*ptr)(Args...), Args2 && ...args)
{
	callOnlyThatBattleInterface(cl, cl.gameState()->curB->sides[0].color, ptr, std::forward<Args2>(args)...);
	callOnlyThatBattleInterface(cl, cl.gameState()->curB->sides[1].color, ptr, std::forward<Args2>(args)...);
	if(settings["session"]["spectate"].Bool() && !settings["session"]["spectate-skip-battle"].Bool() && LOCPLINT->battleInt)
	{
		callOnlyThatBattleInterface(cl, PlayerColor::SPECTATOR, ptr, std::forward<Args2>(args)...);
	}
}

void ApplyClientNetPackVisitor::visitSetResources(SetResources & pack)
{
	//todo: inform on actual resource set transfered
	callInterfaceIfPresent(cl, pack.player, &IGameEventsReceiver::receivedResource);
}

void ApplyClientNetPackVisitor::visitSetPrimSkill(SetPrimSkill & pack)
{
	const CGHeroInstance * h = cl.getHero(pack.id);
	if(!h)
	{
		logNetwork->error("Cannot find hero with pack.id %d", pack.id.getNum());
		return;
	}
	callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroPrimarySkillChanged, h, pack.which, pack.val);
}

void ApplyClientNetPackVisitor::visitSetSecSkill(SetSecSkill & pack)
{
	const CGHeroInstance *h = cl.getHero(pack.id);
	if(!h)
	{
		logNetwork->error("Cannot find hero with pack.id %d", pack.id.getNum());
		return;
	}
	callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroSecondarySkillChanged, h, pack.which, pack.val);
}

void ApplyClientNetPackVisitor::visitHeroVisitCastle(HeroVisitCastle & pack)
{
	const CGHeroInstance *h = cl.getHero(pack.hid);
	
	if(pack.start())
	{
		callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroVisitsTown, h, gs.getTown(pack.tid));
	}
}

void ApplyClientNetPackVisitor::visitSetMana(SetMana & pack)
{
	const CGHeroInstance *h = cl.getHero(pack.hid);
	callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroManaPointsChanged, h);
}

void ApplyClientNetPackVisitor::visitSetMovePoints(SetMovePoints & pack)
{
	const CGHeroInstance *h = cl.getHero(pack.hid);
	cl.invalidatePaths();
	callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroMovePointsChanged, h);
}

void ApplyClientNetPackVisitor::visitFoWChange(FoWChange & pack)
{
	for(auto &i : cl.playerint)
	{
		if(cl.getPlayerRelations(i.first, pack.player) == PlayerRelations::SAME_PLAYER && pack.waitForDialogs && LOCPLINT == i.second.get())
		{
			LOCPLINT->waitWhileDialog();
		}
		if(cl.getPlayerRelations(i.first, pack.player) != PlayerRelations::ENEMIES)
		{
			if(pack.mode)
				i.second->tileRevealed(pack.tiles);
			else
				i.second->tileHidden(pack.tiles);
		}
	}
	cl.invalidatePaths();
}

static void dispatchGarrisonChange(CClient & cl, ObjectInstanceID army1, ObjectInstanceID army2)
{
	auto obj1 = cl.getObj(army1);
	if(!obj1)
	{
		logNetwork->error("Cannot find army with pack.id %d", army1.getNum());
		return;
	}

	callInterfaceIfPresent(cl, obj1->tempOwner, &IGameEventsReceiver::garrisonsChanged, army1, army2);

	if(army2 != ObjectInstanceID() && army2 != army1)
	{
		auto obj2 = cl.getObj(army2);
		if(!obj2)
		{
			logNetwork->error("Cannot find army with pack.id %d", army2.getNum());
			return;
		}

		if(obj1->tempOwner != obj2->tempOwner)
			callInterfaceIfPresent(cl, obj2->tempOwner, &IGameEventsReceiver::garrisonsChanged, army1, army2);
	}
}

void ApplyClientNetPackVisitor::visitChangeStackCount(ChangeStackCount & pack)
{
	dispatchGarrisonChange(cl, pack.army, ObjectInstanceID());
}

void ApplyClientNetPackVisitor::visitSetStackType(SetStackType & pack)
{
	dispatchGarrisonChange(cl, pack.army, ObjectInstanceID());
}

void ApplyClientNetPackVisitor::visitEraseStack(EraseStack & pack)
{
	dispatchGarrisonChange(cl, pack.army, ObjectInstanceID());
}

void ApplyClientNetPackVisitor::visitSwapStacks(SwapStacks & pack)
{
	dispatchGarrisonChange(cl, pack.srcArmy, pack.dstArmy);
}

void ApplyClientNetPackVisitor::visitInsertNewStack(InsertNewStack & pack)
{
	dispatchGarrisonChange(cl, pack.army, ObjectInstanceID());
}

void ApplyClientNetPackVisitor::visitRebalanceStacks(RebalanceStacks & pack)
{
	dispatchGarrisonChange(cl, pack.srcArmy, pack.dstArmy);
}

void ApplyClientNetPackVisitor::visitBulkRebalanceStacks(BulkRebalanceStacks & pack)
{
	if(!pack.moves.empty())
	{
		auto destArmy = pack.moves[0].srcArmy == pack.moves[0].dstArmy
			? ObjectInstanceID()
			: pack.moves[0].dstArmy;
		dispatchGarrisonChange(cl, pack.moves[0].srcArmy, destArmy);
	}
}

void ApplyClientNetPackVisitor::visitBulkSmartRebalanceStacks(BulkSmartRebalanceStacks & pack)
{
	if(!pack.moves.empty())
	{
		assert(pack.moves[0].srcArmy == pack.moves[0].dstArmy);
		dispatchGarrisonChange(cl, pack.moves[0].srcArmy, ObjectInstanceID());
	}
	else if(!pack.changes.empty())
	{
		dispatchGarrisonChange(cl, pack.changes[0].army, ObjectInstanceID());
	}
}

void ApplyClientNetPackVisitor::visitPutArtifact(PutArtifact & pack)
{
	callInterfaceIfPresent(cl, pack.al.owningPlayer(), &IGameEventsReceiver::artifactPut, pack.al);
}

void ApplyClientNetPackVisitor::visitEraseArtifact(EraseArtifact & pack)
{
	callInterfaceIfPresent(cl, pack.al.owningPlayer(), &IGameEventsReceiver::artifactRemoved, pack.al);
}

void ApplyClientNetPackVisitor::visitMoveArtifact(MoveArtifact & pack)
{
	auto moveArtifact = [this, &pack](PlayerColor player) -> void
	{
		callInterfaceIfPresent(cl, player, &IGameEventsReceiver::artifactMoved, pack.src, pack.dst);
		if(pack.askAssemble)
			callInterfaceIfPresent(cl, player, &IGameEventsReceiver::askToAssembleArtifact, pack.dst);
	};

	moveArtifact(pack.src.owningPlayer());
	if(pack.src.owningPlayer() != pack.dst.owningPlayer())
		moveArtifact(pack.dst.owningPlayer());
}

void ApplyClientNetPackVisitor::visitBulkMoveArtifacts(BulkMoveArtifacts & pack)
{
	auto applyMove = [this, &pack](std::vector<BulkMoveArtifacts::LinkedSlots> & artsPack) -> void
	{
		for(auto & slotToMove : artsPack)
		{
			auto srcLoc = ArtifactLocation(pack.srcArtHolder, slotToMove.srcPos);
			auto dstLoc = ArtifactLocation(pack.dstArtHolder, slotToMove.dstPos);
			MoveArtifact ma(&srcLoc, &dstLoc, false);
			visitMoveArtifact(ma);
		}
	};

	// Begin a session of bulk movement of arts. It is not necessary but useful for the client optimization.
	callInterfaceIfPresent(cl, cl.getCurrentPlayer(), &IGameEventsReceiver::bulkArtMovementStart, 
		pack.artsPack0.size() + pack.artsPack1.size());
	applyMove(pack.artsPack0);
	if(pack.swap)
		applyMove(pack.artsPack1);
}

void ApplyClientNetPackVisitor::visitAssembledArtifact(AssembledArtifact & pack)
{
	callInterfaceIfPresent(cl, pack.al.owningPlayer(), &IGameEventsReceiver::artifactAssembled, pack.al);
}

void ApplyClientNetPackVisitor::visitDisassembledArtifact(DisassembledArtifact & pack)
{
	callInterfaceIfPresent(cl, pack.al.owningPlayer(), &IGameEventsReceiver::artifactDisassembled, pack.al);
}

void ApplyClientNetPackVisitor::visitHeroVisit(HeroVisit & pack)
{
	auto hero = cl.getHero(pack.heroId);
	auto obj = cl.getObj(pack.objId, false);
	callInterfaceIfPresent(cl, pack.player, &IGameEventsReceiver::heroVisit, hero, obj, pack.starting);
}

void ApplyClientNetPackVisitor::visitNewTurn(NewTurn & pack)
{
	cl.invalidatePaths();
}

void ApplyClientNetPackVisitor::visitGiveBonus(GiveBonus & pack)
{
	cl.invalidatePaths();
	switch(pack.who)
	{
	case GiveBonus::HERO:
		{
			const CGHeroInstance *h = gs.getHero(ObjectInstanceID(pack.id));
			callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroBonusChanged, h, *h->getBonusList().back(), true);
		}
		break;
	case GiveBonus::PLAYER:
		{
			const PlayerState *p = gs.getPlayerState(PlayerColor(pack.id));
			callInterfaceIfPresent(cl, PlayerColor(pack.id), &IGameEventsReceiver::playerBonusChanged, *p->getBonusList().back(), true);
		}
		break;
	}
}

void ApplyFirstClientNetPackVisitor::visitChangeObjPos(ChangeObjPos & pack)
{
	CGObjectInstance *obj = gs.getObjInstance(pack.objid);
	if(CGI->mh)
		CGI->mh->onObjectFadeOut(obj);

	CGI->mh->waitForOngoingAnimations();
}
void ApplyClientNetPackVisitor::visitChangeObjPos(ChangeObjPos & pack)
{
	CGObjectInstance *obj = gs.getObjInstance(pack.objid);
	if(CGI->mh)
		CGI->mh->onObjectFadeIn(obj);

	CGI->mh->waitForOngoingAnimations();
	cl.invalidatePaths();
}

void ApplyClientNetPackVisitor::visitPlayerEndsGame(PlayerEndsGame & pack)
{
	callAllInterfaces(cl, &IGameEventsReceiver::gameOver, pack.player, pack.victoryLossCheckResult);

	// In auto testing pack.mode we always close client if red pack.player won or lose
	if(!settings["session"]["testmap"].isNull() && pack.player == PlayerColor(0))
		handleQuit(settings["session"]["spectate"].Bool()); // if spectator is active ask to close client or not
}

void ApplyClientNetPackVisitor::visitPlayerReinitInterface(PlayerReinitInterface & pack)
{
	auto initInterfaces = [this]()
	{
		cl.initPlayerInterfaces();
		auto currentPlayer = cl.gameState()->currentPlayer;
		callAllInterfaces(cl, &IGameEventsReceiver::playerStartsTurn, currentPlayer);
		callOnlyThatInterface(cl, currentPlayer, &CGameInterface::yourTurn);
	};
	
	for(auto player : pack.players)
	{
		auto & plSettings = CSH->si->getIthPlayersSettings(player);
		if(pack.playerConnectionId == PlayerSettings::PLAYER_AI)
		{
			plSettings.connectedPlayerIDs.clear();
			cl.initPlayerEnvironments();
			initInterfaces();
		}
		else if(pack.playerConnectionId == CSH->c->connectionID)
		{
			plSettings.connectedPlayerIDs.insert(pack.playerConnectionId);
			cl.playerint.clear();
			initInterfaces();
		}
	}
}

void ApplyClientNetPackVisitor::visitRemoveBonus(RemoveBonus & pack)
{
	cl.invalidatePaths();
	switch(pack.who)
	{
	case RemoveBonus::HERO:
		{
			const CGHeroInstance *h = gs.getHero(ObjectInstanceID(pack.id));
			callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroBonusChanged, h, pack.bonus, false);
		}
		break;
	case RemoveBonus::PLAYER:
		{
			//const PlayerState *p = gs.getPlayerState(pack.id);
			callInterfaceIfPresent(cl, PlayerColor(pack.id), &IGameEventsReceiver::playerBonusChanged, pack.bonus, false);
		}
		break;
	}
}

void ApplyFirstClientNetPackVisitor::visitRemoveObject(RemoveObject & pack)
{
	const CGObjectInstance *o = cl.getObj(pack.id);

	if(CGI->mh)
		CGI->mh->onObjectFadeOut(o);

	//notify interfaces about removal
	for(auto i=cl.playerint.begin(); i!=cl.playerint.end(); i++)
	{
		//below line contains little cheat for AI so it will be aware of deletion of enemy heroes that moved or got re-covered by FoW
		//TODO: loose requirements as next AI related crashes appear, for example another pack.player collects object that got re-covered by FoW, unsure if AI code workarounds this
		if(gs.isVisible(o, i->first) || (!cl.getPlayerState(i->first)->human && o->ID == Obj::HERO && o->tempOwner != i->first))
			i->second->objectRemoved(o);
	}

	CGI->mh->waitForOngoingAnimations();
}

void ApplyClientNetPackVisitor::visitRemoveObject(RemoveObject & pack)
{
	cl.invalidatePaths();
	for(auto i=cl.playerint.begin(); i!=cl.playerint.end(); i++)
		i->second->objectRemovedAfter();
}

void ApplyFirstClientNetPackVisitor::visitTryMoveHero(TryMoveHero & pack)
{
	CGHeroInstance *h = gs.getHero(pack.id);

	//check if playerint will have the knowledge about movement - if not, directly update maphandler
	for(auto i=cl.playerint.begin(); i!=cl.playerint.end(); i++)
	{
		auto ps = gs.getPlayerState(i->first);
		if(ps && (gs.isVisible(h->convertToVisitablePos(pack.start), i->first) || gs.isVisible(h->convertToVisitablePos(pack.end), i->first)))
		{
			if(ps->human)
				pack.humanKnows = true;
		}
	}

	if(CGI->mh && pack.result == TryMoveHero::EMBARK)
	{
		CGI->mh->onObjectFadeOut(h);
		CGI->mh->waitForOngoingAnimations();
	}
}

void ApplyClientNetPackVisitor::visitTryMoveHero(TryMoveHero & pack)
{
	const CGHeroInstance *h = cl.getHero(pack.id);
	cl.invalidatePaths();

	if(CGI->mh)
	{
		switch(pack.result)
		{
			case TryMoveHero::FAILED:
				break; // no-op
			case TryMoveHero::SUCCESS:
				CGI->mh->onHeroMoved(h, pack.start, pack.end);
				break;
			case TryMoveHero::TELEPORTATION:
				CGI->mh->onHeroTeleported(h, pack.start, pack.end);
				break;
			case TryMoveHero::BLOCKING_VISIT:
				CGI->mh->onHeroRotated(h, pack.start, pack.end);
				break;
			case TryMoveHero::EMBARK:
				// handled in ApplyFirst
				//CGI->mh->onObjectFadeOut(h);
				break;
			case TryMoveHero::DISEMBARK:
				CGI->mh->onObjectFadeIn(h);
				break;
		}
	}

	PlayerColor player = h->tempOwner;

	for(auto &i : cl.playerint)
		if(cl.getPlayerRelations(i.first, player) != PlayerRelations::ENEMIES)
			i.second->tileRevealed(pack.fowRevealed);

	for(auto i=cl.playerint.begin(); i!=cl.playerint.end(); i++)
	{
		if(i->first != PlayerColor::SPECTATOR && gs.checkForStandardLoss(i->first)) // Do not notify vanquished pack.player's interface
			continue;

		if(gs.isVisible(h->convertToVisitablePos(pack.start), i->first)
			|| gs.isVisible(h->convertToVisitablePos(pack.end), i->first))
		{
			// pack.src and pack.dst of enemy hero move may be not visible => 'verbose' should be false
			const bool verbose = cl.getPlayerRelations(i->first, player) != PlayerRelations::ENEMIES;
			i->second->heroMoved(pack, verbose);
		}
	}
}

void ApplyClientNetPackVisitor::visitNewStructures(NewStructures & pack)
{
	CGTownInstance *town = gs.getTown(pack.tid);
	for(const auto & id : pack.bid)
	{
		callInterfaceIfPresent(cl, town->tempOwner, &IGameEventsReceiver::buildChanged, town, id, 1);
	}
}
void ApplyClientNetPackVisitor::visitRazeStructures(RazeStructures & pack)
{
	CGTownInstance * town = gs.getTown(pack.tid);
	for(const auto & id : pack.bid)
	{
		callInterfaceIfPresent(cl, town->tempOwner, &IGameEventsReceiver::buildChanged, town, id, 2);
	}
}

void ApplyClientNetPackVisitor::visitSetAvailableCreatures(SetAvailableCreatures & pack)
{
	const CGDwelling * dw = static_cast<const CGDwelling*>(cl.getObj(pack.tid));

	PlayerColor p;
	if(dw->ID == Obj::WAR_MACHINE_FACTORY) //War Machines Factory is not flaggable, it's "owned" by visitor
		p = cl.getTile(dw->visitablePos())->visitableObjects.back()->tempOwner;
	else
		p = dw->tempOwner;

	callInterfaceIfPresent(cl, p, &IGameEventsReceiver::availableCreaturesChanged, dw);
}

void ApplyClientNetPackVisitor::visitSetHeroesInTown(SetHeroesInTown & pack)
{
	CGTownInstance * t = gs.getTown(pack.tid);
	CGHeroInstance * hGarr  = gs.getHero(pack.garrison);
	CGHeroInstance * hVisit = gs.getHero(pack.visiting);

	//inform all players that see this object
	for(auto i = cl.playerint.cbegin(); i != cl.playerint.cend(); ++i)
	{
		if(i->first >= PlayerColor::PLAYER_LIMIT)
			continue;

		if(gs.isVisible(t, i->first) ||
			(hGarr && gs.isVisible(hGarr, i->first)) ||
			(hVisit && gs.isVisible(hVisit, i->first)))
		{
			cl.playerint[i->first]->heroInGarrisonChange(t);
		}
	}
}

void ApplyClientNetPackVisitor::visitHeroRecruited(HeroRecruited & pack)
{
	CGHeroInstance *h = gs.map->heroesOnMap.back();
	if(h->subID != pack.hid)
	{
		logNetwork->error("Something wrong with hero recruited!");
	}

	if(callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroCreated, h))
	{
		if(const CGTownInstance *t = gs.getTown(pack.tid))
			callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroInGarrisonChange, t);
	}
	if(CGI->mh)
		CGI->mh->onObjectInstantAdd(h);
}

void ApplyClientNetPackVisitor::visitGiveHero(GiveHero & pack)
{
	CGHeroInstance *h = gs.getHero(pack.id);
	if(CGI->mh)
		CGI->mh->onObjectInstantAdd(h);
	callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroCreated, h);
}

void ApplyFirstClientNetPackVisitor::visitGiveHero(GiveHero & pack)
{
}

void ApplyClientNetPackVisitor::visitInfoWindow(InfoWindow & pack)
{
	std::string str;
	pack.text.toString(str);

	if(!callInterfaceIfPresent(cl, pack.player, &CGameInterface::showInfoDialog, str, pack.components,(soundBase::soundID)pack.soundID))
		logNetwork->warn("We received InfoWindow for not our player...");
}

void ApplyClientNetPackVisitor::visitSetObjectProperty(SetObjectProperty & pack)
{
	//inform all players that see this object
	for(auto it = cl.playerint.cbegin(); it != cl.playerint.cend(); ++it)
	{
		if(gs.isVisible(gs.getObjInstance(pack.id), it->first))
			callInterfaceIfPresent(cl, it->first, &IGameEventsReceiver::objectPropertyChanged, &pack);
	}
}

void ApplyClientNetPackVisitor::visitHeroLevelUp(HeroLevelUp & pack)
{
	const CGHeroInstance * hero = cl.getHero(pack.heroId);
	assert(hero);
	callOnlyThatInterface(cl, pack.player, &CGameInterface::heroGotLevel, hero, pack.primskill, pack.skills, pack.queryID);
}

void ApplyClientNetPackVisitor::visitCommanderLevelUp(CommanderLevelUp & pack)
{
	const CGHeroInstance * hero = cl.getHero(pack.heroId);
	assert(hero);
	const CCommanderInstance * commander = hero->commander;
	assert(commander);
	assert(commander->armyObj); //is it possible for Commander to exist beyond armed instance?
	callOnlyThatInterface(cl, pack.player, &CGameInterface::commanderGotLevel, commander, pack.skills, pack.queryID);
}

void ApplyClientNetPackVisitor::visitBlockingDialog(BlockingDialog & pack)
{
	std::string str;
	pack.text.toString(str);

	if(!callOnlyThatInterface(cl, pack.player, &CGameInterface::showBlockingDialog, str, pack.components, pack.queryID, (soundBase::soundID)pack.soundID, pack.selection(), pack.cancel()))
		logNetwork->warn("We received YesNoDialog for not our player...");
}

void ApplyClientNetPackVisitor::visitGarrisonDialog(GarrisonDialog & pack)
{
	const CGHeroInstance *h = cl.getHero(pack.hid);
	const CArmedInstance *obj = static_cast<const CArmedInstance*>(cl.getObj(pack.objid));

	callOnlyThatInterface(cl, h->getOwner(), &CGameInterface::showGarrisonDialog, obj, h, pack.removableUnits, pack.queryID);
}

void ApplyClientNetPackVisitor::visitExchangeDialog(ExchangeDialog & pack)
{
	callInterfaceIfPresent(cl, pack.player, &IGameEventsReceiver::heroExchangeStarted, pack.hero1, pack.hero2, pack.queryID);
}

void ApplyClientNetPackVisitor::visitTeleportDialog(TeleportDialog & pack)
{
	callOnlyThatInterface(cl, pack.player, &CGameInterface::showTeleportDialog, pack.channel, pack.exits, pack.impassable, pack.queryID);
}

void ApplyClientNetPackVisitor::visitMapObjectSelectDialog(MapObjectSelectDialog & pack)
{
	callOnlyThatInterface(cl, pack.player, &CGameInterface::showMapObjectSelectDialog, pack.queryID, pack.icon, pack.title, pack.description, pack.objects);
}

void ApplyFirstClientNetPackVisitor::visitBattleStart(BattleStart & pack)
{
	// Cannot use the usual code because curB is not set yet
	callOnlyThatBattleInterface(cl, pack.info->sides[0].color, &IBattleEventsReceiver::battleStartBefore, pack.info->sides[0].armyObject, pack.info->sides[1].armyObject,
		pack.info->tile, pack.info->sides[0].hero, pack.info->sides[1].hero);
	callOnlyThatBattleInterface(cl, pack.info->sides[1].color, &IBattleEventsReceiver::battleStartBefore, pack.info->sides[0].armyObject, pack.info->sides[1].armyObject,
		pack.info->tile, pack.info->sides[0].hero, pack.info->sides[1].hero);
	callOnlyThatBattleInterface(cl, PlayerColor::SPECTATOR, &IBattleEventsReceiver::battleStartBefore, pack.info->sides[0].armyObject, pack.info->sides[1].armyObject,
		pack.info->tile, pack.info->sides[0].hero, pack.info->sides[1].hero);
}

void ApplyClientNetPackVisitor::visitBattleStart(BattleStart & pack)
{
	cl.battleStarted(pack.info);
}

void ApplyFirstClientNetPackVisitor::visitBattleNextRound(BattleNextRound & pack)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleNewRoundFirst, pack.round);
}

void ApplyClientNetPackVisitor::visitBattleNextRound(BattleNextRound & pack)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleNewRound, pack.round);
}

void ApplyClientNetPackVisitor::visitBattleSetActiveStack(BattleSetActiveStack & pack)
{
	if(!pack.askPlayerInterface)
		return;

	const CStack *activated = gs.curB->battleGetStackByID(pack.stack);
	PlayerColor playerToCall; //pack.player that will move activated stack
	if (activated->hasBonusOfType(Bonus::HYPNOTIZED))
	{
		playerToCall = (gs.curB->sides[0].color == activated->owner
			? gs.curB->sides[1].color
			: gs.curB->sides[0].color);
	}
	else
	{
		playerToCall = activated->owner;
	}

	cl.startPlayerBattleAction(playerToCall);
}

void ApplyClientNetPackVisitor::visitBattleLogMessage(BattleLogMessage & pack)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleLogMessage, pack.lines);
}

void ApplyClientNetPackVisitor::visitBattleTriggerEffect(BattleTriggerEffect & pack)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleTriggerEffect, pack);
}

void ApplyFirstClientNetPackVisitor::visitBattleUpdateGateState(BattleUpdateGateState & pack)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleGateStateChanged, pack.state);
}

void ApplyFirstClientNetPackVisitor::visitBattleResult(BattleResult & pack)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleEnd, &pack);
	cl.battleFinished();
}

void ApplyFirstClientNetPackVisitor::visitBattleStackMoved(BattleStackMoved & pack)
{
	const CStack * movedStack = gs.curB->battleGetStackByID(pack.stack);
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleStackMoved, movedStack, pack.tilesToMove, pack.distance, pack.teleporting);
}

void ApplyFirstClientNetPackVisitor::visitBattleAttack(BattleAttack & pack)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleAttack, &pack);
}

void ApplyClientNetPackVisitor::visitBattleAttack(BattleAttack & pack)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleStacksAttacked, pack.bsa, pack.shot());
}

void ApplyFirstClientNetPackVisitor::visitStartAction(StartAction & pack)
{
	cl.curbaction = boost::make_optional(pack.ba);
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::actionStarted, pack.ba);
}

void ApplyClientNetPackVisitor::visitBattleSpellCast(BattleSpellCast & pack)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleSpellCast, &pack);
}

void ApplyClientNetPackVisitor::visitSetStackEffect(SetStackEffect & pack)
{
	//informing about effects
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleStacksEffectsSet, pack);
}

void ApplyClientNetPackVisitor::visitStacksInjured(StacksInjured & pack)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleStacksAttacked, pack.stacks, false);
}

void ApplyClientNetPackVisitor::visitBattleResultsApplied(BattleResultsApplied & pack)
{
	callInterfaceIfPresent(cl, pack.player1, &IGameEventsReceiver::battleResultsApplied);
	callInterfaceIfPresent(cl, pack.player2, &IGameEventsReceiver::battleResultsApplied);
	callInterfaceIfPresent(cl, PlayerColor::SPECTATOR, &IGameEventsReceiver::battleResultsApplied);
}

void ApplyClientNetPackVisitor::visitBattleUnitsChanged(BattleUnitsChanged & pack)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleUnitsChanged, pack.changedStacks);
}

void ApplyClientNetPackVisitor::visitBattleObstaclesChanged(BattleObstaclesChanged & pack)
{
	//inform interfaces about removed obstacles
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleObstaclesChanged, pack.changes);
}

void ApplyClientNetPackVisitor::visitCatapultAttack(CatapultAttack & pack)
{
	//inform interfaces about catapult attack
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::battleCatapultAttacked, pack);
}

void ApplyClientNetPackVisitor::visitEndAction(EndAction & pack)
{
	callBattleInterfaceIfPresentForBothSides(cl, &IBattleEventsReceiver::actionFinished, *cl.curbaction);
	cl.curbaction.reset();
}

void ApplyClientNetPackVisitor::visitPackageApplied(PackageApplied & pack)
{
	callInterfaceIfPresent(cl, pack.player, &IGameEventsReceiver::requestRealized, &pack);
	if(!CClient::waitingRequest.tryRemovingElement(pack.requestID))
		logNetwork->warn("Surprising server message! PackageApplied for unknown requestID!");
}

void ApplyClientNetPackVisitor::visitSystemMessage(SystemMessage & pack)
{
	std::ostringstream str;
	str << "System message: " << pack.text;

	logNetwork->error(str.str()); // usually used to receive error messages from server
	if(LOCPLINT && !settings["session"]["hideSystemMessages"].Bool())
		LOCPLINT->cingconsole->print(str.str());
}

void ApplyClientNetPackVisitor::visitPlayerBlocked(PlayerBlocked & pack)
{
	callInterfaceIfPresent(cl, pack.player, &IGameEventsReceiver::playerBlocked, pack.reason, pack.startOrEnd == PlayerBlocked::BLOCKADE_STARTED);
}

void ApplyClientNetPackVisitor::visitYourTurn(YourTurn & pack)
{
	logNetwork->debug("Server gives turn to %s", pack.player.getStr());

	callAllInterfaces(cl, &IGameEventsReceiver::playerStartsTurn, pack.player);
	callOnlyThatInterface(cl, pack.player, &CGameInterface::yourTurn);
}

void ApplyClientNetPackVisitor::visitSaveGameClient(SaveGameClient & pack)
{
	const auto stem = FileInfo::GetPathStem(pack.fname);
	if(!CResourceHandler::get("local")->createResource(stem.to_string() + ".vcgm1"))
	{
		logNetwork->error("Failed to create resource %s", stem.to_string() + ".vcgm1");
		return;
	}

	try
	{
		CSaveFile save(*CResourceHandler::get()->getResourceName(ResourceID(stem.to_string(), EResType::CLIENT_SAVEGAME)));
		cl.saveCommonState(save);
		save << cl;
	}
	catch(std::exception &e)
	{
		logNetwork->error("Failed to save game:%s", e.what());
	}
}

void ApplyClientNetPackVisitor::visitPlayerMessageClient(PlayerMessageClient & pack)
{
	logNetwork->debug("pack.player %s sends a message: %s", pack.player.getStr(), pack.text);

	std::ostringstream str;
	if(pack.player.isSpectator())
		str << "Spectator: " << pack.text;
	else
		str << cl.getPlayerState(pack.player)->nodeName() <<": " << pack.text;
	if(LOCPLINT)
		LOCPLINT->cingconsole->print(str.str());
}

void ApplyClientNetPackVisitor::visitShowInInfobox(ShowInInfobox & pack)
{
	callInterfaceIfPresent(cl, pack.player, &IGameEventsReceiver::showComp, pack.c, pack.text.toString());
}

void ApplyClientNetPackVisitor::visitAdvmapSpellCast(AdvmapSpellCast & pack)
{
	cl.invalidatePaths();
	auto caster = cl.getHero(pack.casterID);
	if(caster)
		//consider notifying other interfaces that see hero?
		callInterfaceIfPresent(cl, caster->getOwner(), &IGameEventsReceiver::advmapSpellCast, caster, pack.spellID);
	else
		logNetwork->error("Invalid hero instance");
}

void ApplyClientNetPackVisitor::visitShowWorldViewEx(ShowWorldViewEx & pack)
{
	callOnlyThatInterface(cl, pack.player, &CGameInterface::showWorldViewEx, pack.objectPositions, pack.showTerrain);
}

void ApplyClientNetPackVisitor::visitOpenWindow(OpenWindow & pack)
{
	switch(pack.window)
	{
	case OpenWindow::RECRUITMENT_FIRST:
	case OpenWindow::RECRUITMENT_ALL:
		{
			const CGDwelling *dw = dynamic_cast<const CGDwelling*>(cl.getObj(ObjectInstanceID(pack.id1)));
			const CArmedInstance *dst = dynamic_cast<const CArmedInstance*>(cl.getObj(ObjectInstanceID(pack.id2)));
			callInterfaceIfPresent(cl, dst->tempOwner, &IGameEventsReceiver::showRecruitmentDialog, dw, dst, pack.window == OpenWindow::RECRUITMENT_FIRST ? 0 : -1);
		}
		break;
	case OpenWindow::SHIPYARD_WINDOW:
		{
			const IShipyard *sy = IShipyard::castFrom(cl.getObj(ObjectInstanceID(pack.id1)));
			callInterfaceIfPresent(cl, sy->o->tempOwner, &IGameEventsReceiver::showShipyardDialog, sy);
		}
		break;
	case OpenWindow::THIEVES_GUILD:
		{
			//displays Thieves' Guild window (when hero enters Den of Thieves)
			const CGObjectInstance *obj = cl.getObj(ObjectInstanceID(pack.id2));
			callInterfaceIfPresent(cl, PlayerColor(pack.id1), &IGameEventsReceiver::showThievesGuildWindow, obj);
		}
		break;
	case OpenWindow::UNIVERSITY_WINDOW:
		{
			//displays University window (when hero enters University on adventure map)
			const IMarket *market = IMarket::castFrom(cl.getObj(ObjectInstanceID(pack.id1)));
			const CGHeroInstance *hero = cl.getHero(ObjectInstanceID(pack.id2));
			callInterfaceIfPresent(cl, hero->tempOwner, &IGameEventsReceiver::showUniversityWindow, market, hero);
		}
		break;
	case OpenWindow::MARKET_WINDOW:
		{
			//displays Thieves' Guild window (when hero enters Den of Thieves)
			const CGObjectInstance *obj = cl.getObj(ObjectInstanceID(pack.id1));
			const CGHeroInstance *hero = cl.getHero(ObjectInstanceID(pack.id2));
			const IMarket *market = IMarket::castFrom(obj);
			callInterfaceIfPresent(cl, cl.getTile(obj->visitablePos())->visitableObjects.back()->tempOwner, &IGameEventsReceiver::showMarketWindow, market, hero);
		}
		break;
	case OpenWindow::HILL_FORT_WINDOW:
		{
			//displays Hill fort window
			const CGObjectInstance *obj = cl.getObj(ObjectInstanceID(pack.id1));
			const CGHeroInstance *hero = cl.getHero(ObjectInstanceID(pack.id2));
			callInterfaceIfPresent(cl, cl.getTile(obj->visitablePos())->visitableObjects.back()->tempOwner, &IGameEventsReceiver::showHillFortWindow, obj, hero);
		}
		break;
	case OpenWindow::PUZZLE_MAP:
		{
			callInterfaceIfPresent(cl, PlayerColor(pack.id1), &IGameEventsReceiver::showPuzzleMap);
		}
		break;
	case OpenWindow::TAVERN_WINDOW:
		const CGObjectInstance *obj1 = cl.getObj(ObjectInstanceID(pack.id1)),
								*obj2 = cl.getObj(ObjectInstanceID(pack.id2));
		callInterfaceIfPresent(cl, obj1->tempOwner, &IGameEventsReceiver::showTavernWindow, obj2);
		break;
	}
}

void ApplyClientNetPackVisitor::visitCenterView(CenterView & pack)
{
	callInterfaceIfPresent(cl, pack.player, &IGameEventsReceiver::centerView, pack.pos, pack.focusTime);
}

void ApplyClientNetPackVisitor::visitNewObject(NewObject & pack)
{
	cl.invalidatePaths();

	const CGObjectInstance *obj = cl.getObj(pack.id);
	if(CGI->mh)
		CGI->mh->onObjectFadeIn(obj);

	for(auto i=cl.playerint.begin(); i!=cl.playerint.end(); i++)
	{
		if(gs.isVisible(obj, i->first))
			i->second->newObject(obj);
	}
	CGI->mh->waitForOngoingAnimations();
}

void ApplyClientNetPackVisitor::visitSetAvailableArtifacts(SetAvailableArtifacts & pack)
{
	if(pack.id < 0) //artifact merchants globally
	{
		callAllInterfaces(cl, &IGameEventsReceiver::availableArtifactsChanged, nullptr);
	}
	else
	{
		const CGBlackMarket *bm = dynamic_cast<const CGBlackMarket *>(cl.getObj(ObjectInstanceID(pack.id)));
		assert(bm);
		callInterfaceIfPresent(cl, cl.getTile(bm->visitablePos())->visitableObjects.back()->tempOwner, &IGameEventsReceiver::availableArtifactsChanged, bm);
	}
}


void ApplyClientNetPackVisitor::visitEntitiesChanged(EntitiesChanged & pack)
{
	cl.invalidatePaths();
}
