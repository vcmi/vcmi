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

VCMI_LIB_NAMESPACE_BEGIN
class JsonNode;
VCMI_LIB_NAMESPACE_END

class ModStateModel;

class ModStateController : public QObject, public boost::noncopyable
{
	Q_OBJECT

	std::shared_ptr<ModStateModel> modList;

	// check-free version of public method
	bool doInstallMod(QString mod, QString archivePath);
	bool doUninstallMod(QString mod);

	QStringList recentErrors;
	bool addError(QString modname, QString message);
	bool removeModDir(QString mod);

public:
	ModStateController(std::shared_ptr<ModStateModel> modList);
	~ModStateController();

	void setRepositoryData(const JsonNode & repositoriesList);

	QStringList getErrors();

	/// mod management functions. Return true if operation was successful

	/// installs mod from zip archive located at archivePath
	bool installMod(QString mod, QString archivePath);
	bool uninstallMod(QString mod);
	bool enableMods(QStringList mod);
	bool disableMod(QString mod);

	bool canInstallMod(QString mod);
	bool canUninstallMod(QString mod);
	bool canEnableMod(QString mod);
	bool canDisableMod(QString mod);
	
signals:
	void extractionProgress(qint64 currentAmount, qint64 maxAmount);
};
