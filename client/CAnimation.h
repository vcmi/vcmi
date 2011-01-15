#ifndef __CANIMATION_H__
#define __CANIMATION_H__

#include <boost/function.hpp>

#include <vector>
#include <string>
#include <queue>
#include <set>

#include "../global.h"
#include "GUIBase.h"

/*
 * CAnimation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

struct SDL_Surface;
struct BMPPalette;

//class for def loading, methods are based on CDefHandler
//after loading will store general info (palette and frame offsets) and pointer to file itself
class CDefFile
{
private:

	struct SSpriteDef
	{
		ui32 prSize;
		ui32 defType2;
		ui32 FullWidth;
		ui32 FullHeight;
		ui32 SpriteWidth;
		ui32 SpriteHeight;
		ui32 LeftMargin;
		ui32 TopMargin;
	};

	//offset[group][frame] - offset of frame data in file
	std::vector< std::vector <size_t> > offset;

	//sorted list of offsets used to determine size
	std::set <size_t> offList;

	unsigned char * data;
	BMPPalette * colors;
	int datasize;
	unsigned int type;

public:
	CDefFile(std::string Name);
	~CDefFile();

	//true if file was opened correctly
	bool loaded() const;

	//get copy of palette to unpack compressed animation
	BMPPalette * getPalette();

	//true if frame is present in it
	bool haveFrame(size_t frame, size_t group) const;

	//get copy of binary data
	unsigned char * getFrame(size_t frame, size_t group) const;

	//load frame as SDL_Surface
	SDL_Surface * loadFrame(size_t frame, size_t group) const ;

	//static version of previous one for calling from compressed anim
	static SDL_Surface * loadFrame(const unsigned char * FDef, const BMPPalette * palette);
};

// Class for handling animation.
class CAnimation
{
private:

	//internal structure to hold all data of specific frame
	struct AnimEntry
	{
		//surface for this entry
		SDL_Surface * surf;

		//data for CompressedAnim
		unsigned char * data;

		//reference count, changed by loadFrame \ unloadFrame
		size_t refCount;

		//size of compressed data, unused for def files
		size_t dataSize;

		//bitfield, location of image data: 1 - def, 2 - file#9.*, 4 - file#9#2.*
		unsigned char source;

		AnimEntry();
	};

	//palette from def file, used only for compressed anim
	BMPPalette * defPalette;

	//entries[group][position], store all info regarding frames
	std::vector< std::vector <AnimEntry> > entries;

	//animation file name
	std::string name;

	//if true all frames will be stored in compressed state
	const bool compressed;

	//loader, will be called by load(), require opened def file for loading from it. Returns true if image is loaded
	bool loadFrame(CDefFile * file, size_t frame, size_t group);

	//unloadFrame, returns true if image has been unloaded ( either deleted or decreased refCount)
	bool unloadFrame(size_t frame, size_t group);

	//decompress entry data
	void decompress(AnimEntry &entry);

	//initialize animation from file
	void init(CDefFile * file);

	//try to open def file
	CDefFile * getFile() const;

	//to get rid of copy-pasting error message :]
	void printError(size_t frame, size_t group, std::string type) const;

public:

	CAnimation(std::string Name, bool Compressed = false);
	CAnimation();
	~CAnimation();

	//add custom surface to the end of specific group. If shared==true surface needs to be deleted
	//somewhere outside of CAnim as well (SDL_Surface::refcount will be increased)
	void add(SDL_Surface * surf, bool shared=false, size_t group=0);

	//removes all surfaces which have compressed data
	void removeDecompressed(size_t frame, size_t group);

	//get pointer to surface, this function ignores groups (like ourImages in DefHandler)
	SDL_Surface * image (size_t frame);

	//get pointer to surface, from specific group
	SDL_Surface * image(size_t frame, size_t group);

	//removes all frames as well as their entries
	void clear();

	//all available frames
	void load  ();
	void unload();

	//all frames from group
	void loadGroup  (size_t group);
	void unloadGroup(size_t group);

	//single image
	void load  (size_t frame, size_t group=0);
	void unload(size_t frame, size_t group=0);

	//list of frames (first = group ID, second = frame ID)
	void load  (std::vector <std::pair <size_t, size_t> > frames);
	void unload(std::vector <std::pair <size_t, size_t> > frames);

	//helper to fix frame order on some buttons
	void fixButtonPos();

	//size of specific group, 0 if not present
	size_t groupSize(size_t group) const;

	//total count of frames in whole anim
	size_t size() const;
};
/*
//Class for displaying one image from animation
class CAnimImage: public CIntObject
{
private:
	CAnimation anim;
	size_t frame;//displayed frame/group
	size_t group;

public:
	CAnimImage(int x, int y, std::string name, size_t Frame, size_t Group=0);//c-tor
	~CAnimImage();//d-tor

	//change displayed frame on this one
	void setFrame(size_t Frame, size_t Group=0);
	void show(SDL_Surface *to);
	//TODO: showAll();
};
*/
//Base class for displaying animation, used as superclass for different animations
class CShowableAnim: public CIntObject
{
public:
	enum EFlags
	{
		FLAG_BASE=1,       //base frame will be blitted before current one
		FLAG_COMPRESSED=2, //animations will be loaded in compressed state
		FLAG_ROTATED=4,    //will be displayed rotated
		FLAG_ALPHA=8,      //if image is 8bbp it will be printed with transparency (0=opaque, 255=transparent)
		FLAG_USERLE=16,    //not used for now, enable RLE compression from SDL
		FLAG_PREVIEW=32    //for creatures only: several animation (move, attack, defence...) will be randomly selected
	};

protected:
	CAnimation anim;
	size_t group, frame;//current frame

