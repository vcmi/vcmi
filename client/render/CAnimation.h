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

#include "../../lib/GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN
class JsonNode;
VCMI_LIB_NAMESPACE_END

class CDefFile;
class IImage;

/// Class for handling animation
class CAnimation
{
private:
	//source[group][position] - file with this frame, if string is empty - image located in def file
	std::map<size_t, std::vector <JsonNode> > source;

	//bitmap[group][position], store objects with loaded bitmaps
	std::map<size_t, std::map<size_t, std::shared_ptr<IImage> > > images;

	//animation file name
	std::string name;

	bool preloaded;

	std::shared_ptr<CDefFile> defFile;

	//loader, will be called by load(), require opened def file for loading from it. Returns true if image is loaded
	bool loadFrame(size_t frame, size_t group);

	//unloadFrame, returns true if image has been unloaded ( either deleted or decreased refCount)
	bool unloadFrame(size_t frame, size_t group);

	//initialize animation from file
	void initFromJson(const JsonNode & input);
	void init();

	//to get rid of copy-pasting error message :]
	void printError(size_t frame, size_t group, std::string type) const;

	//not a very nice method to get image from another def file
	//TODO: remove after implementing resource manager
	std::shared_ptr<IImage> getFromExtraDef(std::string filename);

public:
	CAnimation(std::string Name);
	CAnimation();
	~CAnimation();

	//duplicates frame at [sourceGroup, sourceFrame] as last frame in targetGroup
	//and loads it if animation is preloaded
	void duplicateImage(const size_t sourceGroup, const size_t sourceFrame, const size_t targetGroup);

	//add custom surface to the selected position.
	void setCustom(std::string filename, size_t frame, size_t group=0);

	std::shared_ptr<IImage> getImage(size_t frame, size_t group=0, bool verbose=true) const;

	void exportBitmaps(const boost::filesystem::path & path) const;

	//all available frames
	void load  ();
	void unload();
	void preload();

	//all frames from group
	void loadGroup  (size_t group);
	void unloadGroup(size_t group);

	//single image
	void load  (size_t frame, size_t group=0);
	void unload(size_t frame, size_t group=0);

	//total count of frames in group (including not loaded)
	size_t size(size_t group=0) const;

	void horizontalFlip();
	void verticalFlip();
	void playerColored(PlayerColor player);

	void createFlippedGroup(const size_t sourceGroup, const size_t targetGroup);
};

