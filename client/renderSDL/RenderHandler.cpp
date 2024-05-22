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

#include "../../lib/json/JsonNode.h"

std::shared_ptr<CDefFile> RenderHandler::getAnimationFile(const AnimationPath & path)
{
	auto it = animationFiles.find(path);

	if (it != animationFiles.end())
		return it->second;

	auto result = std::make_shared<CDefFile>(path);

	animationFiles[path] = result;
	return result;
}

std::shared_ptr<JsonNode> RenderHandler::getJsonFile(const JsonPath & path)
{
	auto it = jsonFiles.find(path);

	if (it != jsonFiles.end())
		return it->second;

	auto result = std::make_shared<JsonNode>(path);

	jsonFiles[path] = result;
	return result;
}

std::shared_ptr<IImage> RenderHandler::loadImage(const AnimationPath & path, int frame, int group)
{
	AnimationLocator locator{path, frame, group};

	auto it = animationFrames.find(locator);
	if (it != animationFrames.end())
		return it->second;

	auto defFile = getAnimationFile(path);
	auto result = std::make_shared<SDLImage>(defFile.get(), frame, group);

	animationFrames[locator] = result;
	return result;
}

//std::vector<std::shared_ptr<IImage>> RenderHandler::loadImageGroup(const AnimationPath & path, int group)
//{
//	const auto defFile = getAnimationFile(path);
//
//	size_t groupSize = defFile->getEntries().at(group);
//
//	std::vector<std::shared_ptr<IImage>> result;
//	for (size_t i = 0; i < groupSize; ++i)
//		loadImage(path, i, group);
//
//	return result;
//}

std::shared_ptr<IImage> RenderHandler::loadImage(const ImagePath & path)
{
	return loadImage(path, EImageBlitMode::ALPHA);
}

std::shared_ptr<IImage> RenderHandler::loadImage(const ImagePath & path, EImageBlitMode mode)
{
	return std::make_shared<SDLImage>(path, mode);
}

std::shared_ptr<IImage> RenderHandler::createImage(SDL_Surface * source)
{
	return std::make_shared<SDLImage>(source, EImageBlitMode::ALPHA);
}

std::shared_ptr<CAnimation> RenderHandler::loadAnimation(const AnimationPath & path)
{
	return std::make_shared<CAnimation>(path);
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
