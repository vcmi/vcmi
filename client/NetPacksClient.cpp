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
#include "windows/GUIClasses.h"
#include "windows/CCastleInterface.h"
#include "mapView/mapHandler.h"
#include "adventureMap/AdventureMapInterface.h"
#include "adventureMap/CInGameConsole.h"
#include "battle/BattleInterface.h"
#include "battle/BattleWindow.h"
#include "GameEngine.h"
#include "GameInstance.h"
#include "gui/WindowHandler.h"
#include "widgets/MiscWidgets.h"
#include "CMT.h"
#include "GameChatHandler.h"
#include "CServerHandler.h"
#include "UIHelper.h"

#include "../CCallback.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/FileInfo.h"
#include "../lib/serializer/Connection.h"
#include "../lib/texts/CGeneralTextHandler.h"
#include "../lib/GameLibrary.h"
#include "../lib/mapping/CMap.h"
#include "../lib/VCMIDirs.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/CSoundBase.h"
#include "../lib/StartInfo.h"
#include "../lib/CConfigHandler.h"
#include "../lib/mapObjects/CGMarket.h"
#include "../lib/mapObjects/CGTownInstance.h"
#include "../lib/gameState/CGameState.h"
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
void callBattleInterfaceIfPresentForBothSides(CClient & cl, const BattleID & battleID, void (T::*ptr)(Args...), Args2 && ...args)
{
	assert(cl.gameState().getBattle(battleID));

	if(!cl.gameState().getBattle(battleID))
	{
		logGlobal->error("Attempt to call battle interface without ongoing battle!");
		return;
	}

	callOnlyThatBattleInterface(cl, cl.gameState().getBattle(battleID)->getSide(BattleSide::ATTACKER).color, ptr, std::forward<Args2>(args)...);
	callOnlyThatBattleInterface(cl, cl.gameState().getBattle(battleID)->getSide(BattleSide::DEFENDER).color, ptr, std::forward<Args2>(args)...);
	if(settings["session"]["spectate"].Bool() && !settings["session"]["spectate-skip-battle"].Bool() && GAME->interface()->battleInt)
	{
		callOnlyThatBattleInterface(cl, PlayerColor::SPECTATOR, ptr, std::forward<Args2>(args)...);
	}
}

void ApplyClientNetPackVisitor::visitSetResources(SetResources & pack)
{
	//todo: inform on actual resource set transferred
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

	if(settings["session"]["headless"].Bool())
		return;

	for(auto window : ENGINE->windows().findWindows<BattleWindow>())
		window->heroManaPointsChanged(h);
}

void ApplyClientNetPackVisitor::visitSetMovePoints(SetMovePoints & pack)
{
	const CGHeroInstance *h = cl.getHero(pack.hid);
	callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroMovePointsChanged, h);
}

void ApplyClientNetPackVisitor::visitSetResearchedSpells(SetResearchedSpells & pack)
{
	for(const auto & win : ENGINE->windows().findWindows<CMageGuildScreen>())
		win->updateSpells(pack.tid);
}

