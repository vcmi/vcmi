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

class IImage;
class CAnimation;
enum class EImageBlitMode : uint8_t;

class IRenderHandler : public boost::noncopyable
{
public:
	virtual ~IRenderHandler() = default;

	/// Must be called once VLC loading is over to initialize icons
	virtual void onLibraryLoadingFinished(const Services * services) = 0;

	/// Loads image using given path
	virtual std::shared_ptr<IImage> loadImage(const ImageLocator & locator, EImageBlitMode mode) = 0;
	virtual std::shared_ptr<IImage> loadImage(const ImagePath & path, EImageBlitMode mode) = 0;
	virtual std::shared_ptr<IImage> loadImage(const AnimationPath & path, int frame, int group, EImageBlitMode mode) = 0;

	/// temporary compatibility method. Creates IImage from existing SDL_Surface
	/// Surface will be shared, caller must still free it with SDL_FreeSurface
	virtual std::shared_ptr<IImage> createImage(SDL_Surface * source) = 0;

	/// Loads animation using given path
	virtual std::shared_ptr<CAnimation> loadAnimation(const AnimationPath & path, EImageBlitMode mode) = 0;
};
