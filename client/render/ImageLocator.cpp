/*
 * ImageLocator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ImageLocator.h"

#include "../../lib/json/JsonNode.h"


ImageLocator::ImageLocator(const JsonNode & config)
	: image(ImagePath::fromJson(config["file"]))
	, defFile(AnimationPath::fromJson(config["defFile"]))
	, defFrame(config["defFrame"].Integer())
	, defGroup(config["defGroup"].Integer())
	, verticalFlip(config["verticalFlip"].Bool())
	, horizontalFlip(config["horizontalFlip"].Bool())
{
}

ImageLocator::ImageLocator(const ImagePath & path)
	: image(path)
{
}

ImageLocator::ImageLocator(const AnimationPath & path, int frame, int group)
	: defFile(path)
	, defFrame(frame)
	, defGroup(group)
{
}

bool ImageLocator::operator<(const ImageLocator & other) const
{
	if(image != other.image)
		return image < other.image;
	if(defFile != other.defFile)
		return defFile < other.defFile;
	if(defGroup != other.defGroup)
		return defGroup < other.defGroup;
	if(defFrame != other.defFrame)
		return defFrame < other.defFrame;
	if(verticalFlip != other.verticalFlip)
		return verticalFlip < other.verticalFlip;
	return horizontalFlip < other.horizontalFlip;
}

bool ImageLocator::empty() const
{
	return !image.has_value() && !defFile.has_value();
}
