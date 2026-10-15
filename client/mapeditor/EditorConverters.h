/*
 * EditorConverters.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class EditorCallback;

/// Conversion of the original maps and campaigns into the VCMI formats
namespace EditorConverters
{
	/// Asks for h3m maps and for a directory, saves the maps into the directory as vmap
	void convertMaps(EditorCallback * callback);

	/// Asks for a h3c campaign and for the file to save it as vcmp
	void convertCampaign();
}
