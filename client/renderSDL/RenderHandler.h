/*
 * RenderHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../render/IRenderHandler.h"

class RenderHandler : public IRenderHandler
{
public:
	std::shared_ptr<IImage> loadImage(const ImagePath & path) override;
	std::shared_ptr<IImage> loadImage(const ImagePath & path, EImageBlitMode mode) override;

	std::shared_ptr<IImage> createImage(SDL_Surface * source) override;

	std::shared_ptr<CAnimation> loadAnimation(const AnimationPath & path) override;

	std::shared_ptr<CAnimation> createAnimation() override;
};
