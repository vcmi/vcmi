/*
 * CBattleObstacleController.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBattleObstacleController.h"

#include "CBattleInterface.h"
#include "CBattleFieldController.h"
#include "CBattleAnimations.h"
#include "CBattleStacksController.h"

#include "../CPlayerInterface.h"
#include "../gui/CAnimation.h"
#include "../gui/CCanvas.h"

#include "../../CCallback.h"
#include "../../lib/battle/CObstacleInstance.h"
#include "../../lib/ObstacleHandler.h"

CBattleObstacleController::CBattleObstacleController(CBattleInterface * owner):
	owner(owner)
{
	auto obst = owner->curInt->cb->battleGetAllObstacles();
	for(auto & elem : obst)
	{
		if ( elem->obstacleType == CObstacleInstance::MOAT )
			continue; // handled by siege controller;
		loadObstacleImage(*elem);
	}
}

void CBattleObstacleController::loadObstacleImage(const CObstacleInstance & oi)
{
	std::string animationName;

	if (auto spellObstacle = dynamic_cast<const SpellCreatedObstacle*>(&oi))
	{
		animationName = spellObstacle->animation;
	}
	else
	{
		assert( oi.obstacleType == CObstacleInstance::USUAL || oi.obstacleType == CObstacleInstance::ABSOLUTE_OBSTACLE);
		animationName = oi.getInfo().animation;
	}

	if (animationsCache.count(animationName) == 0)
	{
		if (oi.obstacleType == CObstacleInstance::ABSOLUTE_OBSTACLE)
		{
			// obstacle use single bitmap image for animations
			auto animation = std::make_shared<CAnimation>();
			animation->setCustom(animationName, 0, 0);
			animationsCache[animationName] = animation;
		}
		else
		{
			auto animation = std::make_shared<CAnimation>(animationName);
			animationsCache[animationName] = animation;
			animation->preload();
		}
	}
	obstacleAnimations[oi.uniqueID] = animationsCache[animationName];
}

void CBattleObstacleController::obstaclePlaced(const std::vector<std::shared_ptr<const CObstacleInstance>> & obstacles)
{
	assert(obstaclesBeingPlaced.empty());
	for (auto const & oi : obstacles)
		obstaclesBeingPlaced.push_back(oi->uniqueID);

	for (auto const & oi : obstacles)
	{
		auto spellObstacle = dynamic_cast<const SpellCreatedObstacle*>(oi.get());

		if (!spellObstacle)
		{
			logGlobal->error("I don't know how to animate appearing obstacle of type %d", (int)oi->obstacleType);
			obstaclesBeingPlaced.erase(obstaclesBeingPlaced.begin());
			continue;
		}

		std::string defname = spellObstacle->appearAnimation;

		//TODO: sound
		//soundBase::QUIKSAND
		//soundBase::LANDMINE
		//soundBase::FORCEFLD
		//soundBase::fireWall

		auto animation = std::make_shared<CAnimation>(defname);
		animation->preload();

		auto first = animation->getImage(0, 0);
		if(!first)
		{
			obstaclesBeingPlaced.erase(obstaclesBeingPlaced.begin());
			continue;
		}

		//we assume here that effect graphics have the same size as the usual obstacle image
		// -> if we know how to blit obstacle, let's blit the effect in the same place
		Point whereTo = getObstaclePosition(first, *oi);
		owner->stacksController->addNewAnim(new CPointEffectAnimation(owner, soundBase::QUIKSAND, defname, whereTo, CPointEffectAnimation::WAIT_FOR_SOUND));

		//so when multiple obstacles are added, they show up one after another
		owner->waitForAnims();

		obstaclesBeingPlaced.erase(obstaclesBeingPlaced.begin());
		loadObstacleImage(*spellObstacle);
	}
}

void CBattleObstacleController::showAbsoluteObstacles(std::shared_ptr<CCanvas> canvas, const Point & offset)
{
	//Blit absolute obstacles
	for(auto & oi : owner->curInt->cb->battleGetAllObstacles())
	{
		if(oi->obstacleType == CObstacleInstance::ABSOLUTE_OBSTACLE)
		{
			auto img = getObstacleImage(*oi);
			if(img)
				canvas->draw(img, Point(offset.x + oi->getInfo().width, offset.y + oi->getInfo().height));
		}
	}
}

void CBattleObstacleController::showBattlefieldObjects(std::shared_ptr<CCanvas> canvas, const BattleHex & location )
{
	for (auto &obstacle : owner->curInt->cb->battleGetAllObstacles())
	{
		if (obstacle->obstacleType == CObstacleInstance::ABSOLUTE_OBSTACLE)
			continue;

		if (obstacle->obstacleType == CObstacleInstance::MOAT)
			continue;

		if ( obstacle->pos != location)
			continue;

		auto img = getObstacleImage(*obstacle);
		if(img)
		{
			Point p = getObstaclePosition(img, *obstacle);
			canvas->draw(img, p);
		}
	}
}

std::shared_ptr<IImage> CBattleObstacleController::getObstacleImage(const CObstacleInstance & oi)
{
	int frameIndex = (owner->animCount+1) *25 / owner->getAnimSpeed();
	std::shared_ptr<CAnimation> animation;

	if (obstacleAnimations.count(oi.uniqueID) == 0)
	{
		if (boost::range::find(obstaclesBeingPlaced, oi.uniqueID) != obstaclesBeingPlaced.end())
		{
			// obstacle is not loaded yet, don't show anything
			return nullptr;
		}
		else
		{
			assert(0); // how?
			loadObstacleImage(oi);
		}
	}

	animation = obstacleAnimations[oi.uniqueID];
	assert(animation);

	if(animation)
	{
		frameIndex %= animation->size(0);
		return animation->getImage(frameIndex, 0);
	}
	return nullptr;
}

Point CBattleObstacleController::getObstaclePosition(std::shared_ptr<IImage> image, const CObstacleInstance & obstacle)
{
	int offset = obstacle.getAnimationYOffset(image->height());

	Rect r = owner->fieldController->hexPosition(obstacle.pos);
	r.y += 42 - image->height() + offset;

	return r.topLeft();
}
