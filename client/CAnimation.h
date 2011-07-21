#ifndef __CANIMATION_H__
#define __CANIMATION_H__

#include <boost/function.hpp>

#include <vector>
#include <string>
#include <queue>
#include <map>

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
class SDLImageLoader;
class CompImageLoader;
class JsonNode;

/// Class for def loading, methods are based on CDefHandler
/// After loading will store general info (palette and frame offsets) and pointer to file itself
class CDefFile
{
private:

	struct SSpriteDef
	{
		ui32 size;
		ui32 format;    /// format in which pixel data is stored
		ui32 fullWidth; /// full width and height of frame, including borders
		ui32 fullHeight;
		ui32 width;     /// width and height of pixel data, borders excluded
		ui32 height;
		si32 leftMargin;
		si32 topMargin;
	};
	//offset[group][frame] - offset of frame data in file
	std::map<size_t, std::vector <size_t> > offset;

	unsigned char * data;
	SDL_Color * palette;

public:
	CDefFile(std::string Name);
	~CDefFile();

	//load frame as SDL_Surface
	template<class ImageLoader>
	void loadFrame(size_t frame, size_t group, ImageLoader &loader) const;

	const std::map<size_t, size_t> getEntries() const;
};

/*
 * Base class for images, can be used for non-animation pictures as well
 */
class IImage
{
	int refCount;
public:

	//draws image on surface "where" at position
	virtual void draw(SDL_Surface *where, int posX=0, int posY=0, Rect *src=NULL, ui8 alpha=255) const=0;

	//decrease ref count, returns true if image can be deleted (refCount <= 0)
	bool decreaseRef();
	void increaseRef();

	//Change palette to specific player
	virtual void playerColored(int player)=0;
	virtual int width() const=0;
	virtual int height() const=0;
	IImage();
	virtual ~IImage() {};
};

/*
 * Wrapper around SDL_Surface
 */
class SDLImage : public IImage
{
public:
	//Surface without empty borders
	SDL_Surface * surf;
	//size of left and top borders
	Point margins;
	//total size including borders
	Point fullSize;

public:
	//Load image from def file
	SDLImage(CDefFile *data, size_t frame, size_t group=0, bool compressed=false);
	//Load from bitmap file
	SDLImage(std::string filename, bool compressed=false);
	//Create using existing surface, extraRef will increase refcount on SDL_Surface
	SDLImage(SDL_Surface * from, bool extraRef);
	~SDLImage();

	void draw(SDL_Surface *where, int posX=0, int posY=0, Rect *src=NULL,  ui8 alpha=255) const;
	void playerColored(int player);
	int width() const;
	int height() const;

	friend class SDLImageLoader;
};

/*
 *  RLE-compressed image data for 8-bit images with alpha-channel, currently far from finished
 *  primary purpose is not high compression ratio but fast drawing.
 *  Consist of repeatable segments with format similar to H3 def compression:
 *  1st byte:
 *  if (byte == 0xff)
 *  	raw data, opaque and semi-transparent data always in separate blocks
 *  else
 *  	RLE-compressed image data with this color
 *  2nd byte = size of segment
 *  raw data (if any)
 */
class CompImage : public IImage
{
	//x,y - margins, w,h - sprite size
	Rect sprite;
	//total size including borders
	Point fullSize;

	//RLE-d data
	ui8 * surf;
	//array of offsets for each line
	unsigned int * line;
	//palette
	SDL_Color *palette;

	//Used internally to blit one block of data
	template<int bpp, int dir>
	void BlitBlock(ui8 type, ui8 size, ui8 *&data, ui8 *&dest, ui8 alpha) const;
	void BlitBlockWithBpp(ui8 bpp, ui8 type, ui8 size, ui8 *&data, ui8 *&dest, ui8 alpha, bool rotated) const;

public:
	//Load image from def file
	CompImage(const CDefFile *data, size_t frame, size_t group=0);
	//TODO: load image from SDL_Surface
	CompImage(SDL_Surface * surf);
	~CompImage();

	void draw(SDL_Surface *where, int posX=0, int posY=0, Rect *src=NULL, ui8 alpha=255) const;
	void playerColored(int player);
	int width() const;
	int height() const;

	friend class CompImageLoader;
};


/// Class for handling animation
class CAnimation
{
private:
	//source[group][position] - file with this frame, if string is empty - image located in def file
	std::map<size_t, std::vector <JsonNode> > source;

	//bitmap[group][position], store objects with loaded bitmaps
	std::map<size_t, std::map<size_t, IImage* > > images;

	//animation file name
	std::string name;

	//if true all frames will be stored in compressed (RLE) state
	const bool compressed;

