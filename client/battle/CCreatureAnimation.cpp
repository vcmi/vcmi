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
#include "CCreatureAnimation.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CCreatureHandler.h"

#include "../gui/CCanvas.h"

static const SDL_Color creatureBlueBorder = { 0, 255, 255, 255 };
static const SDL_Color creatureGoldBorder = { 255, 255, 0, 255 };
static const SDL_Color creatureNoBorder  =  { 0, 0, 0, 0 };

SDL_Color AnimationControls::getBlueBorder()
{
	return creatureBlueBorder;
}

SDL_Color AnimationControls::getGoldBorder()
{
	return creatureGoldBorder;
}

SDL_Color AnimationControls::getNoBorder()
{
	return creatureNoBorder;
}

std::shared_ptr<CCreatureAnimation> AnimationControls::getAnimation(const CCreature * creature)
{
	auto func = std::bind(&AnimationControls::getCreatureAnimationSpeed, creature, _1, _2);
	return std::make_shared<CCreatureAnimation>(creature->animDefName, func);
}

float AnimationControls::getCreatureAnimationSpeed(const CCreature * creature, const CCreatureAnimation * anim, size_t group)
{
	CCreatureAnim::EAnimType type = CCreatureAnim::EAnimType(group);

	assert(creature->animation.walkAnimationTime != 0);
	assert(creature->animation.attackAnimationTime != 0);
	assert(anim->framesInGroup(type) != 0);

	// possible new fields for creature format:
	//split "Attack time" into "Shoot Time" and "Cast Time"

	// a lot of arbitrary multipliers, mostly to make animation speed closer to H3
	const float baseSpeed = 0.1f;
	const float speedMult = static_cast<float>(settings["battle"]["animationSpeed"].Float());
	const float speed = baseSpeed / speedMult;

	switch (type)
	{
	case CCreatureAnim::MOVING:
		return static_cast<float>(speed * 2 * creature->animation.walkAnimationTime / anim->framesInGroup(type));

	case CCreatureAnim::MOUSEON:
		return baseSpeed;
	case CCreatureAnim::HOLDING:
		return static_cast<float>(baseSpeed * creature->animation.idleAnimationTime / anim->framesInGroup(type));

	case CCreatureAnim::SHOOT_UP:
	case CCreatureAnim::SHOOT_FRONT:
	case CCreatureAnim::SHOOT_DOWN:
	case CCreatureAnim::CAST_UP:
	case CCreatureAnim::CAST_FRONT:
	case CCreatureAnim::CAST_DOWN:
	case CCreatureAnim::VCMI_CAST_DOWN:
	case CCreatureAnim::VCMI_CAST_FRONT:
	case CCreatureAnim::VCMI_CAST_UP:
		return static_cast<float>(speed * 4 * creature->animation.attackAnimationTime / anim->framesInGroup(type));

	// as strange as it looks like "attackAnimationTime" does not affects melee attacks
	// necessary because length of these animations must be same for all creatures for synchronization
	case CCreatureAnim::ATTACK_UP:
	case CCreatureAnim::ATTACK_FRONT:
	case CCreatureAnim::ATTACK_DOWN:
	case CCreatureAnim::HITTED:
	case CCreatureAnim::DEFENCE:
	case CCreatureAnim::DEATH:
	case CCreatureAnim::DEATH_RANGED:
	case CCreatureAnim::VCMI_2HEX_DOWN:
	case CCreatureAnim::VCMI_2HEX_FRONT:
	case CCreatureAnim::VCMI_2HEX_UP:
		return speed * 3 / anim->framesInGroup(type);

	case CCreatureAnim::TURN_L:
	case CCreatureAnim::TURN_R:
		return speed / 3;

	case CCreatureAnim::MOVE_START:
	case CCreatureAnim::MOVE_END:
		return speed / 3;

	case CCreatureAnim::DEAD:
	case CCreatureAnim::DEAD_RANGED:
		return speed;

	default:
		return speed;
	}
}

float AnimationControls::getProjectileSpeed()
{
	return static_cast<float>(settings["battle"]["animationSpeed"].Float() * 100);
}

float AnimationControls::getSpellEffectSpeed()
{
	return static_cast<float>(settings["battle"]["animationSpeed"].Float() * 30);
}

float AnimationControls::getMovementDuration(const CCreature * creature)
{
	return static_cast<float>(settings["battle"]["animationSpeed"].Float() * 4 / creature->animation.walkAnimationTime);
}

float AnimationControls::getFlightDistance(const CCreature * creature)
{
	return static_cast<float>(creature->animation.flightAnimationDistance * 200);
}

CCreatureAnim::EAnimType CCreatureAnimation::getType() const
{
	return type;
}

void CCreatureAnimation::setType(CCreatureAnim::EAnimType type)
{
	this->type = type;
	currentFrame = 0;
	once = false;

	play();
}

void CCreatureAnimation::shiftColor(const ColorShifter* shifter)
{
	if(forward)
		forward->shiftColor(shifter);

	if(reverse)
		reverse->shiftColor(shifter);
}

