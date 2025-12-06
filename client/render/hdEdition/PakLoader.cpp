/*
 * PakLoader.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "PakLoader.h"

#include "../../../lib/filesystem/Filesystem.h"
#include "../../../lib/filesystem/CBinaryReader.h"

std::vector<std::vector<std::string>> stringtoTable(const std::string& input)
{
	std::vector<std::vector<std::string>> result;

	std::vector<std::string> lines;
	boost::split(lines, input, boost::is_any_of("\n"));

	for(auto& line : lines)
	{
		boost::trim(line);
		if(line.empty())
			continue;
		std::vector<std::string> tokens;
		boost::split(tokens, line, boost::is_any_of(" "), boost::token_compress_on);
		result.push_back(tokens);
	}

	return result;
}

bool endsWithAny(const std::string & s, const std::unordered_set<std::string> & suffixes)
{
	for(const auto & suf : suffixes)
		if(boost::algorithm::ends_with(s, suf))
			return true;

	return false;
}

void PakLoader::loadPak(ResourcePath path, int scale, std::unordered_set<std::string> animToSkip, std::unordered_set<std::string> imagesToSkip, std::unordered_set<std::string> suffixesToSkip)
{
	auto file = CResourceHandler::get()->load(path);
	CBinaryReader reader(file.get());

	std::vector<ArchiveEntry> archiveEntries;

	[[maybe_unused]] uint32_t magic = reader.readUInt32();
	uint32_t headerOffset = reader.readUInt32();

	assert(magic == 4);
	file->seek(headerOffset);

	uint32_t entriesCount = reader.readUInt32();

	for(uint32_t i = 0; i < entriesCount; ++i)
	{
		ArchiveEntry entry;

		std::string buf(20, '\0');
		reader.read(reinterpret_cast<ui8*>(buf.data()), buf.size());
		size_t len = buf.find('\0');
		std::string s = buf.substr(0, len);
		entry.name = boost::algorithm::to_upper_copy(s);
		
		entry.metadataOffset = reader.readUInt32();
		entry.metadataSize = reader.readUInt32();

		entry.countSheets = reader.readUInt32();
		entry.compressedSize = reader.readUInt32();
		entry.fullSize = reader.readUInt32();

		entry.sheets.resize(entry.countSheets);

		for(uint32_t j = 0; j < entry.countSheets; ++j)
			entry.sheets[j].compressedSize = reader.readUInt32();

		for(uint32_t j = 0; j < entry.countSheets; ++j)
			entry.sheets[j].fullSize = reader.readUInt32();
		
		entry.scale = scale;

		if(animToSkip.find(entry.name) == animToSkip.end() && !endsWithAny(entry.name, suffixesToSkip))
			archiveEntries.push_back(entry);
	}

	for(auto & entry : archiveEntries)
	{
		file->seek(entry.metadataOffset);

		std::string buf(entry.metadataSize, '\0');
		reader.read(reinterpret_cast<ui8*>(buf.data()), buf.size());
		size_t len = buf.find('\0');
		std::string data = buf.substr(0, len);

		auto table = stringtoTable(data);

		for(const auto & sheet : entry.sheets)
			reader.skip(sheet.compressedSize);

		ImageEntry image;
		for(const auto & line : table)
		{
			assert(line.size() == 12 || line.size() == 18);

			image.name = boost::algorithm::to_upper_copy(line[0]);
			image.sheetIndex = std::stol(line[1]);
			image.spriteOffsetX = std::stol(line[2]);
			image.unknown1 = std::stol(line[3]);
			image.spriteOffsetY = std::stol(line[4]);
			image.unknown2 = std::stol(line[5]);
			image.sheetOffsetX = std::stol(line[6]);
			image.sheetOffsetY = std::stol(line[7]);
			image.width = std::stol(line[8]);
			image.height = std::stol(line[9]);
			image.rotation = std::stol(line[10]);
			image.hasShadow = std::stol(line[11]);

			assert(image.rotation == 0 || image.rotation == 1);

			if(image.hasShadow)
			{
				image.shadowSheetIndex = std::stol(line[12]);
				image.shadowSheetOffsetX = std::stol(line[13]);
				image.shadowSheetOffsetY = std::stol(line[14]);
				image.shadowWidth = std::stol(line[15]);
				image.shadowHeight = std::stol(line[16]);
				image.shadowRotation = std::stol(line[17]);

				assert(image.shadowRotation == 0 || image.shadowRotation == 1);
			}

			if(imagesToSkip.find(image.name) == imagesToSkip.end() && !endsWithAny(image.name, suffixesToSkip))
				entry.images.push_back(image);
		}
	}

	content[path] = archiveEntries;

	// Build indices for fast lookup
	for(auto& entry : content[path])
	{
		for(auto& image : entry.images)
			imagesByName[scale].try_emplace(image.name, path, &image, &entry);
	}
}
