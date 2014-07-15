#pragma once

#include "../../lib/vcmi_endian.h"
#include "gui/Geometries.h"
#include "../../lib/GameConstants.h"

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
	} PACKED_STRUCT;
	//offset[group][frame] - offset of frame data in file
	std::map<size_t, std::vector <size_t> > offset;

	ui8 * data;
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
	virtual void draw(SDL_Surface *where, int posX=0, int posY=0, Rect *src=nullptr, ui8 alpha=255) const=0;

	//decrease ref count, returns true if image can be deleted (refCount <= 0)
	bool decreaseRef();
	void increaseRef();

	//Change palette to specific player
	virtual void playerColored(PlayerColor player)=0;
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

	void draw(SDL_Surface *where, int posX=0, int posY=0, Rect *src=nullptr,  ui8 alpha=255) const;
	void playerColored(PlayerColor player);
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
	ui32 * line;
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

	void draw(SDL_Surface *where, int posX=0, int posY=0, Rect *src=nullptr, ui8 alpha=255) const;
	void playerColored(PlayerColor player);
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
	void initFromJson(const JsonNode & input);
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

    //static method for debugging - print info about loaded animations
	static void getAnimInfo();
	static std::set<CAnimation*> loadedAnims;

	//add custom surface to the selected position.
	void setCustom(std::string filename, size_t frame, size_t group=0);

	//get pointer to image from specific group, nullptr if not found
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
