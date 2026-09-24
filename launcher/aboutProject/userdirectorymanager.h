/*
 * userdirectorymanager.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../StdInc.h"

class IVCMIDirs;
class ProgressOverlay;
enum class EUserDirectory;

#if defined(VCMI_WINDOWS)
class WindowsUserDirectoryManager
{
	enum class EExistingTargetAction
	{
		MERGE,
		BACK_UP,
		REPLACE
	};

	QWidget * parent;

	QString tr(const char * text) const;
	QString normalizedPath(const QString & path) const;
	bool isSameOrChildPath(const QString & path, const QString & parentPath) const;
	bool pathsOverlap(const QString & first, const QString & second) const;
	bool isDirectoryWritable(const QString & path) const;
	void reportPermissionError(const QString & message) const;
	bool validateTarget(const IVCMIDirs & dirs, EUserDirectory changedDirectory, const QString & source, const QString & target) const;
	qint64 directorySize(const QString & path) const;
	QString formattedDataSize(qint64 bytes) const;
	bool containsActiveUserDirectory(const IVCMIDirs & dirs, EUserDirectory changedDirectory, const QString & path) const;
	bool copyDirectoryContents(const QString & source, const QString & destination, ProgressOverlay & progress, QString & error, bool overwrite = false) const;
	std::optional<EExistingTargetAction> askExistingTargetAction(const QString & target) const;
	QString availableBackupPath(const QString & target) const;
	bool installStagedDirectory(const QString & staging, const QString & target, EExistingTargetAction action, QString & backupPath, QString & error) const;

public:
	explicit WindowsUserDirectoryManager(QWidget * parent);
	void changeDirectory(EUserDirectory directory, const QString & title, const std::function<void(const QString &)> & onDirectoriesChanged) const;
};
#endif
