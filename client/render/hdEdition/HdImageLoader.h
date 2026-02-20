/*
 * HdImageLoader.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "HdImageLoader.h"
#include "PakLoader.h"

#include "../../../lib/constants/EntityIdentifiers.h"
#include "../../../lib/filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN
class Point;
class PlayerColor;
VCMI_LIB_NAMESPACE_END

struct SDL_Surface;
class SDLImageShared;
class PakLoader;
class DdsFormat;

class HdImageLoader
{
private:
	std::shared_ptr<PakLoader> pakLoader;
	std::shared_ptr<DdsFormat> ddsFormat;
	std::map<std::string, std::vector<int>> flagData;
	void loadFlagData();
	int scalingFactor;

	std::array<SDL_Surface *, 2> flagImg;
public:
	HdImageLoader();
	~HdImageLoader();

	std::shared_ptr<SDLImageShared> getImage(const ImagePath & path, const Point & fullSize, const Point & margins, bool shadow, bool overlay);
	std::optional<std::tuple<ResourcePath, PakLoader::ArchiveEntry, PakLoader::ImageEntry>> find(const ImagePath & path);
	bool exists(const ImagePath & path);
};