void ApplyClientNetPackVisitor::visitFoWChange(FoWChange & pack)
{
	for(auto &i : cl.playerint)
	{
		if(cl.getPlayerRelations(i.first, pack.player) == PlayerRelations::SAME_PLAYER && pack.waitForDialogs && GAME->interface() == i.second.get())
		{
			GAME->interface()->waitWhileDialog();
		}
		if(cl.getPlayerRelations(i.first, pack.player) != PlayerRelations::ENEMIES)
		{
			if(pack.mode == ETileVisibility::REVEALED)
				i.second->tileRevealed(pack.tiles);
			else
				i.second->tileHidden(pack.tiles);
		}
	}
	callAllInterfaces(cl, &CGameInterface::invalidatePaths);
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

void ApplyClientNetPackVisitor::visitPutArtifact(PutArtifact & pack)
{
	callInterfaceIfPresent(cl, cl.getOwner(pack.al.artHolder), &IGameEventsReceiver::artifactPut, pack.al);
	if(pack.askAssemble)
		callInterfaceIfPresent(cl, cl.getOwner(pack.al.artHolder), &IGameEventsReceiver::askToAssembleArtifact, pack.al);
}

void ApplyClientNetPackVisitor::visitEraseArtifact(BulkEraseArtifacts & pack)
{
	for(const auto & slotErase : pack.posPack)
		callInterfaceIfPresent(cl, cl.getOwner(pack.artHolder), &IGameEventsReceiver::artifactRemoved, ArtifactLocation(pack.artHolder, slotErase));
}

void ApplyClientNetPackVisitor::visitBulkMoveArtifacts(BulkMoveArtifacts & pack)
{
	const auto dstOwner = cl.getOwner(pack.dstArtHolder);
	const auto applyMove = [this, &pack, dstOwner](const std::vector<MoveArtifactInfo> & artsPack)
	{
		for(const auto & slotToMove : artsPack)
		{
			const auto srcLoc = ArtifactLocation(pack.srcArtHolder, slotToMove.srcPos);
			const auto dstLoc = ArtifactLocation(pack.dstArtHolder, slotToMove.dstPos);

			callInterfaceIfPresent(cl, pack.interfaceOwner, &IGameEventsReceiver::artifactMoved, srcLoc, dstLoc);
			if(slotToMove.askAssemble)
				callInterfaceIfPresent(cl, pack.interfaceOwner, &IGameEventsReceiver::askToAssembleArtifact, dstLoc);
			if(pack.interfaceOwner != dstOwner)
				callInterfaceIfPresent(cl, dstOwner, &IGameEventsReceiver::artifactMoved, srcLoc, dstLoc);
		}
	};

	size_t possibleAssemblyNumOfArts = 0;
	const auto calcPossibleAssemblyNumOfArts = [&possibleAssemblyNumOfArts](const auto & slotToMove)
	{
		if(slotToMove.askAssemble)
			possibleAssemblyNumOfArts++;
	};
	std::for_each(pack.artsPack0.cbegin(), pack.artsPack0.cend(), calcPossibleAssemblyNumOfArts);
	std::for_each(pack.artsPack1.cbegin(), pack.artsPack1.cend(), calcPossibleAssemblyNumOfArts);


	// Begin a session of bulk movement of arts. It is not necessary but useful for the client optimization.
	callInterfaceIfPresent(cl, pack.interfaceOwner, &IGameEventsReceiver::bulkArtMovementStart,
		pack.artsPack0.size() + pack.artsPack1.size(), possibleAssemblyNumOfArts);
	if(pack.interfaceOwner != dstOwner)
		callInterfaceIfPresent(cl, dstOwner, &IGameEventsReceiver::bulkArtMovementStart,
			pack.artsPack0.size() + pack.artsPack1.size(), possibleAssemblyNumOfArts);

	applyMove(pack.artsPack0);
	if(!pack.artsPack1.empty())
		applyMove(pack.artsPack1);
}

void ApplyClientNetPackVisitor::visitAssembledArtifact(AssembledArtifact & pack)
{
	callInterfaceIfPresent(cl, cl.getOwner(pack.al.artHolder), &IGameEventsReceiver::artifactAssembled, pack.al);
}

void ApplyClientNetPackVisitor::visitDisassembledArtifact(DisassembledArtifact & pack)
{
	callInterfaceIfPresent(cl, cl.getOwner(pack.al.artHolder), &IGameEventsReceiver::artifactDisassembled, pack.al);
}

void ApplyClientNetPackVisitor::visitHeroVisit(HeroVisit & pack)
{
	auto hero = cl.getHero(pack.heroId);
	auto obj = cl.getObj(pack.objId, false);
	callInterfaceIfPresent(cl, pack.player, &IGameEventsReceiver::heroVisit, hero, obj, pack.starting);
}

void ApplyClientNetPackVisitor::visitNewTurn(NewTurn & pack)
{
	callAllInterfaces(cl, &CGameInterface::invalidatePaths);

	if(pack.newWeekNotification)
	{
		const auto & newWeek = *pack.newWeekNotification;

		std::string str = newWeek.text.toString();
		callAllInterfaces(cl, &CGameInterface::showInfoDialog, newWeek.type, str, newWeek.components,(soundBase::soundID)newWeek.soundID);
	}
}

void ApplyClientNetPackVisitor::visitGiveBonus(GiveBonus & pack)
{
	callAllInterfaces(cl, &CGameInterface::invalidatePaths);

	switch(pack.who)
	{
	case GiveBonus::ETarget::OBJECT:
		{
			const CGHeroInstance *h = gs.getHero(pack.id.as<ObjectInstanceID>());
			if(h)
				callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroBonusChanged, h, pack.bonus, true);
		}
		break;
	case GiveBonus::ETarget::PLAYER:
		{
			callInterfaceIfPresent(cl, pack.id.as<PlayerColor>(), &IGameEventsReceiver::playerBonusChanged, pack.bonus, true);
		}
		break;
	}
}

void ApplyFirstClientNetPackVisitor::visitChangeObjPos(ChangeObjPos & pack)
{
	CGObjectInstance *obj = gs.getObjInstance(pack.objid);
	GAME->map().onObjectFadeOut(obj, pack.initiator);
	GAME->map().waitForOngoingAnimations();
}

void ApplyClientNetPackVisitor::visitChangeObjPos(ChangeObjPos & pack)
{
	CGObjectInstance *obj = gs.getObjInstance(pack.objid);
	GAME->map().onObjectFadeIn(obj, pack.initiator);
	GAME->map().waitForOngoingAnimations();
	callAllInterfaces(cl, &CGameInterface::invalidatePaths);
}

