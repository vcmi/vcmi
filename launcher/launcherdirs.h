#pragma once

/// similar to lib/VCMIDirs, controls where all launcher-related data will be stored
class CLauncherDirs
{
public:
	CLauncherDirs();

	static CLauncherDirs & get();

	QString downloadsPath();
	QString modsPath();
};
