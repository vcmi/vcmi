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

namespace CLauncherDirs
{
void prepare()
{
	for(auto path : {downloadsPath(), modsPath(), mapsPath()})
		QDir{}.mkdir(path);
}

QString downloadsPath()
{
	return pathToQString(VCMIDirs::get().userCachePath() / "downloads");
}

QString modsPath()
{
	return pathToQString(VCMIDirs::get().userDataPath() / "Mods");
}

QString mapsPath()
{
	return pathToQString(VCMIDirs::get().userDataPath() / "Maps");
}

QSettings getSettings(QString appName)
{
    return QSettings(pathToQString(VCMIDirs::get().userConfigPath() / (appName.toStdString() + ".ini")), QSettings::IniFormat);
}
}
