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


	if(auto image = getImage(frame, group, false))
	{
		return true;
	}

	//try to get image from def
	if(source[group][frame].getType() == JsonNode::JsonType::DATA_NULL)
	{
		auto image = GH.renderHandler().loadImage(name, frame, group);

		if(image)
		{
			images[group][frame] = image;
			return true;
		}
		// still here? image is missing

		printError(frame, group, "LoadFrame");
		images[group][frame] = GH.renderHandler().loadImage(ImagePath::builtin("DEFAULT"), EImageBlitMode::OPAQUE);
		return false;
	}

	if (!source[group][frame]["file"].isNull())
	{
		auto img = GH.renderHandler().loadImage(ImagePath::fromJson(source[group][frame]["file"]), EImageBlitMode::ALPHA);
		images[group][frame] = img;
		return true;
	}

	if (!source[group][frame]["animation"].isNull())
	{
		AnimationPath animationFile = AnimationPath::fromJson(source[group][frame]["animation"]);
		int32_t animationGroup = source[group][frame]["sourceGroup"].Integer();
		int32_t animationFrame = source[group][frame]["sourceFrame"].Integer();

		auto img = GH.renderHandler().loadImage(animationFile, animationFrame, animationGroup);
		images[group][frame] = img;
		return true;
	}

	return false;
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

CAnimation::CAnimation(const AnimationPath & Name, std::map<size_t, std::vector <JsonNode> > layout):
	name(boost::starts_with(Name.getName(), "SPRITES") ? Name : Name.addPrefix("SPRITES/")),
	source(layout),
	preloaded(false)
{
	if(source.empty())
		logAnim->error("Animation %s failed to load", Name.getOriginalName());
}

CAnimation::CAnimation():
	preloaded(false)
{
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

	//todo: clone actual loaded Image object
	JsonNode clone(source[sourceGroup][sourceFrame]);

	if(clone.getType() == JsonNode::JsonType::DATA_NULL)
	{
		clone["animation"].String() = name.getName();
		clone["sourceGroup"].Integer() = sourceGroup;
		clone["sourceFrame"].Integer() = sourceFrame;
	}

	source[targetGroup].push_back(clone);

	size_t index = source[targetGroup].size() - 1;

	if(preloaded)
		load(index, targetGroup);
}

void CAnimation::setCustom(std::string filename, size_t frame, size_t group)
{
	if (source[group].size() <= frame)
		source[group].resize(frame+1);
	source[group][frame]["file"].String() = filename;
	//FIXME: update image if already loaded
}

std::shared_ptr<IImage> CAnimation::getImage(size_t frame, size_t group, bool verbose) const
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

void CAnimation::load()
{
	for (auto & elem : source)
		for (size_t image=0; image < elem.second.size(); image++)
			loadFrame(image, elem.first);
}

void CAnimation::unload()
{
	for (auto & elem : source)
		for (size_t image=0; image < elem.second.size(); image++)
			unloadFrame(image, elem.first);

}

void CAnimation::preload()
{
	if(!preloaded)
	{
		preloaded = true;
		load();
	}
}

void CAnimation::loadGroup(size_t group)
{
	if (vstd::contains(source, group))
		for (size_t image=0; image < source[group].size(); image++)
			loadFrame(image, group);
}

void CAnimation::unloadGroup(size_t group)
{
	if (vstd::contains(source, group))
		for (size_t image=0; image < source[group].size(); image++)
			unloadFrame(image, group);
}

void CAnimation::load(size_t frame, size_t group)
{
	loadFrame(frame, group);
}

void CAnimation::unload(size_t frame, size_t group)
{
	unloadFrame(frame, group);
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
	for(auto & group : images)
		for(auto & image : group.second)
			image.second->horizontalFlip();
}

void CAnimation::verticalFlip()
{
	for(auto & group : images)
		for(auto & image : group.second)
			image.second->verticalFlip();
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

		auto image = getImage(frame, targetGroup);
		image->verticalFlip();
	}
}

