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

#include "SDLImage.h"
#include "ImageScaled.h"
#include "FontChain.h"

#include "../gui/CGuiHandler.h"

#include "../render/CAnimation.h"
#include "../render/CDefFile.h"
#include "../render/Colors.h"
#include "../render/ColorFilter.h"
#include "../render/IScreenHandler.h"
#include "../../lib/json/JsonUtils.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/VCMIDirs.h"

#include <vcmi/ArtifactService.h>
#include <vcmi/CreatureService.h>
#include <vcmi/Entity.h>
#include <vcmi/FactionService.h>
#include <vcmi/HeroTypeService.h>
#include <vcmi/Services.h>
#include <vcmi/SkillService.h>
#include <vcmi/spells/Service.h>

std::shared_ptr<CDefFile> RenderHandler::getAnimationFile(const AnimationPath & path)
{
	AnimationPath actualPath = boost::starts_with(path.getName(), "SPRITES") ? path : path.addPrefix("SPRITES/");

	auto it = animationFiles.find(actualPath);

	if (it != animationFiles.end())
		return it->second;

	if (!CResourceHandler::get()->existsResource(actualPath))
	{
		animationFiles[actualPath] = nullptr;
		return nullptr;
	}

	auto result = std::make_shared<CDefFile>(actualPath);

	animationFiles[actualPath] = result;
	return result;
}

std::optional<ResourcePath> RenderHandler::getPathForScaleFactor(const ResourcePath & path, const std::string & factor)
{
	if(path.getType() == EResType::IMAGE)
	{
		auto p = ImagePath::builtin(path.getName());
		if(CResourceHandler::get()->existsResource(p.addPrefix("SPRITES" + factor + "X/")))
			return std::optional<ResourcePath>(p.addPrefix("SPRITES" + factor + "X/"));
		if(CResourceHandler::get()->existsResource(p.addPrefix("DATA" + factor + "X/")))
			return std::optional<ResourcePath>(p.addPrefix("DATA" + factor + "X/"));
	}
	else
	{
		auto p = AnimationPath::builtin(path.getName());
		auto pJson = p.toType<EResType::JSON>();
		if(CResourceHandler::get()->existsResource(p.addPrefix("SPRITES" + factor + "X/")))
			return std::optional<ResourcePath>(p.addPrefix("SPRITES" + factor + "X/"));
		if(CResourceHandler::get()->existsResource(pJson))
			return std::optional<ResourcePath>(p);
		if(CResourceHandler::get()->existsResource(pJson.addPrefix("SPRITES" + factor + "X/")))
			return std::optional<ResourcePath>(p.addPrefix("SPRITES" + factor + "X/"));
	}

	return std::nullopt;
}

std::pair<ResourcePath, int> RenderHandler::getScalePath(const ResourcePath & p)
{
	auto path = p;
	int scaleFactor = 1;
	if(getScalingFactor() > 1)
	{
		std::vector<int> factorsToCheck = {getScalingFactor(), 4, 3, 2};
		for(auto factorToCheck : factorsToCheck)
		{
			std::string name = boost::algorithm::to_upper_copy(p.getName());
			boost::replace_all(name, "SPRITES/", std::string("SPRITES") + std::to_string(factorToCheck) + std::string("X/"));
			boost::replace_all(name, "DATA/", std::string("DATA") + std::to_string(factorToCheck) + std::string("X/"));
			ResourcePath scaledPath = ImagePath::builtin(name);
			if(p.getType() != EResType::IMAGE)
				scaledPath = AnimationPath::builtin(name);
			auto tmpPath = getPathForScaleFactor(scaledPath, std::to_string(factorToCheck));
			if(tmpPath)
			{
				path = *tmpPath;
				scaleFactor = factorToCheck;
				break;
			}
		}
	}

	return std::pair<ResourcePath, int>(path, scaleFactor);
};

