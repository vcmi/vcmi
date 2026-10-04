/*
 * modstatecontroller.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QVector>

class JsonNode;

class ModStateModel;

class ModStateController : public QObject, public boost::noncopyable
{
	Q_OBJECT

	std::shared_ptr<ModStateModel> modList;

	// check-free version of public method
	bool doUninstallMod(QString mod);

	static QString getStagingPath(const QString & modname);
	static QString getBackupPath(const QString & modname);
	/// Returns directory of a mod located in user Mods directory, matched case-insensitively, or empty string if none
	static QString findModDirectory(const QString & modname);

	QStringList recentErrors;
	bool addError(QString modname, QString message);
	bool removeModDir(QString mod);

public:
	ModStateController(std::shared_ptr<ModStateModel> modList);
	~ModStateController();

	void setRepositoryData(const JsonNode & repositoriesList);

	QStringList getErrors();

	/// mod management functions. Return true if operation was successful

	/// installs mod from zip archive located at archivePath, replacing installed version of the mod, if any. On failure, installed version remains unchanged
	bool installMod(QString mod, QString archivePath);
	bool uninstallMod(QString mod);
	bool enableMods(QStringList mod);
	bool disableMod(QString mod);

	bool canUninstallMod(QString mod);
	bool canEnableMod(QString mod);
	bool canDisableMod(QString mod);
	
signals:
	void extractionProgress(qint64 currentAmount, qint64 maxAmount);
	void contentExtractionProgress(QString modName, qint64 currentAmount, qint64 maxAmount);
};
