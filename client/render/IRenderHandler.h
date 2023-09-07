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

#include "../../lib/filesystem/ResourcePath.h"

struct SDL_Surface;

class IImage;
class CAnimation;
enum class EImageBlitMode;

class IRenderHandler : public boost::noncopyable
{
public:
	virtual ~IRenderHandler() = default;

	/// Loads image using given path
	virtual std::shared_ptr<IImage> loadImage(const ImagePath & path) = 0;
	virtual std::shared_ptr<IImage> loadImage(const ImagePath & path, EImageBlitMode mode) = 0;

	/// temporary compatibility method. Creates IImage from existing SDL_Surface
	/// Surface will be shared, caller must still free it with SDL_FreeSurface
	virtual std::shared_ptr<IImage> createImage(SDL_Surface * source) = 0;

	/// Loads animation using given path
	virtual std::shared_ptr<CAnimation> loadAnimation(const AnimationPath & path) = 0;

	/// Creates empty CAnimation
	virtual std::shared_ptr<CAnimation> createAnimation() = 0;
};
