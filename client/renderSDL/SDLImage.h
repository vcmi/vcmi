/*
 * SDLImage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../render/IImage.h"
#include "../../lib/Point.h"

VCMI_LIB_NAMESPACE_BEGIN
class JsonNode;
VCMI_LIB_NAMESPACE_END

class CDefFile;

struct SDL_Surface;
struct SDL_Palette;

/*
 * Wrapper around SDL_Surface
 */
class SDLImageConst final : public IConstImage, public std::enable_shared_from_this<SDLImageConst>, boost::noncopyable
{
	//Surface without empty borders
	SDL_Surface * surf;

	SDL_Palette * originalPalette;
	//size of left and top borders
	Point margins;
	//total size including borders
	Point fullSize;

	// Keep the original palette, in order to do color switching operation
	void savePalette();

public:
	//Load image from def file
	SDLImageConst(CDefFile *data, size_t frame, size_t group=0);
	//Load from bitmap file
	SDLImageConst(const ImagePath & filename);
	//Create using existing surface, extraRef will increase refcount on SDL_Surface
	SDLImageConst(SDL_Surface * from);
	~SDLImageConst();

	void draw(SDL_Surface * where, SDL_Palette * palette, const Point & dest, const Rect * src, uint8_t alpha, EImageBlitMode mode) const;

	void exportBitmap(const boost::filesystem::path & path) const override;
	Point dimensions() const override;
	bool isTransparent(const Point & coords) const override;
	std::shared_ptr<IImage> createImageReference(EImageBlitMode mode) override;
	std::shared_ptr<IConstImage> horizontalFlip() const override;
	std::shared_ptr<IConstImage> verticalFlip() const override;
	std::shared_ptr<SDLImageConst> scaleFast(const Point & size) const;

	const SDL_Palette * getPalette() const;

	friend class SDLImageLoader;
};

class SDLImageBase : public IImage, boost::noncopyable
{
protected:
	std::shared_ptr<SDLImageConst> image;

	uint8_t alphaValue;
	EImageBlitMode blitMode;

public:
	SDLImageBase(const std::shared_ptr<SDLImageConst> & image, EImageBlitMode mode);

	void scaleFast(const Point & size) override;
	void exportBitmap(const boost::filesystem::path & path) const override;
	bool isTransparent(const Point & coords) const override;
	Point dimensions() const override;
	void setAlpha(uint8_t value) override;
	void setBlitMode(EImageBlitMode mode) override;
};

class SDLImageIndexed final : public SDLImageBase
{
	SDL_Palette * currentPalette = nullptr;

public:
	SDLImageIndexed(const std::shared_ptr<SDLImageConst> & image, EImageBlitMode mode);
	~SDLImageIndexed();

	void draw(SDL_Surface * where, const Point & pos, const Rect * src) const override;
	void setSpecialPalette(const SpecialPalette & SpecialPalette, uint32_t colorsToSkipMask) override;
	void playerColored(PlayerColor player) override;
	void setFlagColor(PlayerColor player) override;
	void shiftPalette(uint32_t firstColorID, uint32_t colorsToMove, uint32_t distanceToMove) override;
	void adjustPalette(const ColorFilter & shifter, uint32_t colorsToSkipMask) override;
};

class SDLImageRGB final : public SDLImageBase
{
public:
	using SDLImageBase::SDLImageBase;

	void draw(SDL_Surface * where, const Point & pos, const Rect * src) const override;
	void setSpecialPalette(const SpecialPalette & SpecialPalette, uint32_t colorsToSkipMask) override;
	void playerColored(PlayerColor player) override;
	void setFlagColor(PlayerColor player) override;
	void shiftPalette(uint32_t firstColorID, uint32_t colorsToMove, uint32_t distanceToMove) override;
	void adjustPalette(const ColorFilter & shifter, uint32_t colorsToSkipMask) override;
};
