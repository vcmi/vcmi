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
#include <QVector>

#include <atomic>
#include <memory>

class JsonNode;

class ModStateModel;

class ModStateController : public QObject, public boost::noncopyable
{
	Q_OBJECT

public:
	struct ArchiveImportResult
	{
		QStringList installedMods;
		QStringList errors;
		bool canceled = false;
		bool mapArchive = false;
		bool saveArchive = false;
	};

	struct ArchiveImportProgress
	{
		std::atomic<quint64> completedBytes{0};
		std::atomic<quint64> totalBytes{0};
		std::atomic<int> completedMods{0};
		std::atomic<int> totalMods{0};
		std::atomic<bool> canceled{false};
	};

private:
	std::shared_ptr<ModStateModel> modList;

	// check-free version of public method
	bool doUninstallMod(QString mod);

	static QString getStagingPath(const QString & modname);
	static QString getBackupPath(const QString & modname);
	/// Returns directory of a mod located in user Mods directory, matched case-insensitively, or empty string if none
	static QString findModDirectory(const QString & modname);
	/// Returns human-readable reason why mod can not be uninstalled or updated, or empty string if it can
	static QString getUnmanageableReason(const QString & modname);

	QStringList recentErrors;
	bool addError(QString modname, QString message);
	static bool removeModDir(QString mod);

public:
	ModStateController(std::shared_ptr<ModStateModel> modList);
	~ModStateController();

	void setRepositoryData(const JsonNode & repositoriesList);

	QStringList getErrors();

	/// Installs every mod found in the archive. File system only, safe to run outside the GUI thread.
	static ArchiveImportResult installArchive(const QString & archivePath, const QString & modIdHint,
		bool fullModExtraction, const QStringList & protectedMods, ArchiveImportProgress & progress);
	/// Derives the mod ID from the archive filename, including GitHub zip suffixes.
	static QString modIdFromArchivePath(const QString & archivePath);

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
