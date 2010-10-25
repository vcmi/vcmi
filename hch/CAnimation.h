#ifndef __CANIMATION_H__
#define __CANIMATION_H__

#include <vector>
#include <string>
#include <set>
#include "../global.h"

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

	unsigned char * data;
	int datasize;
	BMPPalette * colors;

	//offset[group][frame] - offset of frame data in file
	std::vector< std::vector <size_t> > offset;

	//sorted list of offsets used to determine size
	std::set <size_t> offList;


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

		//bitfield, location of image data: 1 - def, 2 - file#9.*, 4 - file#9#2.*
		unsigned char source;

		//reference count, changed by loadFrame \ unloadFrame
		size_t refCount;

		//data for CompressedAnim
		unsigned char * data;

		//size of compressed data, unused for def files
		size_t dataSize;

		AnimEntry();
	};

	//animation file name
	std::string name;

	//if true all frames will be stored in compressed state
	const bool compressed;

	//palette from def file, used only for compressed anim
	BMPPalette * defPalette;

	//entries[group][position], store all info regarding frames
	std::vector< std::vector <AnimEntry> > entries;

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
	void purgeCompressed();

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

#endif // __CANIMATIONHANDLER_H__
