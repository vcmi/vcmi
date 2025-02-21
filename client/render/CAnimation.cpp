/*
 * CAnimation.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CAnimation.h"

#include "../GameEngine.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"
#include "../render/IScreenHandler.h"

#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/json/JsonUtils.h"

bool CAnimation::loadFrame(size_t frame, size_t group, bool verbose)
{
	if(size(group) <= frame)
	{
		if(verbose)
			printError(frame, group, "LoadFrame");
		return false;
	}

	if(auto image = getImageImpl(frame, group, false))
		return true;

	std::shared_ptr<IImage> image = ENGINE->renderHandler().loadImage(getImageLocator(frame, group));

	if(image)
	{
		images[group][frame] = image;

		if (player.isValidPlayer())
			image->playerColored(player);
		return true;
	}
	else
	{
		// image is missing
		printError(frame, group, "LoadFrame");
		images[group][frame] = ENGINE->renderHandler().loadImage(ImagePath::builtin("DEFAULT"), EImageBlitMode::OPAQUE);
		return false;
	}
}

bool CAnimation::unloadFrame(size_t frame, size_t group)
{
	auto image = getImage(frame, group, false);
	if(image)
	{
		images[group].erase(frame);

		if(images[group].empty())
			images.erase(group);
		return true;
	}
	return false;
}

void CAnimation::exportBitmaps(const boost::filesystem::path& path) const
{
	if(images.empty())
	{
		logGlobal->error("Nothing to export, animation is empty");
		return;
	}

	boost::filesystem::path actualPath = path / "SPRITES" / name.getName();
	boost::filesystem::create_directories(actualPath);

	size_t counter = 0;

	for(const auto & groupPair : images)
	{
		size_t group = groupPair.first;

		for(const auto & imagePair : groupPair.second)
		{
			size_t frame = imagePair.first;
			const auto img = imagePair.second;

			boost::format fmt("%d_%d.bmp");
			fmt % group % frame;

			img->exportBitmap(actualPath / fmt.str());
			counter++;
		}
	}

	logGlobal->info("Exported %d frames to %s", counter, actualPath.string());
}

void CAnimation::printError(size_t frame, size_t group, std::string type) const
{
	logGlobal->error("%s error: Request for frame not present in CAnimation! File name: %s, Group: %d, Frame: %d", type, name.getOriginalName(), group, frame);
}

CAnimation::CAnimation(const AnimationPath & Name, std::map<size_t, std::vector <ImageLocator> > layout, EImageBlitMode mode):
	name(boost::starts_with(Name.getName(), "SPRITES") ? Name : Name.addPrefix("SPRITES/")),
	source(layout),
	mode(mode)
{
	if(source.empty())
		logAnim->error("Animation %s failed to load", Name.getOriginalName());
}

CAnimation::~CAnimation() = default;

void CAnimation::duplicateImage(const size_t sourceGroup, const size_t sourceFrame, const size_t targetGroup)
{
	ImageLocator clone(getImageLocator(sourceFrame, sourceGroup));
	source[targetGroup].push_back(clone);
}

std::shared_ptr<IImage> CAnimation::getImage(size_t frame, size_t group, bool verbose)
{
	if (!loadFrame(frame, group, verbose))
		return nullptr;
	return getImageImpl(frame, group, verbose);
}

std::shared_ptr<IImage> CAnimation::getImageImpl(size_t frame, size_t group, bool verbose)
{
	auto groupIter = images.find(group);
	if (groupIter != images.end())
	{
		auto imageIter = groupIter->second.find(frame);
		if (imageIter != groupIter->second.end())
			return imageIter->second;
	}
	if (verbose)
		printError(frame, group, "GetImage");
	return nullptr;
}

size_t CAnimation::size(size_t group) const
{
	auto iter = source.find(group);
	if (iter != source.end())
		return iter->second.size();
	return 0;
}

void CAnimation::horizontalFlip()
{
	for(auto & group : source)
		for(size_t i = 0; i < group.second.size(); ++i)
			horizontalFlip(i, group.first);
}

void CAnimation::verticalFlip()
{
	for(auto & group : source)
		for(size_t i = 0; i < group.second.size(); ++i)
			verticalFlip(i, group.first);
}

void CAnimation::horizontalFlip(size_t frame, size_t group)
{
	auto i1 = images.find(group);
	if(i1 != images.end())
	{
		auto i2 = i1->second.find(frame);

		if(i2 != i1->second.end())
			i2->second = nullptr;
	}

	auto locator = getImageLocator(frame, group);
	locator.horizontalFlip = !locator.horizontalFlip;
	source[group][frame] = locator;
}

void CAnimation::verticalFlip(size_t frame, size_t group)
{
	auto i1 = images.find(group);
	if(i1 != images.end())
	{
		auto i2 = i1->second.find(frame);

		if(i2 != i1->second.end())
			i2->second = nullptr;
	}

	auto locator = getImageLocator(frame, group);
	locator.verticalFlip = !locator.verticalFlip;
	source[group][frame] = locator;
}

void CAnimation::playerColored(PlayerColor targetPlayer)
{
	player = targetPlayer;
	for(auto & group : images)
		for(auto & image : group.second)
			image.second->playerColored(player);
}

void CAnimation::createFlippedGroup(const size_t sourceGroup, const size_t targetGroup)
{
	for(size_t frame = 0; frame < size(sourceGroup); ++frame)
	{
		duplicateImage(sourceGroup, frame, targetGroup);
		verticalFlip(frame, targetGroup);
	}
}

ImageLocator CAnimation::getImageLocator(size_t frame, size_t group) const
{
	try
	{
		const ImageLocator & locator = source.at(group).at(frame);

		if (!locator.empty())
			return locator;
	}
	catch (std::out_of_range &)
	{
		throw std::runtime_error("Frame " + std::to_string(frame) + " of group " + std::to_string(group) + " is missing from animation " + name.getOriginalName() );
	}

	return ImageLocator(name, frame, group, mode);
}
