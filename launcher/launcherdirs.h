/*
 * launcherdirs.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

/// similar to lib/VCMIDirs, controls where all launcher-related data will be stored
namespace CLauncherDirs
{
	void prepare();

	QString downloadsPath();
	QString modsPath();
	QString mapsPath();
}
