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

#include "../gui/CGuiHandler.h"

#include "../render/CAnimation.h"
#include "../render/CDefFile.h"
#include "../render/Colors.h"
#include "../render/ColorFilter.h"
#include "../render/IScreenHandler.h"

#include "../../lib/json/JsonUtils.h"
#include "../../lib/filesystem/Filesystem.h"

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
		toAdd["file"].String() = basepath + node["file"].String();
		source[group][frame] = ImageLocator(toAdd);
	}
}

RenderHandler::AnimationLayoutMap & RenderHandler::getAnimationLayout(const AnimationPath & path)
{
	AnimationPath actualPath = boost::starts_with(path.getName(), "SPRITES") ? path : path.addPrefix("SPRITES/");

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

		initFromJson(result, config);
	}

	animationLayouts[actualPath] = result;
	return animationLayouts[actualPath];
}

int RenderHandler::getScalingFactor() const
{
	return GH.screenHandler().getScalingFactor();
}

std::shared_ptr<IImage> RenderHandler::createImageReference(const ImageLocator & locator, std::shared_ptr<ISharedImage> input, EImageBlitMode mode)
{
	if (getScalingFactor() == 1 || locator.scalingFactor != 1 || locator.empty())
		return input->createImageReference(mode);
	else
		return std::make_shared<ImageScaled>(locator, input, mode);
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

std::shared_ptr<ISharedImage> RenderHandler::loadImageImpl(const ImageLocator & locator)
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

std::shared_ptr<ISharedImage> RenderHandler::loadImageFromFileUncached(const ImageLocator & locator)
{
	if (locator.image)
	{
		// TODO: create EmptySharedImage class that will be instantiated if image does not exists or fails to load
		return std::make_shared<SDLImageShared>(*locator.image);
	}

	if (locator.defFile)
	{
		auto defFile = getAnimationFile(*locator.defFile);
		return std::make_shared<SDLImageShared>(defFile.get(), locator.defFrame, locator.defGroup);
	}

	throw std::runtime_error("Invalid image locator received!");
}

std::shared_ptr<ISharedImage> RenderHandler::loadImageFromFile(const ImageLocator & locator)
{
	if (imageFiles.count(locator))
		return imageFiles.at(locator);

	auto result = loadImageFromFileUncached(locator);
	imageFiles[locator] = result;
	return result;
}

std::shared_ptr<ISharedImage> RenderHandler::transformImage(const ImageLocator & locator, std::shared_ptr<ISharedImage> image)
{
	if (imageFiles.count(locator))
		return imageFiles.at(locator);

	auto result = image;

	if (locator.verticalFlip)
		result = result->verticalFlip();

	if (locator.horizontalFlip)
		result = result->horizontalFlip();

	imageFiles[locator] = result;
	return result;
}

std::shared_ptr<ISharedImage> RenderHandler::scaleImage(const ImageLocator & locator, std::shared_ptr<ISharedImage> image)
{
	if (imageFiles.count(locator))
		return imageFiles.at(locator);

	auto handle = image->createImageReference(EImageBlitMode::OPAQUE);

	assert(locator.scalingFactor != 1); // should be filtered-out before



	handle->setOverlayEnabled(locator.layerOverlay);
	handle->setBodyEnabled(locator.layerBody);
	handle->setShadowEnabled(locator.layerShadow);
	if (locator.layerBody && locator.playerColored != PlayerColor::CANNOT_DETERMINE)
		handle->playerColored(locator.playerColored);

	handle->scaleInteger(locator.scalingFactor);

	// TODO: try to optimize image size (possibly even before scaling?) - trim image borders if they are completely transparent
	auto result = handle->getSharedImage();
	imageFiles[locator] = result;
	return result;
}

std::shared_ptr<IImage> RenderHandler::loadImage(const ImageLocator & locator, EImageBlitMode mode)
{
	return createImageReference(locator, loadImageImpl(locator), mode);
}

std::shared_ptr<IImage> RenderHandler::loadImage(const AnimationPath & path, int frame, int group, EImageBlitMode mode)
{
	auto locator = getLocatorForAnimationFrame(path, frame, group);
	return loadImage(locator, mode);
}

std::shared_ptr<IImage> RenderHandler::loadImage(const ImagePath & path, EImageBlitMode mode)
{
	return loadImage(ImageLocator(path), mode);
}

std::shared_ptr<IImage> RenderHandler::createImage(SDL_Surface * source)
{
	return createImageReference(ImageLocator(), std::make_shared<SDLImageShared>(source), EImageBlitMode::ALPHA);
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
