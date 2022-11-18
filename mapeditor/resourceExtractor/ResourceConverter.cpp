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

bool split_def_files = false;		// splits TwCrPort, CPRSMALL, FlagPort, ITPA, ITPt, Un32 and Un44 into individual PNG's
bool convert_pcx_to_png = false;	// converts single Images (found in Images folder) from .pcx to png.
bool delete_source_files = false;	// delete source files after splitting / conversion.

// converts all pcx files from /Images into PNG
void convertPcxToPng()
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

				if (delete_source_files)
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
void splitDefFile(std::string fileName, bfs::path spritesPath)
{
	if (CResourceHandler::get()->existsResource(ResourceID("SPRITES/" + fileName)))
	{
		std::string URI = fileName;
		std::unique_ptr<Animation> anim = make_unique<Animation>(URI);
		anim->preload();
		anim->exportBitmaps(VCMIDirs::get().userCachePath() / "extracted", true);

		if(delete_source_files)
			bfs::remove(spritesPath / fileName);
	}
	else
		logGlobal->error("Def File Split error! " + fileName);
}

// splits def files (TwCrPort, CPRSMALL, FlagPort, ITPA, ITPt, Un32 and Un44), this way faction resources are independent
void splitDefFiles()
{
	bfs::path extractedPath = VCMIDirs::get().userDataPath() / "extracted";
	bfs::path spritesPath = extractedPath / "Sprites";

	splitDefFile("TwCrPort.def", spritesPath);	// split town creature portraits
	splitDefFile("CPRSMALL.def", spritesPath);	// split hero army creature portraits 
	splitDefFile("FlagPort.def", spritesPath);	// adventure map dwellings
	splitDefFile("ITPA.def", spritesPath);		// small town icons
	splitDefFile("ITPt.def", spritesPath);		// big town icons
	splitDefFile("Un32.def", spritesPath);		// big town icons
	splitDefFile("Un44.def", spritesPath);		// big town icons
}

// Splits def files that are shared between factions and converts pcx to bmp
void ConvertOriginalResourceFiles()
{
	if (split_def_files)
		splitDefFiles();

	if (convert_pcx_to_png)
		convertPcxToPng();
}