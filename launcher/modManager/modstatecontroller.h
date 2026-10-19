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

#include <QObject>
#include <QStringList>

class JsonNode;

class ModStateModel;

class ModStateController : public QObject, public boost::noncopyable
{
	Q_OBJECT

	std::shared_ptr<ModStateModel> modList;

	// check-free version of public method
	bool doUninstallMod(QString mod);

	/// Returns directory of a mod located in user Mods directory, matched case-insensitively, or empty string if none
	static QString findModDirectory(const QString & modname);
	/// Returns human-readable reason why mod can not be uninstalled or updated, or empty string if it can
	static QString getUnmanageableReason(const QString & modname);

	QStringList recentErrors;
	bool addError(QString modname, QString message);
	bool removeModDir(QString mod);

public:
	ModStateController(std::shared_ptr<ModStateModel> modList);
	~ModStateController();

	void setRepositoryData(const JsonNode & repositoriesList);

	QStringList getErrors();

	/// mod management functions. Return true if operation was successful

	bool uninstallMod(QString mod);
	bool enableMods(QStringList mod);
	bool disableMod(QString mod);

	bool canUninstallMod(QString mod);
	/// mods located outside of user Mods directory, e.g. in system-wide data directory, or under version control can not be uninstalled or updated
	bool isModManageable(QString mod);
	bool canEnableMod(QString mod);
	bool canDisableMod(QString mod);
	
};
