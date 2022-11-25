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

#ifdef IN
#undef IN
#endif

#ifdef OUT
#undef OUT
#endif

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;

VCMI_LIB_NAMESPACE_END

struct SDL_Surface;
class CDefFile;
class ColorShifter;

/*
 * Base class for images, can be used for non-animation pictures as well
 */
class IImage
{
public:
	using BorderPallete = std::array<SDL_Color, 3>;

	//draws image on surface "where" at position
	virtual void draw(SDL_Surface * where, int posX = 0, int posY = 0, Rect * src = nullptr, ui8 alpha = 255) const=0;
	virtual void draw(SDL_Surface * where, SDL_Rect * dest, SDL_Rect * src, ui8 alpha = 255) const = 0;

	virtual std::shared_ptr<IImage> scaleFast(float scale) const = 0;

	virtual void exportBitmap(const boost::filesystem::path & path) const = 0;

	//Change palette to specific player
	virtual void playerColored(PlayerColor player)=0;

	//set special color for flag
	virtual void setFlagColor(PlayerColor player)=0;

	//test transparency of specific pixel
	virtual bool isTransparent(const Point & coords) const = 0;

	virtual Point dimensions() const = 0;
	virtual int width() const=0;
	virtual int height() const=0;

	//only indexed bitmaps, 16 colors maximum
	virtual void shiftPalette(int from, int howMany) = 0;
	virtual void adjustPalette(const ColorShifter * shifter) = 0;
	virtual void resetPalette() = 0;

	//only indexed bitmaps, colors 5,6,7 must be special
	virtual void setBorderPallete(const BorderPallete & borderPallete) = 0;

	virtual void horizontalFlip() = 0;
	virtual void verticalFlip() = 0;

	IImage();
	virtual ~IImage();

	/// loads image from specified file. Returns 0-sized images on failure
	static std::shared_ptr<IImage> createFromFile( const std::string & path );
};

/// Class for handling animation
class CAnimation
{
private:
	//source[group][position] - file with this frame, if string is empty - image located in def file
	std::map<size_t, std::vector <JsonNode> > source;

	//bitmap[group][position], store objects with loaded bitmaps
	std::map<size_t, std::map<size_t, std::shared_ptr<IImage> > > images;

	//animation file name
	std::string name;

	bool preloaded;

	std::shared_ptr<CDefFile> defFile;

	//loader, will be called by load(), require opened def file for loading from it. Returns true if image is loaded
	bool loadFrame(size_t frame, size_t group);

	//unloadFrame, returns true if image has been unloaded ( either deleted or decreased refCount)
	bool unloadFrame(size_t frame, size_t group);

	//initialize animation from file
	void initFromJson(const JsonNode & input);
	void init();

	//to get rid of copy-pasting error message :]
	void printError(size_t frame, size_t group, std::string type) const;

	//not a very nice method to get image from another def file
	//TODO: remove after implementing resource manager
	std::shared_ptr<IImage> getFromExtraDef(std::string filename);

public:
	CAnimation(std::string Name);
	CAnimation();
	~CAnimation();

	//duplicates frame at [sourceGroup, sourceFrame] as last frame in targetGroup
	//and loads it if animation is preloaded
	void duplicateImage(const size_t sourceGroup, const size_t sourceFrame, const size_t targetGroup);

	// adjust the color of the animation, used in battle spell effects, e.g. Cloned objects
	void shiftColor(const ColorShifter * shifter);

	//add custom surface to the selected position.
	void setCustom(std::string filename, size_t frame, size_t group=0);

	std::shared_ptr<IImage> getImage(size_t frame, size_t group=0, bool verbose=true) const;

	void exportBitmaps(const boost::filesystem::path & path) const;

	//all available frames
	void load  ();
	void unload();
	void preload();

	//all frames from group
	void loadGroup  (size_t group);
	void unloadGroup(size_t group);

	//single image
	void load  (size_t frame, size_t group=0);
	void unload(size_t frame, size_t group=0);

	//total count of frames in group (including not loaded)
	size_t size(size_t group=0) const;

	void horizontalFlip();
	void verticalFlip();
	void playerColored(PlayerColor player);

	void createFlippedGroup(const size_t sourceGroup, const size_t targetGroup);
};

const float DEFAULT_DELTA = 0.05f;

class CFadeAnimation
{
public:
	enum class EMode
	{
		NONE, IN, OUT
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
