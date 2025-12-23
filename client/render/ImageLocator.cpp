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

#include "../GameEngine.h"
#include "IScreenHandler.h"

#include "../../lib/json/JsonNode.h"

SharedImageLocator::SharedImageLocator(const JsonNode & config, EImageBlitMode mode)
	: defFrame(config["defFrame"].Integer())
	, defGroup(config["defGroup"].Integer())
	, layer(mode)
{
	if(!config["file"].isNull())
		image = ImagePath::fromJson(config["file"]);

	if(!config["defFile"].isNull())
		defFile = AnimationPath::fromJson(config["defFile"]);

	if(!config["generateShadow"].isNull())
		generateShadow = static_cast<SharedImageLocator::ShadowMode>(config["generateShadow"].Integer());

	if(!config["generateOverlay"].isNull())
		generateOverlay = static_cast<SharedImageLocator::OverlayMode>(config["generateOverlay"].Integer());
}

SharedImageLocator::SharedImageLocator(const ImagePath & path, EImageBlitMode mode)
	: image(path)
	, layer(mode)
{
}

SharedImageLocator::SharedImageLocator(const AnimationPath & path, int frame, int group, EImageBlitMode mode)
	: defFile(path)
	, defFrame(frame)
	, defGroup(group)
	, layer(mode)
{
}

ImageLocator::ImageLocator(const JsonNode & config, EImageBlitMode mode)
	: SharedImageLocator(config, mode)
	, verticalFlip(config["verticalFlip"].Bool())
	, horizontalFlip(config["horizontalFlip"].Bool())
{
}

bool SharedImageLocator::operator < (const SharedImageLocator & other) const
{
	if(image != other.image)
		return image < other.image;
	if(defFile != other.defFile)
		return defFile < other.defFile;
	if(defGroup != other.defGroup)
		return defGroup < other.defGroup;
	if(defFrame != other.defFrame)
		return defFrame < other.defFrame;
	if(layer != other.layer)
		return layer < other.layer;
	if(generateShadow != other.generateShadow)
		return generateShadow < other.generateShadow;
	if(generateOverlay != other.generateOverlay)
		return generateOverlay < other.generateOverlay;

	return false;
}

bool ImageLocator::empty() const
{
	return !image.has_value() && !defFile.has_value();
}
