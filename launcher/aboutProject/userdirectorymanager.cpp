/*
 * userdirectorymanager.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "userdirectorymanager.h"
#include "aboutproject_moc.h"

#if defined(VCMI_WINDOWS)

#include <QCheckBox>
#include <QStorageInfo>
#include <QTemporaryFile>
#include <QUuid>

#include <shellapi.h>

#include "../progressoverlay.h"
#include "../helper.h"
#include "../mainwindow_moc.h"
#include "../modManager/cmodlistview_moc.h"

#include "../../lib/ScopeGuard.h"
#include "../../lib/VCMIDirs.h"

WindowsUserDirectoryManager::WindowsUserDirectoryManager(AboutProjectView * parent)
	: QObject(parent)
	, parent(parent)
{
}

QString WindowsUserDirectoryManager::normalizedPath(const QString & path) const
{
	const QFileInfo info(path);
	const QString canonicalPath = info.canonicalFilePath();
	return QDir::cleanPath(canonicalPath.isEmpty() ? info.absoluteFilePath() : canonicalPath);
}

bool WindowsUserDirectoryManager::pathsOverlap(const QString & first, const QString & second) const
{
	return isSameOrChildPath(first, second) || isSameOrChildPath(second, first);
}

bool WindowsUserDirectoryManager::isDirectoryWritable(const QString & path) const
{
	if(!QFileInfo(path).isDir())
		return false;

	QTemporaryFile probe(QDir(path).filePath(QStringLiteral(".vcmi-write-test-XXXXXX")));
	return probe.open();
}

void WindowsUserDirectoryManager::reportPermissionError(const QString & message) const
{
	QMessageBox dialog(QMessageBox::Critical, tr("Insufficient permissions"), message, QMessageBox::Cancel, parent);
	auto * restartButton = dialog.addButton(tr("Restart as administrator"), QMessageBox::AcceptRole);
	dialog.setDefaultButton(restartButton);
	dialog.exec();

	if(dialog.clickedButton() != restartButton)
		return;

	const std::wstring executable = QCoreApplication::applicationFilePath().toStdWString();
	SHELLEXECUTEINFOW executeInfo{};
	executeInfo.cbSize = sizeof(executeInfo);
	executeInfo.lpVerb = L"runas";
	executeInfo.lpFile = executable.c_str();
	executeInfo.nShow = SW_SHOWNORMAL;
	if(ShellExecuteExW(&executeInfo) != FALSE)
	{
		qApp->quit();
		return;
	}

	QMessageBox::critical(parent, tr("Error"), tr("Failed to restart the launcher with administrator privileges."));
}

bool WindowsUserDirectoryManager::validateTarget(const IVCMIDirs & dirs, EUserDirectory changedDirectory, const QString & source, const QString & target) const
{
	if(isSameOrChildPath(target, source) && isSameOrChildPath(source, target))
	{
		logGlobal->info("User directory change skipped because source and target are the same: %s", source.toStdString());
		QMessageBox::information(parent, tr("Directory unchanged"), tr("The selected directory is the same as the current directory. Please select a different location."));
		return false;
	}

	const QString binaryPath = QCoreApplication::applicationDirPath();
	const QString portableDataPath = QDir(binaryPath).filePath(QStringLiteral("vcmi-data"));
	if(pathsOverlap(target, binaryPath) && normalizedPath(target).compare(normalizedPath(portableDataPath), Qt::CaseInsensitive) != 0)
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
		reportPermissionError(tr("The selected directory is not writable:\n%1\n\nSelect another location or restart the launcher as administrator.").arg(QDir::toNativeSeparators(target)));
		return false;
	}

	return true;
}

qint64 WindowsUserDirectoryManager::directorySize(const QString & path) const
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

QString WindowsUserDirectoryManager::formattedDataSize(qint64 bytes) const
{
	return QLocale().formattedDataSize(bytes, 1, QLocale::DataSizeTraditionalFormat);
}

bool WindowsUserDirectoryManager::isSameOrChildPath(const QString & path, const QString & parentPath) const
{
	const QString cleanPath = normalizedPath(path);
	QString cleanParent = normalizedPath(parentPath);

	if(cleanPath.compare(cleanParent, Qt::CaseInsensitive) == 0)
		return true;

	if(!cleanParent.endsWith(QDir::separator()))
		cleanParent += QDir::separator();

	return cleanPath.startsWith(cleanParent, Qt::CaseInsensitive);
}

bool WindowsUserDirectoryManager::containsActiveUserDirectory(const IVCMIDirs & dirs, EUserDirectory changedDirectory, const QString & path) const
{
	static constexpr std::array userDirectories = {
		EUserDirectory::DATA, EUserDirectory::CACHE, EUserDirectory::CONFIG, EUserDirectory::LOGS, EUserDirectory::SAVES
	};

	for(const auto directory : userDirectories)
		if(directory != changedDirectory && isSameOrChildPath(pathToQString(dirs.userPath(directory)), path))
			return true;
	return false;
}

bool WindowsUserDirectoryManager::copyDirectoryContents(const QString & source, const QString & destination, ProgressOverlay & progress, QString & error, bool overwrite, const QString & excludedPath) const
{
	QDir sourceDir(source);
	int totalFiles = 0;

	QDirIterator counter(source, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
	while(counter.hasNext())
	{
		const QString sourcePath = counter.next();

		if(!excludedPath.isEmpty() && isSameOrChildPath(sourcePath, excludedPath))
			continue;

		counter.next();
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

		if(sourceInfo.isDir())
		{
			if(overwrite && QFileInfo(destinationPath).isFile() && !QFile::remove(destinationPath))
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
			const QFileInfo destinationInfo(destinationPath);
			const bool removed = destinationInfo.isDir() ? QDir(destinationPath).removeRecursively() : QFile::remove(destinationPath);
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

std::optional<WindowsUserDirectoryManager::EExistingTargetAction> WindowsUserDirectoryManager::askExistingTargetAction(const QString & target) const
{
	QMessageBox dialog(QMessageBox::Question, tr("Directory is not empty"), tr("The target directory already contains files:\n%1\n\nHow should they be handled?").arg(QDir::toNativeSeparators(target)), QMessageBox::NoButton, parent);
	dialog.setInformativeText(tr("Merge keeps files that exist only in the target and overwrites conflicts.\nBack up and replace moves the current target to a _backup directory.\nClean replacement removes the current target after the new copy is ready."));

	const auto * const mergeButton = dialog.addButton(tr("Merge and overwrite"), QMessageBox::AcceptRole);
	auto * const backupButton = dialog.addButton(tr("Back up and replace"), QMessageBox::ActionRole);
	const auto * const replaceButton = dialog.addButton(tr("Clean replacement"), QMessageBox::DestructiveRole);

	dialog.addButton(QMessageBox::Cancel);
	dialog.setDefaultButton(backupButton);

	dialog.exec();

	if(dialog.clickedButton() == mergeButton)
		return EExistingTargetAction::MERGE;

	if(dialog.clickedButton() == backupButton)
		return EExistingTargetAction::BACK_UP;

	if(dialog.clickedButton() == replaceButton)
	{
		const auto answer = QMessageBox::warning(parent, tr("Confirm clean replacement"), tr("Clean replacement will permanently remove all files currently in the target directory after the new data has been copied successfully.\n\nDo you really want to continue?"), QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
		if(answer != QMessageBox::Yes)
			return std::nullopt;
		return EExistingTargetAction::REPLACE;
	}

	return std::nullopt;
}

QString WindowsUserDirectoryManager::availableBackupPath(const QString & target) const
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

bool WindowsUserDirectoryManager::installStagedDirectory(const QString & staging, const QString & target, EExistingTargetAction action, QString & displacedPath, QString & error) const
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

bool WindowsUserDirectoryManager::restoreDisplacedDirectory(const QString & target, const QString & displacedPath) const
{
	if(displacedPath.isEmpty())
		return true;

	const QString failedPath = QFileInfo(target).dir().filePath(QStringLiteral(".%1-vcmi-failed-%2").arg(QFileInfo(target).fileName(), QUuid::createUuid().toString(QUuid::Id128)));
	return QDir().rename(target, failedPath) && QDir().rename(displacedPath, target) && QDir(failedPath).removeRecursively();
}

void WindowsUserDirectoryManager::changeDirectory(EUserDirectory directory, const QString & title) const
{
	auto * mainWindow = qobject_cast<MainWindow *>(parent->window());
	auto & dirs = VCMIDirs::get();

	const QString source = pathToQString(dirs.userPath(directory));
	QString selected = QFileDialog::getExistingDirectory(parent, title, source);

	if(selected.isEmpty())
		return;

	logGlobal->info("Changing user directory from '%s' to '%s'", source.toStdString(), selected.toStdString());

	const QString binaryPath = QCoreApplication::applicationDirPath();
	if(normalizedPath(selected).compare(normalizedPath(binaryPath), Qt::CaseInsensitive) == 0)
	{
		selected = QDir(binaryPath).filePath(QStringLiteral("vcmi-data"));
		QMessageBox::information(parent, tr("Using a data subdirectory"), tr("User data cannot be stored directly in the VCMI installation directory.\n\nThe following compatible directory will be used instead:\n%1").arg(QDir::toNativeSeparators(selected)));
		if(!QDir().mkpath(selected))
		{
			reportPermissionError(tr("The launcher could not create the data directory:\n%1\n\nSelect another location or restart the launcher as administrator.").arg(QDir::toNativeSeparators(selected)));
			return;
		}
	}

	if(!validateTarget(dirs, directory, source, selected))
		return;

	const QString oldLogPath = pathToQString(dirs.userLogsPath());
	bool moveExistingData = false;
	bool downloadsPaused = false;
	QString targetBackupPath;
	QString displacedTargetPath;
	EExistingTargetAction completedTargetAction = EExistingTargetAction::MERGE;

	auto cancelPausedDownloads = vstd::makeScopeGuard([&downloadsPaused, mainWindow]()
	{
		if(downloadsPaused && mainWindow)
			mainWindow->getModView()->cancelDownloads();
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
		const bool sourceCanBeRemoved = isDirectoryWritable(source) && isDirectoryWritable(QFileInfo(source).dir().absolutePath());
		const QStorageInfo targetStorage(selected);
		const qint64 availableSpace = targetStorage.bytesAvailable();
		const bool storageSpaceKnown = targetStorage.isValid() && targetStorage.isReady() && availableSpace >= 0;
		const QString availableSpaceText = storageSpaceKnown ? formattedDataSize(availableSpace) : tr("Unknown");
		const QString spaceDetails = tr("Space required: %1\nSpace available: %2").arg(formattedDataSize(sourceSize), availableSpaceText);

		QMessageBox copyDialog(QMessageBox::Question, tr("Copy existing data?"), tr("Do you want to copy the existing files?\n\nFrom:\n%1\n\nTo:\n%2\n\n%3").arg(QDir::toNativeSeparators(source), QDir::toNativeSeparators(selected), spaceDetails), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, parent);
		copyDialog.setDefaultButton(QMessageBox::Yes);

		if(storageSpaceKnown && availableSpace < sourceSize)
		{
			copyDialog.setIcon(QMessageBox::Warning);
			copyDialog.setInformativeText(tr("There is not enough free space to copy the existing data."));
			copyDialog.button(QMessageBox::Yes)->setEnabled(false);
		}

		QCheckBox moveCheckBox(tr("Move existing data (remove the original files after a successful reload)"));
		moveCheckBox.setChecked(sourceCanBeRemoved);
		moveCheckBox.setEnabled(sourceCanBeRemoved);
		if(!sourceCanBeRemoved)
		{
			moveCheckBox.setToolTip(tr("The original directory cannot be removed with the current permissions. Data can only be copied."));
			const QString permissionMessage = tr("The original directory is not writable, so its files can only be copied and will not be removed.");
			copyDialog.setInformativeText(copyDialog.informativeText().isEmpty() ? permissionMessage : copyDialog.informativeText() + QStringLiteral("\n\n") + permissionMessage);
		}
		copyDialog.setCheckBox(&moveCheckBox);

		const auto answer = static_cast<QMessageBox::StandardButton>(copyDialog.exec());

		if(answer == QMessageBox::Cancel)
			return;

		if(answer == QMessageBox::Yes)
		{
			const QString targetParent = QFileInfo(selected).dir().absolutePath();

			moveExistingData = moveCheckBox.isChecked();
			EExistingTargetAction targetAction = EExistingTargetAction::REPLACE;
			const bool targetIsEmpty = QDir(selected).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty();

			if(!targetIsEmpty)
			{
				const auto selectedAction = askExistingTargetAction(selected);
				if(!selectedAction)
					return;
				targetAction = *selectedAction;
			}

			const bool targetParentWritable = isDirectoryWritable(targetParent);
			if(!targetParentWritable && !targetIsEmpty && targetAction != EExistingTargetAction::MERGE)
			{
				reportPermissionError(tr("The launcher cannot replace or back up the selected directory because its parent directory is not writable:\n%1\n\nUse merge instead, select another location, or restart the launcher as administrator.").arg(QDir::toNativeSeparators(targetParent)));
				return;
			}
			completedTargetAction = targetAction;

			const bool installInPlace = !targetParentWritable;
			const qint64 requiredSpace = sourceSize + (!installInPlace && targetAction == EExistingTargetAction::MERGE ? directorySize(selected) : 0);
			const QStorageInfo currentStorage(selected);

			if(currentStorage.isValid() && currentStorage.isReady() && currentStorage.bytesAvailable() >= 0 && currentStorage.bytesAvailable() < requiredSpace)
			{
				QMessageBox::critical(parent, tr("Not enough free space"), tr("The selected operation requires %1, but only %2 is available in the target location.").arg(formattedDataSize(requiredSpace), formattedDataSize(currentStorage.bytesAvailable())));
				return;
			}

			pauseDownloads();

			const bool targetInsideSource = isSameOrChildPath(selected, source);
			const QString stagingParent = installInPlace ? selected : (targetInsideSource ? QFileInfo(source).dir().absolutePath() : targetParent);

			QTemporaryDir stagingDirectory(QDir(stagingParent).filePath(QStringLiteral(".vcmi-transfer-XXXXXX")));

			if(!stagingDirectory.isValid())
			{
				logGlobal->error("Failed to create transfer directory in '%s'", stagingParent.toStdString());
				QMessageBox::critical(parent, tr("Error"), tr("Failed to create a temporary transfer directory."));
				return;
			}

			auto progress = std::make_unique<ProgressOverlay>(parent->window(), 0);
			progress->setTitle(tr("Copying directory..."));
			progress->setIndeterminate(true);
			progress->show();
			progress->raise();

			qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

			QString error;
			const QString excludedSourcePath = targetInsideSource ? selected : QString();
			if((!installInPlace && targetAction == EExistingTargetAction::MERGE && !copyDirectoryContents(selected, stagingDirectory.path(), *progress, error)) || !copyDirectoryContents(source, stagingDirectory.path(), *progress, error, targetAction == EExistingTargetAction::MERGE, excludedSourcePath))
			{
				logGlobal->error("Failed to stage user directory transfer: %s", error.toStdString());{
				progress.reset();
				QMessageBox::critical(parent, tr("Copy failed"), error);
				return;
			}

			if(installInPlace)
			{
				if(!copyDirectoryContents(stagingDirectory.path(), selected, *progress, error, true))
				{
					logGlobal->error("Failed to install staged user directory in place: %s", error.toStdString());
					QMessageBox::critical(parent, tr("Copy failed"), error);
					return;
				}
			}
			else if(!installStagedDirectory(stagingDirectory.path(), selected, targetAction, displacedTargetPath, error))
			{
				logGlobal->error("Failed to install staged user directory: %s", error.toStdString());
				QMessageBox::critical(parent, tr("Copy failed"), error);
				return;
			}

			progress.reset();

			if(!installInPlace)
				stagingDirectory.setAutoRemove(false);

			if(targetAction == EExistingTargetAction::BACK_UP)
				targetBackupPath = displacedTargetPath;
		}
	}

	pauseDownloads();

	if(!dirs.setUserPath(directory, qstringToPath(selected)))
	{
		logGlobal->error("Failed to save new user directory '%s'", selected.toStdString());

		if(!restoreDisplacedDirectory(selected, displacedTargetPath))
			logGlobal->error("Failed to restore target directory '%s' after settings error", selected.toStdString());

		QMessageBox::critical(parent, tr("Error"), tr("Failed to save directory settings to config/dirs.json or the current user's registry."));
		return;
	}

	const QString newLogPath = pathToQString(dirs.userLogsPath());
	parent->directoriesChanged(oldLogPath == newLogPath ? QString() : newLogPath);

	if(!mainWindow || !mainWindow->reloadDirectories())
	{
		logGlobal->error("Failed to reload user directory '%s'", selected.toStdString());

		if(!dirs.setUserPath(directory, qstringToPath(source)))
			logGlobal->error("Failed to restore user directory setting '%s'", source.toStdString());

		if(!restoreDisplacedDirectory(selected, displacedTargetPath))
			logGlobal->error("Failed to restore target directory '%s' after reload error", selected.toStdString());

		return;
	}

	if(completedTargetAction == EExistingTargetAction::REPLACE && !displacedTargetPath.isEmpty() && !QDir(displacedTargetPath).removeRecursively())
	{
		logGlobal->warn("Failed to purge replaced user directory '%s'", displacedTargetPath.toStdString());
		QMessageBox::warning(parent, tr("Cleanup failed"), tr("The new data was installed, but the replaced directory could not be removed: %1").arg(displacedTargetPath));
	}

	if(downloadsPaused)
	{
		mainWindow->getModView()->resumeDownloads();
		downloadsPaused = false;
	}

	if(moveExistingData)
	{
		if(containsActiveUserDirectory(dirs, directory, source))
			QMessageBox::warning(parent, tr("Original files kept"), tr("The original directory still contains another active VCMI directory and cannot be removed safely."));
		else if(isSameOrChildPath(selected, source))
		{
			QDir sourceDirectory(source);
			const QString childToKeep = sourceDirectory.relativeFilePath(selected).section('/', 0, 0);
			for(const auto & entry : sourceDirectory.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries))
			{
				if(entry.fileName() == childToKeep)
					continue;
				const bool removed = entry.isDir() ? QDir(entry.absoluteFilePath()).removeRecursively() : QFile::remove(entry.absoluteFilePath());
				if(!removed)
					logGlobal->warn("Failed to remove old user data '%s'", entry.absoluteFilePath().toStdString());
			}
		}
		else if(QFileInfo::exists(source) && !QDir(source).removeRecursively())
			QMessageBox::warning(parent, tr("Original files kept"), tr("The data was copied and reloaded, but the original directory could not be removed."));
		else
		{
			const QFileInfo sourceParent(QFileInfo(source).dir().absolutePath());
			if(sourceParent.fileName().compare(QStringLiteral("My Games"), Qt::CaseInsensitive) == 0)
				QDir().rmdir(sourceParent.absoluteFilePath());
		}
	}

	const QString message = targetBackupPath.isEmpty() ? tr("The launcher has reloaded files from the new directory.") : tr("The launcher has reloaded files from the new directory.\n\nThe previous target was saved to:\n%1").arg(QDir::toNativeSeparators(targetBackupPath));
	QMessageBox::information(parent, tr("Directory changed"), message);
	logGlobal->info("User directory change to '%s' completed successfully", selected.toStdString());
}

#endif