void ApplyClientNetPackVisitor::visitPlayerEndsGame(PlayerEndsGame & pack)
{
	callAllInterfaces(cl, &IGameEventsReceiver::gameOver, pack.player, pack.victoryLossCheckResult);

	bool localHumanWinsGame = vstd::contains(cl.playerint, pack.player) && cl.getPlayerState(pack.player)->human && pack.victoryLossCheckResult.victory();
	bool lastHumanEndsGame = GAME->server().howManyPlayerInterfaces() == 1 && vstd::contains(cl.playerint, pack.player) && cl.getPlayerState(pack.player)->human && !settings["session"]["spectate"].Bool();

	if(lastHumanEndsGame || localHumanWinsGame)
	{
		assert(adventureInt);
		if(adventureInt)
		{
			ENGINE->windows().popWindows(ENGINE->windows().count());
			adventureInt.reset();
		}

		GAME->server().showHighScoresAndEndGameplay(pack.player, pack.victoryLossCheckResult.victory(), pack.statistic);
	}

	// In auto testing pack.mode we always close client if red pack.player won or lose
	if(!settings["session"]["testmap"].isNull() && pack.player == PlayerColor(0))
	{
		logAi->info("Red player %s. Ending game.", pack.victoryLossCheckResult.victory() ? "won" : "lost");

		GAME->onShutdownRequested(settings["session"]["spectate"].Bool()); // if spectator is active ask to close client or not
	}
}

void ApplyClientNetPackVisitor::visitPlayerReinitInterface(PlayerReinitInterface & pack)
{
	auto initInterfaces = [this]()
	{
		cl.initPlayerInterfaces();

		for(PlayerColor player(0); player < PlayerColor::PLAYER_LIMIT; ++player)
		{
			if(cl.gameState().isPlayerMakingTurn(player))
			{
				callAllInterfaces(cl, &IGameEventsReceiver::playerStartsTurn, player);
				callOnlyThatInterface(cl, player, &CGameInterface::yourTurn, QueryID::NONE);
			}
		}
	};
	
	for(auto player : pack.players)
	{
		auto & plSettings = GAME->server().si->getIthPlayersSettings(player);
		if(pack.playerConnectionId == PlayerSettings::PLAYER_AI)
		{
			plSettings.connectedPlayerIDs.clear();
			cl.initPlayerEnvironments();
			initInterfaces();
		}
		else if(pack.playerConnectionId == GAME->server().logicConnection->connectionID)
		{
			plSettings.connectedPlayerIDs.insert(pack.playerConnectionId);
			cl.playerint.clear();
			initInterfaces();
		}
	}
}

void ApplyClientNetPackVisitor::visitRemoveBonus(RemoveBonus & pack)
{
	switch(pack.who)
	{
	case GiveBonus::ETarget::OBJECT:
		{
			const CGHeroInstance *h = gs.getHero(pack.whoID.as<ObjectInstanceID>());
			if(h)
				callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroBonusChanged, h, pack.bonus, false);
		}
		break;
	case GiveBonus::ETarget::PLAYER:
		{
			//const PlayerState *p = gs.getPlayerState(pack.id);
			callInterfaceIfPresent(cl, pack.whoID.as<PlayerColor>(), &IGameEventsReceiver::playerBonusChanged, pack.bonus, false);
		}
		break;
	}
}

void ApplyFirstClientNetPackVisitor::visitRemoveObject(RemoveObject & pack)
{
	const CGObjectInstance *o = cl.getObj(pack.objectID);

	GAME->map().onObjectFadeOut(o, pack.initiator);

	//notify interfaces about removal
	for(auto i=cl.playerint.begin(); i!=cl.playerint.end(); i++)
	{
		//below line contains little cheat for AI so it will be aware of deletion of enemy heroes that moved or got re-covered by FoW
		//TODO: loose requirements as next AI related crashes appear, for example another pack.player collects object that got re-covered by FoW, unsure if AI code workarounds this
		if(gs.isVisible(o, i->first) || (!cl.getPlayerState(i->first)->human && o->ID == Obj::HERO && o->tempOwner != i->first))
			i->second->objectRemoved(o, pack.initiator);
	}

	GAME->map().waitForOngoingAnimations();
}

void ApplyClientNetPackVisitor::visitRemoveObject(RemoveObject & pack)
{
	callAllInterfaces(cl, &CGameInterface::invalidatePaths);

	for(auto i=cl.playerint.begin(); i!=cl.playerint.end(); i++)
		i->second->objectRemovedAfter();
}

void ApplyFirstClientNetPackVisitor::visitTryMoveHero(TryMoveHero & pack)
{
	CGHeroInstance *h = gs.getHero(pack.id);

	switch (pack.result)
	{
		case TryMoveHero::EMBARK:
			GAME->map().onBeforeHeroEmbark(h, pack.start, pack.end);
			break;
		case TryMoveHero::TELEPORTATION:
			GAME->map().onBeforeHeroTeleported(h, pack.start, pack.end);
			break;
		case TryMoveHero::DISEMBARK:
			GAME->map().onBeforeHeroDisembark(h, pack.start, pack.end);
			break;
	}
}

