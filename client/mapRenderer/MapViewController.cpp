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
#include "MapViewModel.h"
#include "MapViewCache.h"

#include "../adventureMap/CAdvMapInt.h"
#include "../CPlayerInterface.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/spells/ViewSpellInt.h"
#include "../../lib/CPathfinder.h"

void MapViewController::setViewCenter(const int3 & position)
{
	assert(context->isInMap(position));
	setViewCenter(Point(position) * model->getSingleTileSize() + model->getSingleTileSize() / 2, position.z);
}

void MapViewController::setViewCenter(const Point & position, int level)
{
	Point upperLimit = Point(context->getMapSize()) * model->getSingleTileSize() + model->getSingleTileSize();
	Point lowerLimit = Point(0,0);

	if (worldViewContext)
	{
		Point area = model->getPixelsVisibleDimensions();
		Point mapCenter = upperLimit / 2;

		Point desiredLowerLimit = lowerLimit + area / 2;
		Point desiredUpperLimit = upperLimit - area / 2;

		Point actualLowerLimit {
			std::min(desiredLowerLimit.x, mapCenter.x),
			std::min(desiredLowerLimit.y, mapCenter.y)
		};

		Point actualUpperLimit {
			std::max(desiredUpperLimit.x, mapCenter.x),
			std::max(desiredUpperLimit.y, mapCenter.y)
		};

		upperLimit = actualUpperLimit;
		lowerLimit = actualLowerLimit;
	}

	Point betterPosition = {
		vstd::clamp(position.x, lowerLimit.x, upperLimit.x),
		vstd::clamp(position.y, lowerLimit.y, upperLimit.y)
	};

	model->setViewCenter(betterPosition);
	model->setLevel(vstd::clamp(level, 0, context->getMapSize().z));

	if (adventureInt) // may be called before adventureInt is initialized
		adventureInt->onMapViewMoved(model->getTilesTotalRect(), model->getLevel());
}

