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
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/Languages.h"
#include "../../lib/VCMIDirs.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../helper.h"
#include "../languages.h"

#ifdef ENABLE_INNOEXTRACT
#include "cli/extract.hpp"
#endif

#ifdef VCMI_IOS
#include "ios/selectdirectory.h"

#include "iOS_utils.h"
#elif defined(VCMI_ANDROID)
#include <QAndroidJniObject>
#include <QtAndroid>

static FirstLaunchView * thiz;
extern "C" JNIEXPORT void JNICALL Java_eu_vcmi_vcmi_NativeMethods_heroesDataUpdate(JNIEnv * env, jclass cls)
{
	thiz->heroesDataUpdate();
}
#endif

FirstLaunchView::FirstLaunchView(QWidget * parent)
	: QWidget(parent)
	, ui(new Ui::FirstLaunchView)
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
#ifdef VCMI_ANDROID
	thiz = this;
	QtAndroid::androidActivity().callMethod<void>("copyHeroesData");
#else
	// iOS can't display modal dialogs when called directly on button press
	// https://bugreports.qt.io/browse/QTBUG-98651
	QTimer::singleShot(0, this, [this]{ copyHeroesData(); });
#endif
}

void FirstLaunchView::on_pushButtonGogInstall_clicked()
{
	// iOS can't display modal dialogs when called directly on button press
	// https://bugreports.qt.io/browse/QTBUG-98651
	QTimer::singleShot(0, this, &FirstLaunchView::extractGogData);
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
			copyHeroesData(installPath);
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

void FirstLaunchView::exitSetup()
{
	if(auto * mainWindow = dynamic_cast<MainWindow *>(QApplication::activeWindow()))
		mainWindow->exitSetup();
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

#ifdef VCMI_ANDROID
	// selecting directory with ACTION_OPEN_DOCUMENT_TREE is available only since API level 21
	const bool canUseDataCopy = QtAndroid::androidSdkVersion() >= 21;
#elif defined(VCMI_IOS)
	// selecting directory through UIDocumentPickerViewController is available only since iOS 13
	const bool canUseDataCopy = iOS_utils::isOsVersionAtLeast(13);
#else
	const bool canUseDataCopy = true;
#endif

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
	QString gogPath = QSettings("HKEY_LOCAL_MACHINE\\SOFTWARE\\GOG.com\\Games\\1207658787", QSettings::NativeFormat).value("path").toString();
	if(!gogPath.isEmpty())
		return gogPath;

	QString cdPath = QSettings("HKEY_LOCAL_MACHINE\\SOFTWARE\\New World Computing\\Heroes of Might and Magic® III\\1.0", QSettings::NativeFormat).value("AppPath").toString();
	if(!cdPath.isEmpty())
		return cdPath;
#endif
	return QString{};
}

void FirstLaunchView::extractGogData()
{
#ifdef ENABLE_INNOEXTRACT
	auto fileSelection = [this](QString type, QString filter, QString startPath = {}) {
		QString titleSel = tr("Select %1 file...", "param is file extension").arg(filter);
		QString titleErr = tr("You have to select %1 file!", "param is file extension").arg(filter);
#if defined(VCMI_MOBILE)
		filter = tr("GOG file (*.*)");
		QMessageBox::information(this, tr("File selection"), titleSel);
#endif
		QString file = QFileDialog::getOpenFileName(this, titleSel, startPath.isEmpty() ? QDir::homePath() : startPath, filter);
		if(file.isEmpty())
			return QString{};
		else if(!file.endsWith("." + type, Qt::CaseInsensitive))
		{
			QMessageBox::critical(this, tr("Invalid file selected"), titleErr);
			return QString{};
		}

		return file;
	};

	QString fileExe = fileSelection("exe", tr("GOG installer") + " (*.exe)");
	if(fileExe.isEmpty())
		return;
	QString fileBin = fileSelection("bin", tr("GOG data") + " (*.bin)", QFileInfo(fileExe).absolutePath());
	if(fileBin.isEmpty())
		return;

	ui->progressBarGog->setVisible(true);
	ui->pushButtonGogInstall->setVisible(false);
	setEnabled(false);

	QTimer::singleShot(100, this, [this, fileExe, fileBin](){ // background to make sure FileDialog is closed...
		QDir tempDir(pathToQString(VCMIDirs::get().userDataPath()));
		tempDir.mkdir("tmp");
		if(!tempDir.cd("tmp"))
			return; // should not happen - but avoid deleting wrong folder in any case

		QString tmpFileExe = tempDir.filePath("h3_gog.exe");
		QFile(fileExe).copy(tmpFileExe);
		QFile(fileBin).copy(tempDir.filePath("h3_gog-1.bin"));

		::extract_options o;
		o.extract = true;

		// standard settings
		o.gog_galaxy = true;
		o.codepage = 0U;
		o.output_dir = tempDir.path().toStdString();
		o.extract_temp = true;
		o.extract_unknown = true;
		o.filenames.set_expand(true);

		o.preserve_file_times = true; // also correctly closes file -> without it: on Windows the files are not written completly
		
		process_file(tmpFileExe.toStdString(), o, [this](float progress) {
			ui->progressBarGog->setValue(progress * 100);
			qApp->processEvents();
		});

		ui->progressBarGog->setVisible(false);
		ui->pushButtonGogInstall->setVisible(true);
		setEnabled(true);

		QStringList dirData = tempDir.entryList({"data"}, QDir::Filter::Dirs);
		if(dirData.empty() || QDir(tempDir.filePath(dirData.front())).entryList({"*.lod"}, QDir::Filter::Files).empty())
		{
			QMessageBox::critical(this, tr("No Heroes III data!"), tr("Selected files do not contain Heroes III data!"), QMessageBox::Ok, QMessageBox::Ok);
			tempDir.removeRecursively();
			return;
		}
		copyHeroesData(tempDir.path(), true);

		tempDir.removeRecursively();
	});
#endif
}

void FirstLaunchView::copyHeroesData(const QString & path, bool move)
{
	QDir sourceRoot{path};

#ifdef VCMI_IOS
	// TODO: Qt 6.5 can select directories https://codereview.qt-project.org/c/qt/qtbase/+/446449
	SelectDirectory iosDirectorySelector;
	if(path.isEmpty())
		sourceRoot.setPath(iosDirectorySelector.getExistingDirectory());
#else
	if(path.isEmpty())
		sourceRoot.setPath(QFileDialog::getExistingDirectory(this, {}, {}, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks));
#endif

	if(!sourceRoot.exists())
		return;

	if (sourceRoot.dirName().compare("data", Qt::CaseInsensitive) == 0)
	{
		// We got Data folder. Possibly user selected "Data" folder of Heroes III install. Check whether valid data might exist 1 level above

		QStringList dirData = sourceRoot.entryList({"data"}, QDir::Filter::Dirs);
		if (dirData.empty())
		{
			// This is "Data" folder without any "Data" folders inside. Try to check for data 1 level above
			sourceRoot.cdUp();
		}
	}

	QStringList dirData = sourceRoot.entryList({"data"}, QDir::Filter::Dirs);
	QStringList dirMaps = sourceRoot.entryList({"maps"}, QDir::Filter::Dirs);
	QStringList dirMp3 = sourceRoot.entryList({"mp3"}, QDir::Filter::Dirs);

	const auto noDataMessage = tr("Failed to detect valid Heroes III data in chosen directory.\nPlease select directory with installed Heroes III data.");
	if(dirData.empty())
	{
		QMessageBox::critical(this, tr("Heroes III data not found!"), noDataMessage);
		return;
	}

	QDir sourceData = sourceRoot.filePath(dirData.front());
	QStringList roeFiles = sourceData.entryList({"*.lod"}, QDir::Filter::Files);
	QStringList sodFiles = sourceData.entryList({"H3ab*.lod"}, QDir::Filter::Files);
	QStringList hdFiles = sourceData.entryList({"*.pak"}, QDir::Filter::Files);

	if(sodFiles.empty())
	{
		if (roeFiles.empty())
		{
			// Directory structure is correct (Data/Maps/Mp3) but no .lod archives that should be present in any install
			QMessageBox::critical(this, tr("Heroes III data not found!"), noDataMessage);
			return;
		}

		if (!hdFiles.empty())
		{
			// HD Edition contains only RoE data so we can't use even unmodified files from it
			QMessageBox::critical(this, tr("Heroes III data not found!"), tr("Heroes III: HD Edition files are not supported by VCMI.\nPlease select directory with Heroes III: Complete Edition or Heroes III: Shadow of Death."));
			return;
		}

		// RoE or some other unsupported edition. Demo version?
		QMessageBox::critical(this, tr("Heroes III data not found!"), tr("Unknown or unsupported Heroes III version found.\nPlease select directory with Heroes III: Complete Edition or Heroes III: Shadow of Death."));
		return;
	}

	QStringList copyDirectories;

	copyDirectories += dirData.front();
	if (!dirMaps.empty())
		copyDirectories += dirMaps.front();

	if (!dirMp3.empty())
		copyDirectories += dirMp3.front();

	QDir targetRoot = pathToQString(VCMIDirs::get().userDataPath());

	for(const QString & dirName : copyDirectories)
	{
		QDir sourceDir = sourceRoot.filePath(dirName);
		QDir targetDir = targetRoot.filePath(dirName);

		if(!targetRoot.exists(dirName))
			targetRoot.mkdir(dirName);

		for(const QString & filename : sourceDir.entryList(QDir::Filter::Files))
		{
			QFile sourceFile(sourceDir.filePath(filename));
			if(move)
				sourceFile.rename(targetDir.filePath(filename));
			else
				sourceFile.copy(targetDir.filePath(filename));
		}
	}

	heroesDataUpdate();
}

// Tab Mod Preset
void FirstLaunchView::modPresetUpdate()
{
	bool translationExists = !findTranslationModName().isEmpty();

	ui->labelPresetLanguageDescr->setVisible(translationExists);
	ui->buttonPresetLanguage->setVisible(translationExists);

	ui->buttonPresetLanguage->setVisible(checkCanInstallTranslation());
	ui->buttonPresetExtras->setVisible(checkCanInstallExtras());
	ui->buttonPresetHota->setVisible(checkCanInstallHota());
	ui->buttonPresetWog->setVisible(checkCanInstallWog());

	ui->labelPresetLanguageDescr->setVisible(checkCanInstallTranslation());
	ui->labelPresetExtrasDescr->setVisible(checkCanInstallExtras());
	ui->labelPresetHotaDescr->setVisible(checkCanInstallHota());
	ui->labelPresetWogDescr->setVisible(checkCanInstallWog());

	// we can't install anything - either repository checkout is off or all recommended mods are already installed
	if (!checkCanInstallTranslation() && !checkCanInstallExtras() && !checkCanInstallHota() && !checkCanInstallWog())
		exitSetup();
}

QString FirstLaunchView::findTranslationModName()
{
	if (!getModView())
		return QString();

	QString preferredlanguage = QString::fromStdString(settings["general"]["language"].String());
	QString installedlanguage = QString::fromStdString(settings["session"]["language"].String());

	if (preferredlanguage == installedlanguage)
		return QString();

	return getModView()->getTranslationModName(preferredlanguage);
}

bool FirstLaunchView::checkCanInstallTranslation()
{
	QString modName = findTranslationModName();

	if(modName.isEmpty())
		return false;

	return checkCanInstallMod(modName);
}

bool FirstLaunchView::checkCanInstallWog()
{
	return checkCanInstallMod("wake-of-gods");
}

bool FirstLaunchView::checkCanInstallHota()
{
	return checkCanInstallMod("hota");
}

bool FirstLaunchView::checkCanInstallExtras()
{
	return checkCanInstallMod("vcmi-extras");
}

CModListView * FirstLaunchView::getModView()
{
	auto * mainWindow = dynamic_cast<MainWindow *>(QApplication::activeWindow());

	assert(mainWindow);
	if (!mainWindow)
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

	if (ui->buttonPresetLanguage->isChecked() && checkCanInstallTranslation())
		modsToInstall.push_back(findTranslationModName());

	if (ui->buttonPresetExtras->isChecked() && checkCanInstallExtras())
		modsToInstall.push_back("vcmi-extras");

	if (ui->buttonPresetWog->isChecked() && checkCanInstallWog())
		modsToInstall.push_back("wake-of-gods");

	if (ui->buttonPresetHota->isChecked() && checkCanInstallHota())
		modsToInstall.push_back("hota");

	exitSetup();

	for (auto const & modName : modsToInstall)
		getModView()->doInstallMod(modName);
}

void FirstLaunchView::on_pushButtonDiscord_clicked()
{
	QDesktopServices::openUrl(QUrl("https://discord.gg/chBT42V"));
}

void FirstLaunchView::on_pushButtonSlack_clicked()
{
	QDesktopServices::openUrl(QUrl("https://slack.vcmi.eu/"));
}

void FirstLaunchView::on_pushButtonGithub_clicked()
{
	QDesktopServices::openUrl(QUrl("https://github.com/vcmi/vcmi"));
}