void ApplyClientNetPackVisitor::visitTryMoveHero(TryMoveHero & pack)
{
	const CGHeroInstance *h = cl.getHero(pack.id);
	callAllInterfaces(cl, &CGameInterface::invalidatePaths);

	switch(pack.result)
	{
		case TryMoveHero::SUCCESS:
			GAME->map().onHeroMoved(h, pack.start, pack.end);
			break;
		case TryMoveHero::EMBARK:
			GAME->map().onAfterHeroEmbark(h, pack.start, pack.end);
			break;
		case TryMoveHero::TELEPORTATION:
			GAME->map().onAfterHeroTeleported(h, pack.start, pack.end);
			break;
		case TryMoveHero::DISEMBARK:
			GAME->map().onAfterHeroDisembark(h, pack.start, pack.end);
			break;
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
		callInterfaceIfPresent(cl, town->getOwner(), &IGameEventsReceiver::buildChanged, town, id, 1);
	}

	// invalidate section of map view with our object and force an update
	GAME->map().onObjectInstantRemove(town, town->getOwner());
	GAME->map().onObjectInstantAdd(town, town->getOwner());
}

void ApplyClientNetPackVisitor::visitRazeStructures(RazeStructures & pack)
{
	CGTownInstance * town = gs.getTown(pack.tid);
	for(const auto & id : pack.bid)
	{
		callInterfaceIfPresent(cl, town->getOwner(), &IGameEventsReceiver::buildChanged, town, id, 2);
	}

	// invalidate section of map view with our object and force an update
	GAME->map().onObjectInstantRemove(town, town->getOwner());
	GAME->map().onObjectInstantAdd(town, town->getOwner());
}

void ApplyClientNetPackVisitor::visitSetAvailableCreatures(SetAvailableCreatures & pack)
{
	const CGDwelling * dw = static_cast<const CGDwelling*>(cl.getObj(pack.tid));

	PlayerColor p;
	if(dw->ID == Obj::WAR_MACHINE_FACTORY) //War Machines Factory is not flaggable, it's "owned" by visitor
		p = cl.getObjInstance(cl.getTile(dw->visitablePos())->visitableObjects.back())->getOwner();
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
		if(!i->first.isValidPlayer())
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
	auto * h = gs.getMap().getHero(pack.hid);
	if(h->getHeroTypeID() != pack.hid)
	{
		logNetwork->error("Something wrong with hero recruited!");
	}

	if(callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroCreated, h))
	{
		if(const CGTownInstance *t = gs.getTown(pack.tid))
			callInterfaceIfPresent(cl, h->getOwner(), &IGameEventsReceiver::heroInGarrisonChange, t);
	}
	GAME->map().onObjectInstantAdd(h, h->getOwner());
}

void ApplyClientNetPackVisitor::visitGiveHero(GiveHero & pack)
{
	CGHeroInstance *h = gs.getHero(pack.id);
	GAME->map().onObjectInstantAdd(h, h->getOwner());
	callInterfaceIfPresent(cl, h->tempOwner, &IGameEventsReceiver::heroCreated, h);
}

void ApplyFirstClientNetPackVisitor::visitGiveHero(GiveHero & pack)
{
}

void ApplyClientNetPackVisitor::visitInfoWindow(InfoWindow & pack)
{
	std::string str = pack.text.toString();

	if(!callInterfaceIfPresent(cl, pack.player, &CGameInterface::showInfoDialog, pack.type, str, pack.components,(soundBase::soundID)pack.soundID))
		logNetwork->warn("We received InfoWindow for not our player...");
}

void ApplyFirstClientNetPackVisitor::visitSetObjectProperty(SetObjectProperty & pack)
{
	//inform all players that see this object
	for(auto it = cl.playerint.cbegin(); it != cl.playerint.cend(); ++it)
	{
		if(gs.isVisible(gs.getObjInstance(pack.id), it->first))
			callInterfaceIfPresent(cl, it->first, &IGameEventsReceiver::beforeObjectPropertyChanged, &pack);
	}

	// invalidate section of map view with our object and force an update with new flag color
	if(pack.what == ObjProperty::OWNER)
	{
		auto object = gs.getObjInstance(pack.id);
		GAME->map().onObjectInstantRemove(object, object->getOwner());
	}
}

