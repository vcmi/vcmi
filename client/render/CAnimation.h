/*
 * CAnimation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "IImage.h"
#include "ImageLocator.h"

#include "../../lib/GameConstants.h"
#include "../../lib/filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN
class JsonNode;
VCMI_LIB_NAMESPACE_END

class CDefFile;
class RenderHandler;

/// Class for handling animation
class CAnimation
{
private:
	//source[group][position] - location of this frame
	std::map<size_t, std::vector <ImageLocator> > source;

	//bitmap[group][position], store objects with loaded bitmaps
	std::map<size_t, std::map<size_t, std::shared_ptr<IImage> > > images;

	//animation file name
	AnimationPath name;

	EImageBlitMode mode;

	// current player color, if any
	PlayerColor player = PlayerColor::CANNOT_DETERMINE;

	//loader, will be called by load(), require opened def file for loading from it. Returns true if image is loaded
	bool loadFrame(size_t frame, size_t group);

	//unloadFrame, returns true if image has been unloaded ( either deleted or decreased refCount)
	bool unloadFrame(size_t frame, size_t group);

	//to get rid of copy-pasting error message :]
	void printError(size_t frame, size_t group, std::string type) const;

	std::shared_ptr<IImage> getImageImpl(size_t frame, size_t group=0, bool verbose=true);

	ImageLocator getImageLocator(size_t frame, size_t group) const;
public:
	CAnimation(const AnimationPath & Name, std::map<size_t, std::vector <ImageLocator> > layout, EImageBlitMode mode);
	~CAnimation();

	//duplicates frame at [sourceGroup, sourceFrame] as last frame in targetGroup
	//and loads it if animation is preloaded
	void duplicateImage(const size_t sourceGroup, const size_t sourceFrame, const size_t targetGroup);

	std::shared_ptr<IImage> getImage(size_t frame, size_t group=0, bool verbose=true);

	void exportBitmaps(const boost::filesystem::path & path) const;

	//total count of frames in group (including not loaded)
	size_t size(size_t group=0) const;

	void horizontalFlip(size_t frame, size_t group=0);
	void verticalFlip(size_t frame, size_t group=0);
	void horizontalFlip();
	void verticalFlip();
	void playerColored(PlayerColor player);

	void createFlippedGroup(const size_t sourceGroup, const size_t targetGroup);
};

