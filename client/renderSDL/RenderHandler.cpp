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
#include "ScalableImage.h"
#include "FontChain.h"

#include "../GameEngine.h"

#include "../render/AssetGenerator.h"
#include "../render/CAnimation.h"
#include "../render/CanvasImage.h"
#include "../render/CDefFile.h"
#include "../render/Colors.h"
#include "../render/ColorFilter.h"
#include "../render/IScreenHandler.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CThreadHelper.h"
#include "../../lib/ExceptionsCommon.h"
#include "../../lib/VCMIDirs.h"
#include "../../lib/constants/StringConstants.h"
#include "../../lib/entities/building/CBuilding.h"
#include "../../lib/entities/faction/CTown.h"
#include "../../lib/entities/faction/CTownHandler.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/json/JsonUtils.h"

#include <vcmi/ArtifactService.h>
#include <vcmi/CreatureService.h>
#include <vcmi/Entity.h>
#include <vcmi/FactionService.h>
#include <vcmi/HeroTypeService.h>
#include <vcmi/Services.h>
#include <vcmi/SkillService.h>
#include <vcmi/spells/Service.h>

RenderHandler::RenderHandler()
	:assetGenerator(std::make_unique<AssetGenerator>())
{
}

RenderHandler::~RenderHandler() = default;

std::shared_ptr<CDefFile> RenderHandler::getAnimationFile(const AnimationPath & path)
{
	AnimationPath actualPath = boost::starts_with(path.getName(), "SPRITES") ? path : path.addPrefix("SPRITES/");

	auto it = animationFiles.find(actualPath);

	if (it != animationFiles.end())
	{
		auto locked = it->second.lock();
		if (locked)
			return locked;
	}

	if (!CResourceHandler::get()->existsResource(actualPath))
		return nullptr;

	auto result = std::make_shared<CDefFile>(actualPath);

	animationFiles[actualPath] = result;
	return result;
}

void RenderHandler::initFromJson(AnimationLayoutMap & source, const JsonNode & config, EImageBlitMode mode) const
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
			source[groupID].emplace_back(toAdd, mode);
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

		source[group][frame] = ImageLocator(toAdd, mode);
	}
}

RenderHandler::AnimationLayoutMap & RenderHandler::getAnimationLayout(const AnimationPath & path, int scalingFactor, EImageBlitMode mode)
{
	static constexpr std::array scaledSpritesPath = {
		"", // 0x
		"SPRITES/",
		"SPRITES2X/",
		"SPRITES3X/",
		"SPRITES4X/",
	};

	std::string pathString = path.getName();

	if (boost::starts_with(pathString, "SPRITES/"))
		pathString = pathString.substr(std::string("SPRITES/").length());

	AnimationPath actualPath = AnimationPath::builtin(scaledSpritesPath.at(scalingFactor) + pathString);

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

		const JsonNode config(reinterpret_cast<const std::byte*>(textData.get()), stream->getSize(), path.getOriginalName());

		initFromJson(result, config, mode);
	}

	animationLayouts[actualPath] = result;
	return animationLayouts[actualPath];
}

int RenderHandler::getScalingFactor() const
{
	return ENGINE->screenHandler().getScalingFactor();
}

ImageLocator RenderHandler::getLocatorForAnimationFrame(const AnimationPath & path, int frame, int group, int scaling, EImageBlitMode mode)
{
	const auto & layout = getAnimationLayout(path, scaling, mode);
	if (!layout.count(group))
		return ImageLocator();

	if (frame >= layout.at(group).size())
		return ImageLocator();

	const auto & locator = layout.at(group).at(frame);
	if (locator.image || locator.defFile)
		return locator;

	return ImageLocator(path, frame, group, mode);
}

std::shared_ptr<ScalableImageShared> RenderHandler::loadImageImpl(const ImageLocator & locator)
{
	auto it = imageFiles.find(locator);
	if (it != imageFiles.end())
	{
		auto locked = it->second.lock();
		if (locked)
			return locked;
	}

	auto sdlImage = loadImageFromFileUncached(locator);
	auto scaledImage = std::make_shared<ScalableImageShared>(locator, sdlImage);

	storeCachedImage(locator, scaledImage);
	return scaledImage;
}

