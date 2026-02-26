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

#include "vcmiqt.h"

/// similar to lib/VCMIDirs, controls where all launcher-related data will be stored
namespace CLauncherDirs
{
	VCMIQT_LINKAGE void prepare();

	VCMIQT_LINKAGE QString downloadsPath();
	VCMIQT_LINKAGE QString modsPath();
	VCMIQT_LINKAGE QString mapsPath();
	VCMIQT_LINKAGE QSettings getSettings(QString appName);
}
