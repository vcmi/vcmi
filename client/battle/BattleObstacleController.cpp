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
#include "CreatureAnimation.h"

#include "../CPlayerInterface.h"
#include "../GameEngine.h"
#include "../media/ISoundPlayer.h"
#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/IRenderHandler.h"

#include "../../lib/battle/CObstacleInstance.h"
#include "../../lib/ObstacleHandler.h"
#include "../../lib/battle/CPlayerBattleCallback.h"
#include "../../lib/serializer/JsonDeserializer.h"

// how long a removed "usual" obstacle takes to fade out, in ms
static constexpr uint32_t obstacleFadeDuration = 500;

BattleObstacleController::BattleObstacleController(BattleInterface & owner):
	owner(owner),
	timePassed(0.f)
{
	auto obst = owner.getBattle()->battleGetAllObstacles();
	for(auto & elem : obst)
	{
		if ( elem->obstacleType == CObstacleInstance::MOAT )
			continue; // handled by siege controller;
		loadObstacleImage(*elem);
	}
}

void BattleObstacleController::loadObstacleImage(const CObstacleInstance & oi)
{
	AnimationPath animationName = oi.getAnimation();

	if (oi.obstacleType == CObstacleInstance::ABSOLUTE_OBSTACLE)
	{
		// obstacle uses single bitmap image for animations
		obstacleImages[oi.uniqueID] = ENGINE->renderHandler().loadImage(animationName.toType<EResType::IMAGE>(), EImageBlitMode::SIMPLE);
	}
	else
	{
		obstacleAnimations[oi.uniqueID] = ENGINE->renderHandler().loadAnimation(animationName, EImageBlitMode::SIMPLE);
		obstacleImages[oi.uniqueID] = obstacleAnimations[oi.uniqueID]->getImage(0);
	}

	// cache the render position so a fade-out after removal matches where the obstacle was actually drawn
	if(obstacleImages[oi.uniqueID])
		obstaclePositions[oi.uniqueID] = getObstaclePosition(obstacleImages[oi.uniqueID], oi);
}

void BattleObstacleController::obstacleRemoved(const ObstacleChanges & oi)
{
	auto & obstacle = oi.data["obstacle"];

	if (obstacle.isStruct())
	{
		AnimationPath animationPath;
		JsonDeserializer ser(nullptr, obstacle);
		ser.serializeStruct("removalAnimation", animationPath);
		if(animationPath.empty()) // magical obstacles without a dedicated removal effect reuse their appear animation
			ser.serializeStruct("appearAnimation", animationPath);

		if(!animationPath.empty())
		{
			auto animation = ENGINE->renderHandler().loadAnimation(animationPath, EImageBlitMode::SIMPLE);
			auto first = animation->getImage(0, 0);
			if(first)
			{
				Point whereTo = getObstaclePosition(first, obstacle);
				//AFAIK, in H3 there is no sound of obstacle removal
				owner.stacksController->addNewAnim(new EffectAnimation(owner, animationPath, whereTo, obstacle["position"].Integer(), 0, true));
				obstacleAnimations.erase(oi.id);
				obstacleImages.erase(oi.id);
				obstaclePositions.erase(oi.id);
				owner.waitForAnimations();
				return;
			}
		}
		else if(obstacleImages.count(oi.id) && obstacleImages[oi.id])
		{
			// "usual" obstacles (rock/tree) have no removal animation - fade their sprite out instead.
			// hold at full opacity until the removing spell's hit stage so fade and spell effect coincide
			FadingObstacle fade;
			fade.image = obstacleImages[oi.id];
			fade.pos = obstaclePositions.count(oi.id) ? obstaclePositions[oi.id] : getObstaclePosition(obstacleImages[oi.id], obstacle);
			fade.hex = BattleHex(static_cast<si16>(obstacle["position"].Integer()));
			fade.id = oi.id;
			fade.started = !owner.hasQueuedStage(EAnimationEvents::HIT);
			fadingObstacles.push_back(fade);

			if(!fade.started)
			{
				auto obstacleId = oi.id;
				owner.addToAnimationStage(EAnimationEvents::HIT, [this, obstacleId](){
					for(auto & f : fadingObstacles)
						if(f.id == obstacleId)
							f.started = true;
				});
			}
		}
	}
	else
	{
		logGlobal->error("I don't know how to animate removal of this obstacle");
	}

	// fade-out path (and error/no-op paths) run without a blocking animation, so no waitForAnimations here
	obstacleAnimations.erase(oi.id);
	obstacleImages.erase(oi.id);
	obstaclePositions.erase(oi.id);
}

void BattleObstacleController::obstaclePlaced(const std::shared_ptr<const CObstacleInstance> & oi)
{
	auto side = owner.getBattle()->playerToSide(owner.curInt->playerID);

	// second animation from the same spell - enqueue it
	if(oi->visibleForSide(side, owner.getBattle()->battleHasNativeStack(side)))
		obstaclePlacementQueue.push_back(oi);

	placeNextObstacle();
}

