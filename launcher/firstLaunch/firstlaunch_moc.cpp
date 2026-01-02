/*
 * firstlaunch_moc.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "firstlaunch_moc.h"
#include "ui_firstlaunch_moc.h"

#include "mainwindow_moc.h"
#include "modManager/cmodlistview_moc.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/Languages.h"
#include "../../lib/VCMIDirs.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../vcmiqt/MessageBox.h"
#include "../helper.h"
#include "../languages.h"
#include "../innoextract.h"
#include "progressoverlay.h"

// Create and show overlay immediately
static ProgressOverlay* createOverlay(QWidget *parent, const QString &title, bool indeterminate = true)
{
	auto *overlay = new ProgressOverlay(parent, 50);
	overlay->setTitle(title);
	overlay->setIndeterminate(indeterminate);
	overlay->show();
	qApp->processEvents(); // paint before heavy work
	return overlay;
}

FirstLaunchView::FirstLaunchView(QWidget * parent)
	: QWidget(parent)
	, ui(std::make_unique<Ui::FirstLaunchView>())
{
	ui->setupUi(this);

	enterSetup();
	activateTabLanguage();

	ui->lineEditDataSystem->setText(pathToQString(boost::filesystem::absolute(VCMIDirs::get().dataPaths().front())));
	ui->lineEditDataUser->setText(pathToQString(boost::filesystem::absolute(VCMIDirs::get().userDataPath())));

	Helper::enableScrollBySwiping(ui->listWidgetLanguage);

#ifdef VCMI_MOBILE
	// This directory is not accessible to players without rooting of their device
	ui->lineEditDataSystem->hide();
#endif

#ifndef ENABLE_INNOEXTRACT
	ui->pushButtonGogInstall->hide();
	ui->labelDataGogTitle->hide();
	ui->labelDataGogDescr->hide();
#endif
}

FirstLaunchView::~FirstLaunchView() = default;

void FirstLaunchView::on_buttonTabLanguage_clicked()
{
	activateTabLanguage();
}

void FirstLaunchView::on_buttonTabHeroesData_clicked()
{
	activateTabHeroesData();
}

void FirstLaunchView::on_buttonTabModPreset_clicked()
{
	activateTabModPreset();
}

void FirstLaunchView::on_listWidgetLanguage_currentRowChanged(int currentRow)
{
	languageSelected(ui->listWidgetLanguage->item(currentRow)->data(Qt::UserRole).toString());
}

void FirstLaunchView::changeEvent(QEvent * event)
{
	if(event->type() == QEvent::LanguageChange)
	{
		ui->retranslateUi(this);
		Languages::fillLanguages(ui->listWidgetLanguage, false);
	}
	QWidget::changeEvent(event);
}

void FirstLaunchView::on_pushButtonLanguageNext_clicked()
{
	activateTabHeroesData();
}

void FirstLaunchView::on_pushButtonDataNext_clicked()
{
	activateTabModPreset();
}

void FirstLaunchView::on_pushButtonDataBack_clicked()
{
	activateTabLanguage();
}

void FirstLaunchView::on_pushButtonDataSearch_clicked()
{
	heroesDataUpdate();
}

void FirstLaunchView::on_pushButtonDataCopy_clicked()
{
	// iOS can't display modal dialogs when called directly on button press
	// https://bugreports.qt.io/browse/QTBUG-98651
	MessageBoxCustom::showDialog(this, [this]{
		Helper::nativeFolderPicker(this, [this](const QString &picked){
			if(!picked.isEmpty())
				copyHeroesData(picked, false);
		});
	});
}

void FirstLaunchView::on_pushButtonGogInstall_clicked()
{
	// iOS can't display modal dialogs when called directly on button press
	// https://bugreports.qt.io/browse/QTBUG-98651
	MessageBoxCustom::showDialog(this, [this]{extractGogData();});
}

void FirstLaunchView::enterSetup()
{
	Languages::fillLanguages(ui->listWidgetLanguage, false);
}

void FirstLaunchView::setSetupProgress(int progress)
{
	ui->buttonTabLanguage->setDisabled(progress < 1);
	ui->buttonTabHeroesData->setDisabled(progress < 2);
	ui->buttonTabModPreset->setDisabled(progress < 3);
}

void FirstLaunchView::activateTabLanguage()
{
	setSetupProgress(1);
	ui->installerTabs->setCurrentIndex(0);
	ui->buttonTabLanguage->setChecked(true);
	ui->buttonTabHeroesData->setChecked(false);
	ui->buttonTabModPreset->setChecked(false);
}

void FirstLaunchView::activateTabHeroesData()
{
	setSetupProgress(2);
	ui->installerTabs->setCurrentIndex(1);
	ui->buttonTabLanguage->setChecked(false);
	ui->buttonTabHeroesData->setChecked(true);
	ui->buttonTabModPreset->setChecked(false);

	if(heroesDataUpdate())
	{
		activateTabModPreset();
		return;
	}

	QString installPath = getHeroesInstallDir();
	if(!installPath.isEmpty())
	{
		auto reply = QMessageBox::question(this, tr("Heroes III installation found!"), tr("Copy data to VCMI folder?"), QMessageBox::Yes | QMessageBox::No);
		if(reply == QMessageBox::Yes)
			copyHeroesData(installPath, false);
	}
}

void FirstLaunchView::activateTabModPreset()
{
	setSetupProgress(3);
	ui->installerTabs->setCurrentIndex(2);
	ui->buttonTabLanguage->setChecked(false);
	ui->buttonTabHeroesData->setChecked(false);
	ui->buttonTabModPreset->setChecked(true);

	modPresetUpdate();
}

void FirstLaunchView::exitSetup(bool goToMods)
{
	if(auto * mainWindow = dynamic_cast<MainWindow *>(QApplication::activeWindow()))
		mainWindow->exitSetup(goToMods);
}

// Tab Language
void FirstLaunchView::languageSelected(const QString & selectedLanguage)
{
	Settings node = settings.write["general"]["language"];
	node->String() = selectedLanguage.toStdString();

	if(auto * mainWindow = dynamic_cast<MainWindow *>(QApplication::activeWindow()))
		mainWindow->updateTranslation();
}

bool FirstLaunchView::heroesDataUpdate()
{
	bool detected = heroesDataDetect();
	if(detected)
		heroesDataDetected();
	else
		heroesDataMissing();
	return detected;
}

void FirstLaunchView::heroesDataMissing()
{
	QPalette newPalette = palette();
	newPalette.setColor(QPalette::Base, QColor(200, 50, 50));
	ui->lineEditDataSystem->setPalette(newPalette);
	ui->lineEditDataUser->setPalette(newPalette);

	ui->labelDataManualTitle->setVisible(true);
	ui->labelDataManualDescr->setVisible(true);
	ui->pushButtonDataSearch->setVisible(true);

	const bool canUseDataCopy = Helper::canUseFolderPicker();

	ui->labelDataCopyTitle->setVisible(canUseDataCopy);
	ui->labelDataCopyDescr->setVisible(canUseDataCopy);
	ui->pushButtonDataCopy->setVisible(canUseDataCopy);

#ifdef ENABLE_INNOEXTRACT
	ui->pushButtonGogInstall->setVisible(true);
	ui->labelDataGogTitle->setVisible(true);
	ui->labelDataGogDescr->setVisible(true);
#endif

	ui->labelDataFound->setVisible(false);
	ui->pushButtonDataNext->setEnabled(false);
}

void FirstLaunchView::heroesDataDetected()
{
	QPalette newPalette = palette();
	newPalette.setColor(QPalette::Base, QColor(50, 200, 50));
	ui->lineEditDataSystem->setPalette(newPalette);
	ui->lineEditDataUser->setPalette(newPalette);

	ui->pushButtonDataSearch->setVisible(false);
	ui->pushButtonDataCopy->setVisible(false);

	ui->labelDataManualTitle->setVisible(false);
	ui->labelDataManualDescr->setVisible(false);
	ui->labelDataCopyTitle->setVisible(false);
	ui->labelDataCopyDescr->setVisible(false);

#ifdef ENABLE_INNOEXTRACT
	ui->pushButtonGogInstall->setVisible(false);
	ui->labelDataGogTitle->setVisible(false);
	ui->labelDataGogDescr->setVisible(false);
#endif

	ui->labelDataFound->setVisible(true);
	ui->pushButtonDataNext->setEnabled(true);

	CGeneralTextHandler::detectInstallParameters();
}

// Tab Heroes III Data
bool FirstLaunchView::heroesDataDetect()
{
	// user might have copied files to one of our data path.
	// perform full reinitialization of virtual filesystem
	CResourceHandler::destroy();
	CResourceHandler::initialize();
	CResourceHandler::load("config/filesystem.json");

	// use file from lod archive to check presence of H3 data. Very rough estimate, but will work in majority of cases
	bool heroesDataFoundROE = CResourceHandler::get()->existsResource(ResourcePath("DATA/GENRLTXT.TXT"));
	bool heroesDataFoundSOD = CResourceHandler::get()->existsResource(ResourcePath("DATA/TENTCOLR.TXT"));

	return heroesDataFoundROE && heroesDataFoundSOD;
}

QString FirstLaunchView::getHeroesInstallDir()
{
#ifdef VCMI_WINDOWS
	QVector<QPair<QString, QString>> regKeys = {
		{ "HKEY_LOCAL_MACHINE\\SOFTWARE\\GOG.com\\Games\\1207658787",											 "path"	   }, // Gog on x86 system
		{ "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\GOG.com\\Games\\1207658787",							     "path"	   }, // Gog on x64 system
		{ "HKEY_LOCAL_MACHINE\\SOFTWARE\\New World Computing\\Heroes of Might and Magic® III\\1.0",			     "AppPath" }, // H3 Complete on x86 system
		{ "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\New World Computing\\Heroes of Might and Magic® III\\1.0", "AppPath" }, // H3 Complete on x64 system
		{ "HKEY_LOCAL_MACHINE\\SOFTWARE\\New World Computing\\Heroes of Might and Magic III\\1.0",			     "AppPath" }, // some localized H3 on x86 system
		{ "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\New World Computing\\Heroes of Might and Magic III\\1.0",  "AppPath" }, // some localized H3 on x64 system
	};

	for(auto & regKey : regKeys)
	{
		QString path = QSettings(regKey.first, QSettings::NativeFormat).value(regKey.second).toString();
		if(!path.isEmpty())
			return path;
	}
#endif
	return QString{};
}
	
static QString defaultStartDirForOpen()
{
#if defined(VCMI_MOBILE)
	const QStandardPaths::StandardLocation mobilePrefs[] = {
		QStandardPaths::DocumentsLocation,
		QStandardPaths::HomeLocation
	};
	for(auto location : mobilePrefs)
	{
		for(const QString &path : QStandardPaths::standardLocations(location))
			if(QDir(path).exists() && !path.isEmpty())
				return path;
	}
	return QDir::homePath();
#else
	// Desktop: prefer Downloads, then Home, then Desktop
	const QStandardPaths::StandardLocation desktopPrefs[] = {
		QStandardPaths::DownloadLocation,
		QStandardPaths::HomeLocation,
		QStandardPaths::DesktopLocation
	};
	for(auto location : desktopPrefs)
	{
		for(const QString &path : QStandardPaths::standardLocations(location))
			if(QDir(path).exists() && !path.isEmpty())
				return path;
	}
	return QDir::homePath();
#endif
}


QString FirstLaunchView::checkFileMagic(const QString &filename, const QString &filter, const QByteArray &magic, const QString &ext, bool &openFailed) const
{
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly))
    {
		if(openFailed)
		{
			return tr("Failed to open file: %1").arg(file.errorString());
		}
		else
		{
			// Some systems can't access selected file for read, but can copy it, postpone fail fast for next run
			logGlobal->warn("checkMagic: open failed for '%s': %s", filename.toStdString(), file.errorString().toStdString());
			openFailed = true;
			return {};
		}
    }

    QFileInfo fileInfo(filename);
    quint64 fileSize = fileInfo.size();

    logGlobal->info("Checking %s with size: %llu", filename.toStdString(), fileSize);

#if defined(VCMI_MOBILE)
    if(fileInfo.suffix().compare(ext, Qt::CaseInsensitive) != 0)
        return tr("You need to select a %1 file!", "param is file extension").arg(ext);
#endif

    if(fileInfo.suffix().compare("exe", Qt::CaseInsensitive) == 0){
        if(fileSize > 1500000) // 1.5MB
        {
            logGlobal->info("Unknown installer selected: %s", filename.toStdString());
            return tr("Unknown installer selected.\nYou need to select the offline GOG installer.");
        }

        const QByteArray data = file.peek(fileSize);

        constexpr std::u16string_view galaxyID = u"GOG Galaxy";
        const auto galaxyIDBytes = reinterpret_cast<const char*>(galaxyID.data());
        const auto magicId = QByteArray::fromRawData(galaxyIDBytes, galaxyID.size() * sizeof(decltype(galaxyID)::value_type));

        if(data.contains(magicId))
        {
            logGlobal->info("GOG Galaxy detected! Aborting...");
            return tr("You selected a GOG Galaxy installer. This file does not contain the game. Please download the offline backup game installer instead.");
        }
    }

    const QByteArray magicFile = file.peek(magic.length());
    if(!magicFile.startsWith(magic))
        return tr("You need to select a %1 file!", "param is file extension").arg(filter);

    return {};
}

void FirstLaunchView::extractGogData()
{
#ifdef ENABLE_INNOEXTRACT
	auto fileSelection = [this](const QString &title,  QString filter, const QString &startPath = {}) {
#if defined(VCMI_MOBILE)
		filter = tr("GOG file (*.*)");
		QMessageBox::information(this, tr("File selection"), title);
#endif
		QString file = QFileDialog::getOpenFileName(this, title, startPath.isEmpty() ? defaultStartDirForOpen() : startPath, filter);
		if(file.isEmpty())
			return QString{};
		return file;
	};

	needPostCopyCheckExe = false;
    needPostCopyCheckBin = false;

	QString filterExe = tr("GOG installer") + " (*.exe)";
	QString titleExe  = tr("Select the offline GOG installer (.exe)");

	QString fileExe = fileSelection(titleExe, filterExe);
	if(fileExe.isEmpty())
		return;

	QString errorText = checkFileMagic(fileExe, filterExe, QByteArray{"MZP"}, "EXE", needPostCopyCheckExe);
	if(!errorText.isEmpty())
	{
		QMessageBox::critical(this, tr("Invalid file selected"), errorText);
		return;
	}

	QFileInfo exeInfo(fileExe);
	QString expectedBinName = exeInfo.completeBaseName() + "-1.bin";
	QString filterBin = tr("GOG data") + " (*.bin)";
	QString titleBin = tr("Select the offline GOG installer data file: %1", "param is file name").arg(expectedBinName);

	// Try to access BIN based on selected EXE
	QString fileBinCandidate = exeInfo.absoluteDir().filePath(expectedBinName);
	bool haveCandidate = false;

	QFile file(fileBinCandidate);
	if(file.open(QIODevice::ReadOnly))
	{
		haveCandidate = true;
		file.close();
	}

    QString fileBin = haveCandidate ? fileBinCandidate : fileSelection(titleBin, filterBin, exeInfo.absolutePath());
	if(fileBin.isEmpty())
		return;

	errorText = checkFileMagic(fileBin, filterBin, QByteArray{"idska32"}, "BIN", needPostCopyCheckBin);
	if(!errorText.isEmpty())
	{
		QMessageBox::critical(this, tr("Invalid data file"), errorText);
		return;
	}

	QTimer::singleShot(100, this, [this, fileBin, fileExe](){ // background to make sure FileDialog is closed...
		extractGogDataAsync(fileBin, fileExe);
		setEnabled(true);
		heroesDataUpdate();
	});
#endif
}

bool FirstLaunchView::performCopyFlow(const QString& path, ProgressOverlay* overlay, bool removeSource)
{
	// 1) Scan -> "Source \t Target \t Name"
	overlay->setIndeterminate(true);

	const QStringList items = Helper::findFilesForCopy(path);
	if(items.isEmpty())
	{
		QMessageBox::critical(this, tr("Heroes III data not found!"), tr("Failed to detect valid Heroes III data in chosen directory.\nPlease select the directory with installed Heroes III data."));
		return false;
	}

	// 2) Validate signature
	// TODO: Find proper way for pure SoD check in import or way to block pure RoE / AB
	//	     Or prepare RoE / AB Ban mod and allow VCMI to go with any H3 version
	auto validate = [](const QStringList &items)->QString {
		bool anyLOD=false;
		bool anySOD=false;
		bool anyHD=false;

		for(const QString &line : items)
		{
			const auto part = line.split('\t');
			if(part[1].compare("Data", Qt::CaseInsensitive) != 0)
				continue;

			const QString &name = part[2];
			if(name.endsWith(".lod", Qt::CaseInsensitive))
			{
				anyLOD = true;
				if(name.startsWith("H3ab", Qt::CaseInsensitive))
					anySOD = true;
			}

			if(name.endsWith(".pak", Qt::CaseInsensitive))
				anyHD = true;
		}

		if(anySOD) return {};

		if(!anyLOD)
			return tr("Failed to detect valid Heroes III data in chosen directory.\nPlease select the directory with installed Heroes III data.");

		if(anyHD)
			return tr("Heroes III: HD Edition files are not supported by VCMI.\nPlease select the directory with Heroes III: Complete Edition or Heroes III: Shadow of Death.");

		return tr("Unknown or unsupported Heroes III version found.\nPlease select the directory with Heroes III: Complete Edition or Heroes III: Shadow of Death.");
	};

	const QString err = validate(items);
	if(!err.isEmpty())
	{
		QMessageBox::critical(this, tr("Heroes III data not found!"), err);
		return false;
	}

	// 3) Plan destination, create target dirs on demand
	QDir targetRoot = pathToQString(VCMIDirs::get().userDataPath());
	QSet<QString> created;

	struct CopyItem { QString source, destination; };
	QVector<CopyItem> plan;
	plan.reserve(items.size());

	for(const QString &line : items)
	{
		const auto part = line.split('\t');

		const QString &source = part[0];
		const QString &target = part[1]; // Data / Maps / Mp3
		const QString &file   = part[2];

		if(!created.contains(target))
		{
			QDir{}.mkpath(targetRoot.filePath(target));
			created.insert(target);
		}

		const QDir destinationDir = targetRoot.filePath(target);
		plan.push_back({ source, destinationDir.filePath(file) });
	}

	// 4) Copy with progress
	overlay->setTitle(tr("Importing Heroes III data..."));
	overlay->setIndeterminate(false);
	overlay->setRange(plan.size());

	for(int i = 0; i < plan.size(); ++i)
	{
		overlay->setFileName(QFileInfo(plan[i].destination).fileName());
		overlay->setValue(i + 1);
		qApp->processEvents();

		if(QFile::exists(plan[i].destination))
			QFile::remove(plan[i].destination);

		Helper::performNativeCopy(plan[i].source, plan[i].destination);

		logGlobal->info("Copying '%s' -> '%s'", plan[i].source.toStdString(), plan[i].destination.toStdString());
	}

	// 5) Optional cleanup
	if(removeSource)
		QDir(path).removeRecursively();

	return true;
}

void FirstLaunchView::extractGogDataAsync(QString filePathBin, QString filePathExe)
{
	logGlobal->info("Extracting gog data from '%s' and '%s'", filePathBin.toStdString(), filePathExe.toStdString());

#ifdef ENABLE_INNOEXTRACT
	// Defer heavy work to next event-loop tick to ensure overlay is painted
	QTimer::singleShot(0, this, [this, filePathBin, filePathExe]()
	{
		QScopedPointer<ProgressOverlay> overlay(createOverlay(this, tr("Preparing installer..."), true));
		overlay->setFileName(QFileInfo(filePathExe).fileName());
		overlay->raise();
		qApp->processEvents();

		// "Goole TV Tick" without this was never displayed "Preparing installer" on screen
		QEventLoop ev;
		QTimer::singleShot(0, &ev, &QEventLoop::quit);
		ev.exec();

		// 1) Prepare temp dir
		QDir tempDir(pathToQString(VCMIDirs::get().userDataPath()));
		if(tempDir.cd("tmp"))
		{
			logGlobal->info("Cleaning up old temp data");
			tempDir.removeRecursively(); // remove if already exists (e.g. previous crash)
			tempDir.cdUp();
		}
		tempDir.mkdir("tmp");
		if(!tempDir.cd("tmp"))
		{
			return; // should not happen - but avoid deleting wrong folder in any case
		}

		logGlobal->info("Using '%s' as temporary directory", tempDir.path().toStdString());

		const QString tmpFileExe = tempDir.filePath("h3_gog.exe");
		const QString tmpFileBin = tempDir.filePath("h3_gog-1.bin");

		// 2) Copy selected files into tmp
		logGlobal->info("Performing native copy...");
		Helper::performNativeCopy(filePathExe, tmpFileExe);

		if(needPostCopyCheckExe)
		{
			const QString err = checkFileMagic(tmpFileExe, tr("GOG installer") + " (*.exe)", QByteArray{"MZP"}, "EXE", needPostCopyCheckExe);
			if(!err.isEmpty())
			{
				QMessageBox::critical(this, tr("Invalid file selected"), err);
				tempDir.removeRecursively();
				return;
			}
		}

		Helper::performNativeCopy(filePathBin, tmpFileBin);

		if(needPostCopyCheckBin)
		{
			const QString err = checkFileMagic(tmpFileBin, tr("GOG data") + " (*.bin)", QByteArray{"idska32"}, "BIN", needPostCopyCheckBin);
			if(!err.isEmpty())
			{
				QMessageBox::critical(this, tr("Invalid data file"), err);
				tempDir.removeRecursively();
				return;
			}
		}

		logGlobal->info("Native copy completed");

		// 3) Extract
		overlay->setTitle(tr("Extracting installer..."));
		overlay->setIndeterminate(false);
		overlay->setRange(100);
		overlay->setValue(0);

		logGlobal->info("Performing extraction using innoextract...");

		QString errorText;

		errorText = Innoextract::extract(tmpFileExe, tempDir.path(), [overlayPtr = overlay.data()](float progress) {
			overlayPtr->setValue(static_cast<int>(progress * 100));
			qApp->processEvents();
		});

		logGlobal->info("Extraction done!");

		// 4) Post-extract verification and error reporting
		QString hashError;
		if(!errorText.isEmpty())
			hashError = Innoextract::getHashError(tmpFileExe, tmpFileBin, filePathExe, filePathBin);

		QStringList dirData = tempDir.entryList({"data"}, QDir::Filter::Dirs);
		if(!errorText.isEmpty() || dirData.empty() || QDir(tempDir.filePath(dirData.front())).entryList({"*.lod"}, QDir::Filter::Files).empty())
		{
			if(!errorText.isEmpty())
			{
				logGlobal->error("GOG installer extraction failure! Reason: %s", errorText.toStdString());
				QMessageBox::critical(this, tr("Extracting error!"), errorText, QMessageBox::Ok, QMessageBox::Ok);
				if(!hashError.isEmpty())
				{
					logGlobal->error("Hash error: %s", hashError.toStdString());
					QMessageBox::critical(this, tr("Hash error!"), hashError, QMessageBox::Ok, QMessageBox::Ok);
				}
			}
			else
				QMessageBox::critical(this, tr("No Heroes III data!"), tr("Selected files do not contain Heroes III data!"), QMessageBox::Ok, QMessageBox::Ok);
			tempDir.removeRecursively();
			return;
		}

		logGlobal->info("Importing Heroes III data...");

		// 5) Reuse overlay for copy phase
		overlay->setTitle(tr("Importing Heroes III data..."));
		overlay->setFileName({});
		overlay->setRange(100); // performCopyFlow will reset to plan size internally
		overlay->setValue(0);

		if(performCopyFlow(tempDir.path(), overlay.data(), true))
			if(heroesDataUpdate())
				activateTabModPreset();
	});
#endif
}

void FirstLaunchView::copyHeroesData(const QString &path, bool removeSource)
{
	QPointer<ProgressOverlay> overlay = createOverlay(this, tr("Scanning selected folder..."), true);
	overlay->raise();
	auto work = [this, path, removeSource, overlay]() {
		if(performCopyFlow(path, overlay, removeSource))
			if(heroesDataUpdate())
				activateTabModPreset();

		overlay->deleteLater();
	};

	QTimer::singleShot(0, this, work);
}

// Tab Mod Preset
void FirstLaunchView::modPresetUpdate()
{
	bool translationExists = !findTranslationModName().isEmpty();

	ui->labelPresetLanguageDescr->setVisible(translationExists);
	ui->buttonPresetLanguage->setVisible(translationExists);

	bool canTrans  = checkCanInstallTranslation();
	bool canExtras = checkCanInstallExtras();
	bool canDemo   = checkCanInstallDemo();
	bool canHota   = checkCanInstallHota();
	bool canWog	= checkCanInstallWog();
	bool canTow	= checkCanInstallTow();
	bool canFod	= checkCanInstallFod();

	ui->buttonPresetLanguage->setVisible(canTrans);
	ui->buttonPresetExtras->setVisible(canExtras);
	ui->buttonPresetDemo->setVisible(canDemo);
	ui->buttonPresetHota->setVisible(canHota);
	ui->buttonPresetWog->setVisible(canWog);
	ui->buttonPresetTow->setVisible(canTow);
	ui->buttonPresetFod->setVisible(canFod);

	ui->labelPresetLanguageDescr->setVisible(canTrans);
	ui->labelPresetExtrasDescr->setVisible(canExtras);
	ui->labelPresetDemoDescr->setVisible(canDemo);
	ui->labelPresetHotaDescr->setVisible(canHota);
	ui->labelPresetWogDescr->setVisible(canWog);
	ui->labelPresetTowDescr->setVisible(canTow);
	ui->labelPresetFodDescr->setVisible(canFod);

	// we can't install anything - either repository checkout is off or all recommended mods are already installed
	if(!canTrans && !canExtras && !canDemo && !canHota && !canWog && !canTow && !canFod)
		exitSetup(false);
}

QString FirstLaunchView::findTranslationModName()
{
	auto * mainWindow = dynamic_cast<MainWindow *>(QApplication::activeWindow());
	auto status = mainWindow->getTranslationStatus();

	if(status == ETranslationStatus::ACTIVE || status == ETranslationStatus::NOT_AVAILABLE)
		return QString();

	QString preferredlanguage = QString::fromStdString(settings["general"]["language"].String());
	return getModView()->getTranslationModName(preferredlanguage);
}

bool FirstLaunchView::checkCanInstallTranslation()
{
	QString modName = findTranslationModName();

	if(modName.isEmpty())
		return false;

	return checkCanInstallMod(modName);
}

bool FirstLaunchView::checkCanInstallExtras()
{
	return checkCanInstallMod("vcmi-extras");
}

bool FirstLaunchView::checkCanInstallDemo()
{
	if(!checkCanInstallMod("demo-support"))
		return false;

	QDir userRoot = pathToQString(VCMIDirs::get().userDataPath());
	QDir dataDir(userRoot.filePath(QStringLiteral("Data")));
	QDir mapsDir(userRoot.filePath(QStringLiteral("Maps")));

	bool hasDemoMap = false;
	QStringList mapFiles = mapsDir.entryList(QDir::Files | QDir::Readable);
	for(const QString &name : mapFiles)
		if(name.compare(QStringLiteral("h3demo.h3m"), Qt::CaseInsensitive) == 0)
		{
			hasDemoMap = true;
			break;
		}
	
	QStringList files = dataDir.entryList(QDir::Files | QDir::Readable);
	for(const QString &name : files)
	{
		if(name.compare(QStringLiteral("H3ab_spr.lod"), Qt::CaseInsensitive) == 0)
		{
			QFileInfo lodInfo(dataDir.filePath(name));
			quint64 fileSize = static_cast<quint64>(lodInfo.size());
			logGlobal->trace("H3ab_spr.lod size: %llu", fileSize);
			if(fileSize < 8000000 && hasDemoMap) // 8 MB + Demo map = Merged Windows and MacOS Demo
				return true;
		}
	}
	return false;
}

bool FirstLaunchView::checkCanInstallHota()
{
	return checkCanInstallMod("hota");
}

bool FirstLaunchView::checkCanInstallWog()
{
	return checkCanInstallMod("wake-of-gods");
}

bool FirstLaunchView::checkCanInstallTow()
{
	return checkCanInstallMod("tides-of-war");
}

bool FirstLaunchView::checkCanInstallFod()
{
	return checkCanInstallMod("fallen-of-the-depth");
}

CModListView * FirstLaunchView::getModView()
{
	auto * mainWindow = dynamic_cast<MainWindow *>(QApplication::activeWindow());

	assert(mainWindow);
	if(!mainWindow)
		return nullptr;

	return mainWindow->getModView();
}

bool FirstLaunchView::checkCanInstallMod(const QString & modID)
{
	return getModView() && getModView()->isModAvailable(modID);
}

void FirstLaunchView::on_pushButtonPresetBack_clicked()
{
	activateTabHeroesData();
}

void FirstLaunchView::on_pushButtonPresetNext_clicked()
{
	QStringList modsToInstall;

	if(ui->buttonPresetLanguage->isChecked() && checkCanInstallTranslation())
		modsToInstall.push_back(findTranslationModName());

	if(ui->buttonPresetExtras->isChecked() && checkCanInstallExtras())
		modsToInstall.push_back("vcmi-extras");

	if(ui->buttonPresetDemo->isChecked() && checkCanInstallDemo())
		modsToInstall.push_back("demo-support");

	if(ui->buttonPresetWog->isChecked() && checkCanInstallWog())
		modsToInstall.push_back("wake-of-gods");

	if(ui->buttonPresetHota->isChecked() && checkCanInstallHota())
		modsToInstall.push_back("hota");

	if(ui->buttonPresetTow->isChecked() && checkCanInstallTow())
		modsToInstall.push_back("tides-of-war");

	if(ui->buttonPresetFod->isChecked() && checkCanInstallFod())
		modsToInstall.push_back("fallen-of-the-depth");

	bool goToMods = !modsToInstall.empty();
	exitSetup(goToMods);

	for(auto const & modName : modsToInstall)
		getModView()->doInstallMod(modName);
}

void FirstLaunchView::on_pushButtonDiscord_clicked()
{
	QDesktopServices::openUrl(QUrl("https://discord.gg/chBT42V"));
}

void FirstLaunchView::on_pushButtonGithub_clicked()
{
	QDesktopServices::openUrl(QUrl("https://github.com/vcmi/vcmi"));
}
