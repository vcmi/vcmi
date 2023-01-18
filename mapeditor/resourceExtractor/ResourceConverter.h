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
	bool moveExtractedArchivesToSoDMod = false;
};

/**
* Class functionality to be used after extracting original H3 resources and before moving those resources to SoD Mod
* Splits def files containing individual images, so that faction resources are independent. (TwCrPort, CPRSMALL, FlagPort, ITPA, ITPt, Un32 and Un44)
* (town creature portraits, hero army creature portraits, adventure map dwellings, small town icons, big town icons,
* hero speciality small icons, hero speciality large icons)
* Converts all PCX images to PNG
 */
class ResourceConverter
{

public:

	// Splits def files that are shared between factions and converts pcx to PNG depending on Extraction Options
	static void convertExtractedResourceFiles(ConversionOptions conversionOptions);

private:

	/** Converts all .pcx from extractedFolder/Images into .png */
	static void doConvertPcxToPng(const bfs::path & sourceFolder, bool deleteOriginals);

	/** splits a .def file into individual images and converts the output to PNG format */
	static void splitDefFile(const std::string & fileName, const bfs::path & sourceFolder, bool deleteOriginals);

	/** Splits the given .def files into individual images.
	* For each .def file, the resulting images will be output in the same folder, in a subfolder (named just like the .def file)
	*/
	static void splitDefFiles(const std::vector<std::string> & defFileNames, const bfs::path & sourceFolder, bool deleteOriginals);
};
