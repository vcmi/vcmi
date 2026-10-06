/*
 * datamanager.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "StdInc.h"

class IVCMIDirs;
class ProgressOverlay;
enum class EUserDirectory;

/// Moves launcher-managed user directories and safely switches the active VCMI paths.
class datamanager final : public QObject
{
public:
	datamanager(QWidget * parent, std::function<void(const QString &)> onDirectoriesChanged);
	void changeDirectory(EUserDirectory directory, const QString & title) const;

private:
	QWidget * parent;
	std::function<void(const QString &)> onDirectoriesChanged;

	enum class EExistingTargetAction
	{
		MERGE,
		BACK_UP,
		REPLACE
	};

	enum class EChangeResult
	{
		DONE,
		SELECT_ANOTHER
	};

	EChangeResult changeDirectoryOnce(EUserDirectory directory, const QString & title) const;

	QString normalizedPath(const QString & path) const;
	Qt::CaseSensitivity pathCaseSensitivity() const;
	bool isSameOrChildPath(const QString & path, const QString & parentPath) const;
	bool pathsOverlap(const QString & first, const QString & second) const;
	bool isDirectoryWritable(const QString & path) const;
	bool containsActiveUserDirectory(const IVCMIDirs & dirs, EUserDirectory changedDirectory, const QString & path) const;
	bool validateTarget(const IVCMIDirs & dirs, EUserDirectory changedDirectory, const QString & source, const QString & target, bool & selectAnother) const;
	bool confirmOneDriveTarget(const IVCMIDirs & dirs, const QString & target) const;

	bool removePath(const QString & path) const;
	qint64 directorySize(const QString & path) const;
	bool copyDirectoryContents(const QString & source, const QString & destination, ProgressOverlay & progress, QString & error, bool overwrite = false, const QString & excludedPath = {}) const;

	std::optional<EExistingTargetAction> askExistingTargetAction(const QString & target, bool mergeOnly) const;
	QString availableBackupPath(const QString & target) const;
	bool installStagedDirectory(const QString & staging, const QString & target, EExistingTargetAction action, QString & displacedPath, QString & error) const;
	bool restoreDisplacedDirectory(const QString & target, const QString & displacedPath) const;

	QString installationDataPath() const;
	bool reportPermissionError(const QString & message) const;

	QString formattedDataSize(qint64 bytes) const;
};
