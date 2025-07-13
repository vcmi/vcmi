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
	SDL_Surface * surf = nullptr;

	SDL_Palette * originalPalette = nullptr;
	//size of left and top borders
	Point margins;
	//total size including borders
	Point fullSize;

	std::atomic_bool upscalingInProgress = false;

	// Keep the original palette, in order to do color switching operation
	void savePalette();

	void optimizeSurface();

public:
	//Load image from def file
	SDLImageShared(const CDefFile *data, size_t frame, size_t group=0);
	//Load from bitmap file
	SDLImageShared(const ImagePath & filename, bool optimizeImage=true);
	//Create using existing surface, extraRef will increase refcount on SDL_Surface
	SDLImageShared(SDL_Surface * from);
	~SDLImageShared();

	/// Creates image at specified scaling factor from source image
	static std::shared_ptr<SDLImageShared> createScaled(const SDLImageShared * from, int integerScaleFactor, EScalingAlgorithm algorithm);

	void scaledDraw(SDL_Surface * where, SDL_Palette * palette, const Point & scaling, const Point & dest, const Rect * src, const ColorRGBA & colorMultiplier, uint8_t alpha, EImageBlitMode mode) const override;
	void draw(SDL_Surface * where, SDL_Palette * palette, const Point & dest, const Rect * src, const ColorRGBA & colorMultiplier, uint8_t alpha, EImageBlitMode mode) const override;

	void exportBitmap(const boost::filesystem::path & path, SDL_Palette * palette) const override;
	Point dimensions() const override;
	bool isTransparent(const Point & coords) const override;
	Rect contentRect() const override;

	bool isLoading() const override;

	const SDL_Palette * getPalette() const override;

	[[nodiscard]] std::shared_ptr<const ISharedImage> horizontalFlip() const override;
	[[nodiscard]] std::shared_ptr<const ISharedImage> verticalFlip() const override;
	[[nodiscard]] std::shared_ptr<const ISharedImage> scaleInteger(int factor, SDL_Palette * palette, EImageBlitMode blitMode) const override;
	[[nodiscard]] std::shared_ptr<const ISharedImage> scaleTo(const Point & size, SDL_Palette * palette) const override;

	std::shared_ptr<SDLImageShared> drawShadow(bool doSheer) const;
	std::shared_ptr<SDLImageShared> drawOutline(const ColorRGBA & color, int thickness) const;

	friend class SDLImageLoader;
};