std::shared_ptr<ISharedImage> RenderHandler::loadImageFromFileUncached(const ImageLocator & locator)
{
	if(locator.image)
	{
		auto imagePath = *locator.image;
		auto imagePathSprites = imagePath.addPrefix("SPRITES/");
		auto imagePathData = imagePath.addPrefix("DATA/");

		if(CResourceHandler::get()->existsResource(imagePathSprites))
			return std::make_shared<SDLImageShared>(imagePathSprites);

		if(CResourceHandler::get()->existsResource(imagePathData))
			return std::make_shared<SDLImageShared>(imagePathData);

		if(CResourceHandler::get()->existsResource(imagePath))
			return std::make_shared<SDLImageShared>(imagePath);

		auto generated = assetGenerator->generateImage(imagePath);
		if (generated)
			return generated;

		logGlobal->error("Failed to load image %s", locator.image->getOriginalName());
		return std::make_shared<SDLImageShared>(ImagePath::builtin("DEFAULT"));
	}

	if(locator.defFile)
	{
		auto defFile = getAnimationFile(*locator.defFile);
		if(defFile->hasFrame(locator.defFrame, locator.defGroup))
			return std::make_shared<SDLImageShared>(defFile.get(), locator.defFrame, locator.defGroup);
		else
		{
			logGlobal->error("Frame %d in group %d not found in file: %s", 
				locator.defFrame, locator.defGroup, locator.defFile->getName().c_str());
			return std::make_shared<SDLImageShared>(ImagePath::builtin("DEFAULT"));
		}
	}

	throw std::runtime_error("Invalid image locator received!");
}

void RenderHandler::storeCachedImage(const ImageLocator & locator, std::shared_ptr<ScalableImageShared> image)
{
	imageFiles[locator] = image;
}

std::shared_ptr<SDLImageShared> RenderHandler::loadScaledImage(const ImageLocator & locator)
{
	static constexpr std::array scaledDataPath = {
		"", // 0x
		"DATA/",
		"DATA2X/",
		"DATA3X/",
		"DATA4X/",
	};

	static constexpr std::array scaledSpritesPath = {
		"", // 0x
		"SPRITES/",
		"SPRITES2X/",
		"SPRITES3X/",
		"SPRITES4X/",
	};

	ImagePath pathToLoad;

	if(locator.defFile)
	{
		auto remappedLocator = getLocatorForAnimationFrame(*locator.defFile, locator.defFrame, locator.defGroup, locator.scalingFactor, locator.layer);
		// we expect that .def's are only used for 1x data, upscaled assets should use standalone images
		if (!remappedLocator.image)
			return nullptr;

		pathToLoad = *remappedLocator.image;
	}

	if(locator.image)
		pathToLoad = *locator.image;

	if (pathToLoad.empty())
		return nullptr;

	std::string imagePathString = pathToLoad.getName();

	if(locator.layer == EImageBlitMode::ONLY_FLAG_COLOR || locator.layer == EImageBlitMode::ONLY_SELECTION)
		imagePathString += "-OVERLAY";
	if(locator.layer == EImageBlitMode::ONLY_SHADOW_HIDE_SELECTION || locator.layer == EImageBlitMode::ONLY_SHADOW_HIDE_FLAG_COLOR)
		imagePathString += "-SHADOW";
	if(locator.playerColored.isValidPlayer())
		imagePathString += "-" + boost::to_upper_copy(GameConstants::PLAYER_COLOR_NAMES[locator.playerColored.getNum()]);
	if(locator.playerColored == PlayerColor::NEUTRAL)
		imagePathString += "-NEUTRAL";

	auto imagePath = ImagePath::builtin(imagePathString);
	auto imagePathSprites = ImagePath::builtin(imagePathString).addPrefix(scaledSpritesPath.at(locator.scalingFactor));
	auto imagePathData = ImagePath::builtin(imagePathString).addPrefix(scaledDataPath.at(locator.scalingFactor));

	if(CResourceHandler::get()->existsResource(imagePathSprites))
		return std::make_shared<SDLImageShared>(imagePathSprites);

	if(CResourceHandler::get()->existsResource(imagePathData))
		return std::make_shared<SDLImageShared>(imagePathData);

	if(CResourceHandler::get()->existsResource(imagePath))
		return std::make_shared<SDLImageShared>(imagePath);

	return nullptr;
}

std::shared_ptr<IImage> RenderHandler::loadImage(const ImageLocator & locator)
{
	ImageLocator adjustedLocator = locator;

	std::shared_ptr<ScalableImageInstance> result;

	if (adjustedLocator.scalingFactor == 0)
	{
		auto scaledLocator = adjustedLocator;
		scaledLocator.scalingFactor = getScalingFactor();

		result = loadImageImpl(scaledLocator)->createImageReference();
	}
	else
		result = loadImageImpl(adjustedLocator)->createImageReference();

	if (locator.horizontalFlip)
		result->horizontalFlip();
	if (locator.verticalFlip)
		result->verticalFlip();

	return result;
}

std::shared_ptr<IImage> RenderHandler::loadImage(const AnimationPath & path, int frame, int group, EImageBlitMode mode)
{
	ImageLocator locator = getLocatorForAnimationFrame(path, frame, group, 1, mode);
	if (!locator.empty())
		return loadImage(locator);
	else
	{
		logGlobal->error("Failed to load non-existing image");
		return loadImage(ImageLocator(ImagePath::builtin("DEFAULT"), mode));
	}
}

