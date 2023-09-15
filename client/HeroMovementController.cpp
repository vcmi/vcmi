/*
 * CPlayerInterface.cpp, part of VCMI engine
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
#include "CMusicHandler.h"
#include "CPlayerInterface.h"
#include "PlayerLocalState.h"
#include "adventureMap/AdventureMapInterface.h"
#include "eventsSDL/InputHandler.h"
#include "gui/CGuiHandler.h"
#include "gui/CursorHandler.h"
#include "mapView/mapHandler.h"

#include "../CCallback.h"

#include "../lib/pathfinder/CGPathNode.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/mapObjects/MiscObjects.h"
#include "../lib/RoadHandler.h"
#include "../lib/TerrainHandler.h"
#include "../lib/NetPacks.h"
#include "../lib/CondSh.h"

std::optional<int3> HeroMovementController::getLastTile(const CGHeroInstance * hero) const
{
	if (!LOCPLINT->localState->hasPath(hero))
		return std::nullopt;

	return LOCPLINT->localState->getPath(hero).endPos();
}

std::optional<int3> HeroMovementController::getNextTile(const CGHeroInstance * hero) const
{
	if (!LOCPLINT->localState->hasPath(hero))
		return std::nullopt;

	return LOCPLINT->localState->getPath(hero).nextNode().coord;
}

bool HeroMovementController::isHeroMovingThroughGarrison(const CGHeroInstance * hero, const CArmedInstance * garrison) const
{
	assert(LOCPLINT->localState->hasPath(hero));
	assert(duringMovement);

	if (!LOCPLINT->localState->hasPath(hero))
		return false;

	if (garrison->visitableAt(*getLastTile(hero)))
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
}

void HeroMovementController::onBattleStarted()
{
	// when battle starts, game will send battleStart pack *before* movement confirmation
	// and since network thread wait for battle intro to play, movement confirmation will only happen after intro
	// leading to several bugs, such as blocked input during intro
	// FIXME: should be on hero visit
	assert(duringMovement == false);
	assert(stoppingMovement == false);
	duringMovement = false;
}

void HeroMovementController::showTeleportDialog(const CGHeroInstance * hero, TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QueryID askID)
{
	if (!LOCPLINT->localState->hasPath(hero))
	{
		assert(exits.size() == 1); // select random? Let server select?
		LOCPLINT->cb->selectionMade(0, askID);
		return;
	}

	const auto & heroPath = LOCPLINT->localState->getPath(hero);
	const auto & nextNode = heroPath.nextNode();

	for (size_t i = 0; i < exits.size(); ++i)
	{
		const auto * teleporter = LOCPLINT->cb->getObj(exits[i].first);

		if (teleporter && teleporter->visitableAt(nextNode.coord))
		{
			LOCPLINT->cb->selectionMade(i, askID);
			return;
		}
	}

	assert(0); // exit not found? How? select random? Let server select?
	LOCPLINT->cb->selectionMade(0, askID);
	return;
}

void HeroMovementController::updatePath(const CGHeroInstance * hero, const TryMoveHero & details)
{
	if (hero->tempOwner != LOCPLINT->playerID)
		return;

	if (!LOCPLINT->localState->hasPath(hero))
		return; // may happen when hero teleports

	assert(LOCPLINT->makingTurn);
	assert(getNextTile(hero).has_value());

	bool directlyAttackingCreature = details.attackedFrom.has_value() && getLastTile(hero) == details.attackedFrom;

	auto desiredTarget = getNextTile(hero);
	auto actualTarget = hero->convertToVisitablePos(details.end);

	//don't erase path when revisiting with spacebar
	bool heroChangedTile = details.start != details.end;

	if (desiredTarget && heroChangedTile)
	{
		if (*desiredTarget != actualTarget)
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

void HeroMovementController::heroMoved(const CGHeroInstance * hero, const TryMoveHero & details)
{
	//assert(duringMovement == true); // May be false if hero teleported

	if (details.result == TryMoveHero::EMBARK || details.result == TryMoveHero::DISEMBARK)
	{
		if(hero->getRemovalSound() && hero->tempOwner == LOCPLINT->playerID)
			CCS->soundh->playSound(hero->getRemovalSound().value());
	}

	std::unordered_set<int3> changedTiles {
		hero->convertToVisitablePos(details.start),
		hero->convertToVisitablePos(details.end)
	};
	adventureInt->onMapTilesChanged(changedTiles);
	adventureInt->onHeroMovementStarted(hero);

	updatePath(hero, details);

	if(details.stopMovement()) //hero failed to move
	{
		if (duringMovement)
			endHeroMove(hero);
		return;
	}

	CGI->mh->waitForOngoingAnimations();

	//move finished
	adventureInt->onHeroChanged(hero);

//	// Hero attacked creature directly, set direction to face it.
//	if (directlyAttackingCreature)
//	{
//		// Get direction to attacker.
//		int3 posOffset = *details.attackedFrom - details.end + int3(2, 1, 0);
//		static const ui8 dirLookup[3][3] =
//		{
//			{ 1, 2, 3 },
//			{ 8, 0, 4 },
//			{ 7, 6, 5 }
//		};
//		// FIXME: Avoid const_cast, make moveDir mutable in some other way?
//		const_cast<CGHeroInstance *>(hero)->moveDir = dirLookup[posOffset.y][posOffset.x];
//	}
}

void HeroMovementController::onMoveHeroApplied()
{
	auto const * hero = LOCPLINT->localState->getCurrentHero();

	assert(hero);

	//check if user cancelled movement
	if (GH.input().ignoreEventsUntilInput())
		stoppingMovement = true;

	if (duringMovement)
	{
		bool canMove = LOCPLINT->localState->hasPath(hero) && LOCPLINT->localState->getPath(hero).nextNode().turns == 0;
		bool wantStop = stoppingMovement;
		bool canStop = !canMove || canHeroStopAtNode(LOCPLINT->localState->getPath(hero).currNode());

		if (!canMove)
		{
			endHeroMove(hero);
		}
		else if ( wantStop && canStop )
		{
			endHeroMove(hero);
		}
		else
		{
			moveHeroOnce(hero, LOCPLINT->localState->getPath(hero));
		}
	}
}

void HeroMovementController::movementAbortRequested()
{
	if(duringMovement)
		endHeroMove(LOCPLINT->localState->getCurrentHero());
}

void HeroMovementController::endHeroMove(const CGHeroInstance * hero)
{
	assert(duringMovement == true);
	duringMovement = false;
	stoppingMovement = false;
	stopMovementSound();
	adventureInt->onHeroChanged(hero);
	CCS->curh->show();
}

AudioPath HeroMovementController::getMovementSoundFor(const CGHeroInstance * hero, int3 posPrev, int3 posNext, EPathNodeAction moveType)
{
	if (moveType == EPathNodeAction::TELEPORT_BATTLE || moveType == EPathNodeAction::TELEPORT_BLOCKING_VISIT || moveType == EPathNodeAction::TELEPORT_NORMAL)
		return {};

	if (moveType == EPathNodeAction::EMBARK || moveType == EPathNodeAction::DISEMBARK)
		return {};

	if (moveType == EPathNodeAction::BLOCKING_VISIT)
		return {};

	// flying movement sound
	if (hero->hasBonusOfType(BonusType::FLYING_MOVEMENT))
		return AudioPath::builtin("HORSE10.wav");

	auto prevTile = LOCPLINT->cb->getTile(posPrev);
	auto nextTile = LOCPLINT->cb->getTile(posNext);

	auto prevRoad = prevTile->roadType;
	auto nextRoad = nextTile->roadType;
	bool movingOnRoad = prevRoad->getId() != Road::NO_ROAD && nextRoad->getId() != Road::NO_ROAD;

	if (movingOnRoad)
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

		if (currentMovementSoundChannel != -1)
			CCS->soundh->stopSound(currentMovementSoundChannel);

		if (!currentMovementSoundName.empty())
			currentMovementSoundChannel = CCS->soundh->playSound(currentMovementSoundName, -1, true);
		else
			currentMovementSoundChannel = -1;
	}
}

void HeroMovementController::stopMovementSound()
{
	CCS->soundh->stopSound(currentMovementSoundChannel);
	currentMovementSoundChannel = -1;
	currentMovementSoundName = AudioPath();
}

bool HeroMovementController::canHeroStopAtNode(const CGPathNode & node) const
{
	if (node.layer != EPathfindingLayer::LAND && node.layer != EPathfindingLayer::SAIL)
		return false;

	if (node.accessible != EPathAccessibility::ACCESSIBLE)
		return false;

	return true;
}

void HeroMovementController::movementStartRequested(const CGHeroInstance * h, const CGPath & path)
{
	assert(duringMovement == false);
	duringMovement = true;

	CCS->curh->show();
	moveHeroOnce(h, path);
}

void HeroMovementController::moveHeroOnce(const CGHeroInstance * h, const CGPath & path)
{
	assert(duringMovement == true);

	const auto & currNode = path.currNode();
	const auto & nextNode = path.nextNode();

	assert(nextNode.turns == 0);
	assert(currNode.coord == h->visitablePos());

	int3 nextCoord = h->convertFromVisitablePos(nextNode.coord);

	if (nextNode.isTeleportAction())
	{
		stopMovementSound();
		LOCPLINT->cb->moveHero(h, nextCoord, false);
		return;
	}
	else
	{
		updateMovementSound(h, currNode.coord, nextNode.coord, nextNode.action);

		assert(h->pos.z == nextNode.coord.z); // Z should change only if it's movement via teleporter and in this case this code shouldn't be executed at all

		logGlobal->trace("Requesting hero movement to %s", nextNode.coord.toString());

		bool useTransit = nextNode.layer == EPathfindingLayer::AIR;
		LOCPLINT->cb->moveHero(h, nextCoord, useTransit);
		return;
	}

//	bool guarded = LOCPLINT->cb->isInTheMap(LOCPLINT->cb->getGuardingCreaturePosition(nextCoord - int3(1, 0, 0)));
//	if ((!useTransit && guarded) || LOCPLINT->showingDialog->get() == true) // Abort movement if a guard was fought or there is a dialog to display (Mantis #1136)
//		break;

//	stopMovementSound();
//
//	//Update cursor so icon can change if needed when it reappears; doesn;'t apply if a dialog box pops up at the end of the movement
//	if (!LOCPLINT->showingDialog->get())
//		GH.fakeMouseMove();
//
//	CGI->mh->waitForOngoingAnimations();
//	setMovementStatus(false);
}
