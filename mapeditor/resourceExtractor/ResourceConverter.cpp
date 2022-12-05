/*
 * ResourceConverter.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "ResourceConverter.h"

#include "../lib/JsonNode.h"
#include "../lib/VCMIDirs.h"
#include "../lib/filesystem/Filesystem.h"

#include "BitmapHandler.h"
#include "Animation.h"

#include "boost/filesystem/path.hpp"
#include "boost/locale.hpp"

void ResourceConverter::convertExtractedResourceFiles(ConversionOptions conversionOptions)
{
	if (conversionOptions.splitDefs)
		splitDefFiles(conversionOptions.deleteOriginals);

	if (conversionOptions.convertPcxToPng)
		doConvertPcxToPng(conversionOptions.deleteOriginals);
}

void ResourceConverter::doConvertPcxToPng(bool deleteOriginals)
{
	std::string filename;

	bfs::path imagesPath = VCMIDirs::get().userExtractedPath() / "IMAGES";
	bfs::directory_iterator end_iter;

	for(bfs::directory_iterator dir_itr(imagesPath); dir_itr != end_iter; ++dir_itr)
	{
		try
		{
			if (!bfs::is_regular_file(dir_itr->status()))
				return;

			std::string filePath = dir_itr->path().string();
			std::string fileStem = dir_itr->path().stem().string();
			filename = dir_itr->path().filename().string();
			std::string filenameLowerCase = boost::algorithm::to_lower_copy(filename);

			if(bfs::extension(filenameLowerCase) == ".pcx")
			{
				auto img = BitmapHandler::loadBitmap(filenameLowerCase);
				bfs::path pngFilePath = imagesPath / (fileStem + ".png");
				img.save(pathToQString(pngFilePath), "PNG");

				if(deleteOriginals)
					bfs::remove(filePath);
			}
		}
		catch(const std::exception & ex)
		{
			logGlobal->info(filename + " " + ex.what() + "\n");
		}
	}
}

void ResourceConverter::splitDefFile(const std::string & fileName, const bfs::path & spritesPath, bool deleteOriginals)
{
	if(CResourceHandler::get()->existsResource(ResourceID("SPRITES/" + fileName)))
	{
		std::unique_ptr<Animation> anim = make_unique<Animation>(fileName);
		anim->preload();
		anim->exportBitmaps(pathToQString(VCMIDirs::get().userExtractedPath()));

		if(deleteOriginals)
			bfs::remove(spritesPath / fileName);
	}
	else
		logGlobal->error("Def File Split error! " + fileName);
}

void ResourceConverter::splitDefFiles(bool deleteOriginals)
{
	bfs::path spritesPath = VCMIDirs::get().userExtractedPath() / "SPRITES";

	for(std::string defFilename : {"TwCrPort.def", "CPRSMALL.def", "FlagPort.def", "ITPA.def", "ITPt.def", "Un32.def", "Un44.def"})
		splitDefFile(defFilename, spritesPath, deleteOriginals);
}