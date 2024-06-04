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

class CDefFile;
class IConstImage;

class RenderHandler : public IRenderHandler
{
	using AnimationLayoutMap = std::map<size_t, std::vector<JsonNode>>;

	struct AnimationLocator
	{
		AnimationPath animation;
		int frame = -1;
		int group = -1;

		bool operator < (const AnimationLocator & other) const
		{
			if (animation != other.animation)
				return animation < other.animation;
			if (group != other.group)
				return group < other.group;
			return frame < other.frame;
		}
	};

	std::map<AnimationPath, std::shared_ptr<CDefFile>> animationFiles;
	std::map<AnimationPath, AnimationLayoutMap> animationLayouts;
	std::map<ImagePath, std::shared_ptr<IConstImage>> imageFiles;
	std::map<AnimationLocator, std::shared_ptr<IConstImage>> animationFrames;

	std::shared_ptr<CDefFile> getAnimationFile(const AnimationPath & path);
	const AnimationLayoutMap & getAnimationLayout(const AnimationPath & path);
	void initFromJson(AnimationLayoutMap & layout, const JsonNode & config);
public:
	// IRenderHandler implementation

	std::shared_ptr<IImage> loadImage(const ImagePath & path) override;
	std::shared_ptr<IImage> loadImage(const ImagePath & path, EImageBlitMode mode) override;
	std::shared_ptr<IImage> loadImage(const AnimationPath & path, int frame, int group) override;

	std::shared_ptr<CAnimation> loadAnimation(const AnimationPath & path) override;

	std::shared_ptr<IImage> createImage(SDL_Surface * source) override;
	std::shared_ptr<CAnimation> createAnimation() override;
};
