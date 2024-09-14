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

#include "../gui/CGuiHandler.h"
#include "IScreenHandler.h"

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
	if(horizontalFlip != other.horizontalFlip)
		return horizontalFlip < other.horizontalFlip;
	if(scalingFactor != other.scalingFactor)
		return scalingFactor < other.scalingFactor;
	if(playerColored != other.playerColored)
		return playerColored < other.playerColored;
	if(layer != other.layer)
		return layer < other.layer;

	return false;
}

bool ImageLocator::empty() const
{
	return !image.has_value() && !defFile.has_value();
}

ImageLocator ImageLocator::copyFile() const
{
	ImageLocator result;
	result.image = image;
	result.defFile = defFile;
	result.defFrame = defFrame;
	result.defGroup = defGroup;
	return result;
}

ImageLocator ImageLocator::copyFileTransform() const
{
	ImageLocator result = copyFile();
	result.horizontalFlip = horizontalFlip;
	result.verticalFlip = verticalFlip;
	return result;
}

ImageLocator ImageLocator::copyFileTransformScale() const
{
	return *this; // full copy
}
