/*
 * HdImageLoader.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "HdImageLoader.h"
#include "PakLoader.h"
#include "DdsFormat.h"

#include <SDL_image.h>
#include <unordered_set>

#include "../../GameEngine.h"
#include "../../render/CBitmapHandler.h"
#include "../../render/IScreenHandler.h"
#include "../../renderSDL/SDLImage.h"
#include "../../renderSDL/SDL_Extensions.h"
#include "../../../lib/filesystem/ResourcePath.h"
#include "../../../lib/filesystem/Filesystem.h"
#include "../../../lib/filesystem/CCompressedStream.h"
#include "../../../lib/filesystem/CMemoryStream.h"

const std::unordered_set<std::string> animToSkip = {
	// skip menu buttons (RoE)
	"MMENUNG", "MMENULG", "MMENUHS", "MMENUCR", "MMENUQT", "GTSINGL", "GTMULTI", "GTCAMPN", "GTTUTOR", "GTBACK", "GTSINGL", "GTMULTI", "GTCAMPN", "GTTUTOR", "GTBACK",
	// skip dialogbox - coloring not supported yet
	"DIALGBOX",
	// skip water + rivers
	"WATRTL", "LAVATL", "CLRRVR", "MUDRVR", "LAVRVR"
};
const std::unordered_set<std::string> imagesToSkip = {
	// skip RoE specific files
	"MAINMENU", "GAMSELBK", "GSELPOP1", "SCSELBCK", "GSSTRIP", "LOADGAME", "NEWGAME", "LOADBAR"
};
const std::unordered_set<std::string> hdColors = {
	// skip colored variants - coloring not supported yet
	"_RED", "_BLUE", "_SAND", "_GREEN", "_ORANGE", "_PURPLE", "_BLUEWIN", "_FLESH"
};

HdImageLoader::HdImageLoader()
	: pakLoader(std::make_shared<PakLoader>())
	, ddsFormat(std::make_shared<DdsFormat>())
	, scalingFactor(ENGINE->screenHandler().getScalingFactor())
	, flagImg({nullptr, nullptr})
{
	const std::vector<std::pair<int, ResourcePath>> files = {
		{2, ResourcePath("DATA/bitmap_DXT_com_x2.pak", EResType::ARCHIVE_PAK)},
		{2, ResourcePath("DATA/bitmap_DXT_loc_x2.pak", EResType::ARCHIVE_PAK)},
		{2, ResourcePath("DATA/sprite_DXT_com_x2.pak", EResType::ARCHIVE_PAK)},
		{2, ResourcePath("DATA/sprite_DXT_loc_x2.pak", EResType::ARCHIVE_PAK)},
		{3, ResourcePath("DATA/bitmap_DXT_com_x3.pak", EResType::ARCHIVE_PAK)},
		{3, ResourcePath("DATA/bitmap_DXT_loc_x3.pak", EResType::ARCHIVE_PAK)},
		{3, ResourcePath("DATA/sprite_DXT_com_x3.pak", EResType::ARCHIVE_PAK)},
		{3, ResourcePath("DATA/sprite_DXT_loc_x3.pak", EResType::ARCHIVE_PAK)}
	};
	for(auto & file : files)
		if(CResourceHandler::get()->existsResource(file.second) && scalingFactor == file.first)
			pakLoader->loadPak(file.second, file.first, animToSkip, imagesToSkip, hdColors);
	
	loadFlagData();
}

HdImageLoader::~HdImageLoader()
{
	if(flagImg[0])
		SDL_FreeSurface(flagImg[0]);
	if(flagImg[1])
		SDL_FreeSurface(flagImg[1]);
}

void HdImageLoader::loadFlagData()
{
	auto res = ResourcePath("DATA/spriteFlagsInfo.txt", EResType::TEXT);
	if(!CResourceHandler::get()->existsResource(res))
		return;

	auto data = CResourceHandler::get()->load(res)->readAll();
	std::string s(reinterpret_cast<const char*>(data.first.get()), data.second);
	std::istringstream ss(s);
	std::string line;
	while (std::getline(ss, line))
	{
		boost::algorithm::trim(line);
		if(line.empty())
			continue;

		std::vector<std::string> tokens;
		boost::split(tokens, line, boost::is_space(), boost::token_compress_on);

		std::string key = tokens[0];
		std::vector<int> values;
		for (size_t i = 1; i < tokens.size(); ++i)
			values.push_back(std::stoi(tokens[i]));

		flagData[key] = values;
	}

	auto flag = scalingFactor == 3 ? "DATA/flags/flag_grey.png" : "DATA/flags/flag_grey_x2.png";
	flagImg[0] = BitmapHandler::loadBitmap(ImagePath::builtin(flag));
	CSDL_Ext::adjustBrightness(flagImg[0], 2.5f);
	flagImg[1] = CSDL_Ext::verticalFlip(flagImg[0]);
}

std::shared_ptr<SDLImageShared> HdImageLoader::getImage(const ImagePath & path, const Point & fullSize, const Point & margins, bool shadow, bool overlay)
{
	auto imageName = path.getName();
	auto ret = find(path);
	if(!ret)
		return nullptr;
	
	if(overlay && !flagData.contains(imageName))
		return nullptr;
	else if(overlay)
	{
		auto surf = CSDL_Ext::newSurface(fullSize * scalingFactor);

		for(int i = 0; i < flagData[imageName][0]; ++i)
		{
			bool flagMirror = flagData[imageName][3 + i * 3];
			CSDL_Ext::blitSurface(flagMirror ? flagImg[1] : flagImg[0], surf, Point(flagData[imageName][1 + i * 3], flagData[imageName][2 + i * 3]) * scalingFactor);
		}

		auto img = std::make_shared<SDLImageShared>(surf);

		SDL_FreeSurface(surf);

		return img;
	}

	auto [res, entry, image] = *ret;

	auto sheetIndex = shadow ? image.shadowSheetIndex : image.sheetIndex;
	auto sheetOffsetX = shadow ? image.shadowSheetOffsetX : image.sheetOffsetX;
	auto sheetOffsetY = shadow ? image.shadowSheetOffsetY : image.sheetOffsetY;
	auto rotation = shadow ? image.shadowRotation : image.rotation;
	auto width = shadow ? image.shadowWidth : image.width;
	auto height = shadow ? image.shadowHeight : image.height;

	std::unique_ptr<CInputStream> file = CResourceHandler::get()->load(res);
	file->seek(entry.metadataOffset);
	file->skip(entry.metadataSize);
	for(size_t i = 0; i < sheetIndex; ++i)
		file->skip(entry.sheets[i].compressedSize);

	CCompressedStream compressedReader(std::move(file), false, entry.sheets[sheetIndex].fullSize);
	Rect sheetRect(sheetOffsetX, sheetOffsetY, width, height);
	auto surfCropped = ddsFormat->load(&compressedReader, entry.name + std::to_string(sheetIndex), &sheetRect);
	SDL_Surface * surfRotated = rotation ? CSDL_Ext::Rotate90(surfCropped) : nullptr;

	auto img = std::make_shared<SDLImageShared>(surfRotated ? surfRotated : surfCropped);
	if(fullSize.x > 0 && fullSize.y > 0)
		img->setFullSize(fullSize * scalingFactor);
	img->setMargins((margins - Point(image.spriteOffsetX, image.spriteOffsetY)) * scalingFactor);

	SDL_FreeSurface(surfCropped);
	if(surfRotated)
		SDL_FreeSurface(surfRotated);
	
	return img;
}

std::optional<std::tuple<ResourcePath, PakLoader::ArchiveEntry, PakLoader::ImageEntry>> HdImageLoader::find(const ImagePath & path)
{
	const auto targetName = boost::algorithm::to_upper_copy(path.getName());
	int scale = scalingFactor;

	auto scaleIt = pakLoader->imagesByName.find(scale);
	if (scaleIt != pakLoader->imagesByName.end())
	{
		auto &nameMap = scaleIt->second;
		auto imageIt = nameMap.find(targetName);
		if (imageIt != nameMap.end())
		{
			auto &[resourcePath, imagePtr, entryPtr] = imageIt->second;
			return std::make_tuple(resourcePath, *entryPtr, *imagePtr);
		}
	}

	return std::nullopt;
}

bool HdImageLoader::exists(const ImagePath & path)
{
	return find(path).has_value();
}
