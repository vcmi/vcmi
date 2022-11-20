#include "StdInc.h"

#include "ResourceConverter.h"

#include "../lib/JsonNode.h"
#include "../lib/VCMIDirs.h"
#include "../lib/filesystem/Filesystem.h"

#include "BitmapHandler.h"
#include "Animation.h"

#include "boost/filesystem/path.hpp"
#include "boost/locale.hpp"

namespace bfs = boost::filesystem;

// converts all pcx files from /Images into PNG
void ConvertPcxToPng(bool deleteOriginals)
{
	bfs::path imagesPath = VCMIDirs::get().userCachePath() / "extracted" / "Images";
	bfs::directory_iterator end_iter;

	for ( bfs::directory_iterator dir_itr(imagesPath); dir_itr != end_iter; ++dir_itr )
	{
		try
		{
			if (!bfs::is_regular_file(dir_itr->status()))
				return;

			std::string filePath = dir_itr->path().string();
			std::string fileStem = dir_itr->path().stem().string();
			std::string filename = dir_itr->path().filename().string();
			filename = boost::locale::to_lower(filename);

			if(filename.find(".pcx") != std::string::npos)
			{
				auto img = BitmapHandler::loadBitmap(filename);
				bfs::path pngFilePath = imagesPath / (fileStem + ".png");
				img.save(QStringFromPath(pngFilePath), "PNG");

				if (deleteOriginals)
					bfs::remove(filePath);
			}
		}
		catch ( const std::exception & ex )
		{
			logGlobal->info(dir_itr->path().filename().string() + " " + ex.what() + "\n");
		}
	}
}

// splits a def file into individual parts and converts the output to PNG format
void SplitDefFile(std::string fileName, bfs::path spritesPath, bool deleteOriginals)
{
	if (CResourceHandler::get()->existsResource(ResourceID("SPRITES/" + fileName)))
	{
		std::string URI = fileName;
		std::unique_ptr<Animation> anim = make_unique<Animation>(URI);
		anim->preload();
		anim->exportBitmaps(VCMIDirs::get().userCachePath() / "extracted", true);

		if(deleteOriginals)
			bfs::remove(spritesPath / fileName);
	}
	else
		logGlobal->error("Def File Split error! " + fileName);
}

// splits def files (TwCrPort, CPRSMALL, FlagPort, ITPA, ITPt, Un32 and Un44), this way faction resources are independent
void splitDefFiles(bool deleteOriginals)
{
	bfs::path extractedPath = VCMIDirs::get().userDataPath() / "extracted";
	bfs::path spritesPath = extractedPath / "Sprites";

	SplitDefFile("TwCrPort.def", spritesPath, deleteOriginals);	// split town creature portraits
	SplitDefFile("CPRSMALL.def", spritesPath, deleteOriginals);	// split hero army creature portraits 
	SplitDefFile("FlagPort.def", spritesPath, deleteOriginals);	// adventure map dwellings
	SplitDefFile("ITPA.def", spritesPath, deleteOriginals);		// small town icons
	SplitDefFile("ITPt.def", spritesPath, deleteOriginals);		// big town icons
	SplitDefFile("Un32.def", spritesPath, deleteOriginals);		// big town icons
	SplitDefFile("Un44.def", spritesPath, deleteOriginals);		// big town icons
}

void ConvertExtractedResourceFiles(bool splitDefs, bool convertPcxToPng, bool deleteOriginals)
{
	if (splitDefs)
		splitDefFiles(deleteOriginals);

	if (convertPcxToPng)
		ConvertPcxToPng(deleteOriginals);
}