void ApplyClientNetPackVisitor::visitSetObjectProperty(SetObjectProperty & pack)
{
	//inform all players that see this object
	for(auto it = cl.playerint.cbegin(); it != cl.playerint.cend(); ++it)
	{
		if(gs.isVisible(gs.getObjInstance(pack.id), it->first))
			callInterfaceIfPresent(cl, it->first, &IGameEventsReceiver::objectPropertyChanged, &pack);
	}

	// invalidate section of map view with our object and force an update with new flag color
	if(pack.what == ObjProperty::OWNER)
	{
		auto object = gs.getObjInstance(pack.id);
		GAME->map().onObjectInstantAdd(object, object->getOwner());
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
	const auto & commander = hero->getCommander();
	assert(commander);
	assert(commander->getArmy()); //is it possible for Commander to exist beyond armed instance?
	callOnlyThatInterface(cl, pack.player, &CGameInterface::commanderGotLevel, commander, pack.skills, pack.queryID);
}

void ApplyClientNetPackVisitor::visitBlockingDialog(BlockingDialog & pack)
{
	std::string str = pack.text.toString();

	if(!callOnlyThatInterface(cl, pack.player, &CGameInterface::showBlockingDialog, str, pack.components, pack.queryID, (soundBase::soundID)pack.soundID, pack.selection(), pack.cancel(), pack.safeToAutoaccept()))
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
	const CGHeroInstance *h = cl.getHero(pack.hero);
	callOnlyThatInterface(cl, h->getOwner(), &CGameInterface::showTeleportDialog, h, pack.channel, pack.exits, pack.impassable, pack.queryID);
}

void ApplyClientNetPackVisitor::visitMapObjectSelectDialog(MapObjectSelectDialog & pack)
{
	callOnlyThatInterface(cl, pack.player, &CGameInterface::showMapObjectSelectDialog, pack.queryID, pack.icon, pack.title, pack.description, pack.objects);
}

void ApplyFirstClientNetPackVisitor::visitBattleStart(BattleStart & pack)
{
	// Cannot use the usual code because curB is not set yet
	callOnlyThatBattleInterface(cl, pack.info->getSide(BattleSide::ATTACKER).color, &IBattleEventsReceiver::battleStartBefore, pack.battleID, pack.info->getSideArmy(BattleSide::ATTACKER), pack.info->getSideArmy(BattleSide::DEFENDER),
		pack.info->tile, pack.info->getSideHero(BattleSide::ATTACKER), pack.info->getSideHero(BattleSide::DEFENDER));
	callOnlyThatBattleInterface(cl, pack.info->getSide(BattleSide::DEFENDER).color, &IBattleEventsReceiver::battleStartBefore, pack.battleID, pack.info->getSideArmy(BattleSide::ATTACKER), pack.info->getSideArmy(BattleSide::DEFENDER),
		pack.info->tile, pack.info->getSideHero(BattleSide::ATTACKER), pack.info->getSideHero(BattleSide::DEFENDER));
	callOnlyThatBattleInterface(cl, PlayerColor::SPECTATOR, &IBattleEventsReceiver::battleStartBefore, pack.battleID, pack.info->getSideArmy(BattleSide::ATTACKER), pack.info->getSideArmy(BattleSide::DEFENDER),
		pack.info->tile, pack.info->getSideHero(BattleSide::ATTACKER), pack.info->getSideHero(BattleSide::DEFENDER));
}

void ApplyClientNetPackVisitor::visitBattleStart(BattleStart & pack)
{
	cl.battleStarted(pack.battleID);
}

void ApplyFirstClientNetPackVisitor::visitBattleNextRound(BattleNextRound & pack)
{
	callBattleInterfaceIfPresentForBothSides(cl, pack.battleID, &IBattleEventsReceiver::battleNewRoundFirst, pack.battleID);
}

void ApplyClientNetPackVisitor::visitBattleNextRound(BattleNextRound & pack)
{
	callBattleInterfaceIfPresentForBothSides(cl, pack.battleID, &IBattleEventsReceiver::battleNewRound, pack.battleID);
}

void ApplyClientNetPackVisitor::visitBattleSetActiveStack(BattleSetActiveStack & pack)
{
	if(pack.reason == BattleUnitTurnReason::AUTOMATIC_ACTION)
		return;

	const CStack *activated = gs.getBattle(pack.battleID)->battleGetStackByID(pack.stack);
	PlayerColor playerToCall; //pack.player that will move activated stack
	if(activated->isHypnotized())
	{
		playerToCall = gs.getBattle(pack.battleID)->getSide(BattleSide::ATTACKER).color == activated->unitOwner()
			? gs.getBattle(pack.battleID)->getSide(BattleSide::DEFENDER).color
			: gs.getBattle(pack.battleID)->getSide(BattleSide::ATTACKER).color;
	}
	else
	{
		playerToCall = activated->unitOwner();
	}

	cl.startPlayerBattleAction(pack.battleID, playerToCall);
}

void ApplyClientNetPackVisitor::visitBattleLogMessage(BattleLogMessage & pack)
{
	callBattleInterfaceIfPresentForBothSides(cl, pack.battleID, &IBattleEventsReceiver::battleLogMessage, pack.battleID, pack.lines);
}

void ApplyClientNetPackVisitor::visitBattleTriggerEffect(BattleTriggerEffect & pack)
{
	callBattleInterfaceIfPresentForBothSides(cl, pack.battleID, &IBattleEventsReceiver::battleTriggerEffect, pack.battleID, pack);
}

void ApplyFirstClientNetPackVisitor::visitBattleUpdateGateState(BattleUpdateGateState & pack)
{
	callBattleInterfaceIfPresentForBothSides(cl, pack.battleID, &IBattleEventsReceiver::battleGateStateChanged, pack.battleID, pack.state);
}

void ApplyFirstClientNetPackVisitor::visitBattleResult(BattleResult & pack)
{
	callBattleInterfaceIfPresentForBothSides(cl, pack.battleID, &IBattleEventsReceiver::battleEnd, pack.battleID, &pack, pack.queryID);
	cl.battleFinished(pack.battleID);
}

void ApplyFirstClientNetPackVisitor::visitBattleStackMoved(BattleStackMoved & pack)
{
	const CStack * movedStack = gs.getBattle(pack.battleID)->battleGetStackByID(pack.stack);
	callBattleInterfaceIfPresentForBothSides(cl, pack.battleID, &IBattleEventsReceiver::battleStackMoved, pack.battleID, movedStack, pack.tilesToMove, pack.distance, pack.teleporting);
}

void ApplyFirstClientNetPackVisitor::visitBattleAttack(BattleAttack & pack)
{
	callBattleInterfaceIfPresentForBothSides(cl, pack.battleID, &IBattleEventsReceiver::battleAttack, pack.battleID, &pack);

	// battleStacksAttacked should be executed before BattleAttack.applyGs() to play animation before damaging unit
	// so this has to be here instead of ApplyClientNetPackVisitor::visitBattleAttack()
	callBattleInterfaceIfPresentForBothSides(cl, pack.battleID, &IBattleEventsReceiver::battleStacksAttacked, pack.battleID, pack.bsa, pack.shot());
}

void ApplyClientNetPackVisitor::visitBattleAttack(BattleAttack & pack)
{
}

void ApplyFirstClientNetPackVisitor::visitStartAction(StartAction & pack)
{
	cl.currentBattleAction = std::make_unique<BattleAction>(pack.ba);
	callBattleInterfaceIfPresentForBothSides(cl, pack.battleID, &IBattleEventsReceiver::actionStarted, pack.battleID, pack.ba);
}

void ApplyClientNetPackVisitor::visitBattleSpellCast(BattleSpellCast & pack)
{
	callBattleInterfaceIfPresentForBothSides(cl, pack.battleID, &IBattleEventsReceiver::battleSpellCast, pack.battleID, &pack);
}

void ApplyClientNetPackVisitor::visitSetStackEffect(SetStackEffect & pack)
{
	//informing about effects
	callBattleInterfaceIfPresentForBothSides(cl, pack.battleID, &IBattleEventsReceiver::battleStacksEffectsSet, pack.battleID, pack);
}

void ApplyClientNetPackVisitor::visitStacksInjured(StacksInjured & pack)
{
	callBattleInterfaceIfPresentForBothSides(cl, pack.battleID, &IBattleEventsReceiver::battleStacksAttacked, pack.battleID, pack.stacks, false);
}

void ApplyClientNetPackVisitor::visitBattleResultsApplied(BattleResultsApplied & pack)
{
	if(!pack.learnedSpells.spells.empty())
	{
		const auto hero = GAME->interface()->cb->getHero(pack.learnedSpells.hid);
		assert(hero);
		callInterfaceIfPresent(cl, pack.victor, &CGameInterface::showInfoDialog, EInfoWindowMode::MODAL,
			UIHelper::getEagleEyeInfoWindowText(*hero, pack.learnedSpells.spells), UIHelper::getSpellsComponents(pack.learnedSpells.spells), soundBase::soundID(0));
	}

	if(!pack.artifacts.empty())
	{
		const auto artSet = GAME->interface()->cb->getArtSet(ArtifactLocation(pack.artifacts.front().dstArtHolder));
		assert(artSet);
		std::vector<Component> artComponents;
		for(const auto & artPack : pack.artifacts)
		{
			auto packComponents = UIHelper::getArtifactsComponents(*artSet, artPack.artsPack0);
			artComponents.insert(artComponents.end(), std::make_move_iterator(packComponents.begin()), std::make_move_iterator(packComponents.end()));
		}
		callInterfaceIfPresent(cl, pack.victor, &CGameInterface::showInfoDialog, EInfoWindowMode::MODAL, UIHelper::getArtifactsInfoWindowText(),
			artComponents, soundBase::soundID(0));
	}

	for(auto & artPack : pack.artifacts)
		visitBulkMoveArtifacts(artPack);

	if(pack.raisedStack.getCreature())
		callInterfaceIfPresent(cl, pack.victor, &CGameInterface::showInfoDialog, EInfoWindowMode::AUTO,
			UIHelper::getNecromancyInfoWindowText(pack.raisedStack), std::vector<Component>{Component(ComponentType::CREATURE, pack.raisedStack.getId(),
			pack.raisedStack.getCount())}, UIHelper::getNecromancyInfoWindowSound());

	callInterfaceIfPresent(cl, pack.victor, &IGameEventsReceiver::battleResultsApplied);
	callInterfaceIfPresent(cl, pack.loser, &IGameEventsReceiver::battleResultsApplied);
	callInterfaceIfPresent(cl, PlayerColor::SPECTATOR, &IGameEventsReceiver::battleResultsApplied);
}

void ApplyClientNetPackVisitor::visitBattleUnitsChanged(BattleUnitsChanged & pack)
{
	callBattleInterfaceIfPresentForBothSides(cl, pack.battleID, &IBattleEventsReceiver::battleUnitsChanged, pack.battleID, pack.changedStacks);
}

void ApplyClientNetPackVisitor::visitBattleObstaclesChanged(BattleObstaclesChanged & pack)
{
	//inform interfaces about removed obstacles
	callBattleInterfaceIfPresentForBothSides(cl, pack.battleID, &IBattleEventsReceiver::battleObstaclesChanged, pack.battleID, pack.changes);
}

void ApplyClientNetPackVisitor::visitCatapultAttack(CatapultAttack & pack)
{
	//inform interfaces about catapult attack
	callBattleInterfaceIfPresentForBothSides(cl, pack.battleID, &IBattleEventsReceiver::battleCatapultAttacked, pack.battleID, pack);
}

void ApplyClientNetPackVisitor::visitEndAction(EndAction & pack)
{
	callBattleInterfaceIfPresentForBothSides(cl, pack.battleID, &IBattleEventsReceiver::actionFinished, pack.battleID, *cl.currentBattleAction);
	cl.currentBattleAction.reset();
}

void ApplyClientNetPackVisitor::visitPackageApplied(PackageApplied & pack)
{
	callInterfaceIfPresent(cl, pack.player, &IGameEventsReceiver::requestRealized, &pack);
	if(!cl.waitingRequest.tryRemovingElement(pack.requestID))
		logNetwork->warn("Surprising server message! PackageApplied for unknown requestID!");
}

void ApplyClientNetPackVisitor::visitSystemMessage(SystemMessage & pack)
{
	// usually used to receive error messages from server
	logNetwork->error("System message: %s", pack.text.toString());

	GAME->server().getGameChat().onNewSystemMessageReceived(pack.text.toString());
}

void ApplyClientNetPackVisitor::visitPlayerBlocked(PlayerBlocked & pack)
{
	callInterfaceIfPresent(cl, pack.player, &IGameEventsReceiver::playerBlocked, pack.reason, pack.startOrEnd == PlayerBlocked::BLOCKADE_STARTED);
}

void ApplyClientNetPackVisitor::visitPlayerStartsTurn(PlayerStartsTurn & pack)
{
	logNetwork->debug("Server gives turn to %s", pack.player.toString());

	callAllInterfaces(cl, &IGameEventsReceiver::playerStartsTurn, pack.player);
	callOnlyThatInterface(cl, pack.player, &CGameInterface::yourTurn, pack.queryID);
}

void ApplyClientNetPackVisitor::visitPlayerEndsTurn(PlayerEndsTurn & pack)
{
	logNetwork->debug("Server ends turn of %s", pack.player.toString());

	callAllInterfaces(cl, &IGameEventsReceiver::playerEndsTurn, pack.player);
}

void ApplyClientNetPackVisitor::visitTurnTimeUpdate(TurnTimeUpdate & pack)
{
	logNetwork->debug("Server sets turn timer {turn: %d, base: %d, battle: %d, creature: %d} for %s", pack.turnTimer.turnTimer, pack.turnTimer.baseTimer, pack.turnTimer.battleTimer, pack.turnTimer.unitTimer, pack.player.toString());
}

void ApplyClientNetPackVisitor::visitPlayerMessageClient(PlayerMessageClient & pack)
{
	logNetwork->debug("pack.player %s sends a message: %s", pack.player.toString(), pack.text);

	GAME->server().getGameChat().onNewGameMessageReceived(pack.player, pack.text);
}

void ApplyClientNetPackVisitor::visitAdvmapSpellCast(AdvmapSpellCast & pack)
{
	callAllInterfaces(cl, &CGameInterface::invalidatePaths);

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
	case EOpenWindowMode::RECRUITMENT_FIRST:
	case EOpenWindowMode::RECRUITMENT_ALL:
		{
			const CGDwelling *dw = dynamic_cast<const CGDwelling*>(cl.getObj(ObjectInstanceID(pack.object)));
			const CArmedInstance *dst = dynamic_cast<const CArmedInstance*>(cl.getObj(ObjectInstanceID(pack.visitor)));
			callInterfaceIfPresent(cl, dst->tempOwner, &IGameEventsReceiver::showRecruitmentDialog, dw, dst, pack.window == EOpenWindowMode::RECRUITMENT_FIRST ? 0 : -1, pack.queryID);
		}
		break;
	case EOpenWindowMode::SHIPYARD_WINDOW:
		{
			assert(pack.queryID == QueryID::NONE);
			const auto * sy = dynamic_cast<const IShipyard *>(cl.getObj(ObjectInstanceID(pack.object)));
			callInterfaceIfPresent(cl, sy->getObject()->getOwner(), &IGameEventsReceiver::showShipyardDialog, sy);
		}
		break;
	case EOpenWindowMode::THIEVES_GUILD:
		{
			assert(pack.queryID == QueryID::NONE);
			//displays Thieves' Guild window (when hero enters Den of Thieves)
			const CGObjectInstance *obj = cl.getObj(ObjectInstanceID(pack.object));
			const CGHeroInstance *hero = cl.getHero(ObjectInstanceID(pack.visitor));
			callInterfaceIfPresent(cl, hero->getOwner(), &IGameEventsReceiver::showThievesGuildWindow, obj);
		}
		break;
	case EOpenWindowMode::UNIVERSITY_WINDOW:
		{
			//displays University window (when hero enters University on adventure map)
			const auto * market = cl.getMarket(ObjectInstanceID(pack.object));
			const CGHeroInstance *hero = cl.getHero(ObjectInstanceID(pack.visitor));
			callInterfaceIfPresent(cl, hero->tempOwner, &IGameEventsReceiver::showUniversityWindow, market, hero, pack.queryID);
		}
		break;
	case EOpenWindowMode::MARKET_WINDOW:
		{
			//displays Thieves' Guild window (when hero enters Den of Thieves)
			const CGObjectInstance *obj = cl.getObj(ObjectInstanceID(pack.object));
			const CGHeroInstance *hero = cl.getHero(ObjectInstanceID(pack.visitor));
			const auto market = cl.getMarket(pack.object);
			const auto * tile = cl.getTile(obj->visitablePos());
			const auto * topObject = cl.getObjInstance(tile->visitableObjects.back());
			callInterfaceIfPresent(cl, topObject->getOwner(), &IGameEventsReceiver::showMarketWindow, market, hero, pack.queryID);
		}
		break;
	case EOpenWindowMode::HILL_FORT_WINDOW:
		{
			assert(pack.queryID == QueryID::NONE);
			//displays Hill fort window
			const CGObjectInstance *obj = cl.getObj(ObjectInstanceID(pack.object));
			const CGHeroInstance *hero = cl.getHero(ObjectInstanceID(pack.visitor));
			const auto * tile = cl.getTile(obj->visitablePos());
			const auto * topObject = cl.getObjInstance(tile->visitableObjects.back());
			callInterfaceIfPresent(cl, topObject->getOwner(), &IGameEventsReceiver::showHillFortWindow, obj, hero);
		}
		break;
	case EOpenWindowMode::PUZZLE_MAP:
		{
			assert(pack.queryID == QueryID::NONE);
			const CGHeroInstance *hero = cl.getHero(ObjectInstanceID(pack.visitor));
			callInterfaceIfPresent(cl, hero->getOwner(), &IGameEventsReceiver::showPuzzleMap);
		}
		break;
	case EOpenWindowMode::TAVERN_WINDOW:
		{
			const CGObjectInstance *obj1 = cl.getObj(ObjectInstanceID(pack.object));
			const CGHeroInstance * hero = cl.getHero(ObjectInstanceID(pack.visitor));
			callInterfaceIfPresent(cl, hero->tempOwner, &IGameEventsReceiver::showTavernWindow, obj1, hero, pack.queryID);
		}
		break;
	}
}

