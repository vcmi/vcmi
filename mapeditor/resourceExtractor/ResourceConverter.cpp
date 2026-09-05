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

#include "../../lib/VCMIDirs.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/texts/TextOperations.h"

#include "BitmapHandler.h"
#include "Animation.h"

#ifdef ENABLE_SINGLE_APP_BUILD
using namespace MapEditor;
#endif

#include <boost/filesystem/path.hpp>

void ResourceConverter::convertExtractedResourceFiles(ConversionOptions conversionOptions)
{
	boost::filesystem::path spritesPath = VCMIDirs::get().userExtractedPath() / "SPRITES";
	boost::filesystem::path imagesPath = VCMIDirs::get().userExtractedPath() / "IMAGES";
	std::vector<std::string> defFiles = { "TwCrPort.def", "CPRSMALL.def", "FlagPort.def", "ITPA.def", "ITPt.def", "Un32.def", "Un44.def" };

	if(conversionOptions.splitDefs)
		splitDefFiles(defFiles, spritesPath, conversionOptions.deleteOriginals);

	if(conversionOptions.convertPcxToPng)
		doConvertPcxToPng(imagesPath, conversionOptions.deleteOriginals);
}

void ResourceConverter::doConvertPcxToPng(const boost::filesystem::path & sourceFolder, bool deleteOriginals)
{
	logGlobal->info("Converting .pcx to .png from folder: %s ...\n", TextOperations::filesystemPathToUtf8(sourceFolder));

	for(const auto & directoryEntry : boost::filesystem::directory_iterator(sourceFolder))
	{
		const auto filename = directoryEntry.path().filename();
		try
		{
			if(!boost::filesystem::is_regular_file(directoryEntry))
				continue;

			std::string filenameLowerCase = boost::algorithm::to_lower_copy(TextOperations::filesystemPathToUtf8(filename));

			if(boost::algorithm::to_lower_copy(TextOperations::filesystemPathToUtf8(filename.extension())) == ".pcx")
			{
				auto img = BitmapHandler::loadBitmap(filenameLowerCase);
				boost::filesystem::path pngFilePath = boost::filesystem::path(directoryEntry.path()).replace_extension(".png");
				img.save(pathToQString(pngFilePath), "PNG");

				if(deleteOriginals)
					boost::filesystem::remove(directoryEntry.path());
			}
		}
		catch(const std::exception& ex)
		{
			logGlobal->info(TextOperations::filesystemPathToUtf8(filename) + " " + ex.what() + "\n");
		}
	}
}

void ResourceConverter::splitDefFile(const std::string & fileName, const boost::filesystem::path & sourceFolder, bool deleteOriginals)
{
	if(CResourceHandler::get()->existsResource(ResourcePath("SPRITES/" + fileName)))
	{
		auto anim = std::make_unique<Animation>(fileName);
		anim->preload();
		anim->exportBitmaps(pathToQString(sourceFolder));

		if(deleteOriginals)
			boost::filesystem::remove(sourceFolder / TextOperations::Utf8TofilesystemPath(fileName));
	}
	else
		logGlobal->error("Def File Split error! " + fileName);
}

void ResourceConverter::splitDefFiles(const std::vector<std::string> & defFileNames, const boost::filesystem::path & sourceFolder, bool deleteOriginals)
{
	logGlobal->info("Splitting Def Files from folder: %s ...\n", TextOperations::filesystemPathToUtf8(sourceFolder));

	for(const auto & defFilename : defFileNames)
		splitDefFile(defFilename, sourceFolder, deleteOriginals);
}