void BattleObstacleController::placeNextObstacle()
{
	if(placingObstacle || obstaclePlacementQueue.empty())
		return;

	auto oi = obstaclePlacementQueue.front();
	obstaclePlacementQueue.erase(obstaclePlacementQueue.begin());

	auto animation = ENGINE->renderHandler().loadAnimation(oi->getAppearAnimation(), EImageBlitMode::SIMPLE);
	auto first = animation->getImage(0, 0);
	if(!first)
	{
		placeNextObstacle();
		return;
	}

	Point whereTo = getObstaclePosition(first, *oi);
	ENGINE->sound().playSound( oi->getAppearSound() );

	placingObstacle = true;
	auto effect = new EffectAnimation(owner, oi->getAppearAnimation(), whereTo, oi->pos);
	effect->onFinished = [this, oi]()
	{
		loadObstacleImage(*oi);
		placingObstacle = false;
		placeNextObstacle();
	};
	owner.stacksController->addNewAnim(effect);
}

void BattleObstacleController::showAbsoluteObstacles(Canvas & canvas)
{
	//Blit absolute obstacles
	for(auto & obstacle : owner.getBattle()->battleGetAllObstacles())
	{
		if(obstacle->obstacleType == CObstacleInstance::ABSOLUTE_OBSTACLE)
		{
			auto img = getObstacleImage(*obstacle);
			if(img)
				canvas.draw(img, Point(obstacle->getInfo().width, obstacle->getInfo().height));
		}

		if (obstacle->obstacleType == CObstacleInstance::USUAL)
		{
			if (obstacle->getInfo().isForegroundObstacle)
				continue;

			auto img = getObstacleImage(*obstacle);
			if(img)
			{
				Point p = getObstaclePosition(img, *obstacle);
				canvas.draw(img, p);
			}
		}
	}
}

void BattleObstacleController::collectRenderableObjects(BattleRenderer & renderer)
{
	for (auto obstacle : owner.getBattle()->battleGetAllObstacles())
	{
		if (obstacle->obstacleType == CObstacleInstance::ABSOLUTE_OBSTACLE)
			continue;

		if (obstacle->obstacleType == CObstacleInstance::MOAT)
			continue;

		if (obstacle->obstacleType == CObstacleInstance::USUAL && !obstacle->getInfo().isForegroundObstacle)
			continue;

		// passable obstacles (quicksand, landmine, ...) share their hex with a standing
		// creature and must draw below it; blocking obstacles stay above via row ordering
		auto layer = obstacle->blocksTiles() ? EBattleFieldLayer::OBSTACLES : EBattleFieldLayer::GROUND_OBSTACLES;

		renderer.insert(layer, obstacle->pos, [this, obstacle]( BattleRenderer::RendererRef canvas ){
			auto img = getObstacleImage(*obstacle);
			if(img)
			{
				Point p = getObstaclePosition(img, *obstacle);
				canvas.draw(img, p);
			}
		});
	}

	for(const auto & fade : fadingObstacles)
	{
		auto image = fade.image;
		Point pos = fade.pos;
		auto alpha = fade.started ? static_cast<uint8_t>(std::clamp(1.f - static_cast<float>(fade.elapsed) / obstacleFadeDuration, 0.f, 1.f) * 255) : 255;
		renderer.insert(EBattleFieldLayer::OBSTACLES, fade.hex, [image, pos, alpha](BattleRenderer::RendererRef canvas){
			image->setAlpha(alpha);
			canvas.draw(image, pos);
		});
	}
}

void BattleObstacleController::tick(uint32_t msPassed)
{
	timePassed += msPassed / 1000.f;
	int framesCount = timePassed * AnimationControls::getObstaclesSpeed();

	for(auto & animation : obstacleAnimations)
	{
		int frameIndex = framesCount % animation.second->size(0);
		obstacleImages[animation.first] = animation.second->getImage(frameIndex, 0);
	}

	for(auto & fade : fadingObstacles)
		if(fade.started)
			fade.elapsed += msPassed;
	vstd::erase_if(fadingObstacles, [](const FadingObstacle & f){ return f.started && f.elapsed >= obstacleFadeDuration; });
}

std::shared_ptr<IImage> BattleObstacleController::getObstacleImage(const CObstacleInstance & oi)
{
	// obstacle is not loaded yet, don't show anything
	if (obstacleImages.count(oi.uniqueID) == 0)
		return nullptr;

	return obstacleImages[oi.uniqueID];
}

Point BattleObstacleController::getObstaclePosition(std::shared_ptr<IImage> image, const CObstacleInstance & obstacle)
{
	Rect r = owner.fieldController->hexPositionLocal(obstacle.pos);
	return r.bottomLeft() - Point(0, obstacle.getAnimationYOffset(image->height()));
}

Point BattleObstacleController::getObstaclePosition(std::shared_ptr<IImage> image, const JsonNode & obstacle)
{
	Rect r = owner.fieldController->hexPositionLocal(obstacle["position"].Integer());
	return r.bottomLeft() - Point(0, image->height());
}
