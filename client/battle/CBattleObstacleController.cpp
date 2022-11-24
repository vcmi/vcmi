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
#include "../../CCallback.h"
#include "../../lib/battle/CObstacleInstance.h"
#include "../../lib/ObstacleHandler.h"
#include "../gui/CAnimation.h"

CBattleObstacleController::CBattleObstacleController(CBattleInterface * owner):
	owner(owner)
{
	auto obst = owner->curInt->cb->battleGetAllObstacles();
	for(auto & elem : obst)
	{
		if(elem->obstacleType == CObstacleInstance::USUAL)
		{
			std::string animationName = elem->getInfo().animation;

			auto cached = animationsCache.find(animationName);

			if(cached == animationsCache.end())
			{
				auto animation = std::make_shared<CAnimation>(animationName);
				animationsCache[animationName] = animation;
				obstacleAnimations[elem->uniqueID] = animation;
				animation->preload();
			}
			else
			{
				obstacleAnimations[elem->uniqueID] = cached->second;
			}
		}
		else if (elem->obstacleType == CObstacleInstance::ABSOLUTE_OBSTACLE)
		{
			std::string animationName = elem->getInfo().animation;

			auto cached = animationsCache.find(animationName);

			if(cached == animationsCache.end())
			{
				auto animation = std::make_shared<CAnimation>();
				animation->setCustom(animationName, 0, 0);
				animationsCache[animationName] = animation;
				obstacleAnimations[elem->uniqueID] = animation;
				animation->preload();
			}
			else
			{
				obstacleAnimations[elem->uniqueID] = cached->second;
			}
		}
	}
}

void CBattleObstacleController::obstaclePlaced(const CObstacleInstance & oi)
{
	//so when multiple obstacles are added, they show up one after another
	owner->waitForAnims();

	//soundBase::soundID sound; // FIXME(v.markovtsev): soundh->playSound() is commented in the end => warning

	std::string defname;

	switch(oi.obstacleType)
	{
	case CObstacleInstance::SPELL_CREATED:
		{
			auto &spellObstacle = dynamic_cast<const SpellCreatedObstacle&>(oi);
			defname = spellObstacle.appearAnimation;
			//TODO: sound
			//soundBase::QUIKSAND
			//soundBase::LANDMINE
			//soundBase::FORCEFLD
			//soundBase::fireWall
		}
		break;
	default:
		logGlobal->error("I don't know how to animate appearing obstacle of type %d", (int)oi.obstacleType);
		return;
	}

	auto animation = std::make_shared<CAnimation>(defname);
	animation->preload();

	auto first = animation->getImage(0, 0);
	if(!first)
		return;

	//we assume here that effect graphics have the same size as the usual obstacle image
	// -> if we know how to blit obstacle, let's blit the effect in the same place
	Point whereTo = getObstaclePosition(first, oi);
	owner->stacksController->addNewAnim(new CEffectAnimation(owner, animation, whereTo.x, whereTo.y));

	//TODO we need to wait after playing sound till it's finished, otherwise it overlaps and sounds really bad
	//CCS->soundh->playSound(sound);
}

void CBattleObstacleController::showAbsoluteObstacles(SDL_Surface * to)
{
	//Blit absolute obstacles
	for(auto & oi : owner->curInt->cb->battleGetAllObstacles())
	{
		if(oi->obstacleType == CObstacleInstance::ABSOLUTE_OBSTACLE)
		{
			auto img = getObstacleImage(*oi);
			if(img)
				img->draw(to, owner->pos.x + oi->getInfo().width, owner->pos.y + oi->getInfo().height);
		}
	}
}

void CBattleObstacleController::showObstacles(SDL_Surface * to, std::vector<std::shared_ptr<const CObstacleInstance>> & obstacles)
{
	for(auto & obstacle : obstacles)
	{
		auto img = getObstacleImage(*obstacle);
		if(img)
		{
			Point p = getObstaclePosition(img, *obstacle);
			img->draw(to, p.x, p.y);
		}
	}
}

void CBattleObstacleController::sortObjectsByHex(BattleObjectsByHex & sorted)
{
	std::map<BattleHex, std::shared_ptr<const CObstacleInstance>> backgroundObstacles;
	for (auto &obstacle : owner->curInt->cb->battleGetAllObstacles()) {
		if (obstacle->obstacleType != CObstacleInstance::ABSOLUTE_OBSTACLE
			&& obstacle->obstacleType != CObstacleInstance::MOAT) {
			backgroundObstacles[obstacle->pos] = obstacle;
		}
	}
	for (auto &op : backgroundObstacles)
	{
		sorted.beforeAll.obstacles.push_back(op.second);
	}
}


std::shared_ptr<IImage> CBattleObstacleController::getObstacleImage(const CObstacleInstance & oi)
{
	int frameIndex = (owner->animCount+1) *25 / owner->getAnimSpeed();
	std::shared_ptr<CAnimation> animation;

	if(oi.obstacleType == CObstacleInstance::USUAL || oi.obstacleType == CObstacleInstance::ABSOLUTE_OBSTACLE)
	{
		animation = obstacleAnimations[oi.uniqueID];
	}
	else if(oi.obstacleType == CObstacleInstance::SPELL_CREATED)
	{
		const SpellCreatedObstacle * spellObstacle = dynamic_cast<const SpellCreatedObstacle *>(&oi);
		if(!spellObstacle)
			return std::shared_ptr<IImage>();

		std::string animationName = spellObstacle->animation;

		auto cacheIter = animationsCache.find(animationName);

		if(cacheIter == animationsCache.end())
		{
			logAnim->trace("Creating obstacle animation %s", animationName);

			animation = std::make_shared<CAnimation>(animationName);
			animation->preload();
			animationsCache[animationName] = animation;
		}
		else
		{
			animation = cacheIter->second;
		}
	}

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

void CBattleObstacleController::redrawBackgroundWithHexes(SDL_Surface * to)
{
	//draw absolute obstacles (cliffs and so on)
	for(auto & oi : owner->curInt->cb->battleGetAllObstacles())
	{
		if(oi->obstacleType == CObstacleInstance::ABSOLUTE_OBSTACLE)
		{
			auto img = getObstacleImage(*oi);
			if(img)
				img->draw(to, oi->getInfo().width, oi->getInfo().height);
		}
	}
}
