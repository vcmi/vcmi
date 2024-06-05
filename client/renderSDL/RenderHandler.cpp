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

#include "../render/CAnimation.h"
#include "../render/CDefFile.h"

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

RenderHandler::ImageLocator::ImageLocator(const JsonNode & config)
	: image(ImagePath::fromJson(config["file"]))
	, animation(AnimationPath::fromJson(config["animation"]))
	, frame(config["frame"].Integer())
	, group(config["group"].Integer())
	, verticalFlip(config["verticalFlip"].Bool())
	, horizontalFlip(config["horizontalFlip"].Bool())
{
}

RenderHandler::ImageLocator::ImageLocator(const ImagePath & path)
	: image(path)
{
}

RenderHandler::ImageLocator::ImageLocator(const AnimationPath & path, int frame, int group)
	: animation(path)
	, frame(frame)
	, group(group)
{
}

bool RenderHandler::ImageLocator::operator<(const ImageLocator & other) const
{
	if(image != other.image)
		return image < other.image;
	if(animation != other.animation)
		return animation < other.animation;
	if(group != other.group)
		return group < other.group;
	if(frame != other.frame)
		return frame < other.frame;
	if(verticalFlip != other.verticalFlip)
		return verticalFlip < other.verticalFlip;
	return horizontalFlip < other.horizontalFlip;
}

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
			JsonNode toAdd;
			JsonUtils::inherit(toAdd, base);
			toAdd["file"].String() = basepath + frame.String();
			source[groupID].push_back(toAdd);
		}
	}

	for(const JsonNode & node : config["images"].Vector())
	{
		size_t group = node["group"].Integer();
		size_t frame = node["frame"].Integer();

		if (source[group].size() <= frame)
			source[group].resize(frame+1);

		JsonNode toAdd;
		JsonUtils::inherit(toAdd, base);
		toAdd["file"].String() = basepath + node["file"].String();
		source[group][frame] = toAdd;
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

std::shared_ptr<IConstImage> RenderHandler::loadImageFromSingleFile(const ImagePath & path)
{
	auto result = std::make_shared<SDLImageConst>(path, EImageBlitMode::COLORKEY);
	imageFiles[ImageLocator(path)] = result;
	return result;
}

std::shared_ptr<IConstImage> RenderHandler::loadImageFromAnimationFileUncached(const AnimationPath & path, int frame, int group)
{
	const auto & layout = getAnimationLayout(path);
	if (!layout.count(group))
		return loadImageFromSingleFile(ImagePath::builtin("DEFAULT"));

	if (frame >= layout.at(group).size())
		return loadImageFromSingleFile(ImagePath::builtin("DEFAULT"));

	const auto & config = layout.at(group).at(frame);
	if (config.isNull())
	{
		auto defFile = getAnimationFile(path);
		return std::make_shared<SDLImageConst>(defFile.get(), frame, group);
	}
	else
	{
		return loadImageImpl(ImageLocator(config));
	}
}

std::shared_ptr<IConstImage> RenderHandler::loadImageFromAnimationFile(const AnimationPath & path, int frame, int group)
{
	auto result = loadImageFromAnimationFileUncached(path, frame, group);
	imageFiles[ImageLocator(path, frame, group)] = result;
	return result;
}

std::shared_ptr<IConstImage> RenderHandler::loadImageImpl(const ImageLocator & locator)
{
	auto it = imageFiles.find(locator);
	if (it != imageFiles.end())
		return it->second;

	std::shared_ptr<IConstImage> result;

	if (!locator.image.empty())
		result = loadImageFromSingleFile(locator.image);
	else if (!locator.animation.empty())
		result = loadImageFromAnimationFile(locator.animation, locator.frame, locator.group);

	if (!result)
		result = loadImageFromSingleFile(ImagePath::builtin("DEFAULT"));

	if (locator.verticalFlip)
		result = result->verticalFlip();

	if (locator.horizontalFlip)
		result = result->horizontalFlip();

	imageFiles[locator] = result;
	return result;
}

std::shared_ptr<IImage> RenderHandler::loadImage(const JsonNode & config)
{
	if (config.isString())
		return loadImageImpl(ImageLocator(ImagePath::fromJson(config)))->createImageReference();
	else
		return loadImageImpl(ImageLocator(config))->createImageReference();
}

std::shared_ptr<IImage> RenderHandler::loadImage(const AnimationPath & path, int frame, int group)
{
	return loadImageImpl(ImageLocator(path, frame, group))->createImageReference();
}

std::shared_ptr<IImage> RenderHandler::loadImage(const ImagePath & path)
{
	return loadImage(path, EImageBlitMode::ALPHA);
}

std::shared_ptr<IImage> RenderHandler::loadImage(const ImagePath & path, EImageBlitMode mode)
{
	return loadImageImpl(ImageLocator(path))->createImageReference();
}

std::shared_ptr<IImage> RenderHandler::createImage(SDL_Surface * source)
{
	return std::make_shared<SDLImageConst>(source, EImageBlitMode::ALPHA)->createImageReference();
}

std::shared_ptr<CAnimation> RenderHandler::loadAnimation(const AnimationPath & path)
{
	return std::make_shared<CAnimation>(path, getAnimationLayout(path));
}

std::shared_ptr<CAnimation> RenderHandler::createAnimation()
{
	return std::make_shared<CAnimation>();
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

			layout[group][index] = entry;
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
