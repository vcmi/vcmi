/*
 * CAnimation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/vcmi_endian.h"
#include "Geometries.h"
#include "../../lib/GameConstants.h"

struct SDL_Surface;
class JsonNode;
class CDefFile;

/*
 * Base class for images, can be used for non-animation pictures as well
 */
class IImage
{
	int refCount;

public:

	//draws image on surface "where" at position
	virtual void draw(SDL_Surface * where, int posX = 0, int posY = 0, Rect * src = nullptr, ui8 alpha = 255) const = 0;
	virtual void draw(SDL_Surface * where, SDL_Rect * dest, SDL_Rect * src, ui8 alpha = 255) const = 0;

	virtual std::unique_ptr<IImage> scaleFast(float scale) const = 0;

	virtual void exportBitmap(const boost::filesystem::path & path) const = 0;

	//decrease ref count, returns true if image can be deleted (refCount <= 0)
	bool decreaseRef();
	void increaseRef();

	//Change palette to specific player
	virtual void playerColored(PlayerColor player) = 0;

	//set special color for flag
	virtual void setFlagColor(PlayerColor player) = 0;

	virtual int width() const = 0;
	virtual int height() const = 0;

	//only indexed bitmaps, 16 colors maximum
	virtual void shiftPalette(int from, int howMany) = 0;

	virtual void horizontalFlip() = 0;
	virtual void verticalFlip() = 0;

	IImage();
	virtual ~IImage() {};
};

/// Class for handling animation
class CAnimation
{
private:
	//source[group][position] - file with this frame, if string is empty - image located in def file
	std::map<size_t, std::vector<JsonNode>> source;

	//bitmap[group][position], store objects with loaded bitmaps
	std::map<size_t, std::map<size_t, IImage *>> images;

	//animation file name
	std::string name;

	//if true all frames will be stored in compressed (RLE) state
	const bool compressed;

	bool preloaded;

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

	//duplicates frame at [sourceGroup, sourceFrame] as last frame in targetGroup
	//and loads it if animation is preloaded
	void duplicateImage(const size_t sourceGroup, const size_t sourceFrame, const size_t targetGroup);

	//add custom surface to the selected position.
	void setCustom(std::string filename, size_t frame, size_t group = 0);

	//get pointer to image from specific group, nullptr if not found
	IImage * getImage(size_t frame, size_t group = 0, bool verbose = true) const;

	void exportBitmaps(const boost::filesystem::path & path) const;

	//all available frames
	void load();
	void unload();
	void preload();

	//all frames from group
	void loadGroup(size_t group);
	void unloadGroup(size_t group);

	//single image
	void load(size_t frame, size_t group = 0);
	void unload(size_t frame, size_t group = 0);

	//total count of frames in group (including not loaded)
	size_t size(size_t group = 0) const;
};

const float DEFAULT_DELTA = 0.05f;

class CFadeAnimation
{
public:
	enum class EMode
	{
		NONE,
		IN,
		OUT
	};

private:
	float delta;
	SDL_Surface * fadingSurface;
	bool fading;
	float fadingCounter;
	bool shouldFreeSurface;

	float initialCounter() const;
	bool isFinished() const;

public:
	EMode fadingMode;

	CFadeAnimation();
	~CFadeAnimation();
	void init(EMode mode, SDL_Surface * sourceSurface, bool freeSurfaceAtEnd = false, float animDelta = DEFAULT_DELTA);
	void update();
	void draw(SDL_Surface * targetSurface, const SDL_Rect * sourceRect, SDL_Rect * destRect);
	bool isFading() const { return fading; }
};