void ApplyClientNetPackVisitor::visitCenterView(CenterView & pack)
{
	callInterfaceIfPresent(cl, pack.player, &IGameEventsReceiver::centerView, pack.pos, pack.focusTime);
}

void ApplyClientNetPackVisitor::visitNewObject(NewObject & pack)
{
	callAllInterfaces(cl, &CGameInterface::invalidatePaths);

	const CGObjectInstance * obj = pack.newObject.get();
	GAME->map().onObjectFadeIn(obj, pack.initiator);

	for(auto i=cl.playerint.begin(); i!=cl.playerint.end(); i++)
	{
		if(gs.isVisible(obj, i->first))
			i->second->newObject(obj);
	}

	GAME->map().waitForOngoingAnimations();
}

void ApplyClientNetPackVisitor::visitSetAvailableArtifacts(SetAvailableArtifacts & pack)
{
	if(!pack.id.hasValue()) //artifact merchants globally
	{
		callAllInterfaces(cl, &IGameEventsReceiver::availableArtifactsChanged, nullptr);
	}
	else
	{
		const CGBlackMarket *bm = dynamic_cast<const CGBlackMarket *>(cl.getObj(ObjectInstanceID(pack.id)));
		assert(bm);
		const auto * tile = cl.getTile(bm->visitablePos());
		const auto * topObject = cl.getObjInstance(tile->visitableObjects.back());

		callInterfaceIfPresent(cl, topObject->getOwner(), &IGameEventsReceiver::availableArtifactsChanged, bm);
	}
}


void ApplyClientNetPackVisitor::visitEntitiesChanged(EntitiesChanged & pack)
{
	callAllInterfaces(cl, &CGameInterface::invalidatePaths);
}
