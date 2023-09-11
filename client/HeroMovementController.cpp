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

// Code flow of movement process:
// - Player requests movement (e.g. press "move" button)
// -> calls doMoveHero (GUI thread)
// -> -> sends LOCPLINT->cb->moveHero
// - Server sends reply
// -> calls heroMoved (NETWORK thread)
// -> -> animation starts, sound starts
// -> -> waits for animation to finish
// -> -> if player have requested stop and path is not finished, calls doMoveHero again
//
// - Players requests movement stop (e.g. mouse click during movement)
// -> calls movementStopRequested
// -> -> sets flag that movement should be aborted

HeroMovementController::HeroMovementController()
{
	destinationTeleport = ObjectInstanceID();
	destinationTeleportPos = int3(-1);
	duringMovement = false;
}

void HeroMovementController::setMovementStatus(bool value)
{
	duringMovement = value;
	if (value)
		CCS->curh->hide();
	else
		CCS->curh->show();
}

bool HeroMovementController::isHeroMovingThroughGarrison(const CGHeroInstance * hero, const CArmedInstance * garrison) const
{
	assert(LOCPLINT->localState->hasPath(hero));
	assert(movementState == EMoveState::DURING_MOVE);

	if (!LOCPLINT->localState->hasPath(hero))
		return false;

	if (garrison->visitableAt(*LOCPLINT->localState->getLastTile(hero)))
		return false; // hero want to enter garrison, not pass through it

	return true;
}

bool HeroMovementController::isHeroMoving() const
{
	return duringMovement;
}

void HeroMovementController::onMoveHeroApplied()
{
	assert(movementState == EMoveState::DURING_MOVE);

	if(movementState == EMoveState::DURING_MOVE)
	{
		assert(destinationTeleport == ObjectInstanceID::NONE);
		movementState = EMoveState::CONTINUE_MOVE;
	}
}

void HeroMovementController::onQueryReplyApplied()
{
	assert(movementState == EMoveState::DURING_MOVE);

	if(movementState == EMoveState::DURING_MOVE)
	{
		// After teleportation via CGTeleport object is finished
		assert(destinationTeleport != ObjectInstanceID::NONE);
		destinationTeleport = ObjectInstanceID();
		destinationTeleportPos = int3(-1);
		movementState = EMoveState::CONTINUE_MOVE;
	}
}

void HeroMovementController::onPlayerTurnStarted()
{
	assert(movementState == EMoveState::STOP_MOVE);
	movementState = EMoveState::STOP_MOVE;
}

void HeroMovementController::onBattleStarted()
{
	assert(movementState == EMoveState::STOP_MOVE);
	// when battle starts, game will send battleStart pack *before* movement confirmation
	// and since network thread wait for battle intro to play, movement confirmation will only happen after intro
	// leading to several bugs, such as blocked input during intro
	movementState = EMoveState::STOP_MOVE;
}

void HeroMovementController::showTeleportDialog(TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QueryID askID)
{
	assert(destinationTeleport != ObjectInstanceID::NONE);

	auto neededExit = std::make_pair(destinationTeleport, destinationTeleportPos);
	assert(vstd::contains(exits, neededExit));

	int choosenExit = vstd::find_pos(exits, neededExit);
	LOCPLINT->cb->selectionMade(choosenExit, askID);
}

void HeroMovementController::updatePath(const CGHeroInstance * hero, const TryMoveHero & details)
{
	if (hero->tempOwner != LOCPLINT->playerID)
		return;

	assert(LOCPLINT->makingTurn);
	assert(LOCPLINT->localState->hasPath(hero));
	assert(LOCPLINT->localState->getNextTile(hero).has_value());

	bool directlyAttackingCreature = details.attackedFrom.has_value() && LOCPLINT->localState->getLastTile(hero) == details.attackedFrom;

	auto desiredTarget = LOCPLINT->localState->getNextTile(hero);
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
		movementState = EMoveState::STOP_MOVE;
		adventureInt->onHeroChanged(hero);
		return;
	}

	CGI->mh->waitForOngoingAnimations();

	//move finished
	adventureInt->onHeroChanged(hero);

	//check if user cancelled movement
	if (GH.input().ignoreEventsUntilInput())
		movementState = EMoveState::STOP_MOVE;

	if (movementState == EMoveState::WAITING_MOVE)
		movementState = EMoveState::DURING_MOVE;

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

void HeroMovementController::movementStopRequested()
{
	assert(movementState == EMoveState::DURING_MOVE);

	if (movementState == EMoveState::DURING_MOVE)//if we are in the middle of hero movement
		movementState = EMoveState::STOP_MOVE;
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

	auto prevTile = LOCPLINT->cb->getTile(hero->convertToVisitablePos(posPrev));
	auto nextTile = LOCPLINT->cb->getTile(hero->convertToVisitablePos(posNext));

	auto prevRoad = prevTile->roadType;
	auto nextRoad = nextTile->roadType;
	bool movingOnRoad = prevRoad->getId() != Road::NO_ROAD && nextRoad->getId() != Road::NO_ROAD;

	if (movingOnRoad)
		return nextTile->terType->horseSound;
	else
		return nextTile->terType->horseSoundPenalty;
};

void HeroMovementController::updateMovementSound(const CGHeroInstance * hero, int3 posPrev, int3 posNext, EPathNodeAction moveType)
{

	// Start a new sound for the hero movement or let the existing one carry on.
	AudioPath newSoundName = getMovementSoundFor(h, prevCoord, nextCoord, path.nodes[i-1].action);

	if(newSoundName != soundName)
	{
		soundName = newSoundName;

		CCS->soundh->stopSound(soundChannel);
		if (!soundName.empty())
			soundChannel = CCS->soundh->playSound(soundName, -1);
		else
			soundChannel = -1;
	}


	int soundChannel = -1;
	AudioPath soundName;
}

