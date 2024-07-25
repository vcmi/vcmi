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
class SDLImageShared;
class ISharedImage;

class RenderHandler : public IRenderHandler
{
	using AnimationLayoutMap = std::map<size_t, std::vector<ImageLocator>>;

	std::map<AnimationPath, std::shared_ptr<CDefFile>> animationFiles;
	std::map<AnimationPath, AnimationLayoutMap> animationLayouts;
	std::map<ImageLocator, std::shared_ptr<ISharedImage>> imageFiles;

	std::shared_ptr<CDefFile> getAnimationFile(const AnimationPath & path);
	AnimationLayoutMap & getAnimationLayout(const AnimationPath & path);
	void initFromJson(AnimationLayoutMap & layout, const JsonNode & config);

	void addImageListEntry(size_t index, size_t group, const std::string & listName, const std::string & imageName);
	void addImageListEntries(const EntityService * service);

	std::shared_ptr<ISharedImage> loadImageImpl(const ImageLocator & config);

	std::shared_ptr<ISharedImage> loadImageFromFileUncached(const ImageLocator & locator);
	std::shared_ptr<ISharedImage> loadImageFromFile(const ImageLocator & locator);

	std::shared_ptr<ISharedImage> transformImage(const ImageLocator & locator, std::shared_ptr<ISharedImage> image);
	std::shared_ptr<ISharedImage> scaleImage(const ImageLocator & locator, std::shared_ptr<ISharedImage> image);

	ImageLocator getLocatorForAnimationFrame(const AnimationPath & path, int frame, int group);

	int getScalingFactor() const;

	std::shared_ptr<IImage> createImageReference(const ImageLocator & locator, std::shared_ptr<ISharedImage> input, EImageBlitMode mode);
public:

	// IRenderHandler implementation
	void onLibraryLoadingFinished(const Services * services) override;

	std::shared_ptr<IImage> loadImage(const ImageLocator & locator, EImageBlitMode mode) override;
	std::shared_ptr<IImage> loadImage(const ImagePath & path, EImageBlitMode mode) override;
	std::shared_ptr<IImage> loadImage(const AnimationPath & path, int frame, int group, EImageBlitMode mode) override;

	std::shared_ptr<CAnimation> loadAnimation(const AnimationPath & path, EImageBlitMode mode) override;

	std::shared_ptr<IImage> createImage(SDL_Surface * source) override;
};
