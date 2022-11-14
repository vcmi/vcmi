/*
 * Animation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
//code is copied from vcmiclient/CAnimation.h with minimal changes

#include "../lib/JsonNode.h"
#include "../lib/GameConstants.h"
#include <QRgb>
#include <QImage>

/*
 * Base class for images, can be used for non-animation pictures as well
 */

class DefFile;
/// Class for handling animation
class Animation
{
private:
	//source[group][position] - file with this frame, if string is empty - image located in def file
	std::map<size_t, std::vector<JsonNode>> source;

	//bitmap[group][position], store objects with loaded bitmaps
	std::map<size_t, std::map<size_t, std::shared_ptr<QImage> > > images;

	//animation file name
	std::string name;

	bool preloaded;

	std::shared_ptr<DefFile> defFile;

	//loader, will be called by load(), require opened def file for loading from it. Returns true if image is loaded
	bool loadFrame(size_t frame, size_t group);

	//unloadFrame, returns true if image has been unloaded ( either deleted or decreased refCount)
	bool unloadFrame(size_t frame, size_t group);

	//initialize animation from file
	//void initFromJson(const JsonNode & input);
	void init();

	//to get rid of copy-pasting error message :]
	void printError(size_t frame, size_t group, std::string type) const;

	//not a very nice method to get image from another def file
	//TODO: remove after implementing resource manager
	std::shared_ptr<QImage> getFromExtraDef(std::string filename);

public:
	Animation(std::string Name);
	Animation();
	~Animation();

	//duplicates frame at [sourceGroup, sourceFrame] as last frame in targetGroup
	//and loads it if animation is preloaded
	void duplicateImage(size_t sourceGroup, size_t sourceFrame, size_t targetGroup);

	// adjust the color of the animation, used in battle spell effects, e.g. Cloned objects

	//add custom surface to the selected position.
	void setCustom(std::string filename, size_t frame, size_t group = 0);

	std::shared_ptr<QImage> getImage(size_t frame, size_t group = 0, bool verbose = true) const;

	//all available frames
	void load();
	void unload();
	void preload();

	//all frames from group
	void loadGroup  (size_t group);
	void unloadGroup(size_t group);

	//single image
	void load  (size_t frame, size_t group = 0);
	void unload(size_t frame, size_t group = 0);

	void exportBitmaps(const boost::filesystem::path& path, bool prependResourceName) const;

	//total count of frames in group (including not loaded)
	size_t size(size_t group = 0) const;

	void horizontalFlip();
	void verticalFlip();
	void playerColored(PlayerColor player);

	void createFlippedGroup(const size_t sourceGroup, const size_t targetGroup);
};
