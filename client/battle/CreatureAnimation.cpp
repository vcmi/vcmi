/*
 * CCreatureAnimation.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CreatureAnimation.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CCreatureHandler.h"

#include "../GameEngine.h"
#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/ColorFilter.h"
#include "../render/Colors.h"
#include "../render/IRenderHandler.h"

static const ColorRGBA creatureBlueBorder = { 0, 255, 255, 255 };
static const ColorRGBA creatureGoldBorder = { 255, 255, 0, 255 };
static const ColorRGBA creatureNoBorder  =  { 0, 0, 0, 0 };

ColorRGBA AnimationControls::getBlueBorder()
{
	return creatureBlueBorder;
}

ColorRGBA AnimationControls::getGoldBorder()
{
	return creatureGoldBorder;
}

ColorRGBA AnimationControls::getNoBorder()
{
	return creatureNoBorder;
}

std::shared_ptr<CreatureAnimation> AnimationControls::getAnimation(const CCreature * creature)
{
	auto func = std::bind(&AnimationControls::getCreatureAnimationSpeed, creature, _1, _2);
	return std::make_shared<CreatureAnimation>(creature->animDefName, func);
}

float AnimationControls::getAnimationSpeedFactor()
{
	// according to testing, H3 ratios between slow/medium/fast might actually be 36/60/100 (x1.666)
	// exact value is hard to tell due to large rounding errors
	// however we will assume them to be 33/66/100 since these values are better for standard 60 fps displays:
	// with these numbers, base frame display duration will be 100/66/33 ms - exactly 6/4/2 frames
	return settings["battle"]["speedFactor"].Float();
}

float AnimationControls::getCreatureAnimationSpeed(const CCreature * creature, const CreatureAnimation * anim, ECreatureAnimType type)
{
	assert(creature->animation.walkAnimationTime != 0);
	assert(creature->animation.attackAnimationTime != 0);
	assert(anim->framesInGroup(type) != 0);

	// possible new fields for creature format:
	//split "Attack time" into "Shoot Time" and "Cast Time"

	// base speed for all H3 animations on slow speed is 10 frames per second (or 100ms per frame)
	const float baseSpeed = 10.f;
	const float speed = baseSpeed * getAnimationSpeedFactor();

	switch (type)
	{
	case ECreatureAnimType::MOVING:
		return speed / creature->animation.walkAnimationTime;

	case ECreatureAnimType::MOUSEON:
		return baseSpeed;

	case ECreatureAnimType::HOLDING:
			return creature->animation.idleAnimationTime;

	case ECreatureAnimType::SHOOT_UP:
	case ECreatureAnimType::SHOOT_FRONT:
	case ECreatureAnimType::SHOOT_DOWN:
	case ECreatureAnimType::SPECIAL_UP:
	case ECreatureAnimType::SPECIAL_FRONT:
	case ECreatureAnimType::SPECIAL_DOWN:
	case ECreatureAnimType::CAST_DOWN:
	case ECreatureAnimType::CAST_FRONT:
	case ECreatureAnimType::CAST_UP:
		return speed / creature->animation.attackAnimationTime;

	// as strange as it looks like "attackAnimationTime" does not affects melee attacks
	// necessary because length of these animations must be same for all creatures for synchronization
	case ECreatureAnimType::ATTACK_UP:
	case ECreatureAnimType::ATTACK_FRONT:
	case ECreatureAnimType::ATTACK_DOWN:
	case ECreatureAnimType::HITTED:
	case ECreatureAnimType::DEFENCE:
	case ECreatureAnimType::DEATH:
	case ECreatureAnimType::DEATH_RANGED:
	case ECreatureAnimType::RESURRECTION:
	case ECreatureAnimType::GROUP_ATTACK_DOWN:
	case ECreatureAnimType::GROUP_ATTACK_FRONT:
	case ECreatureAnimType::GROUP_ATTACK_UP:
		return speed;

	case ECreatureAnimType::TURN_L:
	case ECreatureAnimType::TURN_R:
		return speed;

	case ECreatureAnimType::MOVE_START:
	case ECreatureAnimType::MOVE_END:
		return speed;

	case ECreatureAnimType::DEAD:
	case ECreatureAnimType::DEAD_RANGED:
		return speed;

	default:
		return speed;
	}
}

float AnimationControls::getProjectileSpeed()
{
	// H3 speed: 1250/2500/3750 pixels per second
	return static_cast<float>(getAnimationSpeedFactor() * 1250);
}

float AnimationControls::getRayProjectileSpeed()
{
	// H3 speed: 4000/8000/12000 pixels per second
	return static_cast<float>(getAnimationSpeedFactor() * 4000);
}

float AnimationControls::getCatapultSpeed()
{
	// H3 speed: 200/400/600 pixels per second
	return static_cast<float>(getAnimationSpeedFactor() * 200);
}

float AnimationControls::getSpellEffectSpeed()
{
	// H3 speed: 10/20/30 frames per second
	return static_cast<float>(getAnimationSpeedFactor() * 10);
}

float AnimationControls::getMovementRange(const CCreature * creature)
{
	// H3 speed: 2/4/6 tiles per second
	return static_cast<float>( 2.0 * getAnimationSpeedFactor() / creature->animation.walkAnimationTime);
}

float AnimationControls::getFlightDistance(const CCreature * creature)
{
	// Note: for whatever reason, H3 uses "Walk Animation Time" here, even though "Flight Animation Distance" also exists
	// H3 speed: 250/500/750 pixels per second
	return static_cast<float>( 250.0 * getAnimationSpeedFactor() / creature->animation.walkAnimationTime);
}

float AnimationControls::getFadeInDuration()
{
	// H3 speed: 500/250/166 ms
	return 0.5f / getAnimationSpeedFactor();
}

float AnimationControls::getObstaclesSpeed()
{
	// H3 speed: 20 frames per second, irregardless of speed setting.
	return 20.f;
}

ECreatureAnimType CreatureAnimation::getType() const
{
	return type;
}

void CreatureAnimation::setType(ECreatureAnimType type)
{
	this->type = type;
	currentFrame = 0;
	once = false;

	speed = speedController(this, type);
}

CreatureAnimation::CreatureAnimation(const AnimationPath & name_, TSpeedController controller)
	: name(name_),
	  speed(0.1f),
	  currentFrame(0),
	  animationEnd(-1),
	  elapsedTime(0),
	  type(ECreatureAnimType::HOLDING),
	  speedController(controller),
	  once(false)
{

	forward = ENGINE->renderHandler().loadAnimation(name_, EImageBlitMode::WITH_SHADOW_AND_SELECTION);
	reverse = ENGINE->renderHandler().loadAnimation(name_, EImageBlitMode::WITH_SHADOW_AND_SELECTION);

	if (forward->size(size_t(ECreatureAnimType::DEATH)) == 0)
		throw std::runtime_error("Animation '" + name_.getOriginalName() + "' has empty death animation!");

	if (forward->size(size_t(ECreatureAnimType::HOLDING)) == 0)
		throw std::runtime_error("Animation '" + name_.getOriginalName() + "' has empty holding animation!");

	// if necessary, add one frame into vcmi-only group DEAD
	if(forward->size(size_t(ECreatureAnimType::DEAD)) == 0)
	{
		forward->duplicateImage(size_t(ECreatureAnimType::DEATH), forward->size(size_t(ECreatureAnimType::DEATH))-1, size_t(ECreatureAnimType::DEAD));
		reverse->duplicateImage(size_t(ECreatureAnimType::DEATH), reverse->size(size_t(ECreatureAnimType::DEATH))-1, size_t(ECreatureAnimType::DEAD));
	}

	if(forward->size(size_t(ECreatureAnimType::DEAD_RANGED)) == 0 && forward->size(size_t(ECreatureAnimType::DEATH_RANGED)) != 0)
	{
		forward->duplicateImage(size_t(ECreatureAnimType::DEATH_RANGED), forward->size(size_t(ECreatureAnimType::DEATH_RANGED))-1, size_t(ECreatureAnimType::DEAD_RANGED));
		reverse->duplicateImage(size_t(ECreatureAnimType::DEATH_RANGED), reverse->size(size_t(ECreatureAnimType::DEATH_RANGED))-1, size_t(ECreatureAnimType::DEAD_RANGED));
	}

	if(forward->size(size_t(ECreatureAnimType::FROZEN)) == 0)
	{
		forward->duplicateImage(size_t(ECreatureAnimType::HOLDING), 0, size_t(ECreatureAnimType::FROZEN));
		reverse->duplicateImage(size_t(ECreatureAnimType::HOLDING), 0, size_t(ECreatureAnimType::FROZEN));
	}

	if(forward->size(size_t(ECreatureAnimType::RESURRECTION)) == 0)
	{
		for (size_t i = 0; i < forward->size(size_t(ECreatureAnimType::DEATH)); ++i)
		{
			size_t current = forward->size(size_t(ECreatureAnimType::DEATH)) - 1 - i;

			forward->duplicateImage(size_t(ECreatureAnimType::DEATH), current, size_t(ECreatureAnimType::RESURRECTION));
			reverse->duplicateImage(size_t(ECreatureAnimType::DEATH), current, size_t(ECreatureAnimType::RESURRECTION));
		}
	}

	//TODO: get dimensions form CAnimation
	auto first = forward->getImage(0, size_t(type), true);

	if(!first)
	{
		fullWidth = 0;
		fullHeight = 0;
		return;
	}
	fullWidth = first->width();
	fullHeight = first->height();

	reverse->verticalFlip();

	speed = speedController(this, type);
}

void CreatureAnimation::endAnimation()
{
	once = false;
	auto copy = onAnimationReset;
	onAnimationReset.clear();
	copy();
}

bool CreatureAnimation::incrementFrame(float timePassed)
{
	elapsedTime += timePassed;
	currentFrame += timePassed * speed;
	if (animationEnd >= 0)
		currentFrame = std::min(currentFrame, animationEnd);

	const auto framesNumber = framesInGroup(type);

	if(framesNumber <= 0)
	{
		endAnimation();
	}
	else if(currentFrame >= float(framesNumber))
	{
		// just in case of extremely low fps (or insanely high speed)
		while(currentFrame >= float(framesNumber))
			currentFrame -= framesNumber;

		if(once)
			setType(ECreatureAnimType::HOLDING);

		endAnimation();
		return true;
	}
	return false;
}

void CreatureAnimation::setBorderColor(ColorRGBA palette)
{
	border = palette;
}

int CreatureAnimation::getWidth() const
{
	return fullWidth;
}

int CreatureAnimation::getHeight() const
{
	return fullHeight;
}

float CreatureAnimation::getCurrentFrame() const
{
	return currentFrame;
}

void CreatureAnimation::playOnce( ECreatureAnimType type )
{
	setType(type);
	once = true;
}

inline int getBorderStrength(float time)
{
	float borderStrength = fabs(std::round(time) - time) * 2; // generate value in range 0-1

	return static_cast<int>(borderStrength * 155 + 100); // scale to 0-255
}

static ColorRGBA genBorderColor(ui8 alpha, const ColorRGBA & base)
{
	return ColorRGBA(base.r, base.g, base.b, ui8(base.a * alpha / 256));
}

void CreatureAnimation::nextFrame(Canvas & canvas, const ColorRGBA & effectColor, uint8_t transparency, bool facingRight)
{
	size_t frame = static_cast<size_t>(floor(currentFrame));

	std::shared_ptr<IImage> image;

	if(facingRight)
		image = forward->getImage(frame, size_t(type));
	else
		image = reverse->getImage(frame, size_t(type));

	if(image)
	{
		if (isIdle())
			image->setOverlayColor(genBorderColor(getBorderStrength(elapsedTime), border));
		else
			image->setOverlayColor(Colors::TRANSPARENCY);

		image->setEffectColor(effectColor);
		image->setAlpha(transparency);

		canvas.draw(image, pos.topLeft(), Rect(0, 0, pos.w, pos.h));
	}
}

void CreatureAnimation::playUntil(size_t frameIndex)
{
	animationEnd = frameIndex;
}

int CreatureAnimation::framesInGroup(ECreatureAnimType group) const
{
	return static_cast<int>(forward->size(size_t(group)));
}

bool CreatureAnimation::isDead() const
{
	return getType() == ECreatureAnimType::DEAD
		|| getType() == ECreatureAnimType::DEAD_RANGED;
}

bool CreatureAnimation::isDying() const
{
	return getType() == ECreatureAnimType::DEATH
		|| getType() == ECreatureAnimType::DEATH_RANGED;
}

bool CreatureAnimation::isDeadOrDying() const
{
	return getType() == ECreatureAnimType::DEAD
		|| getType() == ECreatureAnimType::DEATH
		|| getType() == ECreatureAnimType::DEAD_RANGED
		|| getType() == ECreatureAnimType::DEATH_RANGED;
}

bool CreatureAnimation::isIdle() const
{
	return getType() == ECreatureAnimType::HOLDING
		|| getType() == ECreatureAnimType::MOUSEON;
}

bool CreatureAnimation::isMoving() const
{
	return getType() == ECreatureAnimType::MOVE_START
		|| getType() == ECreatureAnimType::MOVING
		|| getType() == ECreatureAnimType::MOVE_END
		|| getType() == ECreatureAnimType::TURN_L
		|| getType() == ECreatureAnimType::TURN_R;
}

bool CreatureAnimation::isShooting() const
{
	return getType() == ECreatureAnimType::SHOOT_UP
		|| getType() == ECreatureAnimType::SHOOT_FRONT
		|| getType() == ECreatureAnimType::SHOOT_DOWN;
}
