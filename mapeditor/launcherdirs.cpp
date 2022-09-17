/*
 * launcherdirs.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "launcherdirs.h"

#include "../lib/VCMIDirs.h"

static CLauncherDirs launcherDirsGlobal;

CLauncherDirs::CLauncherDirs()
{
	QDir().mkdir(downloadsPath());
	QDir().mkdir(modsPath());
}

CLauncherDirs & CLauncherDirs::get()
{
	return launcherDirsGlobal;
}

QString CLauncherDirs::downloadsPath()
{
	return pathToQString(VCMIDirs::get().userCachePath() / "downloads");
}

QString CLauncherDirs::modsPath()
{
	return pathToQString(VCMIDirs::get().userDataPath() / "Mods");
}
