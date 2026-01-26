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
	ui->lineEditBuildVersion->setText(QString::fromStdString(GameConstants::VCMI_VERSION));
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
	ts << "VCMI version: " << QString::fromStdString(GameConstants::VCMI_VERSION) << '\n';
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

void AboutProjectView::on_pushButtonExportLogs_clicked()
{
	QDir tempDir(ui->lineEditTempDir->text());

#if defined(VCMI_ANDROID) || defined(VCMI_IOS)
    // cleanup old temp archives from previous runs (delete now)
    {
        QDir tdir(QDir::tempPath());
        const QFileInfoList old = tdir.entryInfoList(QStringList() << "vcmi-logs-*.zip", QDir::Files, QDir::Name);
        for (const QFileInfo & fi : old)
            QFile::remove(fi.absoluteFilePath());
    }
	// On mobile: write archive to system temp and send via platform share (no save dialog)
	const QString tmpDir = QDir::tempPath();
	const QString outPath = QDir(tmpDir).filePath(QString("vcmi-logs-%1.zip").arg(QString::number(QDateTime::currentMSecsSinceEpoch())));
#else
	const QString defaultName = QDir::home().filePath("vcmi-logs.zip");
	QString outPath = QFileDialog::getSaveFileName(this, tr("Save logs"), defaultName, tr("Zip archives (*.zip)"));
	if (outPath.isEmpty())
		return;

	if (!outPath.endsWith(".zip", Qt::CaseInsensitive))
		outPath += ".zip";
#endif

	QFileInfoList files = tempDir.entryInfoList({ "*.txt" }, QDir::Files, QDir::Name);
	files.append(QDir(ui->lineEditConfigDir->text()).entryInfoList({ "*.json", "*.ini" }, QDir::Files, QDir::Name));

	// build data dir file/folder listing and add as a virtual text file
	const QString dataDirPath = ui->lineEditUserDataDir->text();
	QString listing;
	QDir dataDir(dataDirPath);
	QDirIterator it(dataDirPath, QDir::NoDotAndDotDot | QDir::AllEntries, QDirIterator::Subdirectories);
	QTextStream ts(&listing);
	while (it.hasNext())
	{
		const QString path = it.next();
		const QString rel = dataDir.relativeFilePath(path);
		QFileInfo info(path);

		if (rel.startsWith(QLatin1String("Saves/")))
			continue;

		if (info.isDir())
			ts << QChar('D') << QLatin1Char(' ') << rel << '\n';
		else
			ts << QChar('F') << QLatin1Char(' ') << rel << QLatin1String(" (") << info.size() << QLatin1String(")") << '\n';
	}

	try
	{
		// create zip and add .txt files
		std::shared_ptr<CIOApi> api = std::make_shared<CDefaultIOApi>();
		boost::filesystem::path archivePath(outPath.toStdString());
		CZipSaver saver(api, archivePath);

		for (const QFileInfo & fi : files)
		{
			// Skip persistent storage to avoid logging private data (e.g. lobby login tokens)
			if (fi.fileName().compare(QStringLiteral("persistentStorage.json"), Qt::CaseInsensitive) == 0)
				continue;

			QFile f(fi.absoluteFilePath());
			if (!f.open(QIODevice::ReadOnly))
				continue;

			QByteArray data = f.readAll();
			auto stream = saver.addFile(fi.fileName().toStdString());
			stream->write(reinterpret_cast<const ui8 *>(data.constData()), data.size());
		}

		// try to include the last save reported in settings.json
		{
			auto json = JsonUtils::assembleFromFiles("config/settings.json");
			if(!json["general"].isNull() && !json["general"]["lastSave"].isNull())
			{
				try
				{
					auto lastSavePath = json["general"]["lastSave"].String();
					const auto rsave = ResourcePath(lastSavePath, EResType::SAVEGAME);
					const auto * rhandler = CResourceHandler::get();
					if(!rhandler->existsResource(rsave))
						return;

					size_t pos = lastSavePath.find_last_of("/\\");
					std::string name = (pos == std::string::npos)? lastSavePath : lastSavePath.substr(pos + 1);

					const auto & [data, length] = rhandler->load(rsave)->readAll();
					auto stream = saver.addFile(name + ".VSGM1");
					stream->write(data.get(), length);
				}
				catch(const std::exception& e)
				{
					// ignore errors here
				}
			}
		}

		// add generated listing as game-directory-structure.txt
		if (!listing.isEmpty())
		{
			QByteArray data = listing.toUtf8();
			auto stream = saver.addFile(std::string("data-directory-structure.txt"));
			stream->write(reinterpret_cast<const ui8 *>(data.constData()), data.size());
		}

		// add device information as device-info.txt
		{
			const QString deviceInfo = gatherDeviceInfo();
			if (!deviceInfo.isEmpty())
			{
				QByteArray dataDev = deviceInfo.toUtf8();
				auto streamDev = saver.addFile(std::string("device-info.txt"));
				streamDev->write(reinterpret_cast<const ui8 *>(dataDev.constData()), dataDev.size());
			}
		}
	}
	catch (const std::exception & e)
	{
		QMessageBox::critical(this, tr("Error"), tr("Failed to create archive: %1").arg(QString::fromUtf8(e.what())));
		return;
	}
	// On mobile platforms, send file via platform and remove temporary file afterwards.
#if defined(VCMI_ANDROID) || defined(VCMI_IOS)
	QMessageBox::information(this, tr("Send logs"), tr("The archive will be sent via another application. Share your logs e.g. over discord to developers."));
	Helper::sendFileToApp(outPath);
#else
	// desktop: notify user and do not auto-send
	QMessageBox::information(this, tr("Success"), tr("Logs saved to %1, please send them to the developers").arg(outPath));
#endif
}
