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
#include "../render/CDefFile.h"

VCMI_LIB_NAMESPACE_BEGIN
class EntityService;
VCMI_LIB_NAMESPACE_END

class CDefFile;
class SDLImageShared;
class ScalableImageShared;
class AssetGenerator;
class HdImageLoader;

class RenderHandler final : public IRenderHandler
{
	using AnimationLayoutMap = std::map<size_t, std::vector<ImageLocator>>;

	std::map<AnimationPath, std::map<int, std::map<int, std::pair<std::string, CDefFile::SSpriteDef>>>> animationSpriteDefs;
	std::map<AnimationPath, std::weak_ptr<CDefFile>> animationFiles;
	std::map<AnimationPath, AnimationLayoutMap> animationLayouts;
	std::map<SharedImageLocator, std::weak_ptr<ScalableImageShared>> imageFiles;
	std::map<EFonts, std::shared_ptr<const IFont>> fonts;
	std::shared_ptr<AssetGenerator> assetGenerator;
	std::shared_ptr<HdImageLoader> hdImageLoader;
	
	std::mutex animationCacheMutex;

	std::pair<std::string, CDefFile::SSpriteDef> getAnimationSpriteDef(const AnimationPath & path, int frame, int group);
	ImagePath getAnimationFrameName(const AnimationPath & path, int frame, int group);
	std::shared_ptr<CDefFile> getAnimationFile(const AnimationPath & path);
	AnimationLayoutMap & getAnimationLayout(const AnimationPath & path, int scalingFactor, EImageBlitMode mode);
	void initFromJson(AnimationLayoutMap & layout, const JsonNode & config, EImageBlitMode mode) const;

	void addImageListEntry(size_t index, size_t group, const std::string & listName, const std::string & imageName);
	void addImageListEntries(const EntityService * service);
	void storeCachedImage(const ImageLocator & locator, std::shared_ptr<ScalableImageShared> image);

	std::shared_ptr<ScalableImageShared> loadImageImpl(const ImageLocator & config);

	std::shared_ptr<ISharedImage> loadImageFromFileUncached(const ImageLocator & locator);

	ImageLocator getLocatorForAnimationFrame(const AnimationPath & path, int frame, int group, int scaling, EImageBlitMode mode);

	int getScalingFactor() const;

public:
	RenderHandler();
	~RenderHandler();

	// IRenderHandler implementation
	void onLibraryLoadingFinished(const Services * services) override;

	std::shared_ptr<IImage> loadImage(const ImageLocator & locator) override;
	std::shared_ptr<IImage> loadImage(const ImagePath & path, EImageBlitMode mode) override;
	std::shared_ptr<IImage> loadImage(const AnimationPath & path, int frame, int group, EImageBlitMode mode) override;

	std::shared_ptr<SDLImageShared> loadScaledImage(const ImageLocator & locator) override;

	std::shared_ptr<CAnimation> loadAnimation(const AnimationPath & path, EImageBlitMode mode) override;

	std::shared_ptr<CanvasImage> createImage(const Point & size, CanvasScalingPolicy scalingPolicy) override;

	/// Returns font with specified identifer
	std::shared_ptr<const IFont> loadFont(EFonts font) override;

	void exportGeneratedAssets() override;

	std::shared_ptr<AssetGenerator> getAssetGenerator() override;
	void updateGeneratedAssets() override;
	
	/// Preload all animation files asynchronously to fill the cache
	void preloadAnimationsAsync();
};
