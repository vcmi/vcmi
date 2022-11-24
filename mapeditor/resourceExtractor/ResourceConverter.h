/*
 * ResourceConverter.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

namespace bfs = boost::filesystem;

// Struct for holding all Convertor Options
struct ConversionOptions
{
	bool splitDefs = false;			// splits TwCrPort, CPRSMALL, FlagPort, ITPA, ITPt, Un32 and Un44 into individual PNG's
	bool convertPcxToPng = false;	// converts single Images (found in Images folder) from .pcx to png.
	bool deleteOriginals = false;	// delete original files, for the ones splitted / converted.
};

// Struct for holding all Resource Extractor / Converter options
struct ExtractionOptions
{
	bool extractArchives = false;	// if set, original H3 archives will be extracted into a separate folder
	ConversionOptions conversionOptions;
};

class ResourceConverter
{

public:

	// Splits def files that are shared between factions and converts pcx to bmp depending on Extraction Options
	static void convertExtractedResourceFiles(ConversionOptions conversionOptions);

private:

	// converts all pcx files from /Images into PNG
	static void doConvertPcxToPng(bool deleteOriginals);

	// splits a def file into individual parts and converts the output to PNG format
	static void splitDefFile(const std::string& fileName, const bfs::path& spritesPath, bool deleteOriginals);

	// splits def files (TwCrPort, CPRSMALL, FlagPort, ITPA, ITPt, Un32 and Un44) so that faction resources are independent
	// (town creature portraits, hero army creature portraits, adventure map dwellings, small town icons, big town icons, 
	// hero speciality small icons, hero speciality large icons)
	static void splitDefFiles(bool deleteOriginals);
};