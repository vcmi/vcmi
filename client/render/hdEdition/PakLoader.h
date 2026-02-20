/*
 * PakLoader.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "PakLoader.h"

#include "../../../lib/filesystem/ResourcePath.h"

class SDLImageShared;

class PakLoader
{
public:
	struct ImageEntry
	{
		std::string name;
		uint32_t sheetIndex = 0;
		uint32_t spriteOffsetX = 0;
		uint32_t unknown1 = 0;
		uint32_t spriteOffsetY = 0;
		uint32_t unknown2 = 0;
		uint32_t sheetOffsetX = 0;
		uint32_t sheetOffsetY = 0;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t rotation = 0;
		uint32_t hasShadow = 0;
		uint32_t shadowSheetIndex = 0;
		uint32_t shadowSheetOffsetX = 0;
		uint32_t shadowSheetOffsetY = 0;
		uint32_t shadowWidth = 0;
		uint32_t shadowHeight = 0;
		uint32_t shadowRotation = 0;
	};
	struct SheetEntry
	{
		uint32_t compressedSize = 0;
		uint32_t fullSize = 0;
	};
	struct ArchiveEntry
	{
		std::string name = "";

		uint32_t metadataOffset = 0;
		uint32_t metadataSize = 0;

		uint32_t countSheets = 0;
		uint32_t compressedSize = 0;
		uint32_t fullSize = 0;

		uint32_t scale = 0;

		std::vector<SheetEntry> sheets;
		std::vector<ImageEntry> images;
	};

	void loadPak(ResourcePath path, int scale, std::unordered_set<std::string> animToSkip, std::unordered_set<std::string> imagesToSkip, std::unordered_set<std::string> suffixesToSkip);
	std::map<ResourcePath, std::vector<ArchiveEntry>> content;
	// Fast lookup: imageName -> (ResourcePath, ImageEntry*, ArchiveEntry*)
	std::unordered_map<int, std::unordered_map<std::string, std::tuple<ResourcePath, ImageEntry*, ArchiveEntry*>>> imagesByName;
};
