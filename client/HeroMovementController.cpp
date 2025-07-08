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

#include "CPlayerInterface.h"
#include "PlayerLocalState.h"
#include "adventureMap/AdventureMapInterface.h"
#include "eventsSDL/InputHandler.h"
#include "GameEngine.h"
#include "GameInstance.h"
#include "gui/CursorHandler.h"
#include "mapView/mapHandler.h"
#include "media/ISoundPlayer.h"

#include "../lib/ConditionalWait.h"
#include "../lib/CConfigHandler.h"
#include "../lib/CRandomGenerator.h"
#include "../lib/callback/CCallback.h"
#include "../lib/pathfinder/CGPathNode.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/mapping/TerrainTile.h"
#include "../lib/networkPacks/PacksForClient.h"
#include "../lib/RoadHandler.h"
#include "../lib/TerrainHandler.h"

bool HeroMovementController::isHeroMovingThroughGarrison(const CGHeroInstance * hero, const CArmedInstance * garrison) const
{
	if(!duringMovement)
		return false;

	if(!GAME->interface()->localState->hasPath(hero))
		return false;

	if(garrison->visitableAt(GAME->interface()->localState->getPath(hero).lastNode().coord))
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
		GAME->interface()->cb->selectionMade(-1, askID);
		return;
	}

	// Player entered teleporter
	// Check whether hero that has entered teleporter has paths that goes through teleporter and select appropriate exit
	// othervice, ask server to select one randomly by sending invalid (-1) value as answer
	assert(waitingForQueryApplyReply == false);
	waitingForQueryApplyReply = true;

	if(!GAME->interface()->localState->hasPath(hero))
	{
		// Hero enters teleporter without specifying exit - select it randomly
		GAME->interface()->cb->selectionMade(-1, askID);
		return;
	}

	const auto & heroPath = GAME->interface()->localState->getPath(hero);
	const auto & nextNode = heroPath.nextNode();

	for(size_t i = 0; i < exits.size(); ++i)
	{
		if(exits[i].second == nextNode.coord)
		{
			// Remove this node from path - it will be covered by teleportation
			//GAME->interface()->localState->removeLastNode(hero);
			GAME->interface()->cb->selectionMade(i, askID);
			return;
		}
	}

	// may happen when hero has path but does not moves alongside it
	// for example, while standing on teleporter set path that does not leads throught teleporter and press space
	GAME->interface()->cb->selectionMade(-1, askID);
	return;
}

void HeroMovementController::updatePath(const CGHeroInstance * hero, const TryMoveHero & details)
{
	// Once hero moved (or attempted to move) we need to update path
	// to make sure that it is still valid or remove it completely if destination has been reached

	if(hero->tempOwner != GAME->interface()->playerID)
		return;

	if(!GAME->interface()->localState->hasPath(hero))
		return; // may happen when hero teleports

	assert(GAME->interface()->makingTurn);

	bool directlyAttackingCreature = details.attackedFrom.isValid() && GAME->interface()->localState->getPath(hero).lastNode().coord == details.attackedFrom;

	int3 desiredTarget = GAME->interface()->localState->getPath(hero).nextNode().coord;
	int3 actualTarget = hero->convertToVisitablePos(details.end);

	//don't erase path when revisiting with spacebar
	bool heroChangedTile = details.start != details.end;

	if(heroChangedTile)
	{
		if(desiredTarget != actualTarget)
		{
			//invalidate path - movement was not along current path
			//possible reasons: teleport, visit of object with "blocking visit" property
			GAME->interface()->localState->erasePath(hero);
		}
		else
		{
			//movement along desired path - remove one node and keep rest of path
			GAME->interface()->localState->removeLastNode(hero);
		}

		if(directlyAttackingCreature)
			GAME->interface()->localState->erasePath(hero);
	}
}

