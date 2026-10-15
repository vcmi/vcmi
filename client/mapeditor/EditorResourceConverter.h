/*
 * EditorResourceConverter.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

/// Conversion of the extracted resources of the original game, which is done before they are moved to a mod
namespace EditorResourceConverter
{
	struct Options
	{
		/// Splits the shared def files (TwCrPort, CPRSMALL, FlagPort, ITPA, ITPt, Un32 and Un44) into the images of the factions
		bool splitDefs = false;
		/// Converts the images of the Images directory from PCX to PNG
		bool convertPcxToPng = false;
		/// Deletes the files that are split or converted
		bool deleteOriginals = false;
	};

	struct Result
	{
		int converted = 0;
		int failed = 0;
	};

	Result convert(const Options & options);

	/// Opens the window that asks for the options and converts the resources
	void showDialog();
}
