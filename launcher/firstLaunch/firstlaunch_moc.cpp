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
#include "../languages.h"

FirstLaunchView::FirstLaunchView(QWidget * parent)
	: QWidget(parent)
	, ui(new Ui::FirstLaunchView)
{
	ui->setupUi(this);

	enterSetup();
	activateTabLanguage();

	ui->lineEditDataSystem->setText(boost::filesystem::absolute(VCMIDirs::get().dataPaths().front()).c_str());
	ui->lineEditDataUser->setText(boost::filesystem::absolute(VCMIDirs::get().userDataPath()).c_str());
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

void FirstLaunchView::on_buttonTabFinish_clicked()
{
	activateTabFinish();
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
		Languages::fillLanguages(ui->listWidgetLanguage);
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
	copyHeroesData();
}

void FirstLaunchView::on_pushButtonDataHelp_clicked()
{
	static const QUrl vcmibuilderWiki("https://wiki.vcmi.eu/Installation_on_Linux#Installing_Heroes_III_data_files");
	QDesktopServices::openUrl(vcmibuilderWiki);
}

void FirstLaunchView::on_comboBoxLanguage_currentIndexChanged(int index)
{
	forceHeroesLanguage(ui->comboBoxLanguage->itemData(index).toString());
}

void FirstLaunchView::enterSetup()
{
	// TODO: block all UI except FirstLaunchView
	Languages::fillLanguages(ui->listWidgetLanguage);
	Languages::fillLanguages(ui->comboBoxLanguage);
}

void FirstLaunchView::setSetupProgress(int progress)
{
	int value = std::max(progress, ui->setupProgressBar->value());

	ui->setupProgressBar->setValue(value);

	ui->buttonTabLanguage->setDisabled(value < 1);
	ui->buttonTabHeroesData->setDisabled(value < 2);
	ui->buttonTabModPreset->setDisabled(value < 3);
	ui->buttonTabFinish->setDisabled(value < 4);
}

void FirstLaunchView::activateTabLanguage()
{
	setSetupProgress(1);
	ui->installerTabs->setCurrentIndex(0);
	ui->buttonTabLanguage->setChecked(true);
	ui->buttonTabHeroesData->setChecked(false);
	ui->buttonTabModPreset->setChecked(false);
	ui->buttonTabFinish->setChecked(false);
}

void FirstLaunchView::activateTabHeroesData()
{
	setSetupProgress(2);
	ui->installerTabs->setCurrentIndex(1);
	ui->buttonTabLanguage->setChecked(false);
	ui->buttonTabHeroesData->setChecked(true);
	ui->buttonTabModPreset->setChecked(false);
	ui->buttonTabFinish->setChecked(false);

	if(!hasVCMIBuilderScript)
	{
		ui->pushButtonDataHelp->hide();
		ui->labelDataHelp->hide();
	}
	heroesDataUpdate();
}

void FirstLaunchView::activateTabModPreset()
{
	setSetupProgress(3);
	ui->installerTabs->setCurrentIndex(2);
	ui->buttonTabLanguage->setChecked(false);
	ui->buttonTabHeroesData->setChecked(false);
	ui->buttonTabModPreset->setChecked(true);
	ui->buttonTabFinish->setChecked(false);

	modPresetUpdate();
}

void FirstLaunchView::activateTabFinish()
{
	setSetupProgress(4);
	ui->installerTabs->setCurrentIndex(3);
	ui->buttonTabLanguage->setChecked(false);
	ui->buttonTabModPreset->setChecked(false);
	ui->buttonTabHeroesData->setChecked(false);
	ui->buttonTabFinish->setChecked(true);
}

void FirstLaunchView::exitSetup()
{
	//TODO: unlock UI, switch to another tab (mods?)
}

// Tab Language
void FirstLaunchView::languageSelected(const QString & selectedLanguage)
{
	Settings node = settings.write["general"]["language"];
	node->String() = selectedLanguage.toStdString();

	if(auto * mainWindow = dynamic_cast<MainWindow *>(qApp->activeWindow()))
		mainWindow->updateTranslation();
}

void FirstLaunchView::heroesDataUpdate()
{
	if(heroesDataDetect())
		heroesDataDetected();
	else
		heroesDataMissing();
}

void FirstLaunchView::heroesDataMissing()
{
	QPalette newPalette = palette();
	newPalette.setColor(QPalette::Base, QColor(200, 50, 50));
	ui->lineEditDataSystem->setPalette(newPalette);
	ui->lineEditDataUser->setPalette(newPalette);

	ui->pushButtonDataSearch->setVisible(true);
	ui->pushButtonDataCopy->setVisible(true);

	ui->labelDataSearch->setVisible(true);
	ui->labelDataCopy->setVisible(true);

	ui->labelDataFound->setVisible(false);

	if(hasVCMIBuilderScript)
	{
		ui->pushButtonDataHelp->setVisible(true);
		ui->labelDataHelp->setVisible(true);
	}
}

void FirstLaunchView::heroesDataDetected()
{
	QPalette newPalette = palette();
	newPalette.setColor(QPalette::Base, QColor(50, 200, 50));
	ui->lineEditDataSystem->setPalette(newPalette);
	ui->lineEditDataUser->setPalette(newPalette);

	ui->pushButtonDataSearch->setVisible(false);
	ui->pushButtonDataCopy->setVisible(false);

	ui->labelDataSearch->setVisible(false);
	ui->labelDataCopy->setVisible(false);

	if(hasVCMIBuilderScript)
	{
		ui->pushButtonDataHelp->setVisible(false);
		ui->labelDataHelp->setVisible(false);
	}

	ui->labelDataFound->setVisible(true);

	heroesLanguageUpdate();
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
	bool heroesDataFound = CResourceHandler::get()->existsResource(ResourceID("DATA/GENRLTXT.TXT"));

	return heroesDataFound;
}

QString FirstLaunchView::heroesLanguageDetect()
{
	CGeneralTextHandler::detectInstallParameters();

	QString language = QString::fromStdString(settings["session"]["language"].String());
	double deviation = settings["session"]["languageDeviation"].Float();

	if(deviation > 0.05)
		return QString();
	return language;
}

void FirstLaunchView::heroesLanguageUpdate()
{
	QString language = heroesLanguageDetect();

	bool success = !language.isEmpty();

	if(!language.isEmpty())
	{
		std::string languageNameEnglish = Languages::getLanguageOptions(language.toStdString()).nameEnglish;
		QString languageName = QApplication::translate( "Languages", languageNameEnglish.c_str());
		QString itemName = tr("Auto (%1)").arg(languageName);

		ui->comboBoxLanguage->insertItem(0, itemName, QString("auto"));
		ui->comboBoxLanguage->setCurrentIndex(0);
	}
	ui->labelDataFailure->setVisible(!success);
	ui->labelDataSuccess->setVisible(success);
	ui->pushButtonDataNext->setEnabled(success);
}

void FirstLaunchView::forceHeroesLanguage(const QString & language)
{
	Settings node = settings.write["general"]["gameDataLanguage"];

	node->String() = language.toStdString();
}

void FirstLaunchView::copyHeroesData()
{
	assert(0); // TODO: test

	QDir dir = QFileDialog::getExistingDirectory(this, "", "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if(!dir.exists())
		return;

	QStringList dirData = dir.entryList({"data"}, QDir::Filter::Dirs);
	QStringList dirMaps = dir.entryList({"maps"}, QDir::Filter::Dirs);
	QStringList dirMp3 = dir.entryList({"mp3"}, QDir::Filter::Dirs);

	if(dirData.empty() || dirMaps.empty() || dirMp3.empty())
		return;

	QStringList lodArchives = QDir(dirData.front()).entryList({"*.lod"}, QDir::Filter::Files);

	if(lodArchives.empty())
		return;

	QStringList copyDirectories = {dirData.front(), dirMaps.front(), dirMp3.front()};

	QDir targetRoot = QString(VCMIDirs::get().userDataPath().c_str());

	for(QDir sourceDir : copyDirectories)
	{
		QString dirName = sourceDir.dirName();
		QDir targetDir = targetRoot.filePath(dirName);

		if(!targetRoot.exists(dirName))
			targetRoot.mkdir(dirName);

		for(QString filename : sourceDir.entryList(QDir::Filter::Files))
		{
			QFile sourceFile(sourceDir.filePath(filename));
			sourceFile.copy(targetDir.filePath(filename));
		}
	}

	heroesDataUpdate();
}

// Tab Mod Preset
void FirstLaunchView::modPresetUpdate()
{
	bool translationExists = !findTranslationModName().isEmpty();

	ui->labelPresetLanguage->setVisible(translationExists);
	ui->toolButtonPresetLanguage->setVisible(translationExists);

	ui->toolButtonPresetLanguage->setEnabled(checkCanInstallTranslation());
	ui->toolButtonPresetExtras->setEnabled(checkCanInstallExtras());
	ui->toolButtonPresetHota->setEnabled(checkCanInstallHota());
	ui->toolButtonPresetWog->setEnabled(checkCanInstallWog());
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
	auto * mainWindow = dynamic_cast<MainWindow *>(qApp->activeWindow());

	assert(mainWindow);
	if (!mainWindow)
		return nullptr;

	return mainWindow->getModView();
}

bool FirstLaunchView::checkCanInstallMod(const QString & modID)
{
	return getModView() && !getModView()->isModInstalled(modID);
}

void FirstLaunchView::installTranslation()
{
	installMod(findTranslationModName());
}

void FirstLaunchView::installWog()
{
	installMod("wake-of-gods");
}

void FirstLaunchView::installHota()
{
	installMod("hota");
}

void FirstLaunchView::instalExtras()
{
	installMod("vcmi-extras");
}

void FirstLaunchView::installMod(const QString & modID)
{
	assert(0); // TODO: test

	return getModView()->doInstallMod(modID);
}

void FirstLaunchView::on_pushButtonPresetBack_clicked()
{
	activateTabHeroesData();
}


void FirstLaunchView::on_pushButtonPresetNext_clicked()
{
	activateTabFinish();
}

