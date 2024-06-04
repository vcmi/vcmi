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

const RenderHandler::AnimationLayoutMap & RenderHandler::getAnimationLayout(const AnimationPath & path)
{
	auto it = animationLayouts.find(path);

	if (it != animationLayouts.end())
		return it->second;

	AnimationLayoutMap result;

	auto defFile = getAnimationFile(path);
	if(defFile)
	{
		const std::map<size_t, size_t> defEntries = defFile->getEntries();

		for (auto & defEntry : defEntries)
			result[defEntry.first].resize(defEntry.second);
	}

	auto jsonResource = path.toType<EResType::JSON>();
	JsonPath actualJsonPath = boost::starts_with(jsonResource.getName(), "SPRITES") ? jsonResource : jsonResource.addPrefix("SPRITES/");;
	auto configList = CResourceHandler::get()->getResourcesWithName(actualJsonPath);

	for(auto & loader : configList)
	{
		auto stream = loader->load(actualJsonPath);
		std::unique_ptr<ui8[]> textData(new ui8[stream->getSize()]);
		stream->read(textData.get(), stream->getSize());

		const JsonNode config(reinterpret_cast<const std::byte*>(textData.get()), stream->getSize(), path.getOriginalName());

		initFromJson(result, config);
	}

	animationLayouts[path] = result;
	return animationLayouts[path];
}

std::shared_ptr<IImage> RenderHandler::loadImage(const AnimationPath & path, int frame, int group)
{
	AnimationLocator locator{path, frame, group};

	auto it = animationFrames.find(locator);
	if (it != animationFrames.end())
		return it->second->createImageReference();

	auto defFile = getAnimationFile(path);
	auto result = std::make_shared<SDLImageConst>(defFile.get(), frame, group);

	animationFrames[locator] = result;
	return result->createImageReference();
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

//
//void Graphics::addImageListEntry(size_t index, size_t group, const std::string & listName, const std::string & imageName)
//{
//	if (!imageName.empty())
//	{
//		JsonNode entry;
//		if (group != 0)
//			entry["group"].Integer() = group;
//		entry["frame"].Integer() = index;
//		entry["file"].String() = imageName;
//
//		imageLists["SPRITES/" + listName]["images"].Vector().push_back(entry);
//	}
//}
//
//void Graphics::addImageListEntries(const EntityService * service)
//{
//	auto cb = std::bind(&Graphics::addImageListEntry, this, _1, _2, _3, _4);
//
//	auto loopCb = [&](const Entity * entity, bool & stop)
//	{
//		entity->registerIcons(cb);
//	};
//
//	service->forEachBase(loopCb);
//}
//
//void Graphics::initializeImageLists()
//{
//	addImageListEntries(CGI->creatures());
//	addImageListEntries(CGI->heroTypes());
//	addImageListEntries(CGI->artifacts());
//	addImageListEntries(CGI->factions());
//	addImageListEntries(CGI->spells());
//	addImageListEntries(CGI->skills());
//}
//
//std::shared_ptr<CAnimation> Graphics::getAnimation(const AnimationPath & path)
//{
//	if (cachedAnimations.count(path) != 0)
//		return cachedAnimations.at(path);
//
//	auto newAnimation = GH.renderHandler().loadAnimation(path);
//
//	newAnimation->preload();
//	cachedAnimations[path] = newAnimation;
//	return newAnimation;
//}
