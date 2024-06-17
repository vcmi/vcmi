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

#include "../gui/CGuiHandler.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"

#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/json/JsonUtils.h"

bool CAnimation::loadFrame(size_t frame, size_t group)
{
	if(size(group) <= frame)
	{
		printError(frame, group, "LoadFrame");
		return false;
	}

	if(auto image = getImageImpl(frame, group, false))
		return true;

	std::shared_ptr<IImage> image = GH.renderHandler().loadImage(source[group][frame], mode);

	if(image)
	{
		images[group][frame] = image;
		return true;
	}
	else
	{
		// image is missing
		printError(frame, group, "LoadFrame");
		images[group][frame] = GH.renderHandler().loadImage(ImagePath::builtin("DEFAULT"), EImageBlitMode::OPAQUE);
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
	if(!source.count(sourceGroup))
	{
		logAnim->error("Group %d missing in %s", sourceGroup, name.getName());
		return;
	}

	if(source[sourceGroup].size() <= sourceFrame)
	{
		logAnim->error("Frame [%d %d] missing in %s", sourceGroup, sourceFrame, name.getName());
		return;
	}

	ImageLocator clone(source[sourceGroup][sourceFrame]);
	source[targetGroup].push_back(clone);
}

std::shared_ptr<IImage> CAnimation::getImage(size_t frame, size_t group, bool verbose)
{
	if (!loadFrame(frame, group))
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
	try
	{
		images.at(group).at(frame) = nullptr;
	}
	catch (const std::out_of_range &)
	{
		// ignore - image not loaded
	}

	ImageLocator & locator = source.at(group).at(frame);
	locator.horizontalFlip = !locator.horizontalFlip;
}

void CAnimation::verticalFlip(size_t frame, size_t group)
{
	try
	{
		images.at(group).at(frame) = nullptr;
	}
	catch (const std::out_of_range &)
	{
		// ignore - image not loaded
	}

	ImageLocator & locator = source.at(group).at(frame);
	locator.verticalFlip = !locator.verticalFlip;
}

void CAnimation::playerColored(PlayerColor player)
{
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
