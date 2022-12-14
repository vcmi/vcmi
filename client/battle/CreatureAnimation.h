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
#include "../widgets/Images.h"
#include "../gui/CAnimation.h"

class CIntObject;
class CreatureAnimation;
class Canvas;

/// Namespace for some common controls of animations
namespace AnimationControls
{
	/// get SDL_Color for creature selection borders
	SDL_Color getBlueBorder();
	SDL_Color getGoldBorder();
	SDL_Color getNoBorder();

	/// creates animation object with preset speed control
	std::shared_ptr<CreatureAnimation> getAnimation(const CCreature * creature);

	/// returns animation speed of specific group, taking in mind game setting (in frames per second)
	float getCreatureAnimationSpeed(const CCreature * creature, const CreatureAnimation * anim, size_t groupID);

	/// returns how far projectile should move each frame
	/// TODO: make it time-based
	float getProjectileSpeed();

	/// returns speed of catapult projectile
	float getCatapultSpeed();

	/// returns speed of any spell effects, including any special effects like morale (in frames per second)
	float getSpellEffectSpeed();

	/// returns duration of full movement animation, in seconds. Needed to move animation on screen
	float getMovementDuration(const CCreature * creature);

	/// Returns distance on which flying creatures should during one animation loop
	float getFlightDistance(const CCreature * creature);

	/// Returns total time for full fade-in effect on newly summoned creatures, in seconds
	float getFadeInDuration();
}

/// Class which manages animations of creatures/units inside battles
/// TODO: split into constant image container and class that does *control* of animation
class CreatureAnimation : public CIntObject
{
public:
	typedef std::function<float(CreatureAnimation *, size_t)> TSpeedController;

private:
	std::string name;

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

	/// cumulative, real-time duration of animation. Used for effects like selection border
	float elapsedTime;

	///type of animation being displayed
	ECreatureAnimType::Type type;

	/// current value of shadow transparency
	uint8_t shadowAlpha;

	/// border color, disabled if alpha = 0
	SDL_Color border;

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
	CreatureAnimation(const std::string & name_, TSpeedController speedController);

	/// sets type of animation and resets framecount
	void setType(ECreatureAnimType::Type type);

	/// returns currently rendered type of animation
	ECreatureAnimType::Type getType() const;

	void nextFrame(Canvas & canvas, bool facingRight);

	/// should be called every frame, return true when animation was reset to beginning
	bool incrementFrame(float timePassed);

	void setBorderColor(SDL_Color palette);

	/// apply color tint effect
	void shiftColor(const ColorShifter * shifter);

	/// Gets the current frame ID within current group.
	float getCurrentFrame() const;

	/// plays once given type of animation, then resets to idle
	void playOnce(ECreatureAnimType::Type type);

	/// returns number of frames in selected animation type
	int framesInGroup(ECreatureAnimType::Type group) const;

	void pause();
	void play();

	/// helpers to classify current type of animation
	bool isDead() const;
	bool isDying() const;
	bool isDeadOrDying() const;
	bool isIdle() const;
	bool isMoving() const;
	bool isShooting() const;
};
