/*
 * RenderHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "RenderHandler.h"

#include "../render/CAnimation.h"
#include "SDLImage.h"


std::shared_ptr<IImage> RenderHandler::loadImage(const ImagePath & path)
{
	return loadImage(path, EImageBlitMode::ALPHA);
}

std::shared_ptr<IImage> RenderHandler::loadImage(const ImagePath & path, EImageBlitMode mode)
{
	return std::make_shared<SDLImage>(path, mode);
}

std::shared_ptr<IImage> RenderHandler::createImage(SDL_Surface * source)
{
	return std::make_shared<SDLImage>(source, EImageBlitMode::ALPHA);
}

std::shared_ptr<CAnimation> RenderHandler::loadAnimation(const AnimationPath & path)
{
	return std::make_shared<CAnimation>(path);
}

std::shared_ptr<CAnimation> RenderHandler::createAnimation()
{
	return std::make_shared<CAnimation>();
}
