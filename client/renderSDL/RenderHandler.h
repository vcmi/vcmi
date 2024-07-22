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

VCMI_LIB_NAMESPACE_BEGIN
class EntityService;
VCMI_LIB_NAMESPACE_END

class CDefFile;
class IConstImage;

class RenderHandler : public IRenderHandler
{
	using AnimationLayoutMap = std::map<size_t, std::vector<ImageLocator>>;

	std::map<AnimationPath, std::shared_ptr<CDefFile>> animationFiles;
	std::map<AnimationPath, AnimationLayoutMap> animationLayouts;
	std::map<ImageLocator, std::shared_ptr<IConstImage>> imageFiles;

	std::shared_ptr<CDefFile> getAnimationFile(const AnimationPath & path);
	AnimationLayoutMap & getAnimationLayout(const AnimationPath & path);
	void initFromJson(AnimationLayoutMap & layout, const JsonNode & config);

	void addImageListEntry(size_t index, size_t group, const std::string & listName, const std::string & imageName);
	void addImageListEntries(const EntityService * service);

	std::shared_ptr<IConstImage> loadImageFromSingleFile(const ImagePath & path);
	std::shared_ptr<IConstImage> loadImageFromAnimationFileUncached(const AnimationPath & path, int frame, int group);
	std::shared_ptr<IConstImage> loadImageFromAnimationFile(const AnimationPath & path, int frame, int group);
	std::shared_ptr<IConstImage> loadImageImpl(const ImageLocator & config);
public:

	// IRenderHandler implementation
	void onLibraryLoadingFinished(const Services * services) override;

	std::shared_ptr<IImage> loadImage(const ImageLocator & locator, EImageBlitMode mode) override;
	std::shared_ptr<IImage> loadImage(const ImagePath & path, EImageBlitMode mode) override;
	std::shared_ptr<IImage> loadImage(const AnimationPath & path, int frame, int group, EImageBlitMode mode) override;

	std::shared_ptr<CAnimation> loadAnimation(const AnimationPath & path, EImageBlitMode mode) override;

	std::shared_ptr<IImage> createImage(SDL_Surface * source) override;
};
