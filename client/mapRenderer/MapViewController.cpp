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
#include "MapViewModel.h"

#include "../adventureMap/CAdvMapInt.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/spells/ViewSpellInt.h"

void MapViewController::setViewCenter(const int3 & position)
{
	assert(context->isInMap(position));
	setViewCenter(Point(position) * model->getSingleTileSize() + model->getSingleTileSize() / 2, position.z);
}

void MapViewController::setViewCenter(const Point & position, int level)
{
	Point upperLimit = Point(context->getMapSize()) * model->getSingleTileSize() + model->getSingleTileSize();
	Point lowerLimit = Point(0,0);

	if (context->worldViewModeActive)
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

MapViewController::MapViewController(std::shared_ptr<MapViewModel> model)
	: context(new MapRendererContext())
	, model(std::move(model))
{
}

std::shared_ptr<IMapRendererContext> MapViewController::getContext() const
{
	return context;
}

void MapViewController::moveFocusToSelection()
{
	const auto * army = adventureInt->curArmy();

	if (army)
		setViewCenter(army->getSightCenter());
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
	static const double heroTeleportDuration = 250;

	//FIXME: remove code duplication?

	if(context->movementAnimation)
	{
		const auto * object = context->getObject(context->movementAnimation->target);
		const auto * hero = dynamic_cast<const CGHeroInstance*>(object);
		const auto * boat = dynamic_cast<const CGBoat*>(object);

		assert(boat || hero);

		if (!hero)
			hero = boat->hero;

		// TODO: enemyMoveTime
		double heroMoveTime = settings["adventure"]["heroMoveTime"].Float();

		context->movementAnimation->progress += timeDelta / heroMoveTime;

		Point positionFrom = Point(hero->convertToVisitablePos(context->movementAnimation->tileFrom)) * model->getSingleTileSize() + model->getSingleTileSize() / 2;
		Point positionDest = Point(hero->convertToVisitablePos(context->movementAnimation->tileDest)) * model->getSingleTileSize() + model->getSingleTileSize() / 2;

		Point positionCurr = vstd::lerp(positionFrom, positionDest, context->movementAnimation->progress);

		if(context->movementAnimation->progress >= 1.0)
		{
			setViewCenter(hero->getSightCenter());

			context->removeObject(context->getObject(context->movementAnimation->target));
			context->addObject(context->getObject(context->movementAnimation->target));
			context->movementAnimation.reset();
		}
		else
		{
			setViewCenter(positionCurr, context->movementAnimation->tileDest.z);
		}
	}

	if(context->teleportAnimation)
	{
		context->teleportAnimation->progress += timeDelta / heroTeleportDuration;
		moveFocusToSelection();
		if(context->teleportAnimation->progress >= 1.0)
			context->teleportAnimation.reset();
	}

	if(context->fadeOutAnimation)
	{
		context->fadeOutAnimation->progress += timeDelta / fadeOutDuration;
		moveFocusToSelection();
		if(context->fadeOutAnimation->progress >= 1.0)
		{
			context->removeObject(context->getObject(context->fadeOutAnimation->target));
			context->fadeOutAnimation.reset();
		}
	}

	if(context->fadeInAnimation)
	{
		context->fadeInAnimation->progress += timeDelta / fadeInDuration;
		moveFocusToSelection();
		if(context->fadeInAnimation->progress >= 1.0)
			context->fadeInAnimation.reset();
	}

	context->animationTime += timeDelta;
	context->worldViewModeActive = model->getSingleTileSize() != Point(32,32);

	context->settingsSessionSpectate = settings["session"]["spectate"].Bool();
	context->settingsAdventureObjectAnimation = settings["adventure"]["objectAnimation"].Bool();
	context->settingsAdventureTerrainAnimation = settings["adventure"]["terrainAnimation"].Bool();
	context->settingsSessionShowGrid = settings["gameTweaks"]["showGrid"].Bool();
	context->settingsSessionShowVisitable = settings["session"]["showVisitable"].Bool();
	context->settingsSessionShowBlockable = settings["session"]["showBlockable"].Bool();
}

void MapViewController::onObjectFadeIn(const CGObjectInstance * obj)
{
	bool actionVisible = context->isVisible(obj->pos);

	assert(!context->fadeInAnimation);

	if (actionVisible)
		context->fadeInAnimation = FadingAnimationState{obj->id, 0.0};
	context->addObject(obj);
}

void MapViewController::onObjectFadeOut(const CGObjectInstance * obj)
{
	bool actionVisible = context->isVisible(obj->pos);

	assert(!context->fadeOutAnimation);

	if (actionVisible)
		context->fadeOutAnimation = FadingAnimationState{obj->id, 0.0};
	else
		context->removeObject(obj);
}

void MapViewController::onObjectInstantAdd(const CGObjectInstance * obj)
{
	context->addObject(obj);
};

void MapViewController::onObjectInstantRemove(const CGObjectInstance * obj)
{
	context->removeObject(obj);
};

void MapViewController::onHeroTeleported(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	assert(!context->teleportAnimation);
	bool actionVisible = context->isVisible(from) || context->isVisible(dest);

	if (actionVisible)
	{
		context->teleportAnimation = HeroAnimationState{obj->id, from, dest, 0.0};
	}
	else
	{
		context->removeObject(obj);
		context->addObject(obj);
	}
}

void MapViewController::onHeroMoved(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	assert(!context->movementAnimation);
	bool actionVisible = context->isVisible(from) || context->isVisible(dest);

	const CGObjectInstance * movingObject = obj;
	if(obj->boat)
		movingObject = obj->boat;

	context->removeObject(movingObject);

	if(settings["adventure"]["heroMoveTime"].Float() > 1 && actionVisible)
	{
		context->addMovingObject(movingObject, from, dest);
		context->movementAnimation = HeroAnimationState{movingObject->id, from, dest, 0.0};
	}
	else
	{
		// instant movement
		context->addObject(movingObject);

		if (actionVisible)
			setViewCenter(movingObject->visitablePos());
	}
}

void MapViewController::onHeroRotated(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	//TODO. Or no-op?
}

bool MapViewController::hasOngoingAnimations()
{
	if(context->movementAnimation)
		return true;

	if(context->teleportAnimation)
		return true;

	if(context->fadeOutAnimation)
		return true;

	if(context->fadeInAnimation)
		return true;

	return false;
}

void MapViewController::setTerrainVisibility(bool showAllTerrain)
{
	context->showAllTerrain = showAllTerrain;
}

void MapViewController::setOverlayVisibility(const std::vector<ObjectPosInfo> & objectPositions)
{
	context->additionalOverlayIcons = objectPositions;
}