void RenderHandler::initFromJson(AnimationLayoutMap & source, const JsonNode & config)
{
	std::string basepath;
	basepath = config["basepath"].String();

	JsonNode base;
	base["margins"] = config["margins"];
	base["width"] = config["width"];
	base["height"] = config["height"];

	for(const JsonNode & group : config["sequences"].Vector())
	{
		size_t groupID = group["group"].Integer();//TODO: string-to-value conversion("moving" -> MOVING)
		source[groupID].clear();

		for(const JsonNode & frame : group["frames"].Vector())
		{
			JsonNode toAdd = frame;
			JsonUtils::inherit(toAdd, base);
			toAdd["file"].String() = basepath + frame.String();
			source[groupID].emplace_back(toAdd);
		}
	}

	for(const JsonNode & node : config["images"].Vector())
	{
		size_t group = node["group"].Integer();
		size_t frame = node["frame"].Integer();

		if (source[group].size() <= frame)
			source[group].resize(frame+1);

		JsonNode toAdd = node;
		JsonUtils::inherit(toAdd, base);

		if (toAdd.Struct().count("file"))
			toAdd["file"].String() = basepath + node["file"].String();

		if (toAdd.Struct().count("defFile"))
			toAdd["defFile"].String() = basepath + node["defFile"].String();

		source[group][frame] = ImageLocator(toAdd);
	}
}

RenderHandler::AnimationLayoutMap & RenderHandler::getAnimationLayout(const AnimationPath & path)
{
	auto tmp = getScalePath(path);
	auto animPath = AnimationPath::builtin(tmp.first.getName());
	AnimationPath actualPath = boost::starts_with(animPath.getName(), "SPRITES") ? animPath : animPath.addPrefix("SPRITES/");

	auto it = animationLayouts.find(actualPath);

	if (it != animationLayouts.end())
		return it->second;

	AnimationLayoutMap result;

	auto defFile = getAnimationFile(actualPath);
	if(defFile)
	{
		const std::map<size_t, size_t> defEntries = defFile->getEntries();

		for (const auto & defEntry : defEntries)
			result[defEntry.first].resize(defEntry.second);
	}

	auto jsonResource = actualPath.toType<EResType::JSON>();
	auto configList = CResourceHandler::get()->getResourcesWithName(jsonResource);

	for(auto & loader : configList)
	{
		auto stream = loader->load(jsonResource);
		std::unique_ptr<ui8[]> textData(new ui8[stream->getSize()]);
		stream->read(textData.get(), stream->getSize());

		const JsonNode config(reinterpret_cast<const std::byte*>(textData.get()), stream->getSize(), animPath.getOriginalName());

		initFromJson(result, config);
	}

	for(auto & g : result)
		for(auto & i : g.second)
			i.preScaledFactor = tmp.second;

	animationLayouts[actualPath] = result;
	return animationLayouts[actualPath];
}

int RenderHandler::getScalingFactor() const
{
	return GH.screenHandler().getScalingFactor();
}

ImageLocator RenderHandler::getLocatorForAnimationFrame(const AnimationPath & path, int frame, int group)
{
	const auto & layout = getAnimationLayout(path);
	if (!layout.count(group))
		return ImageLocator(ImagePath::builtin("DEFAULT"));

	if (frame >= layout.at(group).size())
		return ImageLocator(ImagePath::builtin("DEFAULT"));

	const auto & locator = layout.at(group).at(frame);
	if (locator.image || locator.defFile)
		return locator;

	return ImageLocator(path, frame, group);
}

std::shared_ptr<const ISharedImage> RenderHandler::loadImageImpl(const ImageLocator & locator)
{
	auto it = imageFiles.find(locator);
	if (it != imageFiles.end())
		return it->second;

	// TODO: order should be different:
	// 1) try to find correctly scaled image
	// 2) if fails -> try to find correctly transformed
	// 3) if also fails -> try to find image from correct file
	// 4) load missing part of the sequence
	// TODO: check whether (load -> transform -> scale) or (load -> scale -> transform) order should be used for proper loading of pre-scaled data
	auto imageFromFile = loadImageFromFile(locator.copyFile());
	auto transformedImage = transformImage(locator.copyFileTransform(), imageFromFile);
	auto scaledImage = scaleImage(locator.copyFileTransformScale(), transformedImage);

	return scaledImage;
}

