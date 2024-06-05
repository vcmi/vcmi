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

		for (auto & defEntry : defEntries)
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

std::shared_ptr<IImage> RenderHandler::loadImage(const AnimationPath & path, int frame, int group)
{
	AnimationLocator locator{path, frame, group};

	auto it = animationFrames.find(locator);
	if (it != animationFrames.end())
		return it->second->createImageReference();

	const auto & layout = getAnimationLayout(path);
	if (!layout.count(group))
		return loadImage(ImagePath::builtin("DEFAULT"));

	if (frame >= layout.at(group).size())
		return loadImage(ImagePath::builtin("DEFAULT"));

	const auto & config = layout.at(group).at(frame);

	if (config.isNull())
	{
		auto defFile = getAnimationFile(path);
		auto result = std::make_shared<SDLImageConst>(defFile.get(), frame, group);
		animationFrames[locator] = result;
		return result->createImageReference();
	}

	return loadImage(ImagePath::builtin("DEFAULT"));
}

std::shared_ptr<IImage> RenderHandler::loadImage(const ImagePath & path)
{
	return loadImage(path, EImageBlitMode::ALPHA);
}

std::shared_ptr<IImage> RenderHandler::loadImage(const ImagePath & path, EImageBlitMode mode)
{
	auto it = imageFiles.find(path);
	if (it != imageFiles.end())
		return it->second->createImageReference();

	auto result = std::make_shared<SDLImageConst>(path, mode);
	imageFiles[path] = result;
	return result->createImageReference();
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
