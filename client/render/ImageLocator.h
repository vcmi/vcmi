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

#include "IImage.h"

#include "../../lib/filesystem/ResourcePath.h"
#include "../../lib/constants/EntityIdentifiers.h"

struct ImageLocator
{
	std::optional<ImagePath> image;
	std::optional<AnimationPath> defFile;
	int defFrame = -1;
	int defGroup = -1;

	PlayerColor playerColored = PlayerColor::CANNOT_DETERMINE; // FIXME: treat as identical to blue to avoid double-loading?

	bool verticalFlip = false;
	bool horizontalFlip = false;
	int8_t scalingFactor = 0; // 0 = auto / use default scaling
	int8_t preScaledFactor = 1;
	EImageBlitMode layer = EImageBlitMode::OPAQUE;

	ImageLocator() = default;
	ImageLocator(const AnimationPath & path, int frame, int group);
	explicit ImageLocator(const JsonNode & config);
	explicit ImageLocator(const ImagePath & path);

	bool operator < (const ImageLocator & other) const;
	bool empty() const;

	ImageLocator copyFile() const;
	ImageLocator copyFileTransform() const;
	ImageLocator copyFileTransformScale() const;

	// generates string representation of this image locator
	// guaranteed to be a valid file path with no extension
	// but may contain '/' if source file is in directory
	std::string toString() const;
};
