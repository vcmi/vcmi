#pragma once

#include "../../lib/FunctionList.h"
#include "../gui/SDL_Extensions.h"
#include "../widgets/Images.h"

/*
 * CCreatureAnimation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CIntObject;
class CCreatureAnimation;

/// Namespace for some common controls of animations
namespace AnimationControls
{
	/// get SDL_Color for creature selection borders
	SDL_Color getBlueBorder();
	SDL_Color getGoldBorder();
	SDL_Color getNoBorder();

	/// creates animation object with preset speed control
	CCreatureAnimation * getAnimation(const CCreature * creature);

	/// returns animation speed of specific group, taking in mind game setting (in frames per second)
	float getCreatureAnimationSpeed(const CCreature * creature, const CCreatureAnimation * anim, size_t groupID);

	/// returns how far projectile should move each frame
	/// TODO: make it time-based
	float getProjectileSpeed();

	/// returns speed of any spell effects, including any special effects like morale (in frames per second)
	float getSpellEffectSpeed();

	/// returns duration of full movement animation, in seconds. Needed to move animation on screen
	float getMovementDuration(const CCreature * creature);

	/// Returns distance on which flying creatures should during one animation loop
	float getFlightDistance(const CCreature * creature);
}

/// Class which manages animations of creatures/units inside battles
/// TODO: split into constant image container and class that does *control* of animation
class CCreatureAnimation : public CIntObject
{
public:
	typedef std::function<float(CCreatureAnimation *, size_t)> TSpeedController;

private:
	std::string defName;

	int fullWidth, fullHeight;

	// palette, as read from def file
	std::array<SDL_Color, 256> palette;

	//key = id of group (note that some groups may be missing)
	//value = offset of pixel data for each frame, vector size = number of frames in group
	std::map<int, std::vector<unsigned int>> dataOffsets;

	//animation raw data
	//TODO: use vector instead?
	std::unique_ptr<ui8[]> pixelData;
	size_t pixelDataSize;

	// speed of animation, measure in frames per second
	float speed;

	// currently displayed frame. Float to allow H3-style animations where frames
	// don't display for integer number of frames
	float currentFrame;
	// cumulative, real-time duration of animation. Used for effects like selection border
	float elapsedTime;
	CCreatureAnim::EAnimType type; //type of animation being displayed

	// border color, disabled if alpha = 0
	SDL_Color border;

	TSpeedController speedController;

	bool once; // animation will be played once and the reset to idling

	ui8 * getPixelAddr(SDL_Surface * dest, int ftcpX, int ftcpY) const;

	template<int bpp>
	void putPixelAt(SDL_Surface * dest, int X, int Y, size_t index, const std::array<SDL_Color, 8> & special) const;

	template<int bpp>
	void putPixel( ui8 * dest, const SDL_Color & color, size_t index, const std::array<SDL_Color, 8> & special) const;

	template<int bpp>
	void nextFrameT(SDL_Surface * dest, bool rotate);

	void endAnimation();

	/// creates 8 special colors for current frame
	std::array<SDL_Color, 8> genSpecialPalette();
public:

	// function(s) that will be called when animation ends, after reset to 1st frame
	// NOTE that these function will be fired only once
	CFunctionList<void()> onAnimationReset;

	int getWidth() const;
	int getHeight() const;

	/// Constructor
	/// name - path to .def file, relative to SPRITES/ directory
	/// controller - function that will return for how long *each* frame
	/// in specified group of animation should be played, measured in seconds
	CCreatureAnimation(std::string name, TSpeedController speedController); //c-tor

	void setType(CCreatureAnim::EAnimType type); //sets type of animation and cleares framecount
	CCreatureAnim::EAnimType getType() const; //returns type of animation

	void nextFrame(SDL_Surface * dest, bool rotate);

	// should be called every frame, return true when animation was reset to beginning
	bool incrementFrame(float timePassed);
	void setBorderColor(SDL_Color palette);

	float getCurrentFrame() const; // Gets the current frame ID relative to frame group.

	void playOnce(CCreatureAnim::EAnimType type); //plays once given stage of animation, then resets to 2

	int framesInGroup(CCreatureAnim::EAnimType group) const; //retirns number of fromes in given group

	void pause();
	void play();

	//helpers. TODO: move them somewhere else
	bool isDead() const;
	bool isIdle() const;
	bool isMoving() const;
	bool isShooting() const;
};
