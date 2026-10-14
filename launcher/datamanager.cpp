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
#include <QStandardPaths>
#include <QStorageInfo>
#include <QTemporaryFile>
#include <QUuid>

#if defined(VCMI_WINDOWS)
#include <shellapi.h>

#include "../lib/platform/windows/CloudStorageDetection.h"
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

bool datamanager::isProtectedTargetRoot(const QString & path) const
{
	const QString normalizedTarget = normalizedPath(path);
	if(QFileInfo(normalizedTarget).isRoot())
		return true;

	QStringList protectedPaths = {
		QDir::homePath(),
		QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
		QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
		QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
		QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
		QStandardPaths::writableLocation(QStandardPaths::MoviesLocation),
		QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
	};

	for(const auto & protectedPath : protectedPaths)
	{
		if(!protectedPath.isEmpty() && normalizedTarget.compare(normalizedPath(protectedPath), pathCaseSensitivity()) == 0)
			return true;
	}

	return false;
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

datamanager::ETargetConfirmation datamanager::confirmSynchronizedTarget(EUserDirectory directory, const QString & target) const
{
#if defined(VCMI_WINDOWS)
	const std::wstring nativeTarget = QDir::toNativeSeparators(target).toStdWString();
	const bool isOneDrive = VCMI::Windows::isOneDrivePath(nativeTarget.c_str());
	const bool isCloudStorage = isOneDrive || VCMI::Windows::isCloudStoragePath(nativeTarget.c_str());
	logGlobal->debug("User directory synchronization check for '%s': OneDrive=%d, cloud=%d", target.toStdString(), isOneDrive, isCloudStorage);
	if(!isCloudStorage)
		return ETargetConfirmation::ACCEPT;

	const QString synchronizationDescription = isOneDrive
		? tr("The selected directory is synchronized by OneDrive:")
		: tr("The selected directory is probably managed by a cloud synchronization service:");

	if(directory == EUserDirectory::SAVES)
	{
		QMessageBox information(QMessageBox::Information, tr("Synchronized save directory selected"), tr("%1\n%2\n\nSynchronizing saved games can be useful across multiple devices. Avoid running VCMI on multiple devices at the same time because synchronization conflicts may duplicate or overwrite saves.").arg(synchronizationDescription, QDir::toNativeSeparators(target)), QMessageBox::NoButton, parent);
		auto * continueButton = information.addButton(tr("Use synchronized directory"), QMessageBox::AcceptRole);
		auto * selectButton = information.addButton(tr("Select another location"), QMessageBox::ActionRole);
		information.addButton(QMessageBox::Cancel);
		information.setDefaultButton(continueButton);
		information.exec();
		if(information.clickedButton() == continueButton)
			return ETargetConfirmation::ACCEPT;
		if(information.clickedButton() == selectButton)
			return ETargetConfirmation::SELECT_ANOTHER;
		return ETargetConfirmation::CANCEL;
	}

	QMessageBox warning(QMessageBox::Warning, tr("Synchronized directory selected"), tr("%1\n%2\n\nThe synchronization service may lock files while processing them. This can prevent VCMI from writing data and may cause mod installation, game startup, saving, or other operations to fail.").arg(synchronizationDescription, QDir::toNativeSeparators(target)), QMessageBox::NoButton, parent);
	auto * continueButton = warning.addButton(tr("Continue anyway"), QMessageBox::DestructiveRole);
	auto * selectButton = warning.addButton(tr("Select another location"), QMessageBox::ActionRole);
	warning.addButton(QMessageBox::Cancel);
	warning.setDefaultButton(selectButton);
	warning.exec();

	if(warning.clickedButton() == selectButton)
		return ETargetConfirmation::SELECT_ANOTHER;
	if(warning.clickedButton() != continueButton)
		return ETargetConfirmation::CANCEL;

	const auto confirmation = QMessageBox::warning(parent, tr("Confirm synchronized directory"), tr("Using a synchronized directory can make VCMI unreliable and may cause data-writing operations to fail.\n\nDo you really want to use this directory?"), QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
	return confirmation == QMessageBox::Yes ? ETargetConfirmation::ACCEPT : ETargetConfirmation::CANCEL;
#else
	Q_UNUSED(directory);
	Q_UNUSED(target);
	return ETargetConfirmation::ACCEPT;
#endif
}

bool datamanager::validateTarget(const IVCMIDirs & dirs, EUserDirectory changedDirectory, const QString & source, const QString & target, bool & selectAnother) const
{
	if(isSameOrChildPath(target, source) && isSameOrChildPath(source, target))
	{
		logGlobal->info("User directory change skipped because source and target are the same: %s", source.toStdString());
		QMessageBox::information(parent, tr("Directory unchanged"), tr("The selected directory is the same as the current directory. Please select a different location."));
		return false;
	}

	if(isProtectedTargetRoot(target))
	{
		const QString suggestedPath = QDir(target).filePath(QStringLiteral("VCMI"));
		QMessageBox::warning(parent, tr("Invalid directory"), tr("A drive root or personal folder such as Documents, Desktop, or Downloads cannot be used directly because replacing or cleaning it could affect unrelated files.\n\nCreate or select a dedicated VCMI subdirectory, for example:\n%1").arg(QDir::toNativeSeparators(suggestedPath)));
		selectAnother = true;
		return false;
	}

	const QString binaryPath = QCoreApplication::applicationDirPath();
	const QString compatibleInstallationPath = installationDataPath();
	const bool usesCompatibleInstallationPath = !compatibleInstallationPath.isEmpty() && normalizedPath(target).compare(normalizedPath(compatibleInstallationPath), pathCaseSensitivity()) == 0;

	if(isSameOrChildPath(target, binaryPath) && !usesCompatibleInstallationPath)
	{
		QMessageBox::warning(parent, tr("Invalid directory"), tr("A user directory cannot be the VCMI installation directory or be located inside it."));
		return false;
	}

	if(changedDirectory == EUserDirectory::DATA && dirs.isReservedUserDataSubdirectory(qstringToPath(target)))
	{
		QMessageBox::warning(parent, tr("Invalid directory"), tr("The selected directory is reserved for data inside the VCMI user directory. Select its parent or another directory instead."));
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
			const bool activeDirectoryMovesWithSource = pathsOverlap(source, target) && isSameOrChildPath(activePath, source);
			if(activeDirectoryMovesWithSource)
				continue;

			QMessageBox::warning(parent, tr("Invalid directory"), tr("The selected directory conflicts with another active VCMI user directory:\n%1\n\nUser directories cannot be the same or contain each other.").arg(QDir::toNativeSeparators(activePath)));
			return false;
		}
	}

	if(!isDirectoryWritable(target))
	{
		selectAnother = reportPermissionError(tr("The selected directory is not writable:\n%1\n\nSelect another location or adjust the directory permissions.").arg(QDir::toNativeSeparators(target)));
		return false;
	}

	const auto synchronizationConfirmation = confirmSynchronizedTarget(changedDirectory, target);
	if(synchronizationConfirmation != ETargetConfirmation::ACCEPT)
	{
		selectAnother = synchronizationConfirmation == ETargetConfirmation::SELECT_ANOTHER;
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

std::optional<datamanager::EExistingTargetAction> datamanager::askExistingTargetAction(const QString & source, const QString & target, bool mergeOnly, QString & backupPath) const
{
	if(mergeOnly)
	{
		QMessageBox dialog(QMessageBox::Question, tr("Directory is not empty"), tr("The target directory already contains files:\n%1").arg(QDir::toNativeSeparators(target)), QMessageBox::NoButton, parent);
		dialog.setInformativeText(tr("The source and target are in the same directory tree. Only merge is available. Files that exist only in the target will be kept and conflicts will be overwritten."));
		auto * const mergeButton = dialog.addButton(tr("Merge and overwrite"), QMessageBox::AcceptRole);
		dialog.addButton(QMessageBox::Cancel);
		dialog.setDefaultButton(mergeButton);
		dialog.exec();
		if(dialog.clickedButton() == mergeButton)
			return EExistingTargetAction::MERGE;
		return std::nullopt;
	}

	while(true)
	{
		QMessageBox dialog(QMessageBox::Question, tr("Directory is not empty"), tr("The target directory already contains files:\n%1\n\nHow should they be handled?").arg(QDir::toNativeSeparators(target)), QMessageBox::NoButton, parent);
		dialog.setInformativeText(tr("Back up and replace is recommended. Merge keeps target-only files and overwrites conflicts."));
		auto * const backupButton = dialog.addButton(tr("Back up and replace"), QMessageBox::AcceptRole);
		auto * const mergeButton = dialog.addButton(tr("Merge"), QMessageBox::ActionRole);
		auto * const moreButton = dialog.addButton(tr("More options..."), QMessageBox::ActionRole);
		dialog.addButton(QMessageBox::Cancel);
		dialog.setDefaultButton(backupButton);
		dialog.exec();

		if(dialog.clickedButton() == backupButton)
			return EExistingTargetAction::BACK_UP;
		if(dialog.clickedButton() == mergeButton)
			return EExistingTargetAction::MERGE;
		if(dialog.clickedButton() != moreButton)
			return std::nullopt;

		QMessageBox advanced(QMessageBox::Warning, tr("More replacement options"), tr("Choose an advanced replacement option."), QMessageBox::NoButton, parent);
		advanced.setInformativeText(tr("A custom backup saves the current target in a directory you choose. Clean replacement keeps no backup."));
		auto * const customBackupButton = advanced.addButton(tr("Back up to..."), QMessageBox::ActionRole);
		auto * const replaceButton = advanced.addButton(tr("Clean replacement"), QMessageBox::DestructiveRole);
		auto * const backButton = advanced.addButton(tr("Back"), QMessageBox::RejectRole);
		advanced.setDefaultButton(backButton);
		advanced.exec();

		if(advanced.clickedButton() == customBackupButton)
		{
			const auto selectedBackupPath = chooseBackupPath(source, target);
			if(!selectedBackupPath)
				continue;

			backupPath = *selectedBackupPath;
			return EExistingTargetAction::BACK_UP;
		}

		if(advanced.clickedButton() == replaceButton)
		{
			const auto answer = QMessageBox::warning(parent, tr("Confirm clean replacement"), tr("Clean replacement will permanently remove all files currently in the target directory after the new data has been copied successfully.\n\nDo you really want to continue?"), QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
			if(answer == QMessageBox::Yes)
				return EExistingTargetAction::REPLACE;
		}
	}
}

std::optional<QString> datamanager::chooseBackupPath(const QString & source, const QString & target) const
{
	const QString targetParent = QFileInfo(target).dir().absolutePath();
	while(true)
	{
		const QString backupParent = QFileDialog::getExistingDirectory(parent, tr("Select directory where the backup will be created"), targetParent);
		if(backupParent.isEmpty())
			return std::nullopt;

		const QString backupPath = availableBackupPath(target, backupParent);
		if(pathsOverlap(backupPath, source) || pathsOverlap(backupPath, target))
		{
			QMessageBox::warning(parent, tr("Invalid backup directory"), tr("The backup cannot be created inside the source or target directory, and cannot contain either directory."));
			continue;
		}

		if(!isDirectoryWritable(backupParent))
		{
			if(reportPermissionError(tr("The selected backup directory is not writable:\n%1\n\nSelect another location or adjust the directory permissions.").arg(QDir::toNativeSeparators(backupParent))))
				continue;
			return std::nullopt;
		}

		return backupPath;
	}
}

QString datamanager::availableBackupPath(const QString & target, const QString & backupParent) const
{
	const QFileInfo targetInfo(target);
	const QString parentPath = backupParent.isEmpty() ? targetInfo.dir().absolutePath() : backupParent;
	const QString basePath = QDir(parentPath).filePath(targetInfo.fileName() + QStringLiteral("_backup"));
	if(!QFileInfo::exists(basePath))
		return basePath;

	for(int index = 2; ; ++index)
	{
		const QString candidate = basePath + QStringLiteral("_%1").arg(index);
		if(!QFileInfo::exists(candidate))
			return candidate;
	}
}

bool datamanager::installStagedDirectory(const QString & staging, const QString & target, EExistingTargetAction action, const QString & backupPath, QString & displacedPath, QString & error) const
{
	const QFileInfo targetInfo(target);
	const QString temporaryPath = targetInfo.dir().filePath(QStringLiteral(".%1-vcmi-old-%2").arg(targetInfo.fileName(), QUuid::createUuid().toString(QUuid::Id128)));
	displacedPath = action == EExistingTargetAction::BACK_UP && backupPath.isEmpty()
		? availableBackupPath(target)
		: temporaryPath;

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
	QString selectedBackupPath;
	bool customBackupCreated = false;
	bool keepCustomBackup = false;
	EExistingTargetAction completedTargetAction = EExistingTargetAction::MERGE;
	auto removeRejectedCustomBackup = vstd::makeScopeGuard([this, &customBackupCreated, &keepCustomBackup, &selectedBackupPath]()
	{
		if(customBackupCreated && !keepCustomBackup && !removePath(selectedBackupPath))
			logGlobal->warn("Failed to remove incomplete custom backup '%s'", selectedBackupPath.toStdString());
	});

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
				const auto selectedAction = askExistingTargetAction(source, selected, sameDirectoryTree, selectedBackupPath);
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
			const bool sourceInsideTarget = isSameOrChildPath(source, selected);
			// When moving to a parent directory, do not put staging inside the new
			// active root. Keeping it beside that root prevents an incomplete cleanup
			// from exposing .vcmi-transfer-* as user data after the switch.
			const bool stageBesideTarget = !installInPlace || (sourceInsideTarget && targetParentWritable);
			const QString stagingParent = stageBesideTarget ? targetParent : selected;

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
			if(targetAction == EExistingTargetAction::BACK_UP && !selectedBackupPath.isEmpty())
			{
				const qint64 backupSize = directorySize(selected);
				const QStorageInfo backupStorage(QFileInfo(selectedBackupPath).dir().absolutePath());
				if(backupStorage.isValid() && backupStorage.isReady() && backupStorage.bytesAvailable() >= 0 && backupStorage.bytesAvailable() < backupSize)
				{
					progress.reset();
					QMessageBox::critical(parent, tr("Not enough free space"), tr("The selected backup location requires %1, but only %2 is available.").arg(formattedDataSize(backupSize), formattedDataSize(backupStorage.bytesAvailable())));
					return EChangeResult::DONE;
				}

				if(!QDir().mkpath(selectedBackupPath))
				{
					progress.reset();
					QMessageBox::critical(parent, tr("Backup failed"), tr("Failed to create the backup directory: %1").arg(QDir::toNativeSeparators(selectedBackupPath)));
					return EChangeResult::DONE;
				}
				customBackupCreated = true;

				progress->setTitle(tr("Backing up directory..."));
				if(!copyDirectoryContents(selected, selectedBackupPath, *progress, error))
				{
					logGlobal->error("Failed to back up target directory: %s", error.toStdString());
					progress.reset();
					QMessageBox::critical(parent, tr("Backup failed"), error);
					return EChangeResult::DONE;
				}
				progress->setTitle(tr("Copying directory..."));
			}

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

				// Do not defer normal cleanup until this function returns: selected is
				// about to become an active VCMI directory and must not contain staging.
				if(!stagingDirectory.remove())
					logGlobal->warn("Failed to remove transfer directory '%s' immediately after installation", stagingDirectory.path().toStdString());
			}
			else if(!installStagedDirectory(stagingDirectory.path(), selected, targetAction, selectedBackupPath, displacedTargetPath, error))
			{
				// The custom backup may be the only complete copy if restoring the displaced target also failed.
				keepCustomBackup = customBackupCreated;
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
			keepCustomBackup = customBackupCreated;
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
		{
			logGlobal->error("Failed to restore target directory '%s' after reload error", selected.toStdString());
			keepCustomBackup = customBackupCreated;
		}

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
	keepCustomBackup = true;

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
#if defined(VCMI_WINDOWS)
		else
		{
			// Windows historically stored VCMI in Documents\My Games. Also clean up a parent merely
			// named OneDrive, but preserve it when it is an actual cloud-synchronized directory.
			// QDir::rmdir intentionally leaves every non-empty folder intact.
			const QFileInfo parentInfo(QFileInfo(source).dir().absolutePath());
			const bool isMyGames = parentInfo.fileName().compare(QStringLiteral("My Games"), Qt::CaseInsensitive) == 0;
			const bool isNamedOneDrive = parentInfo.fileName().compare(QStringLiteral("OneDrive"), Qt::CaseInsensitive) == 0;
			const std::wstring nativeParent = QDir::toNativeSeparators(parentInfo.absoluteFilePath()).toStdWString();
			const bool isActualCloudDirectory = VCMI::Windows::isCloudStoragePath(nativeParent.c_str());
			if(isMyGames || (isNamedOneDrive && !isActualCloudDirectory))
				parentInfo.dir().rmdir(parentInfo.fileName());
		}
#endif
	}

	const bool displacedTargetCanBeRemoved = completedTargetAction == EExistingTargetAction::REPLACE
		|| (completedTargetAction == EExistingTargetAction::BACK_UP && !selectedBackupPath.isEmpty());
	if(displacedTargetCanBeRemoved && !displacedTargetPath.isEmpty() && !removePath(displacedTargetPath))
	{
		logGlobal->warn("Failed to purge replaced user directory '%s'", displacedTargetPath.toStdString());
		QMessageBox::warning(parent, tr("Cleanup failed"), tr("The new data was installed, but the replaced directory could not be removed: %1").arg(displacedTargetPath));
	}

	const QString completedBackupPath = selectedBackupPath.isEmpty() ? displacedTargetPath : selectedBackupPath;
	const QString message = completedTargetAction == EExistingTargetAction::BACK_UP
		? tr("The launcher has reloaded files from the new directory.\n\nThe previous target was saved to:\n%1").arg(QDir::toNativeSeparators(completedBackupPath))
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