	//loader, will be called by load(), require opened def file for loading from it. Returns true if image is loaded
	bool loadFrame(CDefFile * file, size_t frame, size_t group);

	//unloadFrame, returns true if image has been unloaded ( either deleted or decreased refCount)
	bool unloadFrame(size_t frame, size_t group);

	//initialize animation from file
	void init(CDefFile * file);

	//try to open def file
	CDefFile * getFile() const;

	//to get rid of copy-pasting error message :]
	void printError(size_t frame, size_t group, std::string type) const;

	//not a very nice method to get image from another def file
	//TODO: remove after implementing resource manager
	IImage * getFromExtraDef(std::string filename);

public:

	CAnimation(std::string Name, bool Compressed = false);
	CAnimation();
	~CAnimation();

	//static method for debugging - print info about loaded animations in tlog1
	static void getAnimInfo();
	static std::set<CAnimation*> loadedAnims;

	//add custom surface to the selected position.
	void setCustom(std::string filename, size_t frame, size_t group=0);

	//get pointer to image from specific group, NULL if not found
	IImage * getImage(size_t frame, size_t group=0, bool verbose=true) const;

	//all available frames
	void load  ();
	void unload();

	//all frames from group
	void loadGroup  (size_t group);
	void unloadGroup(size_t group);

	//single image
	void load  (size_t frame, size_t group=0);
	void unload(size_t frame, size_t group=0);

	//total count of frames in group (including not loaded)
	size_t size(size_t group=0) const;
};


/// Class for displaying one image from animation
class CAnimImage: public CIntObject
{
private:
	CAnimation* anim;
	//displayed frame/group
	size_t frame;
	size_t group;
	int player;
	unsigned char flags;

	void init();

public:
	CAnimImage(std::string name, size_t Frame, size_t Group=0, int x=0, int y=0, unsigned char Flags=0);
	CAnimImage(CAnimation* anim, size_t Frame, size_t Group=0, int x=0, int y=0, unsigned char Flags=0);
	~CAnimImage();//d-tor

	//size of animation
	size_t size();

	//change displayed frame on this one
	void setFrame(size_t Frame, size_t Group=0);

	//makes image player-colored
	void playerColored(int player);

	void showAll(SDL_Surface *to);
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
	};
protected:
	CAnimation anim;

	size_t group, frame;//current frame

	size_t first, last; //animation range

	//TODO: replace with time delay(needed for battles)
	unsigned int frameDelay;//delay in frames of each image
	unsigned int value;//how many times current frame was showed

	unsigned char flags;//Flags from EFlags enum

	//blit image with optional rotation, fitting into rect, etc
	void blitImage(size_t frame, size_t group, SDL_Surface *to);

	//For clipping in rect, offsets of picture coordinates
	int xOffset, yOffset;

	ui8 alpha;

public:
	//called when next animation sequence is required
	boost::function<void()> callback;

	//Set per-surface alpha, 0 = transparent, 255 = opaque
	void setAlpha(unsigned int alphaValue);

	CShowableAnim(int x, int y, std::string name, unsigned char flags=0, unsigned int Delay=4, size_t Group=0);
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
	void show(SDL_Surface *to);
	void showAll(SDL_Surface *to);
};

/// Creature-dependend animations like attacking, moving,...
class CCreatureAnim: public CShowableAnim
{
public:

	enum EAnimType // list of creature animations, numbers were taken from def files
	{
		WHOLE_ANIM=-1, //just for convenience
		MOVING=0, //will automatically add MOVE_START and MOVE_END to queue
		MOUSEON=1, //rename to IDLE
		HOLDING=2, //rename to STANDING
		HITTED=3,
		DEFENCE=4,
		DEATH=5,
		//DEATH2=6, //unused?
		TURN_L=7, //will automatically play second part of anim and rotate creature
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
		DHEX_ATTACK_UP=17,
		DHEX_ATTACK_FRONT=18,
		DHEX_ATTACK_DOWN=19,
		MOVE_START=20, //no need to use this two directly - MOVING will be enought
		MOVE_END=21
		//MOUSEON=22 //special group for border-only images - IDLE will be used as base
		//READY=23 //same but STANDING is base
	};

private:
	//queue of animations waiting to be displayed
	std::queue<EAnimType> queue;

	//this funcction is used as callback if preview flag was set during construction
	void loopPreview();

public:
	//change anim to next if queue is not empty, call callback othervice
	void reset();

	//add sequence to the end of queue
	void addLast(EAnimType newType);

	void startPreview();

	//clear queue and set animation to this sequence
	void clearAndSet(EAnimType type);

	CCreatureAnim(int x, int y, std::string name, Rect picPos,
	              unsigned char flags= USE_RLE, EAnimType = HOLDING );

};

#endif // __CANIMATIONHANDLER_H__
