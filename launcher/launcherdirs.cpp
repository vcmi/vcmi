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
