/*
 * IRenderHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ImageLocator.h"

VCMI_LIB_NAMESPACE_BEGIN
class Services;
VCMI_LIB_NAMESPACE_END

struct SDL_Surface;

class IFont;
class IImage;
class CAnimation;
class CanvasImage;
class SDLImageShared;
class AssetGenerator;
enum class EImageBlitMode : uint8_t;
enum class CanvasScalingPolicy;
enum EFonts : int8_t;

class IRenderHandler : public boost::noncopyable
{
public:
	virtual ~IRenderHandler() = default;

	/// Must be called once LIBRARY loading is over to initialize icons
	virtual void onLibraryLoadingFinished(const Services * services) = 0;

	/// Loads image using given path
	virtual std::shared_ptr<IImage> loadImage(const ImageLocator & locator) = 0;
	virtual std::shared_ptr<IImage> loadImage(const ImagePath & path, EImageBlitMode mode) = 0;
	virtual std::shared_ptr<IImage> loadImage(const AnimationPath & path, int frame, int group, EImageBlitMode mode) = 0;

	/// Loads single upscaled image without auto-scaling support
	virtual std::shared_ptr<SDLImageShared> loadScaledImage(const ImageLocator & locator) = 0;

	/// Creates image which can be used as target for drawing on
	virtual std::shared_ptr<CanvasImage> createImage(const Point & size, CanvasScalingPolicy scalingPolicy) = 0;

	/// Loads animation using given path
	virtual std::shared_ptr<CAnimation> loadAnimation(const AnimationPath & path, EImageBlitMode mode) = 0;

	/// Returns font with specified identifer
	virtual std::shared_ptr<const IFont> loadFont(EFonts font) = 0;

	virtual void exportGeneratedAssets() = 0;

	virtual std::shared_ptr<AssetGenerator> getAssetGenerator() = 0;

	virtual void updateGeneratedAssets() = 0;
};