std::shared_ptr<const ISharedImage> RenderHandler::loadImageFromFileUncached(const ImageLocator & locator)
{
	if (locator.image)
	{
		// TODO: create EmptySharedImage class that will be instantiated if image does not exists or fails to load
		return std::make_shared<SDLImageShared>(*locator.image, locator.preScaledFactor);
	}

	if (locator.defFile)
	{
		auto defFile = getAnimationFile(*locator.defFile);
		int preScaledFactor = locator.preScaledFactor;
		if(!defFile) // no prescale for this frame
		{
			auto tmpPath = (*locator.defFile).getName();
			boost::algorithm::replace_all(tmpPath, "SPRITES2X/", "SPRITES/");
			boost::algorithm::replace_all(tmpPath, "SPRITES3X/", "SPRITES/");
			boost::algorithm::replace_all(tmpPath, "SPRITES4X/", "SPRITES/");
			preScaledFactor = 1;
			defFile = getAnimationFile(AnimationPath::builtin(tmpPath));
		}
		return std::make_shared<SDLImageShared>(defFile.get(), locator.defFrame, locator.defGroup, preScaledFactor);
	}

	throw std::runtime_error("Invalid image locator received!");
}

void RenderHandler::storeCachedImage(const ImageLocator & locator, std::shared_ptr<const ISharedImage> image)
{
	imageFiles[locator] = image;

#if 0
	const boost::filesystem::path outPath = VCMIDirs::get().userExtractedPath() / "imageCache" / (locator.toString() + ".png");
	boost::filesystem::path outDir = outPath;
	outDir.remove_filename();
	boost::filesystem::create_directories(outDir);
	image->exportBitmap(outPath , nullptr);
#endif
}

std::shared_ptr<const ISharedImage> RenderHandler::loadImageFromFile(const ImageLocator & locator)
{
	if (imageFiles.count(locator))
		return imageFiles.at(locator);

	auto result = loadImageFromFileUncached(locator);
	storeCachedImage(locator, result);
	return result;
}

std::shared_ptr<const ISharedImage> RenderHandler::transformImage(const ImageLocator & locator, std::shared_ptr<const ISharedImage> image)
{
	if (imageFiles.count(locator))
		return imageFiles.at(locator);

	auto result = image;

	if (locator.verticalFlip)
		result = result->verticalFlip();

	if (locator.horizontalFlip)
		result = result->horizontalFlip();

	storeCachedImage(locator, result);
	return result;
}

std::shared_ptr<const ISharedImage> RenderHandler::scaleImage(const ImageLocator & locator, std::shared_ptr<const ISharedImage> image)
{
	if (imageFiles.count(locator))
		return imageFiles.at(locator);

	auto handle = image->createImageReference(locator.layer);

	assert(locator.scalingFactor != 1); // should be filtered-out before
	if (locator.playerColored != PlayerColor::CANNOT_DETERMINE)
		handle->playerColored(locator.playerColored);

	handle->scaleInteger(locator.scalingFactor);

	auto result = handle->getSharedImage();
	storeCachedImage(locator, result);
	return result;
}

std::shared_ptr<IImage> RenderHandler::loadImage(const ImageLocator & locator, EImageBlitMode mode)
{
	ImageLocator adjustedLocator = locator;

	if(adjustedLocator.image)
	{
		std::string imgPath = (*adjustedLocator.image).getName();
		if(adjustedLocator.layer == EImageBlitMode::ONLY_OVERLAY)
			imgPath += "-OVERLAY";
		if(adjustedLocator.layer == EImageBlitMode::ONLY_SHADOW)
			imgPath += "-SHADOW";

		if(CResourceHandler::get()->existsResource(ImagePath::builtin(imgPath)) ||
		   CResourceHandler::get()->existsResource(ImagePath::builtin(imgPath).addPrefix("DATA/")) ||
		   CResourceHandler::get()->existsResource(ImagePath::builtin(imgPath).addPrefix("SPRITES/")))
			adjustedLocator.image = ImagePath::builtin(imgPath);
	}

	if(adjustedLocator.defFile && adjustedLocator.scalingFactor == 0)
	{
		auto tmp = getScalePath(*adjustedLocator.defFile);
		adjustedLocator.defFile = AnimationPath::builtin(tmp.first.getName());
		adjustedLocator.preScaledFactor = tmp.second;
	}
	if(adjustedLocator.image && adjustedLocator.scalingFactor == 0)
	{
		auto tmp = getScalePath(*adjustedLocator.image);
		adjustedLocator.image = ImagePath::builtin(tmp.first.getName());
		adjustedLocator.preScaledFactor = tmp.second;
	}

	if (adjustedLocator.scalingFactor == 0 && getScalingFactor() != 1 )
	{
		auto unscaledLocator = adjustedLocator;
		auto scaledLocator = adjustedLocator;

		unscaledLocator.scalingFactor = 1;
		scaledLocator.scalingFactor = getScalingFactor();
		auto unscaledImage = loadImageImpl(unscaledLocator);

		return std::make_shared<ImageScaled>(scaledLocator, unscaledImage, mode);
	}

	if (adjustedLocator.scalingFactor == 0)
	{
		auto scaledLocator = adjustedLocator;
		scaledLocator.scalingFactor = getScalingFactor();

		return loadImageImpl(scaledLocator)->createImageReference(mode);
	}
	else
		return loadImageImpl(adjustedLocator)->createImageReference(mode);
}

