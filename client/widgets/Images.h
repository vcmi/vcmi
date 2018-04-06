/*
 * Images.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"
#include "../gui/SDL_Extensions.h"

struct SDL_Surface;
struct Rect;
class CAnimImage;
class CLabel;
class CAnimation;

// Image class
class CPicture : public CIntObject
{
	void setSurface(SDL_Surface *to);
public:
	SDL_Surface * bg;
	Rect * srcRect; //if nullptr then whole surface will be used
	bool freeSurf; //whether surface will be freed upon CPicture destruction
	bool needRefresh;//Surface needs to be displayed each frame
	bool visible;
	operator SDL_Surface*()
	{
		return bg;
	}

	CPicture(const Rect & r, const SDL_Color & color, bool screenFormat = false); //rect filled with given color
	CPicture(const Rect & r, ui32 color, bool screenFormat = false); //rect filled with given color
	CPicture(SDL_Surface * BG, int x = 0, int y=0, bool Free = true); //wrap existing SDL_Surface
	CPicture(const std::string &bmpname, int x=0, int y=0);
	CPicture(SDL_Surface *BG, const Rect &SrcRext, int x = 0, int y = 0, bool free = false); //wrap subrect of given surface
	~CPicture();
	void init();

	//set alpha value for whole surface. Note: may be messed up if surface is shared
	// 0=transparent, 255=opaque
	void setAlpha(int value);

	void scaleTo(Point size);
	void createSimpleRect(const Rect &r, bool screenFormat, ui32 color);
	void show(SDL_Surface * to) override;
	void showAll(SDL_Surface * to) override;
	void convertToScreenBPP();
	void colorize(PlayerColor player);
};

/// area filled with specific texture
class CFilledTexture : CIntObject
{
	SDL_Surface * texture;

public:
	CFilledTexture(std::string imageName, Rect position);
	~CFilledTexture();
	void showAll(SDL_Surface *to) override;
};

/// Class for displaying one image from animation
class CAnimImage: public CIntObject
{
private:
	std::shared_ptr<CAnimation> anim;
	//displayed frame/group
	size_t frame;
	size_t group;
	PlayerColor player;
	ui8 flags;

	void init();

public:
	bool visible;

	CAnimImage(const std::string & name, size_t Frame, size_t Group=0, int x=0, int y=0, ui8 Flags=0);
	CAnimImage(std::shared_ptr<CAnimation> Anim, size_t Frame, size_t Group=0, int x=0, int y=0, ui8 Flags=0);
	~CAnimImage();

	//size of animation
	size_t size();

	//change displayed frame on this one
	void setFrame(size_t Frame, size_t Group=0);

	//makes image player-colored
	void playerColored(PlayerColor player);

	void showAll(SDL_Surface * to) override;
};

/// Base class for displaying animation, used as superclass for different animations
class CShowableAnim: public CIntObject
{
public:
	enum EFlags
	{
		BASE=1,            //base frame will be blitted before current one
		HORIZONTAL_FLIP=2, //TODO: will be displayed rotated
		VERTICAL_FLIP=4,   //TODO: will be displayed rotated
		USE_RLE=8,         //RLE-d version, support full alpha-channel for 8-bit images
		PLAYER_COLORED=16, //TODO: all loaded images will be player-colored
		PLAY_ONCE=32       //play animation only once and stop at last frame
	};
protected:
	CAnimation * anim;

	size_t group, frame;//current frame

	size_t first, last; //animation range

	//TODO: replace with time delay(needed for battles)
	ui32 frameDelay;//delay in frames of each image
	ui32 value;//how many times current frame was showed

	ui8 flags;//Flags from EFlags enum

	//blit image with optional rotation, fitting into rect, etc
	void blitImage(size_t frame, size_t group, SDL_Surface *to);

	//For clipping in rect, offsets of picture coordinates
	int xOffset, yOffset;

	ui8 alpha;

public:
	//called when next animation sequence is required
	std::function<void()> callback;

	//Set per-surface alpha, 0 = transparent, 255 = opaque
	void setAlpha(ui32 alphaValue);

	CShowableAnim(int x, int y, std::string name, ui8 flags=0, ui32 Delay=4, size_t Group=0);
	~CShowableAnim();

	//set animation to group or part of group
	bool set(size_t Group);
	bool set(size_t Group, size_t from, size_t to=-1);

	//set rotation flags
	void rotate(bool on, bool vertical=false);

	//move displayed part of picture (if picture is clipped to rect)
	void clipRect(int posX, int posY, int width, int height);

	//set frame to first, call callback
	virtual void reset();

	//show current frame and increase counter
	void show(SDL_Surface * to) override;
	void showAll(SDL_Surface * to) override;
};

/// Creature-dependend animations like attacking, moving,...
class CCreatureAnim: public CShowableAnim
{
public:

	enum EHeroAnimType
	{
		HERO_HOLDING = 0,
		HERO_IDLE = 1, // idling movement that happens from time to time
		HERO_DEFEAT = 2, // played when army loses stack or on friendly fire
		HERO_VICTORY = 3, // when enemy stack killed or huge damage is dealt
		HERO_CAST_SPELL = 4 // spellcasting
	};

	enum EAnimType // list of creature animations, numbers were taken from def files
	{
		MOVING=0,
		MOUSEON=1,
		HOLDING=2,
		HITTED=3,
		DEFENCE=4,
		DEATH=5,
		DEATH_RANGED=6,
		TURN_L=7,
		TURN_R=8, //same
		//TURN_L2=9, //identical to previous?
		//TURN_R2=10,
		ATTACK_UP=11,
		ATTACK_FRONT=12,
		ATTACK_DOWN=13,
		SHOOT_UP=14,
		SHOOT_FRONT=15,
		SHOOT_DOWN=16,
		CAST_UP=17,
		CAST_FRONT=18,
		CAST_DOWN=19,
		MOVE_START=20,
		MOVE_END=21,

		DEAD = 22, // new group, used to show dead stacks. If empty - last frame from "DEATH" will be copied here
		DEAD_RANGED = 23, // new group, used to show dead stacks (if DEATH_RANGED was used). If empty - last frame from "DEATH_RANGED" will be copied here

		VCMI_CAST_UP    = 30,
		VCMI_CAST_FRONT = 31,
		VCMI_CAST_DOWN  = 32,
		VCMI_2HEX_UP    = 40,
		VCMI_2HEX_FRONT = 41,
		VCMI_2HEX_DOWN  = 42
	};

private:
	//queue of animations waiting to be displayed
	std::queue<EAnimType> queue;

	//this function is used as callback if preview flag was set during construction
	void loopPreview(bool warMachine);

public:
	//change anim to next if queue is not empty, call callback othervice
	void reset() override;

	//add sequence to the end of queue
	void addLast(EAnimType newType);

	void startPreview(bool warMachine);

	//clear queue and set animation to this sequence
	void clearAndSet(EAnimType type);

	CCreatureAnim(int x, int y, std::string name, Rect picPos,
				  ui8 flags= USE_RLE, EAnimType = HOLDING );

};
