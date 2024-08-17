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
class SDLImageShared final : public ISharedImage, public std::enable_shared_from_this<SDLImageShared>, boost::noncopyable
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

	void optimizeSurface();

public:
	//Load image from def file
	SDLImageShared(const CDefFile *data, size_t frame, size_t group=0);
	//Load from bitmap file
	SDLImageShared(const ImagePath & filename);
	//Create using existing surface, extraRef will increase refcount on SDL_Surface
	SDLImageShared(SDL_Surface * from);
	~SDLImageShared();

	void draw(SDL_Surface * where, SDL_Palette * palette, const Point & dest, const Rect * src, const ColorRGBA & colorMultiplier, uint8_t alpha, EImageBlitMode mode) const override;

	void exportBitmap(const boost::filesystem::path & path) const override;
	Point dimensions() const override;
	bool isTransparent(const Point & coords) const override;
	std::shared_ptr<IImage> createImageReference(EImageBlitMode mode) override;
	std::shared_ptr<ISharedImage> horizontalFlip() const override;
	std::shared_ptr<ISharedImage> verticalFlip() const override;
	std::shared_ptr<ISharedImage> scaleInteger(int factor, SDL_Palette * palette) const override;
	std::shared_ptr<ISharedImage> scaleTo(const Point & size, SDL_Palette * palette) const override;

	friend class SDLImageLoader;
};

class SDLImageBase : public IImage, boost::noncopyable
{
protected:
	std::shared_ptr<ISharedImage> image;

	uint8_t alphaValue;
	EImageBlitMode blitMode;

public:
	SDLImageBase(const std::shared_ptr<ISharedImage> & image, EImageBlitMode mode);

	void exportBitmap(const boost::filesystem::path & path) const override;
	bool isTransparent(const Point & coords) const override;
	Point dimensions() const override;
	void setAlpha(uint8_t value) override;
	void setBlitMode(EImageBlitMode mode) override;
	std::shared_ptr<ISharedImage> getSharedImage() const override;
};

class SDLImageIndexed final : public SDLImageBase
{
	SDL_Palette * currentPalette = nullptr;
	SDL_Palette * originalPalette = nullptr;

	bool bodyEnabled = true;
	bool shadowEnabled = false;
	bool overlayEnabled = false;

	void setShadowTransparency(float factor);
public:
	SDLImageIndexed(const std::shared_ptr<ISharedImage> & image, SDL_Palette * palette, EImageBlitMode mode);
	~SDLImageIndexed();

	void draw(SDL_Surface * where, const Point & pos, const Rect * src) const override;
	void setOverlayColor(const ColorRGBA & color) override;
	void playerColored(PlayerColor player) override;
	void shiftPalette(uint32_t firstColorID, uint32_t colorsToMove, uint32_t distanceToMove) override;
	void adjustPalette(const ColorFilter & shifter, uint32_t colorsToSkipMask) override;
	void scaleInteger(int factor) override;
	void scaleTo(const Point & size) override;

	void setShadowEnabled(bool on) override;
	void setBodyEnabled(bool on) override;
	void setOverlayEnabled(bool on) override;
};

class SDLImageRGB final : public SDLImageBase
{
public:
	using SDLImageBase::SDLImageBase;

	void draw(SDL_Surface * where, const Point & pos, const Rect * src) const override;
	void setOverlayColor(const ColorRGBA & color) override;
	void playerColored(PlayerColor player) override;
	void shiftPalette(uint32_t firstColorID, uint32_t colorsToMove, uint32_t distanceToMove) override;
	void adjustPalette(const ColorFilter & shifter, uint32_t colorsToSkipMask) override;
	void scaleInteger(int factor) override;
	void scaleTo(const Point & size) override;

	void setShadowEnabled(bool on) override;
	void setBodyEnabled(bool on) override;
	void setOverlayEnabled(bool on) override;
};
