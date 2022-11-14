#include "StdInc.h"

#include "ResourceConverter.h"

#include "../lib/JsonNode.h"
#include "../lib/VCMIDirs.h"
#include "../lib/filesystem/Filesystem.h"

//#include "SDL.h"
#include "Animation.h"
//#include "CBitmapHandler.h"

#include "boost/filesystem/path.hpp"
#include "boost/locale.hpp"

namespace bfs = boost::filesystem;

bool split_def_files = 1;
bool convert_pcx_to_bmp = 0; // converts Images from .pcx to bmp. Can be used when you have .pcx converted already
bool delete_source_files = 0; // delete source files or leave a copy in place.

// converts all pcx files into bmp (H3 saves images as .pcx)
//void convertPcxToBmp()
//{
//	bfs::path extractedPath = VCMIDirs::get().userDataPath() / "extracted";
//	bfs::path imagesPath = extractedPath / "Images";
//
//	bfs::directory_iterator end_iter;
//
//	for ( bfs::directory_iterator dir_itr(imagesPath); dir_itr != end_iter; ++dir_itr )
//	{
//		try
//		{
//			if ( bfs::is_regular_file( dir_itr->status() ) )
//			{
//				std::string filename = dir_itr->path().filename().string();
//				filename = boost::locale::to_lower(filename);
//
//				if(filename.find(".pcx") != std::string::npos)
//				{
//					SDL_Surface *bitmap;
//					
//					bitmap = BitmapHandler::loadBitmap(filename);
//					
//					if(delete_source_files)
//						bfs::remove(imagesPath / filename);
//
//					bfs::path outFilePath = imagesPath / filename;
//					outFilePath.replace_extension(".bmp");
//	 				SDL_SaveBMP(bitmap, outFilePath.string().c_str());
//				}
//			}
//			else
//			{
//				logGlobal->info(dir_itr->path().filename().string() + " [other]\n");
//			}
//		}
//		catch ( const std::exception & ex )
//		{
//			logGlobal->info(dir_itr->path().filename().string() + " " + ex.what() + "\n");
//		}
//	}
//}

// splits a def file into individual parts
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

// split def files, this way faction resources are independent
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

	//if (convert_pcx_to_bmp)
	//	convertPcxToBmp();
}





















