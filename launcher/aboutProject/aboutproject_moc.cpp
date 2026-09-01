/*
 * aboutproject_moc.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "aboutproject_moc.h"
#include "ui_aboutproject_moc.h"

#if defined(VCMI_ANDROID)
#include <QAndroidJniObject>
#endif
#if defined(VCMI_IOS)
#include "ios/iOS_utils.h"
#endif

#include "../updatedialog_moc.h"
#include "../helper.h"

#include "../../lib/GameConstants.h"
#include "../../lib/VCMIDirs.h"
#include "../../lib/filesystem/CZipSaver.h"
#include "../../lib/json/JsonUtils.h"
#include "../../lib/filesystem/Filesystem.h"

namespace
{
void addLastSaveToArchiveIfAvailable(CZipSaver & saver)
{
	const auto json = JsonUtils::assembleFromFiles("config/settings.json");
	const auto lastSavePath = json["general"]["lastSave"].String();
	if (lastSavePath.empty())
		return;
	const auto rsave = ResourcePath(lastSavePath, EResType::SAVEGAME);
	const auto * rhandler = CResourceHandler::get();
	if(!rhandler->existsResource(rsave))
		return;

	size_t pos = lastSavePath.find_last_of("/\\");
	std::string name = (pos == std::string::npos) ? lastSavePath : lastSavePath.substr(pos + 1);

	const auto & [data, length] = rhandler->load(rsave)->readAll();
	auto stream = saver.addFile(name + ".vsgm1");
	stream->write(data.get(), length);
}
}

void AboutProjectView::hideAndStretchWidget(QGridLayout * layout, QWidget * toHide, QWidget * toStretch)
{
	toHide->hide();

	int index = layout->indexOf(toStretch);
	int row;
	int col;
	int unused;
	layout->getItemPosition(index, &row, &col, &unused, &unused);
	layout->removeWidget(toHide);
	layout->removeWidget(toStretch);
	layout->addWidget(toStretch, row, col, 1, -1);
}

AboutProjectView::AboutProjectView(QWidget * parent)
	: QWidget(parent)
	, ui(std::make_unique<Ui::AboutProjectView>())
{
	ui->setupUi(this);

	ui->lineEditUserDataDir->setText(pathToQString(VCMIDirs::get().userDataPath()));
	ui->lineEditGameDir->setText(pathToQString(VCMIDirs::get().binaryPath()));
	ui->lineEditTempDir->setText(pathToQString(VCMIDirs::get().userLogsPath()));
	ui->lineEditConfigDir->setText(pathToQString(VCMIDirs::get().userConfigPath()));
	ui->lineEditBuildVersion->setText(QString(GameConstants::VCMI_VERSION));
	ui->lineEditOperatingSystem->setText(QSysInfo::prettyProductName());

#ifdef VCMI_MOBILE
	// On mobile platforms these directories are generally not accessible from phone itself, only via USB connection from PC
	// Remove "Open" buttons and stretch line with text into now-empty space
	hideAndStretchWidget(ui->gridLayout, ui->openGameDataDir, ui->lineEditGameDir);
#ifdef VCMI_ANDROID
	hideAndStretchWidget(ui->gridLayout, ui->openUserDataDir, ui->lineEditUserDataDir);
	hideAndStretchWidget(ui->gridLayout, ui->openTempDir, ui->lineEditTempDir);
	hideAndStretchWidget(ui->gridLayout, ui->openConfigDir, ui->lineEditConfigDir);
#endif
#endif
}

AboutProjectView::~AboutProjectView() = default;

void AboutProjectView::changeEvent(QEvent *event)
{
	if(event->type() == QEvent::LanguageChange)
		ui->retranslateUi(this);

	QWidget::changeEvent(event);
}

void AboutProjectView::on_updatesButton_clicked()
{
	UpdateDialog::showUpdateDialog(true);
}

void AboutProjectView::on_openGameDataDir_clicked()
{
	Helper::revealDirectoryInFileBrowser(ui->lineEditGameDir->text());
}

void AboutProjectView::on_openUserDataDir_clicked()
{
	Helper::revealDirectoryInFileBrowser(ui->lineEditUserDataDir->text());
}

void AboutProjectView::on_openTempDir_clicked()
{
	Helper::revealDirectoryInFileBrowser(ui->lineEditTempDir->text());
}

void AboutProjectView::on_openConfigDir_clicked()
{
	Helper::revealDirectoryInFileBrowser(ui->lineEditConfigDir->text());
}

void AboutProjectView::on_pushButtonDiscord_clicked()
{
	QDesktopServices::openUrl(QUrl("https://discord.gg/chBT42V"));
}

void AboutProjectView::on_pushButtonGithub_clicked()
{
	QDesktopServices::openUrl(QUrl("https://github.com/vcmi/vcmi"));
}

void AboutProjectView::on_pushButtonHomepage_clicked()
{
	QDesktopServices::openUrl(QUrl("https://vcmi.eu/"));
}

void AboutProjectView::on_pushButtonBugreport_clicked()
{
	QDesktopServices::openUrl(QUrl("https://github.com/vcmi/vcmi/issues"));
}

static QString gatherDeviceInfo()
{
	QString info;
	QTextStream ts(&info);
	ts << "VCMI version: " << QString(GameConstants::VCMI_VERSION) << '\n';
	ts << "Operating system: " << QSysInfo::prettyProductName() << " (" << QSysInfo::kernelVersion() << ")" << '\n';
	ts << "CPU architecture: " << QSysInfo::currentCpuArchitecture() << '\n';
	ts << "Qt version: " << QT_VERSION_STR << '\n';
#if defined(VCMI_ANDROID)
	QString model = QAndroidJniObject::getStaticObjectField(
		"android/os/Build",
		"MODEL",
		"Ljava/lang/String;"
	).toString();
	QString manufacturer = QAndroidJniObject::getStaticObjectField(
		"android/os/Build",
		"MANUFACTURER",
		"Ljava/lang/String;"
	).toString();
	ts << "Device model: " << model << '\n';
	ts << "Manufacturer: " << manufacturer << '\n';
#endif
#if defined(VCMI_IOS)
	ts << "Device model: " << QString::fromStdString(iOS_utils::iphoneHardwareId()) << '\n';
	ts << "Manufacturer: " << "Apple" << '\n';
#endif
	return info;
}

static void setupExportProgressDialog(QProgressDialog & progress, const QString & title, const QString & labelText, int maximum)
{
	progress.setWindowTitle(title);
	progress.setWindowModality(Qt::WindowModal);
	progress.setMinimumDuration(0);
	progress.setAutoReset(false);
	progress.setAutoClose(false);
	progress.setWindowFlag(Qt::WindowCloseButtonHint, false);
	progress.setWindowFlag(Qt::WindowContextHelpButtonHint, false);
#if defined(VCMI_MOBILE)
	progress.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
	progress.setObjectName(QStringLiteral("contentPanel"));
	progress.setStyleSheet(QStringLiteral("QWidget#contentPanel { background: palette(window); border: 2px solid rgba(0,0,0,160); border-radius: 6px; }"));
#endif
	if(auto * progressBar = progress.findChild<QProgressBar *>())
	{
		progressBar->setRange(0, maximum);
		progressBar->setValue(0);
		progressBar->setAlignment(Qt::AlignCenter);
		progressBar->setFormat(QStringLiteral("%v / %m"));
	}
	progress.setLabelText(labelText);
}

static bool exportSavesToLocalArchive(AboutProjectView * view, const QString & outPath)
{
	QDir savesDir(pathToQString(VCMIDirs::get().userSavePath()));
	if(!savesDir.exists())
	{
		logGlobal->warn("Save export failed: saves directory does not exist at %s", savesDir.absolutePath().toStdString());
		QMessageBox::warning(view, view->tr("Error"), view->tr("Saves directory does not exist"));
		return false;
	}

	QStringList saveFiles;
	QDirIterator scanner(savesDir.absolutePath(), QDir::Files, QDirIterator::Subdirectories);
	while(scanner.hasNext())
	{
		const QString savePath = scanner.next();
		if(savePath.endsWith(".vsgm1", Qt::CaseInsensitive))
			saveFiles.push_back(savePath);
	}

	if(saveFiles.empty())
	{
		logGlobal->warn("Save export failed: no save files found in %s", savesDir.absolutePath().toStdString());
		QMessageBox::warning(view, view->tr("Error"), view->tr("No save files were found"));
		return false;
	}

	QProgressDialog progress(view->tr("Exporting saves..."), view->tr("Cancel"), 0, saveFiles.size(), view);
	setupExportProgressDialog(progress, view->tr("Save export"), view->tr("Exporting saves..."), saveFiles.size());

	try
	{
		std::shared_ptr<CIOApi> api = std::make_shared<CDefaultIOApi>();
		boost::filesystem::path archivePath = qstringToPath(outPath);
		CZipSaver saver(api, archivePath);

		for(int index = 0; index < saveFiles.size(); ++index)
		{
			if(progress.wasCanceled())
			{
				QFile::remove(outPath);
				return false;
			}

			const QString savePath = saveFiles[index];
			const QString relativePath = savesDir.relativeFilePath(savePath);
			progress.setValue(index + 1);
			qApp->processEvents();

			QFile saveFile(savePath);
			if(!saveFile.open(QIODevice::ReadOnly))
				continue;

			QByteArray data = saveFile.readAll();
			QByteArray relativePathUtf8 = relativePath.toUtf8();
			auto stream = saver.addFile(std::string(relativePathUtf8.constData(), relativePathUtf8.size()));
			stream->write(reinterpret_cast<const ui8 *>(data.constData()), data.size());
		}

		progress.setValue(saveFiles.size());
		qApp->processEvents();
		progress.hide();
	}
	catch(const std::exception & e)
	{
		logGlobal->error("Save export failed while creating archive %s. Reason: %s", outPath.toStdString(), e.what());
		QMessageBox::critical(view, view->tr("Error"), view->tr("Failed to create archive: %1").arg(QString::fromUtf8(e.what())));
		return false;
	}

	return true;
}

static QString chooseLogArchivePath(AboutProjectView * view)
{
#if defined(VCMI_MOBILE)
	QDir tempDir(QDir::tempPath());
	const QFileInfoList oldArchives = tempDir.entryInfoList(QStringList() << "vcmi-logs-*.zip", QDir::Files, QDir::Name);
	for(const QFileInfo & file : oldArchives)
		QFile::remove(file.absoluteFilePath());

	return tempDir.filePath(QString("vcmi-logs-%1.zip").arg(QString::number(QDateTime::currentMSecsSinceEpoch())));
#else
	const QString defaultName = QDir::home().filePath("vcmi-logs.zip");
	QString outPath = QFileDialog::getSaveFileName(view, view->tr("Save logs"), defaultName, view->tr("Zip archives (*.zip)"));
	if(outPath.isEmpty())
		return {};
	if(!outPath.endsWith(".zip", Qt::CaseInsensitive))
		outPath += ".zip";
	return outPath;
#endif
}

static QString buildDataDirListing(const QString & dataDirPath)
{
	QString listing;
	QDir dataDir(dataDirPath);
	QDirIterator it(dataDirPath, QDir::NoDotAndDotDot | QDir::AllEntries, QDirIterator::Subdirectories);
	QTextStream ts(&listing);
	while(it.hasNext())
	{
		const QString path = it.next();
		const QString rel = dataDir.relativeFilePath(path);
		const QFileInfo info(path);

		if(rel.startsWith(QLatin1String("Saves/")))
			continue;

		if(info.isDir())
			ts << QChar('D') << QLatin1Char(' ') << rel << '\n';
		else
			ts << QChar('F') << QLatin1Char(' ') << rel << QLatin1String(" (") << info.size() << QLatin1String(")") << '\n';
	}
	return listing;
}

static bool advanceProgress(QProgressDialog & progress, int & value, const QString & outPath)
{
	++value;
	progress.setValue(value);
	qApp->processEvents();
	if(progress.wasCanceled())
	{
		QFile::remove(outPath);
		return false;
	}
	return true;
}

#if defined(VCMI_MOBILE)
static void saveLogArchiveToDestination(AboutProjectView * view, const QString & sourcePath, const QString & targetPath)
{
	if(targetPath.isEmpty())
		return;

	if(Helper::performNativeCopy(sourcePath, targetPath))
		QMessageBox::information(view, view->tr("Success"), view->tr("Logs saved to %1").arg(Helper::getRealPath(targetPath)));
	else
	{
		logGlobal->error("Log export failed: could not copy archive from %s to %s", sourcePath.toStdString(), targetPath.toStdString());
		QMessageBox::critical(view, view->tr("Error"), view->tr("Failed to save archive to selected destination"));
	}
}

static void handleMobileLogArchiveDestination(AboutProjectView * view, const QString & outPath)
{
	QMessageBox destinationChoice(view);
	destinationChoice.setIcon(QMessageBox::Question);
	destinationChoice.setWindowTitle(view->tr("Export logs"));
	destinationChoice.setText(view->tr("Choose what to do with the exported logs archive."));
	QPushButton * shareButton = destinationChoice.addButton(view->tr("Share"), QMessageBox::AcceptRole);
	QPushButton * saveButton = destinationChoice.addButton(view->tr("Save"), QMessageBox::ActionRole);
	destinationChoice.addButton(QMessageBox::Cancel);
	destinationChoice.exec();

	if(destinationChoice.clickedButton() == shareButton)
	{
		QMessageBox::information(view, view->tr("Send logs"), view->tr("The archive will be sent via another application. Share your logs e.g. over discord to developers."));
		Helper::sendFileToApp(outPath);
		return;
	}

	if(destinationChoice.clickedButton() != saveButton)
		return;

	if(Helper::canUseFolderPicker())
	{
		Helper::nativeFolderPicker(view, [view, outPath](QString picked)
		{
			if(picked.isEmpty())
				return;

			const QString destination = Helper::createFile(picked, QStringLiteral("vcmi-logs.zip"), QStringLiteral("application/zip"));
			saveLogArchiveToDestination(view, outPath, destination);
		});
		return;
	}

	logGlobal->warn("Log export: folder picker unavailable, using manual file selection fallback");
	QMessageBox::information(view, view->tr("Select destination file"), view->tr("Please select destination file and save the archive as vcmi-logs.zip."));
	const QString defaultName = QDir::home().filePath("vcmi-logs.zip");
	QString pickedPath = QFileDialog::getSaveFileName(view, view->tr("Select destination file"), defaultName, view->tr("Zip archives (*.zip);;All files (*.*)"));
	if(pickedPath.isEmpty())
		return;
	if(!pickedPath.endsWith(".zip", Qt::CaseInsensitive))
		pickedPath += ".zip";
	saveLogArchiveToDestination(view, outPath, pickedPath);
}
#endif

void AboutProjectView::on_pushButtonExportSaves_clicked()
{
	auto ensureZipSuffix = [](QString path)
	{
		if(path.endsWith(".zip", Qt::CaseInsensitive))
			return path;
		return path + ".zip";
	};

#if defined(VCMI_MOBILE)
	auto exportViaTarget = [this](const QString & target, bool targetIsFile)
	{
		if(target.isEmpty())
			return;

		const QString cacheDir = pathToQString(VCMIDirs::get().userCachePath());
		const QString cacheArchivePath = QDir(cacheDir).filePath(QStringLiteral("vcmi-saves-export.zip"));
		if(!exportSavesToLocalArchive(this, cacheArchivePath))
			return;

		const QString dstPath = targetIsFile ? target : Helper::createFile(target, QStringLiteral("vcmi-saves.zip"), QStringLiteral("application/zip"));

		if(!dstPath.isEmpty() && Helper::performNativeCopy(cacheArchivePath, dstPath))
			QMessageBox::information(this, tr("Success"), tr("Saves exported to %1").arg(Helper::getRealPath(dstPath)));
		else
		{
			logGlobal->error("Save export failed: could not copy archive from %s to %s", cacheArchivePath.toStdString(), dstPath.toStdString());
			QMessageBox::critical(this, tr("Error"), tr("Failed to save archive to selected destination"));
		}

		QFile::remove(cacheArchivePath);
	};

	if(Helper::canUseFolderPicker())
	{
		Helper::nativeFolderPicker(this, [exportViaTarget](QString picked)
		{
			if(picked.isEmpty())
				return;
			exportViaTarget(picked, false);
		});
	}
	else
	{
		logGlobal->warn("Save export: folder picker unavailable, using manual file selection fallback");
		QMessageBox::information(this, tr("Select destination file"), tr("Please select destination file and save the archive as vcmi-saves.zip."));
		const QString defaultName = QDir::home().filePath("vcmi-saves.zip");
		const QString pickedPath = QFileDialog::getSaveFileName(this, tr("Select destination file"), defaultName, tr("Zip archives (*.zip);;All files (*.*)"));
		if(pickedPath.isEmpty())
			return;
		exportViaTarget(ensureZipSuffix(pickedPath), true);
	}
#else
	const QString defaultName = QDir::home().filePath("vcmi-saves.zip");
	QString outPath = QFileDialog::getSaveFileName(this, tr("Export saves"), defaultName, tr("Zip archives (*.zip)"));
	if(outPath.isEmpty())
		return;

	outPath = ensureZipSuffix(outPath);
	if(exportSavesToLocalArchive(this, outPath))
		QMessageBox::information(this, tr("Success"), tr("Saves exported to %1").arg(Helper::getRealPath(outPath)));
#endif
}

void AboutProjectView::on_pushButtonExportLogs_clicked()
{
	QDir tempDir(ui->lineEditTempDir->text());
	const QString outPath = chooseLogArchivePath(this);
	if(outPath.isEmpty())
		return;

	QFileInfoList files = tempDir.entryInfoList({ "*.txt" }, QDir::Files, QDir::Name);
	files.append(QDir(ui->lineEditConfigDir->text()).entryInfoList({ "*.json", "*.ini" }, QDir::Files, QDir::Name));
	const QString listing = buildDataDirListing(ui->lineEditUserDataDir->text());

	const int progressMaximum = files.size() + 3;
	QProgressDialog progress(tr("Exporting logs..."), tr("Cancel"), 0, progressMaximum, this);
	setupExportProgressDialog(progress, tr("Log export"), tr("Exporting logs..."), progressMaximum);

	int progressValue = 0;
	try
	{
		// create zip and add .txt files
		std::shared_ptr<CIOApi> api = std::make_shared<CDefaultIOApi>();
		boost::filesystem::path archivePath = qstringToPath(outPath);
		CZipSaver saver(api, archivePath);

		for(const QFileInfo & file : files)
		{
			// Skip persistent storage to avoid logging private data (e.g. lobby login tokens)
			if(file.fileName().compare(QStringLiteral("persistentStorage.json"), Qt::CaseInsensitive) == 0)
			{
				if(!advanceProgress(progress, progressValue, outPath))
					return;
				continue;
			}

			QFile f(file.absoluteFilePath());
			if(f.open(QIODevice::ReadOnly))
			{
				QByteArray data = f.readAll();
				QByteArray fileNameUtf8 = file.fileName().toUtf8();
				auto stream = saver.addFile(std::string(fileNameUtf8.constData(), fileNameUtf8.size()));
				stream->write(reinterpret_cast<const ui8 *>(data.constData()), data.size());
			}

			if(!advanceProgress(progress, progressValue, outPath))
				return;
		}

		// try to include the last save reported in settings.json
		addLastSaveToArchiveIfAvailable(saver);
		if(!advanceProgress(progress, progressValue, outPath))
			return;

		// add generated listing as game-directory-structure.txt
		if(!listing.isEmpty())
		{
			QByteArray data = listing.toUtf8();
			auto stream = saver.addFile(std::string("data-directory-structure.txt"));
			stream->write(reinterpret_cast<const ui8 *>(data.constData()), data.size());
		}
		if(!advanceProgress(progress, progressValue, outPath))
			return;

		// add device information as device-info.txt
		{
			const QString deviceInfo = gatherDeviceInfo();
			if(!deviceInfo.isEmpty())
			{
				QByteArray dataDev = deviceInfo.toUtf8();
				auto streamDev = saver.addFile(std::string("device-info.txt"));
				streamDev->write(reinterpret_cast<const ui8 *>(dataDev.constData()), dataDev.size());
			}
		}
		if(!advanceProgress(progress, progressValue, outPath))
			return;

		progress.hide();
	}
	catch(const boost::filesystem::filesystem_error & e)
	{
		QFile::remove(outPath);
		logGlobal->error("Log export failed while creating archive %s. Reason: %s", outPath.toStdString(), e.what());
		QMessageBox::critical(this, tr("Error"), tr("Failed to create archive: %1").arg(QString::fromUtf8(e.what())));
		return;
	}

	// On mobile platforms, let user choose between share and saving to selected destination.
#if defined(VCMI_MOBILE)
	handleMobileLogArchiveDestination(this, outPath);
#else
	// desktop: notify user and do not auto-send
	QMessageBox::information(this, tr("Success"), tr("Logs saved to %1, please send them to the developers").arg(Helper::getRealPath(outPath)));
#endif
}
