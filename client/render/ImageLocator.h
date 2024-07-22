/*
 * ImageLocator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/filesystem/ResourcePath.h"

struct ImageLocator
{
	std::optional<ImagePath> image;
	std::optional<AnimationPath> defFile;
	int defFrame = -1;
	int defGroup = -1;

	bool verticalFlip = false;
	bool horizontalFlip = false;

	ImageLocator() = default;
	ImageLocator(const AnimationPath & path, int frame, int group);
	explicit ImageLocator(const JsonNode & config);
	explicit ImageLocator(const ImagePath & path);

	bool operator < (const ImageLocator & other) const;
	bool empty() const;
};
