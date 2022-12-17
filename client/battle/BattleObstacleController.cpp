/*
 * BattleObstacleController.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleObstacleController.h"

#include "BattleInterface.h"
#include "BattleFieldController.h"
#include "BattleAnimationClasses.h"
#include "BattleStacksController.h"
#include "BattleRenderer.h"

#include "../CPlayerInterface.h"
#include "../gui/CAnimation.h"
#include "../gui/Canvas.h"

#include "../../CCallback.h"
#include "../../lib/battle/CObstacleInstance.h"
#include "../../lib/ObstacleHandler.h"

BattleObstacleController::BattleObstacleController(BattleInterface & owner):
	owner(owner)
{
	auto obst = owner.curInt->cb->battleGetAllObstacles();
	for(auto & elem : obst)
	{
		if ( elem->obstacleType == CObstacleInstance::MOAT )
			continue; // handled by siege controller;
		loadObstacleImage(*elem);
	}
}

void BattleObstacleController::loadObstacleImage(const CObstacleInstance & oi)
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
			// obstacle uses single bitmap image for animations
			auto animation = std::make_shared<CAnimation>();
			animation->setCustom(animationName, 0, 0);
			animationsCache[animationName] = animation;
			animation->preload();
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

void BattleObstacleController::obstaclePlaced(const std::vector<std::shared_ptr<const CObstacleInstance>> & obstacles)
{
	for (auto const & oi : obstacles)
	{
		auto spellObstacle = dynamic_cast<const SpellCreatedObstacle*>(oi.get());

		if (!spellObstacle)
		{
			logGlobal->error("I don't know how to animate appearing obstacle of type %d", (int)oi->obstacleType);
			continue;
		}

		auto animation = std::make_shared<CAnimation>(spellObstacle->appearAnimation);
		animation->preload();

		auto first = animation->getImage(0, 0);
		if(!first)
			continue;

		//we assume here that effect graphics have the same size as the usual obstacle image
		// -> if we know how to blit obstacle, let's blit the effect in the same place
		Point whereTo = getObstaclePosition(first, *oi);
		owner.stacksController->addNewAnim(new PointEffectAnimation(owner, spellObstacle->appearSound, spellObstacle->appearAnimation, whereTo, oi->pos));

		//so when multiple obstacles are added, they show up one after another
		owner.waitForAnimationCondition(EAnimationEvents::ACTION, false);

		loadObstacleImage(*spellObstacle);
	}
}

void BattleObstacleController::showAbsoluteObstacles(Canvas & canvas, const Point & offset)
{
	//Blit absolute obstacles
	for(auto & oi : owner.curInt->cb->battleGetAllObstacles())
	{
		if(oi->obstacleType == CObstacleInstance::ABSOLUTE_OBSTACLE)
		{
			auto img = getObstacleImage(*oi);
			if(img)
				canvas.draw(img, Point(offset.x + oi->getInfo().width, offset.y + oi->getInfo().height));
		}
	}
}

void BattleObstacleController::collectRenderableObjects(BattleRenderer & renderer)
{
	for (auto obstacle : owner.curInt->cb->battleGetAllObstacles())
	{
		if (obstacle->obstacleType == CObstacleInstance::ABSOLUTE_OBSTACLE)
			continue;

		if (obstacle->obstacleType == CObstacleInstance::MOAT)
			continue;

		renderer.insert(EBattleFieldLayer::OBSTACLES, obstacle->pos, [this, obstacle]( BattleRenderer::RendererRef canvas ){
			auto img = getObstacleImage(*obstacle);
			if(img)
			{
				Point p = getObstaclePosition(img, *obstacle);
				canvas.draw(img, p);
			}
		});
	}
}

std::shared_ptr<IImage> BattleObstacleController::getObstacleImage(const CObstacleInstance & oi)
{
	int frameIndex = (owner.animCount+1) *25 / owner.getAnimSpeed();
	std::shared_ptr<CAnimation> animation;

	// obstacle is not loaded yet, don't show anything
	if (obstacleAnimations.count(oi.uniqueID) == 0)
		return nullptr;

	animation = obstacleAnimations[oi.uniqueID];
	assert(animation);

	if(animation)
	{
		frameIndex %= animation->size(0);
		return animation->getImage(frameIndex, 0);
	}
	return nullptr;
}

Point BattleObstacleController::getObstaclePosition(std::shared_ptr<IImage> image, const CObstacleInstance & obstacle)
{
	int offset = obstacle.getAnimationYOffset(image->height());

	Rect r = owner.fieldController->hexPositionAbsolute(obstacle.pos);
	r.y += 42 - image->height() + offset;

	return r.topLeft();
}
