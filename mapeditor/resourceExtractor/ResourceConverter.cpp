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
	bfs::path spritesPath = VCMIDirs::get().userExtractedPath() / "SPRITES";
	bfs::path imagesPath = VCMIDirs::get().userExtractedPath() / "IMAGES";
	std::vector<std::string> defFiles = { "TwCrPort.def", "CPRSMALL.def", "FlagPort.def", "ITPA.def", "ITPt.def", "Un32.def", "Un44.def" };

	if(conversionOptions.splitDefs)
		splitDefFiles(defFiles, spritesPath, conversionOptions.deleteOriginals);

	if(conversionOptions.convertPcxToPng)
		doConvertPcxToPng(imagesPath, conversionOptions.deleteOriginals);
}

void ResourceConverter::doConvertPcxToPng(const bfs::path & sourceFolder, bool deleteOriginals)
{
	logGlobal->info("Converting .pcx to .png from folder: %s ...\n", sourceFolder);

	for(const auto & directoryEntry : bfs::directory_iterator(sourceFolder))
	{
		const auto filename = directoryEntry.path().filename();
		try
		{
			if(!bfs::is_regular_file(directoryEntry))
				continue;

			std::string fileStem = directoryEntry.path().stem().string();
			std::string filenameLowerCase = boost::algorithm::to_lower_copy(filename.string());

			if(boost::algorithm::to_lower_copy(filename.extension().string()) == ".pcx")
			{
				auto img = BitmapHandler::loadBitmap(filenameLowerCase);
				bfs::path pngFilePath = sourceFolder / (fileStem + ".png");
				img.save(pathToQString(pngFilePath), "PNG");

				if(deleteOriginals)
					bfs::remove(directoryEntry.path());
			}
		}
		catch(const std::exception& ex)
		{
			logGlobal->info(filename.string() + " " + ex.what() + "\n");
		}
	}
}

void ResourceConverter::splitDefFile(const std::string & fileName, const bfs::path & sourceFolder, bool deleteOriginals)
{
	if(CResourceHandler::get()->existsResource(ResourceID("SPRITES/" + fileName)))
	{
		std::unique_ptr<Animation> anim = std::make_unique<Animation>(fileName);
		anim->preload();
		anim->exportBitmaps(pathToQString(sourceFolder));

		if(deleteOriginals)
			bfs::remove(sourceFolder / fileName);
	}
	else
		logGlobal->error("Def File Split error! " + fileName);
}

void ResourceConverter::splitDefFiles(const std::vector<std::string> & defFileNames, const bfs::path & sourceFolder, bool deleteOriginals)
{
	logGlobal->info("Splitting Def Files from folder: %s ...\n", sourceFolder);

	for(const auto & defFilename : defFileNames)
		splitDefFile(defFilename, sourceFolder, deleteOriginals);
}
