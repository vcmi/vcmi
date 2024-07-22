/*
 * CCreatureAnimation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/FunctionList.h"
#include "../../lib/Color.h"
#include "../widgets/Images.h"
#include "../render/IImage.h"

class CIntObject;
class CreatureAnimation;
class Canvas;

/// Namespace for some common controls of animations
namespace AnimationControls
{
	/// get color for creature selection borders
	ColorRGBA getBlueBorder();
	ColorRGBA getGoldBorder();
	ColorRGBA getNoBorder();

	/// returns animation speed factor according to game settings,
	/// slow speed is considered to be "base speed" and will return 1.0
	float getAnimationSpeedFactor();

	/// creates animation object with preset speed control
	std::shared_ptr<CreatureAnimation> getAnimation(const CCreature * creature);

	/// returns animation speed of specific group, taking in mind game setting (in frames per second)
	float getCreatureAnimationSpeed(const CCreature * creature, const CreatureAnimation * anim, ECreatureAnimType groupID);

	/// returns how far projectile should move per second, in pixels per second
	float getProjectileSpeed();

	/// returns how far projectile should move per second, in pixels per second
	float getRayProjectileSpeed();

	/// returns speed of catapult projectile, in pixels per second, on a straight line, without parabola correction
	float getCatapultSpeed();

	/// returns speed of any spell effects, including any special effects like morale (in frames per second)
	float getSpellEffectSpeed();

	/// returns speed of movement animation across the screen, in tiles per second
	float getMovementRange(const CCreature * creature);

	/// returns speed of movement animation across the screen, in pixels per seconds
	float getFlightDistance(const CCreature * creature);

	/// Returns total time for full fade-in effect on newly summoned creatures, in seconds
	float getFadeInDuration();

	/// Returns animation speed for obstacles, in frames per second
	float getObstaclesSpeed();
}

/// Class which manages animations of creatures/units inside battles
/// TODO: split into constant image container and class that does *control* of animation
class CreatureAnimation : public CIntObject
{
public:
	using TSpeedController = std::function<float(CreatureAnimation *, ECreatureAnimType)>;

private:
	AnimationPath name;

	/// animation for rendering stack in default orientation - facing right
	std::shared_ptr<CAnimation> forward;

	/// animation that has all its frames flipped for rendering stack facing left
	std::shared_ptr<CAnimation> reverse;

	int fullWidth;
	int fullHeight;

	/// speed of animation, measure in frames per second
	float speed;

	/// currently displayed frame. Float to allow H3-style animations where frames
	/// don't display for integer number of frames
	float currentFrame;
	float animationEnd;

	/// cumulative, real-time duration of animation. Used for effects like selection border
	float elapsedTime;

	///type of animation being displayed
	ECreatureAnimType type;

	/// current value of shadow transparency
	uint8_t shadowAlpha;

	/// border color, disabled if alpha = 0
	ColorRGBA border;

	TSpeedController speedController;

	/// animation will be played once and the reset to idling
	bool once;

	void endAnimation();

	void genSpecialPalette(IImage::SpecialPalette & target);
public:

	/// function(s) that will be called when animation ends, after reset to 1st frame
	/// NOTE that these functions will be fired only once
	CFunctionList<void()> onAnimationReset;

	int getWidth() const;
	int getHeight() const;

	/// Constructor
	/// name - path to .def file, relative to SPRITES/ directory
	/// controller - function that will return for how long *each* frame
	/// in specified group of animation should be played, measured in seconds
	CreatureAnimation(const AnimationPath & name_, TSpeedController speedController);

	/// sets type of animation and resets framecount
	void setType(ECreatureAnimType type);

	/// returns currently rendered type of animation
	ECreatureAnimType getType() const;

	void nextFrame(Canvas & canvas, const ColorFilter & shifter, bool facingRight);

	/// should be called every frame, return true when animation was reset to beginning
	bool incrementFrame(float timePassed);

	void setBorderColor(ColorRGBA palette);

	/// Gets the current frame ID within current group.
	float getCurrentFrame() const;

	/// plays once given type of animation, then resets to idle
	void playOnce(ECreatureAnimType type);

	/// returns number of frames in selected animation type
	int framesInGroup(ECreatureAnimType group) const;

	void playUntil(size_t frameIndex);

	/// helpers to classify current type of animation
	bool isDead() const;
	bool isDying() const;
	bool isDeadOrDying() const;
	bool isIdle() const;
	bool isMoving() const;
	bool isShooting() const;
};
