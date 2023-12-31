/*
 * MapViewController.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "MapViewController.h"

#include "MapRendererContext.h"
#include "MapRendererContextState.h"
#include "MapViewCache.h"
#include "MapViewModel.h"

#include "../CPlayerInterface.h"
#include "../adventureMap/AdventureMapInterface.h"
#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../eventsSDL/InputHandler.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/pathfinder/CGPathNode.h"
#include "../../lib/spells/ViewSpellInt.h"

void MapViewController::setViewCenter(const int3 & position)
{
	setViewCenter(Point(position) * model->getSingleTileSize() + model->getSingleTileSize() / 2, position.z);
}

void MapViewController::setViewCenter(const Point & position, int level)
{
	Point upperLimit = Point(context->getMapSize()) * model->getSingleTileSize();
	Point lowerLimit = Point(0, 0);

	if(worldViewContext)
	{
		Point area = model->getPixelsVisibleDimensions();
		Point mapCenter = upperLimit / 2;

		Point desiredLowerLimit = lowerLimit + area / 2;
		Point desiredUpperLimit = upperLimit - area / 2;

		Point actualLowerLimit{
			std::min(desiredLowerLimit.x, mapCenter.x),
			std::min(desiredLowerLimit.y, mapCenter.y)
		};

		Point actualUpperLimit{
			std::max(desiredUpperLimit.x, mapCenter.x),
			std::max(desiredUpperLimit.y, mapCenter.y)
		};

		upperLimit = actualUpperLimit;
		lowerLimit = actualLowerLimit;
	}

	Point betterPosition = {std::clamp(position.x, lowerLimit.x, upperLimit.x), std::clamp(position.y, lowerLimit.y, upperLimit.y)};

	model->setViewCenter(betterPosition);
	model->setLevel(std::clamp(level, 0, context->getMapSize().z));

	if(adventureInt && !puzzleMapContext) // may be called before adventureInt is initialized
		adventureInt->onMapViewMoved(model->getTilesTotalRect(), model->getLevel());
}

void MapViewController::setTileSize(const Point & tileSize)
{
	Point oldSize = model->getSingleTileSize();
	model->setTileSize(tileSize);

	double scaleChangeX = 1.0 * tileSize.x / oldSize.x;
	double scaleChangeY = 1.0 * tileSize.y / oldSize.y;

	Point newViewCenter {
		static_cast<int>(std::round(model->getMapViewCenter().x * scaleChangeX)),
		static_cast<int>(std::round(model->getMapViewCenter().y * scaleChangeY))
	};

	// force update of view center since changing tile size may invalidated it
	setViewCenter(newViewCenter, model->getLevel());
}

void MapViewController::modifyTileSize(int stepsChange)
{
	// we want to zoom in/out in fixed 10% steps, to allow player to return back to exactly 100% zoom just by scrolling
	// so, zooming in for 5 steps will put game at 1.1^5 = 1.61 scale
	// try to determine current zooming level and change it by requested number of steps
	double currentZoomFactor = targetTileSize.x / static_cast<double>(defaultTileSize);
	double currentZoomSteps = std::round(std::log(currentZoomFactor) / std::log(1.01));
	double newZoomSteps = stepsChange != 0 ? currentZoomSteps + stepsChange : stepsChange;
	double newZoomFactor = std::pow(1.01, newZoomSteps);

	Point currentZoom = targetTileSize;
	Point desiredZoom = Point(defaultTileSize,defaultTileSize) * newZoomFactor;

	if (desiredZoom == currentZoom && stepsChange < 0)
		desiredZoom -= Point(1,1);

	if (desiredZoom == currentZoom && stepsChange > 0)
		desiredZoom += Point(1,1);

	Point minimal = model->getSingleTileSizeLowerLimit();
	Point maximal = model->getSingleTileSizeUpperLimit();
	Point actualZoom = {
		std::clamp(desiredZoom.x, minimal.x, maximal.x),
		std::clamp(desiredZoom.y, minimal.y, maximal.y)
	};

	if (actualZoom != currentZoom)
	{
		targetTileSize = actualZoom;
		if(actualZoom.x >= defaultTileSize - zoomTileDeadArea && actualZoom.x <= defaultTileSize + zoomTileDeadArea)
			actualZoom.x = defaultTileSize;
		if(actualZoom.y >= defaultTileSize - zoomTileDeadArea && actualZoom.y <= defaultTileSize + zoomTileDeadArea)
			actualZoom.y = defaultTileSize;
		
		bool isInDeadZone = targetTileSize != actualZoom || actualZoom == Point(defaultTileSize, defaultTileSize);

		if(!wasInDeadZone && isInDeadZone)
			GH.input().hapticFeedback();

		wasInDeadZone = isInDeadZone;

		setTileSize(actualZoom);
	}
}

MapViewController::MapViewController(std::shared_ptr<MapViewModel> model, std::shared_ptr<MapViewCache> view)
	: state(new MapRendererContextState())
	, model(std::move(model))
	, view(view)
{
	adventureContext = std::make_shared<MapRendererAdventureContext>(*state);
	context = adventureContext;
}

std::shared_ptr<IMapRendererContext> MapViewController::getContext() const
{
	return context;
}

void MapViewController::tick(uint32_t timeDelta)
{
	// confirmed to match H3 for
	// - hero embarking on boat (500 ms)
	// - hero disembarking from boat (500 ms)
	// - TODO: picking up resources
	// - TODO: killing mosters
	// - teleporting ( 250 ms)
	static const double fadeOutDuration = 500;
	static const double fadeInDuration = 500;
	static const double heroTeleportDuration = 250;

	if(movementContext)
	{
		const auto * object = context->getObject(movementContext->target);
		const auto * hero = dynamic_cast<const CGHeroInstance *>(object);
		const auto * boat = dynamic_cast<const CGBoat *>(object);

		assert(boat || hero);

		if(!hero)
			hero = boat->hero;

		double heroMoveTime = LOCPLINT->playerID == hero->getOwner() ?
			settings["adventure"]["heroMoveTime"].Float() :
			settings["adventure"]["enemyMoveTime"].Float();

		movementContext->progress += timeDelta / heroMoveTime;
		movementContext->progress = std::min( 1.0, movementContext->progress);

		Point positionFrom = Point(hero->convertToVisitablePos(movementContext->tileFrom)) * model->getSingleTileSize() + model->getSingleTileSize() / 2;
		Point positionDest = Point(hero->convertToVisitablePos(movementContext->tileDest)) * model->getSingleTileSize() + model->getSingleTileSize() / 2;

		Point positionCurr = vstd::lerp(positionFrom, positionDest, movementContext->progress);

		setViewCenter(positionCurr, movementContext->tileDest.z);
	}

	if(teleportContext)
	{
		teleportContext->progress += timeDelta / heroTeleportDuration;
		teleportContext->progress = std::min( 1.0, teleportContext->progress);
	}

	if(fadingOutContext)
	{
		fadingOutContext->progress -= timeDelta / fadeOutDuration;
		fadingOutContext->progress = std::max( 0.0, fadingOutContext->progress);
	}

	if(fadingInContext)
	{
		fadingInContext->progress += timeDelta / fadeInDuration;
		fadingInContext->progress = std::min( 1.0, fadingInContext->progress);
	}

	if (adventureContext)
		adventureContext->animationTime += timeDelta;

	updateState();
}

void MapViewController::updateState()
{
	if(adventureContext)
	{
		adventureContext->settingsSessionSpectate = settings["session"]["spectate"].Bool();
		adventureContext->settingsAdventureObjectAnimation = settings["adventure"]["objectAnimation"].Bool();
		adventureContext->settingsAdventureTerrainAnimation = settings["adventure"]["terrainAnimation"].Bool();
		adventureContext->settingShowGrid = settings["gameTweaks"]["showGrid"].Bool();
		adventureContext->settingShowVisitable = settings["session"]["showVisitable"].Bool();
		adventureContext->settingShowBlocked = settings["session"]["showBlocked"].Bool();
		adventureContext->settingSpellRange = settings["session"]["showSpellRange"].Bool();
	}
}

void MapViewController::afterRender()
{
	if(movementContext)
	{
		const auto * object = context->getObject(movementContext->target);
		const auto * hero = dynamic_cast<const CGHeroInstance *>(object);
		const auto * boat = dynamic_cast<const CGBoat *>(object);

		assert(boat || hero);

		if(!hero)
			hero = boat->hero;

		if(movementContext->progress >= 0.999)
		{
			logGlobal->debug("Ending movement animation");
			setViewCenter(hero->getSightCenter());

			removeObject(context->getObject(movementContext->target));
			addObject(context->getObject(movementContext->target));

			activateAdventureContext(movementContext->animationTime);
		}
	}

	if(teleportContext && teleportContext->progress >= 0.999)
	{
		logGlobal->debug("Ending teleport animation");
		activateAdventureContext(teleportContext->animationTime);
	}

	if(fadingOutContext && fadingOutContext->progress <= 0.001)
	{
		logGlobal->debug("Ending fade out animation");
		removeObject(context->getObject(fadingOutContext->target));

		activateAdventureContext(fadingOutContext->animationTime);
	}

	if(fadingInContext && fadingInContext->progress >= 0.999)
	{
		logGlobal->debug("Ending fade in animation");
		activateAdventureContext(fadingInContext->animationTime);
	}
}

bool MapViewController::isEventInstant(const CGObjectInstance * obj, const PlayerColor & initiator)
{
	if(settings["gameTweaks"]["skipAdventureMapAnimations"].Bool())
		return true;

	if (!isEventVisible(obj, initiator))
		return true;

	if(initiator != LOCPLINT->playerID && settings["adventure"]["enemyMoveTime"].Float() <= 0)
		return true; // instant movement speed

	if(initiator == LOCPLINT->playerID && settings["adventure"]["heroMoveTime"].Float() <= 0)
		return true; // instant movement speed

	return false;
}

bool MapViewController::isEventVisible(const CGObjectInstance * obj, const PlayerColor & initiator)
{
	if(adventureContext == nullptr)
		return false;

	if(initiator != LOCPLINT->playerID && settings["adventure"]["enemyMoveTime"].Float() < 0)
		return false; // enemy move speed set to "hidden/none"

	if(!GH.windows().isTopWindow(adventureInt))
		return false;

	// do not focus on actions of other players during our turn (e.g. simturns)
	if (LOCPLINT->makingTurn && initiator != LOCPLINT->playerID)
		return false;

	if(obj->isVisitable())
		return context->isVisible(obj->visitablePos());
	else
		return context->isVisible(obj->pos);
}

bool MapViewController::isEventVisible(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	if(adventureContext == nullptr)
		return false;

	if(obj->getOwner() != LOCPLINT->playerID && settings["adventure"]["enemyMoveTime"].Float() < 0)
		return false; // enemy move speed set to "hidden/none"

	if(!GH.windows().isTopWindow(adventureInt))
		return false;

	// do not focus on actions of other players during our turn (e.g. simturns)
	if (LOCPLINT->makingTurn && obj->getOwner() != LOCPLINT->playerID)
		return false;

	if(context->isVisible(obj->convertToVisitablePos(from)))
		return true;

	if(context->isVisible(obj->convertToVisitablePos(dest)))
		return true;

	return false;
}

void MapViewController::fadeOutObject(const CGObjectInstance * obj)
{
	logGlobal->debug("Starting fade out animation");
	fadingOutContext = std::make_shared<MapRendererAdventureFadingContext>(*state);
	fadingOutContext->animationTime = adventureContext->animationTime;
	adventureContext = fadingOutContext;
	context = fadingOutContext;

	const CGObjectInstance * movingObject = obj;
	if (obj->ID == Obj::HERO)
	{
		auto * hero = dynamic_cast<const CGHeroInstance*>(obj);
		if (hero->boat)
			movingObject = hero->boat;
	}

	fadingOutContext->target = movingObject->id;
	fadingOutContext->progress = 1.0;
}

void MapViewController::fadeInObject(const CGObjectInstance * obj)
{
	logGlobal->debug("Starting fade in animation");
	fadingInContext = std::make_shared<MapRendererAdventureFadingContext>(*state);
	fadingInContext->animationTime = adventureContext->animationTime;
	adventureContext = fadingInContext;
	context = fadingInContext;

	const CGObjectInstance * movingObject = obj;
	if (obj->ID == Obj::HERO)
	{
		auto * hero = dynamic_cast<const CGHeroInstance*>(obj);
		if (hero->boat)
			movingObject = hero->boat;
	}

	fadingInContext->target = movingObject->id;
	fadingInContext->progress = 0.0;
}

void MapViewController::removeObject(const CGObjectInstance * obj)
{
	if (obj->ID == Obj::BOAT)
	{
		auto * boat = dynamic_cast<const CGBoat*>(obj);
		if (boat->hero)
		{
			view->invalidate(context, boat->hero->id);
			state->removeObject(boat->hero);
		}
	}

	if (obj->ID == Obj::HERO)
	{
		auto * hero = dynamic_cast<const CGHeroInstance*>(obj);
		if (hero->boat)
		{
			view->invalidate(context, hero->boat->id);
			state->removeObject(hero->boat);
		}
	}

	view->invalidate(context, obj->id);
	state->removeObject(obj);
}

void MapViewController::addObject(const CGObjectInstance * obj)
{
	state->addObject(obj);
	view->invalidate(context, obj->id);
}

void MapViewController::onBeforeHeroEmbark(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	if(isEventVisible(obj, from, dest))
	{
		if (!isEventInstant(obj, obj->getOwner()))
			fadeOutObject(obj);
		setViewCenter(obj->getSightCenter());
	}
	else
		removeObject(obj);
}

void MapViewController::onAfterHeroEmbark(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	if(isEventVisible(obj, from, dest))
		setViewCenter(obj->getSightCenter());
}

void MapViewController::onBeforeHeroDisembark(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	if(isEventVisible(obj, from, dest))
		setViewCenter(obj->getSightCenter());
}

void MapViewController::onAfterHeroDisembark(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	if(isEventVisible(obj, from, dest))
	{
		if (!isEventInstant(obj, obj->getOwner()))
			fadeInObject(obj);
		setViewCenter(obj->getSightCenter());
	}
	addObject(obj);
}

void MapViewController::onObjectFadeIn(const CGObjectInstance * obj, const PlayerColor & initiator)
{
	assert(!hasOngoingAnimations());

	if(isEventVisible(obj, initiator) && !isEventInstant(obj, initiator) )
		fadeInObject(obj);

	addObject(obj);
}

void MapViewController::onObjectFadeOut(const CGObjectInstance * obj, const PlayerColor & initiator)
{
	assert(!hasOngoingAnimations());

	if(isEventVisible(obj, initiator) && !isEventInstant(obj, initiator) )
		fadeOutObject(obj);
	else
		removeObject(obj);
}

void MapViewController::onObjectInstantAdd(const CGObjectInstance * obj, const PlayerColor & initiator)
{
	addObject(obj);
};

void MapViewController::onObjectInstantRemove(const CGObjectInstance * obj, const PlayerColor & initiator)
{
	removeObject(obj);
};

void MapViewController::onBeforeHeroTeleported(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	assert(!hasOngoingAnimations());

	if(isEventVisible(obj, from, dest))
	{
		setViewCenter(obj->getSightCenter());
		view->createTransitionSnapshot(context);
	}
}

void MapViewController::onAfterHeroTeleported(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	assert(!hasOngoingAnimations());

	const CGObjectInstance * movingObject = obj;
	if(obj->boat)
		movingObject = obj->boat;

	removeObject(movingObject);
	addObject(movingObject);

	if(isEventVisible(obj, from, dest))
	{
		logGlobal->debug("Starting teleport animation");
		teleportContext = std::make_shared<MapRendererAdventureTransitionContext>(*state);
		teleportContext->animationTime = adventureContext->animationTime;
		adventureContext = teleportContext;
		context = teleportContext;
		setViewCenter(movingObject->getSightCenter());
	}
}

void MapViewController::onHeroMoved(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	assert(!hasOngoingAnimations());

	// revisiting via spacebar, no need to animate
	if(from == dest)
		return;

	const CGObjectInstance * movingObject = obj;
	if(obj->boat)
		movingObject = obj->boat;

	removeObject(movingObject);

	if(!isEventVisible(obj, from, dest))
	{
		addObject(movingObject);
		return;
	}

	double movementTime = LOCPLINT->playerID == obj->tempOwner ?
		settings["adventure"]["heroMoveTime"].Float() :
		settings["adventure"]["enemyMoveTime"].Float();

	if(movementTime > 1)
	{
		logGlobal->debug("Starting movement animation");
		movementContext = std::make_shared<MapRendererAdventureMovingContext>(*state);
		movementContext->animationTime = adventureContext->animationTime;
		adventureContext = movementContext;
		context = movementContext;

		state->addMovingObject(movingObject, from, dest);

		movementContext->target = movingObject->id;
		movementContext->tileFrom = from;
		movementContext->tileDest = dest;
		movementContext->progress = 0.0;
	}
	else // instant movement
	{
		addObject(movingObject);
		setViewCenter(movingObject->visitablePos());
	}
}

bool MapViewController::hasOngoingAnimations()
{
	if(movementContext)
		return true;

	if(fadingOutContext)
		return true;

	if(fadingInContext)
		return true;

	if(teleportContext)
		return true;

	return false;
}

void MapViewController::activateAdventureContext(uint32_t animationTime)
{
	resetContext();

	adventureContext = std::make_shared<MapRendererAdventureContext>(*state);
	adventureContext->animationTime = animationTime;
	context = adventureContext;
	updateState();
}

void MapViewController::activateAdventureContext()
{
	activateAdventureContext(0);
}

void MapViewController::activateWorldViewContext()
{
	if(worldViewContext)
		return;

	resetContext();

	worldViewContext = std::make_shared<MapRendererWorldViewContext>(*state);
	context = worldViewContext;
}

void MapViewController::activateSpellViewContext()
{
	if(spellViewContext)
		return;

	resetContext();

	spellViewContext = std::make_shared<MapRendererSpellViewContext>(*state);
	worldViewContext = spellViewContext;
	context = spellViewContext;
}

void MapViewController::activatePuzzleMapContext(const int3 & grailPosition)
{
	resetContext();

	puzzleMapContext = std::make_shared<MapRendererPuzzleMapContext>(*state);
	context = puzzleMapContext;

	CGPathNode fakeNode;
	fakeNode.coord = grailPosition;

	puzzleMapContext->grailPos = std::make_unique<CGPath>();

	// create two nodes since 1st one is normally not visible
	puzzleMapContext->grailPos->nodes.push_back(fakeNode);
	puzzleMapContext->grailPos->nodes.push_back(fakeNode);
}

void MapViewController::resetContext()
{
	adventureContext.reset();
	movementContext.reset();
	fadingOutContext.reset();
	fadingInContext.reset();
	teleportContext.reset();
	worldViewContext.reset();
	spellViewContext.reset();
	puzzleMapContext.reset();
}

void MapViewController::setTerrainVisibility(bool showAllTerrain)
{
	assert(spellViewContext);
	spellViewContext->showAllTerrain = showAllTerrain;
}

void MapViewController::setOverlayVisibility(const std::vector<ObjectPosInfo> & objectPositions)
{
	assert(spellViewContext);
	spellViewContext->additionalOverlayIcons = objectPositions;
}