void MapViewController::setTileSize(const Point & tileSize)
{
	model->setTileSize(tileSize);

	// force update of view center since changing tile size may invalidated it
	setViewCenter(model->getMapViewCenter(), model->getLevel());
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

void MapViewController::update(uint32_t timeDelta)
{
	// confirmed to match H3 for
	// - hero embarking on boat (500 ms)
	// - hero disembarking from boat (500 ms)
	// - TODO: picking up resources
	// - TODO: killing mosters
	// - teleporting ( 250 ms)
	static const double fadeOutDuration = 500;
	static const double fadeInDuration = 500;
	//static const double heroTeleportDuration = 250;

	//FIXME: remove code duplication?

	if(movementContext)
	{
		const auto * object = context->getObject(movementContext->target);
		const auto * hero = dynamic_cast<const CGHeroInstance*>(object);
		const auto * boat = dynamic_cast<const CGBoat*>(object);

		assert(boat || hero);

		if (!hero)
			hero = boat->hero;

		double heroMoveTime =
			LOCPLINT->makingTurn ?
			settings["adventure"]["heroMoveTime"].Float():
			settings["adventure"]["enemyMoveTime"].Float();

		movementContext->progress += timeDelta / heroMoveTime;

		Point positionFrom = Point(hero->convertToVisitablePos(movementContext->tileFrom)) * model->getSingleTileSize() + model->getSingleTileSize() / 2;
		Point positionDest = Point(hero->convertToVisitablePos(movementContext->tileDest)) * model->getSingleTileSize() + model->getSingleTileSize() / 2;

		Point positionCurr = vstd::lerp(positionFrom, positionDest, movementContext->progress);

		if(movementContext->progress >= 1.0)
		{
			setViewCenter(hero->getSightCenter());

			removeObject(context->getObject(movementContext->target));
			addObject(context->getObject(movementContext->target));

			activateAdventureContext(movementContext->animationTime);
		}
		else
		{
			setViewCenter(positionCurr, movementContext->tileDest.z);
		}
	}

	//if(teleportContext)
	//{
	//	teleportContext->progress += timeDelta / heroTeleportDuration;
	//	moveFocusToSelection();
	//	if(teleportContext->progress >= 1.0)
	//		teleportContext.reset();
	//}

	if(fadingOutContext)
	{
		fadingOutContext->progress -= timeDelta / fadeOutDuration;

		if(fadingOutContext->progress <= 0.0)
		{
			removeObject(context->getObject(fadingOutContext->target));

			activateAdventureContext(fadingOutContext->animationTime);
		}
	}

	if(fadingInContext)
	{
		fadingInContext->progress += timeDelta / fadeInDuration;

		if(fadingInContext->progress >= 1.0)
		{
			activateAdventureContext(fadingInContext->animationTime);
		}
	}

	if (adventureContext)
	{
		adventureContext->animationTime += timeDelta;
		adventureContext->settingsSessionSpectate = settings["session"]["spectate"].Bool();
		adventureContext->settingsAdventureObjectAnimation = settings["adventure"]["objectAnimation"].Bool();
		adventureContext->settingsAdventureTerrainAnimation = settings["adventure"]["terrainAnimation"].Bool();
		adventureContext->settingShowGrid = settings["gameTweaks"]["showGrid"].Bool();
		adventureContext->settingShowVisitable = settings["session"]["showVisitable"].Bool();
		adventureContext->settingShowBlockable = settings["session"]["showBlockable"].Bool();
	}
}

bool MapViewController::isEventVisible(const CGObjectInstance * obj)
{
	if (adventureContext == nullptr)
		return false;

	if (!LOCPLINT->makingTurn && settings["adventure"]["enemyMoveTime"].Float() < 0)
		return false; // enemy move speed set to "hidden/none"

	if (obj->isVisitable())
		return context->isVisible(obj->visitablePos());
	else
		return context->isVisible(obj->pos);
}

bool MapViewController::isEventVisible(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	if (adventureContext == nullptr)
		return false;

	if (!LOCPLINT->makingTurn && settings["adventure"]["enemyMoveTime"].Float() < 0)
		return false; // enemy move speed set to "hidden/none"

	if (context->isVisible(obj->convertToVisitablePos(from)))
		return true;

	if (context->isVisible(obj->convertToVisitablePos(dest)))
		return true;

	return false;
}

void MapViewController::fadeOutObject(const CGObjectInstance * obj)
{
	fadingOutContext = std::make_shared<MapRendererAdventureFadingContext>(*state);
	fadingOutContext->animationTime = adventureContext->animationTime;
	adventureContext = fadingOutContext;
	context = fadingOutContext;

	fadingOutContext->target = obj->id;
	fadingOutContext->progress = 1.0;
}

void MapViewController::fadeInObject(const CGObjectInstance * obj)
{
	fadingInContext = std::make_shared<MapRendererAdventureFadingContext>(*state);
	fadingInContext->animationTime = adventureContext->animationTime;
	adventureContext = fadingInContext;
	context = fadingInContext;

	fadingInContext->target = obj->id;
	fadingInContext->progress = 0.0;
}

void MapViewController::removeObject(const CGObjectInstance * obj)
{
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
	if (isEventVisible(obj, from, dest))
	{
		onObjectFadeOut(obj);
		setViewCenter(obj->getSightCenter());
	}
	else
		removeObject(obj);
}

void MapViewController::onAfterHeroEmbark(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	if (isEventVisible(obj, from, dest))
		setViewCenter(obj->getSightCenter());
}

void MapViewController::onBeforeHeroDisembark(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	if (isEventVisible(obj, from, dest))
		setViewCenter(obj->getSightCenter());
}

void MapViewController::onAfterHeroDisembark(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	if (isEventVisible(obj, from, dest))
	{
		onObjectFadeIn(obj);
		setViewCenter(obj->getSightCenter());
	}
	addObject(obj);
}

void MapViewController::onObjectFadeIn(const CGObjectInstance * obj)
{
	assert(!hasOngoingAnimations());

	if (isEventVisible(obj))
		fadeInObject(obj);

	addObject(obj);
}

void MapViewController::onObjectFadeOut(const CGObjectInstance * obj)
{
	assert(!hasOngoingAnimations());

	if (isEventVisible(obj))
		fadeOutObject(obj);
	else
		removeObject(obj);
}

void MapViewController::onObjectInstantAdd(const CGObjectInstance * obj)
{
	addObject(obj);
};

void MapViewController::onObjectInstantRemove(const CGObjectInstance * obj)
{
	removeObject(obj);
};

void MapViewController::onBeforeHeroTeleported(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	assert(!hasOngoingAnimations());

	if (isEventVisible(obj, from, dest))
	{
		// TODO: generate view with old state
		setViewCenter(obj->getSightCenter());
	}
}

void MapViewController::onAfterHeroTeleported(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	assert(!hasOngoingAnimations());

	if (isEventVisible(obj, from, dest))
	{
		// TODO: animation
		setViewCenter(obj->getSightCenter());
	}
	else
	{
		removeObject(obj);
		addObject(obj);
	}
}

void MapViewController::onHeroMoved(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	assert(!hasOngoingAnimations());

	const CGObjectInstance * movingObject = obj;
	if(obj->boat)
		movingObject = obj->boat;

	removeObject(movingObject);

	if (!isEventVisible(obj, from, dest))
	{
		addObject(movingObject);
		return;
	}

	double movementTime =
		LOCPLINT->playerID == obj->tempOwner ?
		settings["adventure"]["heroMoveTime"].Float():
		settings["adventure"]["enemyMoveTime"].Float();

	if(movementTime > 1)
	{
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

	return false;
}

void MapViewController::activateAdventureContext(uint32_t animationTime)
{
	resetContext();

	adventureContext = std::make_shared<MapRendererAdventureContext>(*state);
	adventureContext->animationTime = animationTime;
	context = adventureContext;
}

void MapViewController::activateAdventureContext()
{
	activateAdventureContext(0);
}

void MapViewController::activateWorldViewContext()
{
	if (worldViewContext)
		return;

	resetContext();

	worldViewContext = std::make_shared<MapRendererWorldViewContext>(*state);
	context = worldViewContext;
}

void MapViewController::activateSpellViewContext()
{
	if (spellViewContext)
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