CCreatureAnimation::CCreatureAnimation(const std::string & name_, TSpeedController controller)
	: name(name_),
	  speed(0.1f),
	  currentFrame(0),
	  elapsedTime(0),
	  type(CCreatureAnim::HOLDING),
	  border(CSDL_Ext::makeColor(0, 0, 0, 0)),
	  speedController(controller),
	  once(false)
{
	forward = std::make_shared<CAnimation>(name_);
	reverse = std::make_shared<CAnimation>(name_);

	//todo: optimize
	forward->preload();
	reverse->preload();

	// if necessary, add one frame into vcmi-only group DEAD
	if(forward->size(CCreatureAnim::DEAD) == 0)
	{
		forward->duplicateImage(CCreatureAnim::DEATH, forward->size(CCreatureAnim::DEATH)-1, CCreatureAnim::DEAD);
		reverse->duplicateImage(CCreatureAnim::DEATH, reverse->size(CCreatureAnim::DEATH)-1, CCreatureAnim::DEAD);
	}

	if(forward->size(CCreatureAnim::DEAD_RANGED) == 0 && forward->size(CCreatureAnim::DEATH_RANGED) != 0)
	{
		forward->duplicateImage(CCreatureAnim::DEATH_RANGED, forward->size(CCreatureAnim::DEATH_RANGED)-1, CCreatureAnim::DEAD_RANGED);
		reverse->duplicateImage(CCreatureAnim::DEATH_RANGED, reverse->size(CCreatureAnim::DEATH_RANGED)-1, CCreatureAnim::DEAD_RANGED);
	}

	//TODO: get dimensions form CAnimation
	auto first = forward->getImage(0, type, true);

	if(!first)
	{
		fullWidth = 0;
		fullHeight = 0;
		return;
	}
	fullWidth = first->width();
	fullHeight = first->height();

	reverse->verticalFlip();

	play();
}

void CCreatureAnimation::endAnimation()
{
	once = false;
	auto copy = onAnimationReset;
	onAnimationReset.clear();
	copy();
}

bool CCreatureAnimation::incrementFrame(float timePassed)
{
	elapsedTime += timePassed;
	currentFrame += timePassed * speed;
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
			setType(CCreatureAnim::HOLDING);

		endAnimation();
		return true;
	}
	return false;
}

void CCreatureAnimation::setBorderColor(SDL_Color palette)
{
	border = palette;
}

int CCreatureAnimation::getWidth() const
{
	return fullWidth;
}

int CCreatureAnimation::getHeight() const
{
	return fullHeight;
}

float CCreatureAnimation::getCurrentFrame() const
{
	return currentFrame;
}

void CCreatureAnimation::playOnce( CCreatureAnim::EAnimType type )
{
	setType(type);
	once = true;
}

inline int getBorderStrength(float time)
{
	float borderStrength = fabs(vstd::round(time) - time) * 2; // generate value in range 0-1

	return static_cast<int>(borderStrength * 155 + 100); // scale to 0-255
}

static SDL_Color genShadow(ui8 alpha)
{
	return CSDL_Ext::makeColor(0, 0, 0, alpha);
}

static SDL_Color genBorderColor(ui8 alpha, const SDL_Color & base)
{
	return CSDL_Ext::makeColor(base.r, base.g, base.b, ui8(base.a * alpha / 256));
}

static ui8 mixChannels(ui8 c1, ui8 c2, ui8 a1, ui8 a2)
{
	return c1*a1 / 256 + c2*a2*(255 - a1) / 256 / 256;
}

static SDL_Color addColors(const SDL_Color & base, const SDL_Color & over)
{
	return CSDL_Ext::makeColor(
			mixChannels(over.r, base.r, over.a, base.a),
			mixChannels(over.g, base.g, over.a, base.a),
			mixChannels(over.b, base.b, over.a, base.a),
			ui8(over.a + base.a * (255 - over.a) / 256)
			);
}

void CCreatureAnimation::genBorderPalette(IImage::BorderPallete & target)
{
	target[0] = genBorderColor(getBorderStrength(elapsedTime), border);
	target[1] = addColors(genShadow(128), genBorderColor(getBorderStrength(elapsedTime), border));
	target[2] = addColors(genShadow(64),  genBorderColor(getBorderStrength(elapsedTime), border));
}

void CCreatureAnimation::nextFrame(std::shared_ptr<CCanvas> canvas, bool attacker)
{
	size_t frame = static_cast<size_t>(floor(currentFrame));

	std::shared_ptr<IImage> image;

	if(attacker)
		image = forward->getImage(frame, type);
	else
		image = reverse->getImage(frame, type);

	if(image)
	{
		IImage::BorderPallete borderPallete;
		genBorderPalette(borderPallete);

		image->setBorderPallete(borderPallete);

		canvas->draw(image, pos.topLeft());
	}
}

int CCreatureAnimation::framesInGroup(CCreatureAnim::EAnimType group) const
{
	return static_cast<int>(forward->size(group));
}

bool CCreatureAnimation::isDead() const
{
	return getType() == CCreatureAnim::DEAD
	    || getType() == CCreatureAnim::DEATH
	    || getType() == CCreatureAnim::DEAD_RANGED
	    || getType() == CCreatureAnim::DEATH_RANGED;
}

bool CCreatureAnimation::isIdle() const
{
	return getType() == CCreatureAnim::HOLDING
	    || getType() == CCreatureAnim::MOUSEON;
}

bool CCreatureAnimation::isMoving() const
{
	return getType() == CCreatureAnim::MOVE_START
	    || getType() == CCreatureAnim::MOVING
	    || getType() == CCreatureAnim::MOVE_END;
}

bool CCreatureAnimation::isShooting() const
{
	return getType() == CCreatureAnim::SHOOT_UP
	    || getType() == CCreatureAnim::SHOOT_FRONT
	    || getType() == CCreatureAnim::SHOOT_DOWN;
}

void CCreatureAnimation::pause()
{
	speed = 0;
}

void CCreatureAnimation::play()
{
	//logAnim->trace("Play %s group %d at %d:%d", name, static_cast<int>(getType()), pos.x, pos.y);
    speed = 0;
    if(speedController(this, type) != 0)
        speed = 1 / speedController(this, type);
}