	size_t first, last; //animation range

	unsigned char flags;//flags from EFlags enum

	unsigned int frameDelay;//delay in frames of each image

	unsigned int value;//how many times current frame was showed

	//blit image with optional rotation, fitting into rect, etc
	void blitImage(SDL_Surface *what, SDL_Surface *to);

	//For clipping in rect, offsets of picture coordinates
	int xOffset, yOffset;

public:
	//called when next animation sequence is required
	boost::function<void()> callback;

	CShowableAnim(int x, int y, std::string name, unsigned char flags, unsigned int Delay=4, size_t Group=0);
	~CShowableAnim();

	//set animation to group or part of group
	bool set(size_t Group);
	bool set(size_t Group, size_t from, size_t to=-1);

	//set rotation flag
	void rotate(bool on);

	//move displayed part of picture (if picture is clipped to rect)
	void movePic( int byX, int byY);

	//set frame to first, call callback
	virtual void reset();

	//show current frame and increase counter
	void show(SDL_Surface *to);
	void showAll(SDL_Surface *to);
};

class CCreatureAnim: public CShowableAnim
{
public:

	enum EAnimType // list of creature animations, numbers were taken from def files
	{
		ANIM_MOVING=0, //will automatically add MOVE_START and MOVE_END to queue
		ANIM_MOUSEON=1,
		ANIM_HOLDING=2,
		ANIM_HITTED=3,
		ANIM_DEFENCE=4,
		ANIM_DEATH=5,
		//ANIM_DEATH2=6, //unused?
		ANIM_TURN_L=7, //will automatically play second part of anim and rotate creature
		ANIM_TURN_R=8, //same
		//ANIM_TURN_L2=9, //identical to previous?
		//ANIM_TURN_R2=10,
		ANIM_ATTACK_UP=11,
		ANIM_ATTACK_FRONT=12,
		ANIM_ATTACK_DOWN=13,
		ANIM_SHOOT_UP=14,
		ANIM_SHOOT_FRONT=15,
		ANIM_SHOOT_DOWN=16,
		ANIM_CAST_UP=17,
		ANIM_CAST_FRONT=18,
		ANIM_CAST_DOWN=19,
		ANIM_2HEX_ATTACK_UP=17,
		ANIM_2HEX_ATTACK_FRONT=18,
		ANIM_2HEX_ATTACK_DOWN=19,
		ANIM_MOVE_START=20, //no need to use this two directly - ANIM_MOVING will be enought
		ANIM_MOVE_END=21
	};

private:
	// queue of animations waiting to be displayed
	std::queue<EAnimType> queue;

	//this funcction is used as callback if preview flag was set during construction
	void loopPreview();

public:
	//change anim to next if queue is not empty, call callback othervice
	void reset();

	//add sequence to the end of queue
	void addLast(EAnimType newType);

	//clear queue and set animation to this sequence
	void clearAndSet(EAnimType type);

	CCreatureAnim(int x, int y, std::string name, unsigned char flags=FLAG_COMPRESSED | FLAG_ALPHA | FLAG_PREVIEW,
	              EAnimType type=ANIM_HOLDING);

};

#endif // __CANIMATIONHANDLER_H__