void HeroMovementController::onTryMoveHero(const CGHeroInstance * hero, const TryMoveHero & details)
{
	// Server initiated movement -> start movement animation
	// Note that this movement is not necessarily of owned heroes - other players movement will also pass through this method

	if(details.result == TryMoveHero::EMBARK || details.result == TryMoveHero::DISEMBARK)
	{
		if (hero->tempOwner == GAME->interface()->playerID)
		{
			auto removalSound = hero->getRemovalSound(CRandomGenerator::getDefault());
			if (removalSound)
				ENGINE->sound().playSound(removalSound.value());
		}
	}

	bool directlyAttackingCreature =
		details.attackedFrom.isValid() &&
		GAME->interface()->localState->hasPath(hero) &&
		GAME->interface()->localState->getPath(hero).lastNode().coord == details.attackedFrom;

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
	GAME->map().waitForOngoingAnimations();

	//move finished
	adventureInt->onHeroChanged(hero);

	// Hero attacked creature, set direction to face it.
	if(directlyAttackingCreature)
	{
		// Get direction to attacker.
		int3 posOffset = details.attackedFrom - details.end + int3(2, 1, 0);
		static const ui8 dirLookup[3][3] =
		{
			{ 1, 2, 3 },
			{ 8, 0, 4 },
			{ 7, 6, 5 }
		};

		//FIXME: better handling of this case without const_cast
		const_cast<CGHeroInstance *>(hero)->moveDir = dirLookup[posOffset.y][posOffset.x];
	}
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

	if(ENGINE->input().ignoreEventsUntilInput())
		stoppingMovement = true;

	assert(currentlyMovingHero);
	const auto * hero = currentlyMovingHero;

	bool canMove = GAME->interface()->localState->hasPath(hero) && GAME->interface()->localState->getPath(hero).nextNode().turns == 0 && !GAME->interface()->showingDialog->isBusy();
	bool wantStop = stoppingMovement;
	bool canStop = !canMove || canHeroStopAtNode(GAME->interface()->localState->getPath(hero).currNode());

	if(!canMove || (wantStop && canStop))
	{
		endMove(hero);
	}
	else
	{
		sendMovementRequest(hero, GAME->interface()->localState->getPath(hero));
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
	ENGINE->cursor().show();
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

	auto prevTile = GAME->interface()->cb->getTile(posPrev);
	auto nextTile = GAME->interface()->cb->getTile(posNext);

	bool movingOnRoad = prevTile->hasRoad() && nextTile->hasRoad();

	if(movingOnRoad)
		return nextTile->getTerrain()->horseSound;
	else
		return nextTile->getTerrain()->horseSoundPenalty;
};

void HeroMovementController::updateMovementSound(const CGHeroInstance * h, int3 posPrev, int3 nextCoord, EPathNodeAction moveType)
{
	// Start a new sound for the hero movement or let the existing one carry on.
	AudioPath newSoundName = getMovementSoundFor(h, posPrev, nextCoord, moveType);

	if(newSoundName != currentMovementSoundName)
	{
		currentMovementSoundName = newSoundName;

		if(currentMovementSoundChannel != -1)
			ENGINE->sound().stopSound(currentMovementSoundChannel);

		if(!currentMovementSoundName.empty())
			currentMovementSoundChannel = ENGINE->sound().playSoundLooped(currentMovementSoundName);
		else
			currentMovementSoundChannel = -1;
	}
}

void HeroMovementController::stopMovementSound()
{
	if(currentMovementSoundChannel != -1)
		ENGINE->sound().stopSound(currentMovementSoundChannel);
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

	ENGINE->cursor().hide();
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
		GAME->interface()->cb->moveHero(h, h->pos, false);
		return;
	}

	if (!useMovementBatching)
	{
		updateMovementSound(h, currNode.coord, nextNode.coord, nextNode.action);

		assert(h->anchorPos().z == nextNode.coord.z); // Z should change only if it's movement via teleporter and in this case this code shouldn't be executed at all

		logGlobal->trace("Requesting hero movement to %s", nextNode.coord.toString());

		bool useTransit = nextNode.layer == EPathfindingLayer::AIR || nextNode.layer == EPathfindingLayer::WATER;
		int3 nextCoord = h->convertFromVisitablePos(nextNode.coord);

		GAME->interface()->cb->moveHero(h, nextCoord, useTransit);
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

		if (GAME->interface()->cb->guardingCreaturePosition(node.coord) != int3(-1, -1, -1))
			break; // we reached zone-of-control of wandering monster

		if (!GAME->interface()->cb->getVisitableObjs(node.coord).empty())
			break; // we reached event, garrison or some other visitable object - end this movement batch
	}

	assert(!pathToMove.empty());
	if (!pathToMove.empty())
	{
		updateMovementSound(h, currNode.coord, nextNode.coord, nextNode.action);
		GAME->interface()->cb->moveHero(h, pathToMove, useTransitAtStart);
	}
}