void HeroMovementController::stopMovementSound()
{
	CCS->soundh->stopSound(soundChannel);
}

void HeroMovementController::movementStartRequested(const CGHeroInstance * h, const CGPath & path)
{
	setMovementStatus(true);

	auto getObj = [&](int3 coord, bool ignoreHero)
	{
		return LOCPLINT->cb->getTile(h->convertToVisitablePos(coord))->topVisitableObj(ignoreHero);
	};

	auto isTeleportAction = [&](EPathNodeAction action) -> bool
	{
		if (action != EPathNodeAction::TELEPORT_NORMAL &&
			action != EPathNodeAction::TELEPORT_BLOCKING_VISIT &&
			action != EPathNodeAction::TELEPORT_BATTLE)
		{
			return false;
		}

		return true;
	};

	auto getDestTeleportObj = [&](const CGObjectInstance * currentObject, const CGObjectInstance * nextObjectTop, const CGObjectInstance * nextObject) -> const CGObjectInstance *
	{
		if (CGTeleport::isConnected(currentObject, nextObjectTop))
			return nextObjectTop;

		if (nextObjectTop && nextObjectTop->ID == Obj::HERO && CGTeleport::isConnected(currentObject, nextObject))
		{
			return nextObject;
		}

		return nullptr;
	};

	auto doMovement = [&](int3 dst, bool transit)
	{
		movementState = EMoveState::WAITING_MOVE;
		LOCPLINT->cb->moveHero(h, dst, transit);
		while(movementState != EMoveState::STOP_MOVE && movementState != EMoveState::CONTINUE_MOVE)
		{
			assert(0);
			boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
		}
	};

	auto canStop = [&](const CGPathNode * node) -> bool
	{
		if (node->layer != EPathfindingLayer::LAND && node->layer != EPathfindingLayer::SAIL)
			return false;

		if (node->accessible != EPathAccessibility::ACCESSIBLE)
			return false;

		return true;
	};

	int i = 1;
	movementState = EMoveState::CONTINUE_MOVE;

	for (i=(int)path.nodes.size()-1; i>0 && (movementState == EMoveState::CONTINUE_MOVE || !canStop(&path.nodes[i])); i--)
	{
		int3 prevCoord = h->convertFromVisitablePos(path.nodes[i].coord);
		int3 nextCoord = h->convertFromVisitablePos(path.nodes[i-1].coord);

		auto prevObject = getObj(prevCoord, prevCoord == h->pos);
		auto nextObjectTop = getObj(nextCoord, false);
		auto nextObject = getObj(nextCoord, true);
		auto destTeleportObj = getDestTeleportObj(prevObject, nextObjectTop, nextObject);
		if (isTeleportAction(path.nodes[i-1].action) && destTeleportObj != nullptr)
		{
			stopMovementSound();
			destinationTeleport = destTeleportObj->id;
			destinationTeleportPos = nextCoord;
			doMovement(h->pos, false);
			if (path.nodes[i-1].action == EPathNodeAction::TELEPORT_BLOCKING_VISIT
				|| path.nodes[i-1].action == EPathNodeAction::TELEPORT_BATTLE)
			{
				destinationTeleport = ObjectInstanceID();
				destinationTeleportPos = int3(-1);
			}

			if(i != path.nodes.size() - 1)
				updateMovementSound(h, prevCoord, nextCoord, path.nodes[i-1].action);

			continue;
		}

		if (path.nodes[i-1].turns)
		{ //stop sending move requests if the next node can't be reached at the current turn (hero exhausted his move points)
			movementState = EMoveState::STOP_MOVE;
			break;
		}

		updateMovementSound(h, prevCoord, nextCoord, path.nodes[i-1].action);

		assert(h->pos.z == nextCoord.z); // Z should change only if it's movement via teleporter and in this case this code shouldn't be executed at all
		int3 endpos(nextCoord.x, nextCoord.y, h->pos.z);
		logGlobal->trace("Requesting hero movement to %s", endpos.toString());

		bool useTransit = false;
		if ((i-2 >= 0) // Check there is node after next one; otherwise transit is pointless
			&& (CGTeleport::isConnected(nextObjectTop, getObj(h->convertFromVisitablePos(path.nodes[i-2].coord), false))
				|| CGTeleport::isTeleport(nextObjectTop)))
		{ // Hero should be able to go through object if it's allow transit
			useTransit = true;
		}
		else if (path.nodes[i-1].layer == EPathfindingLayer::AIR)
			useTransit = true;

		doMovement(endpos, useTransit);

		logGlobal->trace("Resuming %s", __FUNCTION__);
		bool guarded = LOCPLINT->cb->isInTheMap(LOCPLINT->cb->getGuardingCreaturePosition(endpos - int3(1, 0, 0)));
		if ((!useTransit && guarded) || LOCPLINT->showingDialog->get() == true) // Abort movement if a guard was fought or there is a dialog to display (Mantis #1136)
			break;
	}
	stopMovementSound();

	//Update cursor so icon can change if needed when it reappears; doesn;'t apply if a dialog box pops up at the end of the movement
	if (!LOCPLINT->showingDialog->get())
		GH.fakeMouseMove();

	CGI->mh->waitForOngoingAnimations();
	setMovementStatus(false);
}
