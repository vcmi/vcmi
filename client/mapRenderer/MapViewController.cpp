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

#include "../../lib/CConfigHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/spells/ViewSpellInt.h"

void MapViewController::setViewCenter(const int3 & position)
{
	assert(context->isInMap(position));
	setViewCenter(Point(position) * model->getSingleTileSize(), position.z);
}

void MapViewController::setViewCenter(const Point & position, int level)
{
	Point betterPosition = {
		vstd::clamp(position.x, 0, context->getMapSize().x * model->getSingleTileSize().x),
		vstd::clamp(position.y, 0, context->getMapSize().y * model->getSingleTileSize().y)
	};

	model->setViewCenter(betterPosition);
	model->setLevel(vstd::clamp(level, 0, context->getMapSize().z));
}

void MapViewController::setTileSize(const Point & tileSize)
{
	model->setTileSize(tileSize);
}

MapViewController::MapViewController(std::shared_ptr<MapViewModel> model)
	: context(new MapRendererContext())
	, model(std::move(model))
{
}

std::shared_ptr<const IMapRendererContext> MapViewController::getContext() const
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
	static const double heroTeleportDuration = 250;

	//FIXME: remove code duplication?

	if(context->movementAnimation)
	{
		// TODO: enemyMoveTime
		double heroMoveTime = settings["adventure"]["heroMoveTime"].Float();

		context->movementAnimation->progress += timeDelta / heroMoveTime;

		Point positionFrom = Point(context->movementAnimation->tileFrom) * model->getSingleTileSize();
		Point positionDest = Point(context->movementAnimation->tileDest) * model->getSingleTileSize();

		Point positionCurr = vstd::lerp(positionFrom, positionDest, context->movementAnimation->progress);

		setViewCenter(positionCurr, context->movementAnimation->tileDest.z);

		if(context->movementAnimation->progress >= 1.0)
		{
			setViewCenter(context->movementAnimation->tileDest);

			context->removeObject(context->getObject(context->movementAnimation->target));
			context->addObject(context->getObject(context->movementAnimation->target));
			context->movementAnimation.reset();
		}
	}

	if(context->teleportAnimation)
	{
		context->teleportAnimation->progress += timeDelta / heroTeleportDuration;
		if(context->teleportAnimation->progress >= 1.0)
			context->teleportAnimation.reset();
	}

	if(context->fadeOutAnimation)
	{
		context->fadeOutAnimation->progress += timeDelta / fadeOutDuration;
		if(context->fadeOutAnimation->progress >= 1.0)
		{
			context->removeObject(context->getObject(context->fadeOutAnimation->target));
			context->fadeOutAnimation.reset();
		}
	}

	if(context->fadeInAnimation)
	{
		context->fadeInAnimation->progress += timeDelta / fadeInDuration;
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
	assert(!context->fadeInAnimation);
	context->fadeInAnimation = FadingAnimationState{obj->id, 0.0};
	context->addObject(obj);
}

void MapViewController::onObjectFadeOut(const CGObjectInstance * obj)
{
	assert(!context->fadeOutAnimation);
	context->fadeOutAnimation = FadingAnimationState{obj->id, 0.0};
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
	context->teleportAnimation = HeroAnimationState{obj->id, from, dest, 0.0};
}

void MapViewController::onHeroMoved(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	assert(!context->movementAnimation);

	const CGObjectInstance * movingObject = obj;
	if(obj->boat)
		movingObject = obj->boat;

	context->removeObject(movingObject);

	if(settings["adventure"]["heroMoveTime"].Float() > 1)
	{
		context->addMovingObject(movingObject, from, dest);
		context->movementAnimation = HeroAnimationState{movingObject->id, from, dest, 0.0};
	}
	else
	{
		// instant movement
		context->addObject(movingObject);
		setViewCenter(movingObject->visitablePos());
	}
}

void MapViewController::onHeroRotated(const CGHeroInstance * obj, const int3 & from, const int3 & dest)
{
	//TODO
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

