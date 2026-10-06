/*
 * datamanager.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "datamanager.h"

#include <QCheckBox>
#include <QStorageInfo>
#include <QTemporaryFile>
#include <QUuid>

#if defined(VCMI_WINDOWS)
#include <shellapi.h>
#endif

#include "progressoverlay.h"
#include "helper.h"
#include "mainwindow_moc.h"
#include "modManager/cmodlistview_moc.h"

#include "../lib/ScopeGuard.h"
#include "../lib/VCMIDirs.h"

datamanager::datamanager(QWidget * parent, std::function<void(const QString &)> onDirectoriesChanged)
	: QObject(parent)
	, parent(parent)
	, onDirectoriesChanged(std::move(onDirectoriesChanged))
{
}

QString datamanager::normalizedPath(const QString & path) const
{
	const QFileInfo info(path);
	const QString canonicalPath = info.canonicalFilePath();
	return QDir::cleanPath(canonicalPath.isEmpty() ? info.absoluteFilePath() : canonicalPath);
}

Qt::CaseSensitivity datamanager::pathCaseSensitivity() const
{
#if defined(VCMI_WINDOWS)
	return Qt::CaseInsensitive;
#else
	return Qt::CaseSensitive;
#endif
}

bool datamanager::pathsOverlap(const QString & first, const QString & second) const
{
	return isSameOrChildPath(first, second) || isSameOrChildPath(second, first);
}

bool datamanager::isDirectoryWritable(const QString & path) const
{
	if(!QFileInfo(path).isDir())
		return false;

	QTemporaryFile probe(QDir(path).filePath(QStringLiteral(".vcmi-write-test-XXXXXX")));
	return probe.open();
}

bool datamanager::removePath(const QString & path) const
{
	const auto remove = [&path]()
	{
		const QFileInfo pathInfo(path);
		return pathInfo.isDir() && !pathInfo.isSymLink() ? QDir(path).removeRecursively() : QFile::remove(path);
	};

	if(remove() || !QFileInfo::exists(path))
		return true;

#if defined(VCMI_WINDOWS)
	// QFile/QDir may not remove entries carrying the Windows read-only attribute, even when the user has permission - deleting using Windows Explorer clears it automatically
	QStringList remainingPaths{path};
	QDirIterator iterator(path, QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden | QDir::System, QDirIterator::Subdirectories);
	while(iterator.hasNext())
		remainingPaths.push_back(iterator.next());

	for(const auto & remainingPath : remainingPaths)
	{
		const QString nativePath = QDir::toNativeSeparators(remainingPath);
		const DWORD attributes = GetFileAttributesW(reinterpret_cast<LPCWSTR>(nativePath.utf16()));
		if(attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_READONLY) != 0)
			SetFileAttributesW(reinterpret_cast<LPCWSTR>(nativePath.utf16()), attributes & ~FILE_ATTRIBUTE_READONLY);
	}

	return remove() || !QFileInfo::exists(path);
#else
	return false;
#endif
}

QString datamanager::installationDataPath() const
{
	return pathToQString(VCMIDirs::get().portableUserDataPath());
}

bool datamanager::reportPermissionError(const QString & message) const
{
	QMessageBox dialog(QMessageBox::Critical, tr("Insufficient permissions"), message, QMessageBox::NoButton, parent);
	auto * selectButton = dialog.addButton(tr("Select another location"), QMessageBox::ActionRole);

#if defined(VCMI_WINDOWS)
	auto * restartButton = dialog.addButton(tr("Restart as administrator"), QMessageBox::AcceptRole);
	dialog.addButton(QMessageBox::Cancel);
	dialog.setDefaultButton(restartButton);
#else
	dialog.addButton(QMessageBox::Cancel);
	dialog.setDefaultButton(selectButton);
#endif

	dialog.exec();

	if(dialog.clickedButton() == selectButton)
		return true;

#if defined(VCMI_WINDOWS)
	if(dialog.clickedButton() != restartButton)
		return false;

	const std::wstring executable = QCoreApplication::applicationFilePath().toStdWString();
	SHELLEXECUTEINFOW executeInfo{};
	executeInfo.cbSize = sizeof(executeInfo);
	executeInfo.lpVerb = L"runas";
	executeInfo.lpFile = executable.c_str();
	executeInfo.nShow = SW_SHOWNORMAL;
	if(ShellExecuteExW(&executeInfo) != FALSE)
	{
		qApp->quit();
		return false;
	}

	QMessageBox::critical(parent, tr("Error"), tr("Failed to restart the launcher with administrator privileges."));
#endif
	return false;
}

bool datamanager::confirmOneDriveTarget(const IVCMIDirs & dirs, const QString & target) const
{
	if(!dirs.isOneDrivePath(qstringToPath(target)))
		return true;

	QMessageBox warning(QMessageBox::Warning, tr("OneDrive directory selected"), tr("The selected directory is synchronized by OneDrive:\n%1\n\nOneDrive may lock files while synchronizing them. This can prevent VCMI from writing data and may cause mod installation, game startup, saving, or other operations to fail.").arg(QDir::toNativeSeparators(target)), QMessageBox::NoButton, parent);
	auto * continueButton = warning.addButton(tr("Continue anyway"), QMessageBox::DestructiveRole);
	auto * selectButton = warning.addButton(tr("Select another location"), QMessageBox::RejectRole);
	warning.setDefaultButton(selectButton);
	warning.exec();

	if(warning.clickedButton() != continueButton)
		return false;

	const auto confirmation = QMessageBox::warning(parent, tr("Confirm OneDrive directory"), tr("Using a OneDrive-synchronized directory can make VCMI unreliable and may cause data-writing operations to fail.\n\nDo you really want to use this directory?"), QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
	return confirmation == QMessageBox::Yes;
}

bool datamanager::validateTarget(const IVCMIDirs & dirs, EUserDirectory changedDirectory, const QString & source, const QString & target, bool & selectAnother) const
{
	if(isSameOrChildPath(target, source) && isSameOrChildPath(source, target))
	{
		logGlobal->info("User directory change skipped because source and target are the same: %s", source.toStdString());
		QMessageBox::information(parent, tr("Directory unchanged"), tr("The selected directory is the same as the current directory. Please select a different location."));
		return false;
	}

	const QString binaryPath = QCoreApplication::applicationDirPath();
	const QString compatibleInstallationPath = installationDataPath();
	const bool usesCompatibleInstallationPath = !compatibleInstallationPath.isEmpty() && normalizedPath(target).compare(normalizedPath(compatibleInstallationPath), pathCaseSensitivity()) == 0;

	if(pathsOverlap(target, binaryPath) && !usesCompatibleInstallationPath)
	{
		QMessageBox::warning(parent, tr("Invalid directory"), tr("A user directory cannot be the VCMI installation directory, contain it, or be located inside it."));
		return false;
	}

	static constexpr std::array userDirectories = {
		EUserDirectory::DATA, EUserDirectory::CACHE, EUserDirectory::CONFIG, EUserDirectory::LOGS, EUserDirectory::SAVES
	};

	for(const auto directory : userDirectories)
	{
		if(directory == changedDirectory)
			continue;

		const QString activePath = pathToQString(dirs.userPath(directory));
		if(pathsOverlap(target, activePath))
		{
			QMessageBox::warning(parent, tr("Invalid directory"), tr("The selected directory conflicts with another active VCMI user directory:\n%1\n\nUser directories cannot be the same or contain each other.").arg(QDir::toNativeSeparators(activePath)));
			return false;
		}
	}

	if(!isDirectoryWritable(target))
	{
		selectAnother = reportPermissionError(tr("The selected directory is not writable:\n%1\n\nSelect another location or adjust the directory permissions.").arg(QDir::toNativeSeparators(target)));
		return false;
	}

	if(!confirmOneDriveTarget(dirs, target))
	{
		selectAnother = true;
		return false;
	}

	return true;
}

qint64 datamanager::directorySize(const QString & path) const
{
	qint64 result = 0;
	QDirIterator iterator(path, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

	while(iterator.hasNext())
	{
		iterator.next();
		result += iterator.fileInfo().size();
	}

	return result;
}

QString datamanager::formattedDataSize(qint64 bytes) const
{
	return QLocale().formattedDataSize(bytes, 1, QLocale::DataSizeTraditionalFormat);
}

bool datamanager::isSameOrChildPath(const QString & path, const QString & parentPath) const
{
	const QString cleanPath = normalizedPath(path);
	QString cleanParent = normalizedPath(parentPath);

	if(cleanPath.compare(cleanParent, pathCaseSensitivity()) == 0)
		return true;

	if(!cleanParent.endsWith(QLatin1Char('/')))
		cleanParent += QLatin1Char('/');

	return cleanPath.startsWith(cleanParent, pathCaseSensitivity());
}

bool datamanager::containsActiveUserDirectory(const IVCMIDirs & dirs, EUserDirectory changedDirectory, const QString & path) const
{
	static constexpr std::array userDirectories = {
		EUserDirectory::DATA, EUserDirectory::CACHE, EUserDirectory::CONFIG, EUserDirectory::LOGS, EUserDirectory::SAVES
	};

	for(const auto directory : userDirectories)
		if(directory != changedDirectory && isSameOrChildPath(pathToQString(dirs.userPath(directory)), path))
			return true;

	return false;
}

bool datamanager::copyDirectoryContents(const QString & source, const QString & destination, ProgressOverlay & progress, QString & error, bool overwrite, const QString & excludedPath) const
{
	QDir sourceDir(source);
	int totalFiles = 0;

	QDirIterator counter(source, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
	while(counter.hasNext())
	{
		const QString sourcePath = counter.next();

		if(!excludedPath.isEmpty() && isSameOrChildPath(sourcePath, excludedPath))
			continue;

		if(++totalFiles % 256 == 0)
		{
			progress.setFileName(tr("Scanning files..."));
			qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
		}
	}
	progress.setRange(totalFiles);

	int copiedFiles = 0;
	QDirIterator iterator(source, QDir::NoDotAndDotDot | QDir::AllEntries, QDirIterator::Subdirectories);
	while(iterator.hasNext())
	{
		const QString sourcePath = iterator.next();

		if(!excludedPath.isEmpty() && isSameOrChildPath(sourcePath, excludedPath))
			continue;

		const QString relativePath = sourceDir.relativeFilePath(sourcePath);
		const QString destinationPath = QDir(destination).filePath(relativePath);
		const QFileInfo sourceInfo(sourcePath);

		if(sourceInfo.isSymLink())
		{
			error = tr("The directory contains a symbolic link, which cannot be migrated safely: %1").arg(sourcePath);
			return false;
		}

		if(!sourceInfo.isDir() && !sourceInfo.isFile())
		{
			error = tr("The directory contains an unsupported filesystem entry, which cannot be migrated safely: %1").arg(sourcePath);
			return false;
		}

		if(sourceInfo.isDir())
		{
			if(overwrite && QFileInfo(destinationPath).isFile() && !removePath(destinationPath))
			{
				error = tr("Failed to replace file with directory: %1").arg(destinationPath);
				return false;
			}

			if(!QDir().mkpath(destinationPath))
			{
				error = tr("Failed to create directory: %1").arg(destinationPath);
				return false;
			}
			continue;
		}

		progress.setValue(copiedFiles);
		progress.setFileName(QDir::toNativeSeparators(relativePath));
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

		if(overwrite && QFileInfo::exists(destinationPath))
		{
			const bool removed = removePath(destinationPath);
			if(!removed)
			{
				error = tr("Failed to overwrite: %1").arg(destinationPath);
				return false;
			}
		}

		if(!QDir().mkpath(QFileInfo(destinationPath).absolutePath()) || !Helper::performNativeCopy(sourcePath, destinationPath))
		{
			error = tr("Failed to copy file: %1").arg(sourcePath);
			return false;
		}
		++copiedFiles;
	}

	progress.setValue(totalFiles);
	progress.setFileName(QString());

	qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

	return true;
}

std::optional<datamanager::EExistingTargetAction> datamanager::askExistingTargetAction(const QString & target, bool mergeOnly) const
{
	QMessageBox dialog(QMessageBox::Question, tr("Directory is not empty"), tr("The target directory already contains files:\n%1\n\nHow should they be handled?").arg(QDir::toNativeSeparators(target)), QMessageBox::NoButton, parent);
	dialog.setInformativeText(mergeOnly
		? tr("The source and target are in the same directory tree. Only merge is available for this move. Merge keeps files that exist only in the target and overwrites conflicts.")
		: tr("Merge keeps files that exist only in the target and overwrites conflicts.\nBack up and replace moves the current target to a _backup directory.\nClean replacement removes the current target after the new copy is ready."));

	auto * const mergeButton = dialog.addButton(tr("Merge and overwrite"), QMessageBox::AcceptRole);
	QPushButton * backupButton = nullptr;
	QPushButton * replaceButton = nullptr;
	if(!mergeOnly)
	{
		backupButton = dialog.addButton(tr("Back up and replace"), QMessageBox::ActionRole);
		replaceButton = dialog.addButton(tr("Clean replacement"), QMessageBox::DestructiveRole);
	}

	dialog.addButton(QMessageBox::Cancel);
	dialog.setDefaultButton(mergeOnly ? mergeButton : backupButton);

	dialog.exec();

	if(dialog.clickedButton() == mergeButton)
		return EExistingTargetAction::MERGE;

	if(!mergeOnly && dialog.clickedButton() == backupButton)
		return EExistingTargetAction::BACK_UP;

	if(!mergeOnly && dialog.clickedButton() == replaceButton)
	{
		const auto answer = QMessageBox::warning(parent, tr("Confirm clean replacement"), tr("Clean replacement will permanently remove all files currently in the target directory after the new data has been copied successfully.\n\nDo you really want to continue?"), QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
		if(answer != QMessageBox::Yes)
			return std::nullopt;

		return EExistingTargetAction::REPLACE;
	}

	return std::nullopt;
}

QString datamanager::availableBackupPath(const QString & target) const
{
	const QString basePath = target + QStringLiteral("_backup");
	if(!QFileInfo::exists(basePath))
		return basePath;

	for(int index = 2; ; ++index)
	{
		const QString candidate = basePath + QStringLiteral("_%1").arg(index);
		if(!QFileInfo::exists(candidate))
			return candidate;
	}
}

bool datamanager::installStagedDirectory(const QString & staging, const QString & target, EExistingTargetAction action, QString & displacedPath, QString & error) const
{
	const QFileInfo targetInfo(target);
	const QString temporaryPath = targetInfo.dir().filePath(QStringLiteral(".%1-vcmi-old-%2").arg(targetInfo.fileName(), QUuid::createUuid().toString(QUuid::Id128)));
	displacedPath = action == EExistingTargetAction::BACK_UP ? availableBackupPath(target) : temporaryPath;

	if(!QDir().rename(target, displacedPath))
	{
		error = tr("Failed to move the existing target directory: %1").arg(target);
		return false;
	}

	if(!QDir().rename(staging, target))
	{
		QDir().rename(displacedPath, target);
		error = tr("Failed to place copied files in the selected directory.");
		return false;
	}

	return true;
}

bool datamanager::restoreDisplacedDirectory(const QString & target, const QString & displacedPath) const
{
	if(displacedPath.isEmpty())
		return true;

	const QString failedPath = QFileInfo(target).dir().filePath(QStringLiteral(".%1-vcmi-failed-%2").arg(QFileInfo(target).fileName(), QUuid::createUuid().toString(QUuid::Id128)));
	if(!QDir().rename(target, failedPath))
		return false;

	if(!QDir().rename(displacedPath, target))
	{
		QDir().rename(failedPath, target);
		return false;
	}

	if(!removePath(failedPath))
		logGlobal->warn("Restored directory '%s', but failed to remove the rejected data at '%s'", target.toStdString(), failedPath.toStdString());

	return true;
}

datamanager::EChangeResult datamanager::changeDirectoryOnce(EUserDirectory directory, const QString & title) const
{
	auto * mainWindow = qobject_cast<MainWindow *>(parent->window());
	auto & dirs = VCMIDirs::get();

	if(!dirs.supportsUserPathChange())
	{
		QMessageBox::critical(parent, tr("Unsupported operation"), tr("Changing user directories is not supported on this platform."));
		return EChangeResult::DONE;
	}

	const QString source = pathToQString(dirs.userPath(directory));
	QString selected;
	const QString binaryPath = QCoreApplication::applicationDirPath();
	while(true)
	{
		selected = QFileDialog::getExistingDirectory(parent, title, selected.isEmpty() ? source : selected);
		if(selected.isEmpty())
			return EChangeResult::DONE;

		logGlobal->info("Changing user directory from '%s' to '%s'", source.toStdString(), selected.toStdString());

		const QString compatibleInstallationPath = installationDataPath();
		if(!compatibleInstallationPath.isEmpty() && normalizedPath(selected).compare(normalizedPath(binaryPath), pathCaseSensitivity()) == 0)
		{
			selected = compatibleInstallationPath;
			QMessageBox::information(parent, tr("Using a data subdirectory"), tr("User data cannot be stored directly in the VCMI installation directory.\n\nThe following compatible directory will be used instead:\n%1").arg(QDir::toNativeSeparators(selected)));
			if(!QDir().mkpath(selected))
			{
				if(reportPermissionError(tr("The launcher could not create the data directory:\n%1\n\nSelect another location or adjust the directory permissions.").arg(QDir::toNativeSeparators(selected))))
					continue;
				return EChangeResult::DONE;
			}
		}

		bool selectAnother = false;
		if(validateTarget(dirs, directory, source, selected, selectAnother))
			break;

		if(!selectAnother)
			return EChangeResult::DONE;
	}

	const QString oldLogPath = pathToQString(dirs.userLogsPath());
	bool moveExistingData = false;
	bool downloadsPaused = false;
	bool cancelPausedDownloadsOnExit = false;
	QString displacedTargetPath;
	EExistingTargetAction completedTargetAction = EExistingTargetAction::MERGE;

	auto finishPausedDownloads = vstd::makeScopeGuard([&downloadsPaused, &cancelPausedDownloadsOnExit, mainWindow]()
	{
		if(downloadsPaused && mainWindow)
		{
			if(cancelPausedDownloadsOnExit)
				mainWindow->getModView()->cancelDownloads();
			else
				mainWindow->getModView()->resumeDownloads();
		}
	});

	auto pauseDownloads = [&downloadsPaused, mainWindow]()
	{
		if(!downloadsPaused && mainWindow)
			downloadsPaused = mainWindow->getModView()->pauseDownloads();
	};

	QDir sourceDir(source);
	if(sourceDir.exists() && !sourceDir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty())
	{
		const qint64 sourceSize = directorySize(source);
		const QStorageInfo targetStorage(selected);
		const qint64 availableSpace = targetStorage.bytesAvailable();
		const bool storageSpaceKnown = targetStorage.isValid() && targetStorage.isReady() && availableSpace >= 0;
		const QString availableSpaceText = storageSpaceKnown ? formattedDataSize(availableSpace) : tr("Unknown");
		const QString spaceDetails = tr("Space required: %1\nSpace available: %2").arg(formattedDataSize(sourceSize), availableSpaceText);

		QMessageBox copyDialog(QMessageBox::Question, tr("Copy existing data?"), tr("Do you want to copy the existing files?\n\nFrom:\n%1\n\nTo:\n%2\n\n%3\n").arg(QDir::toNativeSeparators(source), QDir::toNativeSeparators(selected), spaceDetails), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, parent);
		copyDialog.setDefaultButton(QMessageBox::Yes);

		if(storageSpaceKnown && availableSpace < sourceSize)
		{
			copyDialog.setIcon(QMessageBox::Warning);
			copyDialog.setInformativeText(tr("There is not enough free space to copy the existing data."));
			copyDialog.button(QMessageBox::Yes)->setEnabled(false);
		}

		QCheckBox moveCheckBox(tr("Move existing data (remove the original files after a successful reload)"));
		moveCheckBox.setChecked(true);
		copyDialog.setCheckBox(&moveCheckBox);

		const auto answer = static_cast<QMessageBox::StandardButton>(copyDialog.exec());

		if(answer == QMessageBox::Cancel)
			return EChangeResult::DONE;

		if(answer == QMessageBox::Yes)
		{
			const QString targetParent = QFileInfo(selected).dir().absolutePath();
			const bool sameDirectoryTree = pathsOverlap(source, selected);

			moveExistingData = moveCheckBox.isChecked();
			EExistingTargetAction targetAction = sameDirectoryTree ? EExistingTargetAction::MERGE : EExistingTargetAction::REPLACE;
			const bool targetIsEmpty = QDir(selected).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty();

			if(!targetIsEmpty)
			{
				const auto selectedAction = askExistingTargetAction(selected, sameDirectoryTree);
				if(!selectedAction)
					return EChangeResult::DONE;

				targetAction = *selectedAction;
			}

			const bool targetParentWritable = isDirectoryWritable(targetParent);
			if(!targetParentWritable && !targetIsEmpty && targetAction != EExistingTargetAction::MERGE)
			{
				if(reportPermissionError(tr("The launcher cannot replace or back up the selected directory because its parent directory is not writable:\n%1\n\nUse merge instead, select another location, or adjust the directory permissions.").arg(QDir::toNativeSeparators(targetParent))))
					return EChangeResult::SELECT_ANOTHER;

				return EChangeResult::DONE;
			}
			completedTargetAction = targetAction;

			const bool installInPlace = targetAction == EExistingTargetAction::MERGE || !targetParentWritable;
			const qint64 requiredSpace = installInPlace ? sourceSize * 2 : sourceSize;
			const QStorageInfo currentStorage(selected);

			if(currentStorage.isValid() && currentStorage.isReady() && currentStorage.bytesAvailable() >= 0 && currentStorage.bytesAvailable() < requiredSpace)
			{
				QMessageBox::critical(parent, tr("Not enough free space"), tr("The selected operation requires %1, but only %2 is available in the target location.").arg(formattedDataSize(requiredSpace), formattedDataSize(currentStorage.bytesAvailable())));
				return EChangeResult::DONE;
			}

			pauseDownloads();

			const bool targetInsideSource = isSameOrChildPath(selected, source);
			const QString stagingParent = installInPlace ? selected : targetParent;

			QTemporaryDir stagingDirectory(QDir(stagingParent).filePath(QStringLiteral(".vcmi-transfer-XXXXXX")));

			if(!stagingDirectory.isValid())
			{
				logGlobal->error("Failed to create transfer directory in '%s'", stagingParent.toStdString());
				QMessageBox::critical(parent, tr("Error"), tr("Failed to create a temporary transfer directory."));
				return EChangeResult::DONE;
			}

			auto progress = std::make_unique<ProgressOverlay>(parent->window(), 0);
			progress->setTitle(tr("Copying directory..."));
			progress->setIndeterminate(true);
			progress->show();
			progress->raise();

			qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

			QString error;
			const QString excludedSourcePath = targetInsideSource ? selected : QString();
			if(!copyDirectoryContents(source, stagingDirectory.path(), *progress, error, targetAction == EExistingTargetAction::MERGE, excludedSourcePath))
			{
				logGlobal->error("Failed to stage user directory transfer: %s", error.toStdString());
				progress.reset();
				QMessageBox::critical(parent, tr("Copy failed"), error);
				return EChangeResult::DONE;
			}

			if(installInPlace)
			{
				if(!copyDirectoryContents(stagingDirectory.path(), selected, *progress, error, true))
				{
					logGlobal->error("Failed to install staged user directory in place: %s", error.toStdString());
					QMessageBox::critical(parent, tr("Copy failed"), error);
					return EChangeResult::DONE;
				}
			}
			else if(!installStagedDirectory(stagingDirectory.path(), selected, targetAction, displacedTargetPath, error))
			{
				logGlobal->error("Failed to install staged user directory: %s", error.toStdString());
				QMessageBox::critical(parent, tr("Copy failed"), error);
				return EChangeResult::DONE;
			}

			progress.reset();

			if(!installInPlace)
				stagingDirectory.setAutoRemove(false);

		}
	}

	pauseDownloads();

	if(!dirs.setUserPath(directory, qstringToPath(selected)))
	{
		logGlobal->error("Failed to save new user directory '%s'", selected.toStdString());

		if(!restoreDisplacedDirectory(selected, displacedTargetPath))
		{
			logGlobal->error("Failed to restore target directory '%s' after settings error", selected.toStdString());
			cancelPausedDownloadsOnExit = true;
		}

		QMessageBox::critical(parent, tr("Error"), tr("Failed to save the directory setting."));
		return EChangeResult::DONE;
	}

	const QString newLogPath = pathToQString(dirs.userLogsPath());
	onDirectoriesChanged(oldLogPath == newLogPath ? QString() : newLogPath);

	if(!mainWindow || !mainWindow->reloadDirectories())
	{
		logGlobal->error("Failed to reload user directory '%s'", selected.toStdString());

		const bool settingRestored = dirs.setUserPath(directory, qstringToPath(source));
		if(!settingRestored)
			logGlobal->error("Failed to restore user directory setting '%s'", source.toStdString());

		const bool targetRestored = restoreDisplacedDirectory(selected, displacedTargetPath);
		if(!targetRestored)
			logGlobal->error("Failed to restore target directory '%s' after reload error", selected.toStdString());

		bool oldDirectoryReloaded = false;
		if(settingRestored)
		{
			const QString restoredLogPath = pathToQString(dirs.userLogsPath());
			onDirectoriesChanged(newLogPath == restoredLogPath ? QString() : restoredLogPath);
			oldDirectoryReloaded = mainWindow && mainWindow->reloadDirectories();
			if(!oldDirectoryReloaded)
				logGlobal->error("Failed to reload restored user directory '%s'", source.toStdString());
		}

		cancelPausedDownloadsOnExit = !settingRestored || !targetRestored || !oldDirectoryReloaded;
		const QString message = cancelPausedDownloadsOnExit
			? tr("The launcher could not reload data from the selected directory and could not fully restore the previous state. Active downloads were cancelled to prevent data corruption.")
			: tr("The launcher could not reload data from the selected directory. The previous directory has been restored.");
		QMessageBox::critical(parent, tr("Reload failed"), message);
		return EChangeResult::DONE;
	}

	if(downloadsPaused)
	{
		mainWindow->getModView()->resumeDownloads();
		downloadsPaused = false;
	}

	if(moveExistingData)
	{
		if(isSameOrChildPath(selected, source) || (!displacedTargetPath.isEmpty() && isSameOrChildPath(displacedTargetPath, source)))
		{
			QDir sourceDirectory(source);
			bool removalFailed = false;
			bool activeDirectoryKept = false;

			for(const auto & entry : sourceDirectory.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries))
			{
				const QString entryPath = entry.absoluteFilePath();
				if(isSameOrChildPath(selected, entryPath) || (!displacedTargetPath.isEmpty() && isSameOrChildPath(displacedTargetPath, entryPath)))
					continue;

				if(containsActiveUserDirectory(dirs, directory, entryPath))
				{
					activeDirectoryKept = true;
					continue;
				}

				if(!removePath(entryPath))
				{
					removalFailed = true;
					logGlobal->warn("Failed to remove old user data '%s'", entryPath.toStdString());
				}
			}
			if(activeDirectoryKept)
				QMessageBox::warning(parent, tr("Original files kept"), tr("Some original files contain another active VCMI directory and were kept."));

			if(removalFailed)
				QMessageBox::warning(parent, tr("Original files kept"), tr("The data was copied and reloaded, but some original files could not be removed."));
		}
		else if(containsActiveUserDirectory(dirs, directory, source))
			QMessageBox::warning(parent, tr("Original files kept"), tr("The original directory still contains another active VCMI directory and cannot be removed safely."));
		else if(QFileInfo::exists(source) && !removePath(source))
			QMessageBox::warning(parent, tr("Original files kept"), tr("The data was copied and reloaded, but the original directory could not be removed."));
		else
			dirs.removeObsoleteUserDataParent(qstringToPath(source));
	}

	if(completedTargetAction == EExistingTargetAction::REPLACE && !displacedTargetPath.isEmpty() && !removePath(displacedTargetPath))
	{
		logGlobal->warn("Failed to purge replaced user directory '%s'", displacedTargetPath.toStdString());
		QMessageBox::warning(parent, tr("Cleanup failed"), tr("The new data was installed, but the replaced directory could not be removed: %1").arg(displacedTargetPath));
	}

	const QString message = completedTargetAction == EExistingTargetAction::BACK_UP
		? tr("The launcher has reloaded files from the new directory.\n\nThe previous target was saved to:\n%1").arg(QDir::toNativeSeparators(displacedTargetPath))
		: tr("The launcher has reloaded files from the new directory.");
	QMessageBox::information(parent, tr("Directory changed"), message);
	logGlobal->info("User directory change to '%s' completed successfully", selected.toStdString());
	return EChangeResult::DONE;
}

void datamanager::changeDirectory(EUserDirectory directory, const QString & title) const
{
	while(true)
	{
		if(changeDirectoryOnce(directory, title) == EChangeResult::DONE)
			return;
	}
}
