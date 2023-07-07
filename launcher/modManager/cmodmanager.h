/*
 * cmodmanager.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "cmodlist.h"

class CModManager : public QObject
{
	Q_OBJECT

	CModList * modList;

	QString settingsPath();

	// check-free version of public method
	bool doEnableMod(QString mod, bool on);
	bool doInstallMod(QString mod, QString archivePath);
	bool doUninstallMod(QString mod);

	QVariantMap modSettings;
	QVariantMap localMods;

	QStringList recentErrors;
	bool addError(QString modname, QString message);
	bool removeModDir(QString mod);

public:
	CModManager(CModList * modList);

	void resetRepositories();
	void loadRepository(QVariantMap repomap);
	void loadModSettings();
	void loadMods();

	QStringList getErrors();

	/// mod management functions. Return true if operation was successful

	/// installs mod from zip archive located at archivePath
	bool installMod(QString mod, QString archivePath);
	bool uninstallMod(QString mod);
	bool enableMod(QString mod);
	bool disableMod(QString mod);

	bool canInstallMod(QString mod);
	bool canUninstallMod(QString mod);
	bool canEnableMod(QString mod);
	bool canDisableMod(QString mod);
};
