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

#if defined(VCMI_WINDOWS)

#include <QCheckBox>
#include <QStorageInfo>
#include <QTemporaryFile>
#include <QUuid>

#include <shellapi.h>

#include "../firstLaunch/progressoverlay.h"
#include "../helper.h"
#include "../mainwindow_moc.h"
#include "../modManager/cmodlistview_moc.h"

#include "../../lib/ScopeGuard.h"
#include "../../lib/VCMIDirs.h"

WindowsUserDirectoryManager::WindowsUserDirectoryManager(QWidget * parent)
	: parent(parent)
{
}

QString WindowsUserDirectoryManager::tr(const char * text) const
{
	return QCoreApplication::translate("AboutProjectView", text);
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

	const QString executable = QCoreApplication::applicationFilePath();
	const auto result = reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"runas", reinterpret_cast<LPCWSTR>(executable.utf16()), nullptr, nullptr, SW_SHOWNORMAL));
	if(result > 32)
	{
		qApp->quit();
		return;
	}

	QMessageBox::critical(parent, tr("Error"), tr("Failed to restart the launcher with administrator privileges."));
}

bool WindowsUserDirectoryManager::validateTarget(const IVCMIDirs & dirs, EUserDirectory changedDirectory, const QString & source, const QString & target) const
{
	if(pathsOverlap(target, source))
	{
		const bool samePath = isSameOrChildPath(target, source) && isSameOrChildPath(source, target);
		const QString message = samePath ? tr("The selected directory is the same as the current directory. Please select a different location.") : tr("The current and selected directories cannot contain each other.");
		QMessageBox::warning(parent, samePath ? tr("Directory unchanged") : tr("Invalid directory"), message);
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

bool WindowsUserDirectoryManager::copyDirectoryContents(const QString & source, const QString & destination, ProgressOverlay & progress, QString & error, bool overwrite) const
{
	QDir sourceDir(source);
	int totalFiles = 0;

	QDirIterator counter(source, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
	while(counter.hasNext())
	{
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

	auto * mergeButton = dialog.addButton(tr("Merge and overwrite"), QMessageBox::AcceptRole);
	auto * backupButton = dialog.addButton(tr("Back up and replace"), QMessageBox::ActionRole);
	auto * replaceButton = dialog.addButton(tr("Clean replacement"), QMessageBox::DestructiveRole);

	dialog.addButton(QMessageBox::Cancel);
	dialog.setDefaultButton(backupButton);

	dialog.exec();

	if(dialog.clickedButton() == mergeButton)
		return EExistingTargetAction::MERGE;

	if(dialog.clickedButton() == backupButton)
		return EExistingTargetAction::BACK_UP;

	if(dialog.clickedButton() == replaceButton)
		return EExistingTargetAction::REPLACE;

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

bool WindowsUserDirectoryManager::installStagedDirectory(const QString & staging, const QString & target, EExistingTargetAction action, QString & backupPath, QString & error) const
{
	const QFileInfo targetInfo(target);
	const QString temporaryPath = targetInfo.dir().filePath(QStringLiteral(".%1-vcmi-old-%2").arg(targetInfo.fileName(), QUuid::createUuid().toString(QUuid::Id128)));
	const QString displacedPath = action == EExistingTargetAction::BACK_UP ? availableBackupPath(target) : temporaryPath;

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

	if(action == EExistingTargetAction::BACK_UP)
		backupPath = displacedPath;
	else if(!QDir(displacedPath).removeRecursively())
		error = tr("The new data was installed, but the replaced directory could not be removed: %1").arg(displacedPath);
	return true;
}

void WindowsUserDirectoryManager::changeDirectory(EUserDirectory directory, const QString & title, const std::function<void(const QString &)> & onDirectoriesChanged) const
{
	auto * mainWindow = qobject_cast<MainWindow *>(parent->window());
	auto & dirs = VCMIDirs::get();

	const QString source = pathToQString(dirs.userPath(directory));
	QString selected = QFileDialog::getExistingDirectory(parent, title, source);

	if(selected.isEmpty())
		return;

	const QString binaryPath = QCoreApplication::applicationDirPath();
	if(normalizedPath(selected).compare(normalizedPath(binaryPath), Qt::CaseInsensitive) == 0)
	{
		selected = QDir(binaryPath).filePath(QStringLiteral("vcmi-data"));
		QMessageBox::information(parent, tr("Using a data subdirectory"), tr("User data cannot be stored directly in the VCMI installation directory. The following compatible directory will be used instead:\n%1").arg(QDir::toNativeSeparators(selected)));
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

	auto cancelPausedDownloads = vstd::makeScopeGuard([&]()
	{
		if(downloadsPaused && mainWindow)
			mainWindow->getModView()->cancelDownloads();
	});

	auto pauseDownloads = [&]()
	{
		if(!downloadsPaused && mainWindow)
			downloadsPaused = mainWindow->getModView()->pauseDownloads();
	};

	QDir sourceDir(source);
	if(sourceDir.exists() && !sourceDir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty())
	{
		const qint64 sourceSize = directorySize(source);
		const bool sourceCanBeRemoved = QFileInfo(source).isWritable() && isDirectoryWritable(source) && isDirectoryWritable(QFileInfo(source).dir().absolutePath());
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

		auto * moveCheckBox = new QCheckBox(tr("Move existing data (remove the original files after a successful reload)"));
		moveCheckBox->setChecked(sourceCanBeRemoved);
		moveCheckBox->setEnabled(sourceCanBeRemoved);
		if(!sourceCanBeRemoved)
		{
			moveCheckBox->setToolTip(tr("The original directory cannot be removed with the current permissions. Data can only be copied."));
			const QString permissionMessage = tr("The original directory is not writable, so its files can only be copied and will not be removed.");
			copyDialog.setInformativeText(copyDialog.informativeText().isEmpty() ? permissionMessage : copyDialog.informativeText() + QStringLiteral("\n\n") + permissionMessage);
		}
		copyDialog.setCheckBox(moveCheckBox);

		const auto answer = static_cast<QMessageBox::StandardButton>(copyDialog.exec());

		if(answer == QMessageBox::Cancel)
			return;

		if(answer == QMessageBox::Yes)
		{
			const QString targetParent = QFileInfo(selected).dir().absolutePath();
			if(!isDirectoryWritable(targetParent))
			{
				reportPermissionError(tr("The launcher cannot prepare or replace the selected directory because its parent directory is not writable:\n%1\n\nSelect another location or restart the launcher as administrator.").arg(QDir::toNativeSeparators(targetParent)));
				return;
			}

			moveExistingData = moveCheckBox->isChecked();
			EExistingTargetAction targetAction = EExistingTargetAction::REPLACE;

			if(!QDir(selected).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty())
			{
				const auto selectedAction = askExistingTargetAction(selected);
				if(!selectedAction)
					return;
				targetAction = *selectedAction;
			}

			const qint64 requiredSpace = sourceSize + (targetAction == EExistingTargetAction::MERGE ? directorySize(selected) : 0);
			const QStorageInfo currentStorage(selected);

			if(currentStorage.isValid() && currentStorage.isReady() && currentStorage.bytesAvailable() >= 0 && currentStorage.bytesAvailable() < requiredSpace)
			{
				QMessageBox::critical(parent, tr("Not enough free space"), tr("The selected operation requires %1, but only %2 is available in the target location.").arg(formattedDataSize(requiredSpace), formattedDataSize(currentStorage.bytesAvailable())));
				return;
			}

			pauseDownloads();

			QTemporaryDir stagingDirectory(QFileInfo(selected).dir().filePath(QStringLiteral(".vcmi-transfer-XXXXXX")));

			if(!stagingDirectory.isValid())
			{
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
			if((targetAction == EExistingTargetAction::MERGE && !copyDirectoryContents(selected, stagingDirectory.path(), *progress, error)) || !copyDirectoryContents(source, stagingDirectory.path(), *progress, error, targetAction == EExistingTargetAction::MERGE))
			{
				progress.reset();
				QMessageBox::critical(parent, tr("Copy failed"), error);
				return;
			}

			progress.reset();

			if(!installStagedDirectory(stagingDirectory.path(), selected, targetAction, targetBackupPath, error))
			{
				QMessageBox::critical(parent, tr("Copy failed"), error);
				return;
			}

			stagingDirectory.setAutoRemove(false);
			if(!error.isEmpty())
				QMessageBox::warning(parent, tr("Cleanup failed"), error);
		}
	}

	pauseDownloads();

	if(!dirs.setUserPath(directory, qstringToPath(selected)))
	{
		QMessageBox::critical(parent, tr("Error"), tr("Failed to save directory settings to config/dirs.json or the current user's registry."));
		return;
	}

	const QString newLogPath = pathToQString(dirs.userLogsPath());

	onDirectoriesChanged(oldLogPath == newLogPath ? QString() : newLogPath);

	if(!mainWindow || !mainWindow->reloadDirectories())
		return;

	if(downloadsPaused)
	{
		mainWindow->getModView()->resumeDownloads();
		downloadsPaused = false;
	}

	if(moveExistingData)
	{
		if(containsActiveUserDirectory(dirs, directory, source))
			QMessageBox::warning(parent, tr("Original files kept"), tr("The original directory still contains another active VCMI directory and cannot be removed safely."));
		else if(!QDir(source).removeRecursively())
			QMessageBox::warning(parent, tr("Original files kept"), tr("The data was copied and reloaded, but the original directory could not be removed."));
	}

	const QString message = targetBackupPath.isEmpty() ? tr("The launcher has reloaded files from the new directory.") : tr("The launcher has reloaded files from the new directory.\n\nThe previous target was saved to:\n%1").arg(QDir::toNativeSeparators(targetBackupPath));
	QMessageBox::information(parent, tr("Directory changed"), message);
	return;
}

#endif
