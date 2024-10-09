/*
 * HeroMovementController.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "HeroMovementController.h"

#include "CGameInfo.h"
#include "CPlayerInterface.h"
#include "PlayerLocalState.h"
#include "adventureMap/AdventureMapInterface.h"
#include "eventsSDL/InputHandler.h"
#include "gui/CGuiHandler.h"
#include "gui/CursorHandler.h"
#include "mapView/mapHandler.h"
#include "media/ISoundPlayer.h"

#include "../CCallback.h"

#include "ConditionalWait.h"
#include "../lib/CConfigHandler.h"
#include "../lib/CRandomGenerator.h"
#include "../lib/pathfinder/CGPathNode.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/networkPacks/PacksForClient.h"
#include "../lib/RoadHandler.h"
#include "../lib/TerrainHandler.h"

bool HeroMovementController::isHeroMovingThroughGarrison(const CGHeroInstance * hero, const CArmedInstance * garrison) const
{
	if(!duringMovement)
		return false;

	if(!LOCPLINT->localState->hasPath(hero))
		return false;

	if(garrison->visitableAt(LOCPLINT->localState->getPath(hero).lastNode().coord))
		return false; // hero want to enter garrison, not pass through it

	return true;
}

bool HeroMovementController::isHeroMoving() const
{
	return duringMovement;
}

void HeroMovementController::onPlayerTurnStarted()
{
	assert(duringMovement == false);
	assert(stoppingMovement == false);
	duringMovement = false;
	currentlyMovingHero = nullptr;
}

void HeroMovementController::onBattleStarted()
{
	// when battle starts, game will send battleStart pack *before* movement confirmation
	// and since network thread wait for battle intro to play, movement confirmation will only happen after intro
	// leading to several bugs, such as blocked input during intro
	requestMovementAbort();
}

void HeroMovementController::showTeleportDialog(const CGHeroInstance * hero, TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QueryID askID)
{
	if (impassable || exits.empty()) //FIXME: why we even have this dialog in such case?
	{
		LOCPLINT->cb->selectionMade(-1, askID);
		return;
	}

	// Player entered teleporter
	// Check whether hero that has entered teleporter has paths that goes through teleporter and select appropriate exit
	// othervice, ask server to select one randomly by sending invalid (-1) value as answer
	assert(waitingForQueryApplyReply == false);
	waitingForQueryApplyReply = true;

	if(!LOCPLINT->localState->hasPath(hero))
	{
		// Hero enters teleporter without specifying exit - select it randomly
		LOCPLINT->cb->selectionMade(-1, askID);
		return;
	}

	const auto & heroPath = LOCPLINT->localState->getPath(hero);
	const auto & nextNode = heroPath.nextNode();

	for(size_t i = 0; i < exits.size(); ++i)
	{
		if(exits[i].second == nextNode.coord)
		{
			// Remove this node from path - it will be covered by teleportation
			//LOCPLINT->localState->removeLastNode(hero);
			LOCPLINT->cb->selectionMade(i, askID);
			return;
		}
	}

	// may happen when hero has path but does not moves alongside it
	// for example, while standing on teleporter set path that does not leads throught teleporter and press space
	LOCPLINT->cb->selectionMade(-1, askID);
	return;
}

void HeroMovementController::updatePath(const CGHeroInstance * hero, const TryMoveHero & details)
{
	// Once hero moved (or attempted to move) we need to update path
	// to make sure that it is still valid or remove it completely if destination has been reached

	if(hero->tempOwner != LOCPLINT->playerID)
		return;

	if(!LOCPLINT->localState->hasPath(hero))
		return; // may happen when hero teleports

	assert(LOCPLINT->makingTurn);

	bool directlyAttackingCreature = details.attackedFrom.has_value() && LOCPLINT->localState->getPath(hero).lastNode().coord == details.attackedFrom;

	int3 desiredTarget = LOCPLINT->localState->getPath(hero).nextNode().coord;
	int3 actualTarget = hero->convertToVisitablePos(details.end);

	//don't erase path when revisiting with spacebar
	bool heroChangedTile = details.start != details.end;

	if(heroChangedTile)
	{
		if(desiredTarget != actualTarget)
		{
			//invalidate path - movement was not along current path
			//possible reasons: teleport, visit of object with "blocking visit" property
			LOCPLINT->localState->erasePath(hero);
		}
		else
		{
			//movement along desired path - remove one node and keep rest of path
			LOCPLINT->localState->removeLastNode(hero);
		}

		if(directlyAttackingCreature)
			LOCPLINT->localState->erasePath(hero);
	}
}

void HeroMovementController::onTryMoveHero(const CGHeroInstance * hero, const TryMoveHero & details)
{
	// Server initiated movement -> start movement animation
	// Note that this movement is not necessarily of owned heroes - other players movement will also pass through this method

	if(details.result == TryMoveHero::EMBARK || details.result == TryMoveHero::DISEMBARK)
	{
		if (hero->tempOwner == LOCPLINT->playerID)
		{
			auto removalSound = hero->getRemovalSound(CRandomGenerator::getDefault());
			if (removalSound)
				CCS->soundh->playSound(removalSound.value());
		}
	}

	std::unordered_set<int3> changedTiles {
		hero->convertToVisitablePos(details.start),
		hero->convertToVisitablePos(details.end)
	};
	adventureInt->onMapTilesChanged(changedTiles);
	adventureInt->onHeroMovementStarted(hero);

	updatePath(hero, details);

	if(details.stopMovement())
	{
		if(duringMovement)
			endMove(hero);
		return;
	}

	// We are in network thread
	// Block netpack processing until movement animation is over
	CGI->mh->waitForOngoingAnimations();

	//move finished
	adventureInt->onHeroChanged(hero);
}

void HeroMovementController::onQueryReplyApplied()
{
	if (!waitingForQueryApplyReply)
		return;

	waitingForQueryApplyReply = false;

	// Server accepted our TeleportDialog query reply and moved hero
	// Continue moving alongside our path, if any
	if(duringMovement)
		onMoveHeroApplied();
}

void HeroMovementController::onMoveHeroApplied()
{
	// at this point, server have finished processing of hero movement request
	// as well as all side effectes from movement, such as object visit or combat start

	// this was request to move alongside path from player, but either another player or teleport action
	if(!duringMovement)
		return;

	// hero has moved onto teleporter and activated it
	// in this case next movement should be done only after query reply has been acknowledged
	// and hero has been moved to teleport destination
	if(waitingForQueryApplyReply)
		return;

	if(GH.input().ignoreEventsUntilInput())
		stoppingMovement = true;

	assert(currentlyMovingHero);
	const auto * hero = currentlyMovingHero;

	bool canMove = LOCPLINT->localState->hasPath(hero) && LOCPLINT->localState->getPath(hero).nextNode().turns == 0 && !LOCPLINT->showingDialog->isBusy();
	bool wantStop = stoppingMovement;
	bool canStop = !canMove || canHeroStopAtNode(LOCPLINT->localState->getPath(hero).currNode());

	if(!canMove || (wantStop && canStop))
	{
		endMove(hero);
	}
	else
	{
		sendMovementRequest(hero, LOCPLINT->localState->getPath(hero));
	}
}

void HeroMovementController::requestMovementAbort()
{
	if(duringMovement)
		endMove(currentlyMovingHero);
}

void HeroMovementController::endMove(const CGHeroInstance * hero)
{
	assert(duringMovement == true);
	assert(currentlyMovingHero != nullptr);
	duringMovement = false;
	stoppingMovement = false;
	currentlyMovingHero = nullptr;
	stopMovementSound();
	adventureInt->onHeroChanged(hero);
	CCS->curh->show();
}

AudioPath HeroMovementController::getMovementSoundFor(const CGHeroInstance * hero, int3 posPrev, int3 posNext, EPathNodeAction moveType)
{
	if(moveType == EPathNodeAction::TELEPORT_BATTLE || moveType == EPathNodeAction::TELEPORT_BLOCKING_VISIT || moveType == EPathNodeAction::TELEPORT_NORMAL)
		return {};

	if(moveType == EPathNodeAction::EMBARK || moveType == EPathNodeAction::DISEMBARK)
		return {};

	if(moveType == EPathNodeAction::BLOCKING_VISIT)
		return {};

	// flying movement sound
	if(hero->hasBonusOfType(BonusType::FLYING_MOVEMENT))
		return AudioPath::builtin("HORSE10.wav");

	auto prevTile = LOCPLINT->cb->getTile(posPrev);
	auto nextTile = LOCPLINT->cb->getTile(posNext);

	auto prevRoad = prevTile->roadType;
	auto nextRoad = nextTile->roadType;
	bool movingOnRoad = prevRoad->getId() != Road::NO_ROAD && nextRoad->getId() != Road::NO_ROAD;

	if(movingOnRoad)
		return nextTile->terType->horseSound;
	else
		return nextTile->terType->horseSoundPenalty;
};

void HeroMovementController::updateMovementSound(const CGHeroInstance * h, int3 posPrev, int3 nextCoord, EPathNodeAction moveType)
{
	// Start a new sound for the hero movement or let the existing one carry on.
	AudioPath newSoundName = getMovementSoundFor(h, posPrev, nextCoord, moveType);

	if(newSoundName != currentMovementSoundName)
	{
		currentMovementSoundName = newSoundName;

		if(currentMovementSoundChannel != -1)
			CCS->soundh->stopSound(currentMovementSoundChannel);

		if(!currentMovementSoundName.empty())
			currentMovementSoundChannel = CCS->soundh->playSound(currentMovementSoundName, -1, true);
		else
			currentMovementSoundChannel = -1;
	}
}

void HeroMovementController::stopMovementSound()
{
	if(currentMovementSoundChannel != -1)
		CCS->soundh->stopSound(currentMovementSoundChannel);
	currentMovementSoundChannel = -1;
	currentMovementSoundName = AudioPath();
}

bool HeroMovementController::canHeroStopAtNode(const CGPathNode & node) const
{
	if(node.layer != EPathfindingLayer::LAND && node.layer != EPathfindingLayer::SAIL)
		return false;

	if(node.accessible != EPathAccessibility::ACCESSIBLE)
		return false;

	return true;
}

void HeroMovementController::requestMovementStart(const CGHeroInstance * h, const CGPath & path)
{
	assert(duringMovement == false);

	duringMovement = true;
	currentlyMovingHero = h;

	CCS->curh->hide();
	sendMovementRequest(h, path);
}

void HeroMovementController::sendMovementRequest(const CGHeroInstance * h, const CGPath & path)
{
	assert(duringMovement == true);

	int heroMovementSpeed = settings["adventure"]["heroMoveTime"].Integer();
	bool useMovementBatching = heroMovementSpeed == 0;

	const auto & currNode = path.currNode();
	const auto & nextNode = path.nextNode();

	assert(nextNode.turns == 0);
	assert(currNode.coord == h->visitablePos());

	if(nextNode.isTeleportAction())
	{
		stopMovementSound();
		logGlobal->trace("Requesting hero teleportation to %s", nextNode.coord.toString());
		LOCPLINT->cb->moveHero(h, h->pos, false);
		return;
	}

	if (!useMovementBatching)
	{
		updateMovementSound(h, currNode.coord, nextNode.coord, nextNode.action);

		assert(h->pos.z == nextNode.coord.z); // Z should change only if it's movement via teleporter and in this case this code shouldn't be executed at all

		logGlobal->trace("Requesting hero movement to %s", nextNode.coord.toString());

		bool useTransit = nextNode.layer == EPathfindingLayer::AIR || nextNode.layer == EPathfindingLayer::WATER;
		int3 nextCoord = h->convertFromVisitablePos(nextNode.coord);

		LOCPLINT->cb->moveHero(h, nextCoord, useTransit);

		if (nextNode.action == EPathNodeAction::NORMAL || nextNode.action == EPathNodeAction::VISIT)
			CGI->mh->onHeroMoved(h, h->pos, h->convertFromVisitablePos(nextNode.coord));
		return;
	}

	bool useTransitAtStart = path.nextNode().layer == EPathfindingLayer::AIR || path.nextNode().layer == EPathfindingLayer::WATER;
	std::vector<int3> pathToMove;

	for (auto const & node : boost::adaptors::reverse(path.nodes))
	{
		if (node.coord == h->visitablePos())
			continue; // first node, ignore - this is hero current position

		if(node.isTeleportAction())
			break; // pause after monolith / subterra gates

		if (node.turns != 0)
			break; // ran out of move points

		bool useTransitHere = node.layer == EPathfindingLayer::AIR || node.layer == EPathfindingLayer::WATER;
		if (useTransitHere != useTransitAtStart)
			break;

		int3 coord = h->convertFromVisitablePos(node.coord);
		pathToMove.push_back(coord);

		if (LOCPLINT->cb->guardingCreaturePosition(node.coord) != int3(-1, -1, -1))
			break; // we reached zone-of-control of wandering monster

		if (!LOCPLINT->cb->getVisitableObjs(node.coord).empty())
			break; // we reached event, garrison or some other visitable object - end this movement batch
	}

	assert(!pathToMove.empty());
	if (!pathToMove.empty())
	{
		updateMovementSound(h, currNode.coord, nextNode.coord, nextNode.action);
		LOCPLINT->cb->moveHero(h, pathToMove, useTransitAtStart);
	}
}