std::shared_ptr<IImage> RenderHandler::loadImage(const ImagePath & path, EImageBlitMode mode)
{
	ImageLocator locator(path, mode);
	return loadImage(locator);
}

std::shared_ptr<CanvasImage> RenderHandler::createImage(const Point & size, CanvasScalingPolicy scalingPolicy)
{
	return std::make_shared<CanvasImage>(size, scalingPolicy);
}

std::shared_ptr<CAnimation> RenderHandler::loadAnimation(const AnimationPath & path, EImageBlitMode mode)
{
	return std::make_shared<CAnimation>(path, getAnimationLayout(path, 1, mode), mode);
}

void RenderHandler::addImageListEntries(const EntityService * service)
{
	service->forEachBase([this](const Entity * entity, bool & stop)
	{
		entity->registerIcons([this](size_t index, size_t group, const std::string & listName, const std::string & imageName)
		{
			if (imageName.empty())
				return;

			auto & layout = getAnimationLayout(AnimationPath::builtin("SPRITES/" + listName), 1, EImageBlitMode::COLORKEY);

			JsonNode entry;
			entry["file"].String() = imageName;

			if (index >= layout[group].size())
				layout[group].resize(index + 1);

			layout[group][index] = ImageLocator(entry, EImageBlitMode::SIMPLE);
		});
	});
}

static void detectOverlappingBuildings(RenderHandler * renderHandler, const Faction * factionBase)
{
	if (!factionBase->hasTown())
		return;

	auto faction = dynamic_cast<const CFaction*>(factionBase);

	for (const auto & left : faction->town->clientInfo.structures)
	{
		for (const auto & right : faction->town->clientInfo.structures)
		{
			if (left->identifier <= right->identifier)
				continue; // only a<->b comparison is needed, not a<->a or b<->a

			if (left->building && right->building && left->building->getBase() == right->building->getBase())
			{
				if (left->pos.z != right->pos.z)
					logMod->warn("Town %s: Upgrades of same building have different z-index: '%s' and '%s'", faction->getJsonKey(), left->identifier, right->identifier);

				continue; // upgrades of the same buildings are expected to overlap
			}

			if (left->pos.z != right->pos.z)
				continue; // buildings already have different z-index and have well-defined overlap logic

			auto leftImage = renderHandler->loadImage(left->defName, 0, 0, EImageBlitMode::COLORKEY);
			auto rightImage = renderHandler->loadImage(right->defName, 0, 0, EImageBlitMode::COLORKEY);

			Rect leftRect( left->pos.x, left->pos.y, leftImage->width(), leftImage->height());
			Rect rightRect( right->pos.x, right->pos.y, rightImage->width(), rightImage->height());

			Rect intersection = leftRect.intersect(rightRect);

			Point intersectionPosition;
			bool intersectionFound = false;

			for (int y = 0; y < intersection.h && !intersectionFound; ++y)
			{
				for (int x = 0; x < intersection.w && !intersectionFound; ++x)
				{
					Point leftPoint = Point(x,y) - leftRect.topLeft() + intersection.topLeft();
					Point rightPoint = Point(x,y) - rightRect.topLeft() + intersection.topLeft();

					if (!leftImage->isTransparent(leftPoint) && !rightImage->isTransparent(rightPoint))
					{
						intersectionFound = true;
						intersectionPosition = intersection.topLeft() + Point(x,y);
					}
				}
			}

			if (intersectionFound)
				logMod->warn("Town %s: Detected overlapping buildings '%s' and '%s' at (%d, %d) with same z-index!", faction->getJsonKey(), left->identifier, right->identifier, intersectionPosition.x, intersectionPosition.y);
		}
	}
};

void RenderHandler::onLibraryLoadingFinished(const Services * services)
{
	assert(animationLayouts.empty());
	assetGenerator->initialize();
	animationLayouts = assetGenerator->generateAllAnimations();

	addImageListEntries(services->creatures());
	addImageListEntries(services->heroTypes());
	addImageListEntries(services->artifacts());
	addImageListEntries(services->factions());
	addImageListEntries(services->spells());
	addImageListEntries(services->skills());

	if (settings["mods"]["validation"].String() == "full")
	{
		services->factions()->forEach([this](const Faction * factionBase, bool & stop)
		{
			detectOverlappingBuildings(this, factionBase);
		});
	}
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
			loadedFont->addTrueTypeFont(ttfConf[bitmapPath], !config["lowPriority"].Bool());
	}
	loadedFont->addBitmapFont(bitmapPath);

	fonts[font] = loadedFont;
	return loadedFont;
}

void RenderHandler::exportGeneratedAssets()
{
	for (const auto & entry : assetGenerator->generateAllImages())
		entry.second->exportBitmap(VCMIDirs::get().userDataPath() / "Generated" / (entry.first.getOriginalName() + ".png"), nullptr);
}