std::shared_ptr<IImage> RenderHandler::loadImage(const AnimationPath & path, int frame, int group, EImageBlitMode mode)
{
	auto tmp = getScalePath(path);
	ImageLocator locator = getLocatorForAnimationFrame(AnimationPath::builtin(tmp.first.getName()), frame, group);
	locator.preScaledFactor = tmp.second;
	return loadImage(locator, mode);
}

std::shared_ptr<IImage> RenderHandler::loadImage(const ImagePath & path, EImageBlitMode mode)
{
	ImageLocator locator(path);
	return loadImage(locator, mode);
}

std::shared_ptr<IImage> RenderHandler::createImage(SDL_Surface * source)
{
	return std::make_shared<SDLImageShared>(source)->createImageReference(EImageBlitMode::SIMPLE);
}

std::shared_ptr<CAnimation> RenderHandler::loadAnimation(const AnimationPath & path, EImageBlitMode mode)
{
	return std::make_shared<CAnimation>(path, getAnimationLayout(path), mode);
}

void RenderHandler::addImageListEntries(const EntityService * service)
{
	service->forEachBase([this](const Entity * entity, bool & stop)
	{
		entity->registerIcons([this](size_t index, size_t group, const std::string & listName, const std::string & imageName)
		{
			if (imageName.empty())
				return;

			auto & layout = getAnimationLayout(AnimationPath::builtin("SPRITES/" + listName));

			JsonNode entry;
			entry["file"].String() = imageName;

			if (index >= layout[group].size())
				layout[group].resize(index + 1);

			layout[group][index] = ImageLocator(entry);
		});
	});
}

void RenderHandler::onLibraryLoadingFinished(const Services * services)
{
	addImageListEntries(services->creatures());
	addImageListEntries(services->heroTypes());
	addImageListEntries(services->artifacts());
	addImageListEntries(services->factions());
	addImageListEntries(services->spells());
	addImageListEntries(services->skills());
}

std::shared_ptr<const IFont> RenderHandler::loadFont(EFonts font)
{
	if (fonts.count(font))
		return fonts.at(font);

	const int8_t index = static_cast<int8_t>(font);
	logGlobal->debug("Loading font %d", static_cast<int>(index));

	auto configList = CResourceHandler::get()->getResourcesWithName(JsonPath::builtin("config/fonts.json"));
	std::shared_ptr<FontChain> loadedFont = std::make_shared<FontChain>();
	std::string bitmapPath;

	for(auto & loader : configList)
	{
		auto stream = loader->load(JsonPath::builtin("config/fonts.json"));
		std::unique_ptr<ui8[]> textData(new ui8[stream->getSize()]);
		stream->read(textData.get(), stream->getSize());
		const JsonNode config(reinterpret_cast<const std::byte*>(textData.get()), stream->getSize(), "config/fonts.json");
		const JsonVector & bmpConf = config["bitmap"].Vector();
		const JsonNode   & ttfConf = config["trueType"];

		bitmapPath = bmpConf[index].String();
		if (!ttfConf[bitmapPath].isNull())
			loadedFont->addTrueTypeFont(ttfConf[bitmapPath]);
	}
	loadedFont->addBitmapFont(bitmapPath);

	fonts[font] = loadedFont;
	return loadedFont;
}
