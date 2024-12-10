/*
 * StartGameTab.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "StartGameTab.h"
#include "ui_StartGameTab.h"

#include "../mainwindow_moc.h"
#include "../main.h"
#include "../updatedialog_moc.h"

#include "../modManager/cmodlistview_moc.h"

#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/VCMIDirs.h"

StartGameTab::StartGameTab(QWidget * parent)
	: QWidget(parent)
	, ui(new Ui::StartGameTab)
{
	ui->setupUi(this);

	ui->buttonGameResume->setIcon(QIcon{":/icons/menu-game.png"}); //TODO: different icon?
	ui->buttonGameStart->setIcon(QIcon{":/icons/menu-game.png"});
	ui->buttonGameEditor->setIcon(QIcon{":/icons/menu-editor.png"});

	refreshState();

	ui->buttonGameResume->setVisible(false); // TODO: implement
	ui->buttonPresetExport->setVisible(false); // TODO: implement
	ui->buttonPresetImport->setVisible(false); // TODO: implement

#ifndef ENABLE_EDITOR
	ui->buttonGameEditor->hide();
#endif
}

StartGameTab::~StartGameTab()
{
	delete ui;
}

MainWindow * StartGameTab::getMainWindow()
{
	foreach(QWidget *w, qApp->allWidgets())
		if(QMainWindow* mainWin = qobject_cast<QMainWindow*>(w))
			return dynamic_cast<MainWindow *>(mainWin);
	return nullptr;
}

void StartGameTab::refreshState()
{
	refreshGameData();
	refreshUpdateStatus(EGameUpdateStatus::NOT_CHECKED);//TODO - follow automatic check on startup setting
	refreshTranslation(getMainWindow()->getTranslationStatus());
	refreshPresets();
	refreshMods();
}

void StartGameTab::refreshPresets()
{
	QSignalBlocker blocker(ui->comboBoxModPresets);

	QStringList allPresets = getMainWindow()->getModView()->getAllPresets();
	ui->comboBoxModPresets->clear();
	ui->comboBoxModPresets->addItems(allPresets);
	ui->comboBoxModPresets->setCurrentText(getMainWindow()->getModView()->getActivePreset());
	ui->buttonPresetDelete->setVisible(allPresets.size() > 1);
}

void StartGameTab::refreshGameData()
{
	// Some players are using pirated version of the game with some of the files missing
	// leading to broken town hall menu (and possibly other dialogs)
	// Provide diagnostics to indicate problem with chair-monitor adaptor layer and not with VCMI
	static constexpr std::array potentiallyMissingFiles = {
		"Data/TpThBkDg.bmp",
		"Data/TpThBkFr.bmp",
		"Data/TpThBkIn.bmp",
		"Data/TpThBkNc.bmp",
		"Data/TpThBkSt.bmp",
		"Data/TpThBRrm.bmp",
		"Data/TpThBkCs.bmp",
		"Data/TpThBkRm.bmp",
		"Data/TpThBkTw.bmp",
	};

	// Some players for some reason don't have AB expansion campaign files
	static constexpr std::array armaggedonBladeCampaigns = {
		"DATA/AB",
		"DATA/BLOOD",
		"DATA/SLAYER",
		"DATA/FESTIVAL",
		"DATA/FIRE",
		"DATA/FOOL",
	};

	bool missingSoundtrack = !CResourceHandler::get()->existsResource(AudioPath::builtin("Music/MainMenu"));
	bool missingVideoFiles = !CResourceHandler::get()->existsResource(VideoPath::builtin("Video/H3Intro"));
	bool missingGameFiles = false;
	bool missingCampaings = false;

	for (const auto & filename : potentiallyMissingFiles)
		missingGameFiles &= !CResourceHandler::get()->existsResource(ImagePath::builtin(filename));

	for (const auto & filename : armaggedonBladeCampaigns)
		missingCampaings &= !CResourceHandler::get()->existsResource(ResourcePath(filename, EResType::CAMPAIGN));

	ui->labelMissingCampaigns->setVisible(missingCampaings);
	ui->labelMissingFiles->setVisible(missingGameFiles);
	ui->labelMissingVideo->setVisible(missingVideoFiles);
	ui->labelMissingSoundtrack->setVisible(missingSoundtrack);

	ui->buttonMissingCampaignsHelp->setVisible(missingCampaings);
	ui->buttonMissingFilesHelp->setVisible(missingGameFiles);
	ui->buttonMissingVideoHelp->setVisible(missingVideoFiles);
	ui->buttonMissingSoundtrackHelp->setVisible(missingSoundtrack);
}

void StartGameTab::refreshTranslation(ETranslationStatus status)
{
	ui->buttonInstallTranslation->setVisible(status == ETranslationStatus::NOT_INSTALLLED);
	ui->buttonInstallTranslationHelp->setVisible(status == ETranslationStatus::NOT_INSTALLLED);

	ui->buttonActivateTranslation->setVisible(status == ETranslationStatus::NOT_INSTALLLED);
	ui->buttonActivateTranslationHelp->setVisible(status == ETranslationStatus::NOT_INSTALLLED);
}

void StartGameTab::refreshMods()
{
	constexpr int chroniclesCount = 8;
	QStringList updateableMods = getMainWindow()->getModView()->getUpdateableMods();
	QStringList chroniclesMods = getMainWindow()->getModView()->getInstalledChronicles();

	ui->buttonUpdateMods->setText(tr("Update %n mods", "", updateableMods.size()));
	ui->buttonUpdateMods->setVisible(!updateableMods.empty());
	ui->buttonUpdateModsHelp->setVisible(!updateableMods.empty());

	ui->labelChronicles->setText(tr("Heroes Chronicles:\n%n/%1 installed", "", chroniclesMods.size()).arg(chroniclesCount));
	ui->labelChronicles->setVisible(chroniclesMods.size() != chroniclesCount);
	ui->buttonChroniclesHelp->setVisible(chroniclesMods.size() != chroniclesCount);
}

void StartGameTab::refreshUpdateStatus(EGameUpdateStatus status)
{
	QString availableVersion; // TODO

	ui->labelTitleEngine->setText("VCMI " VCMI_VERSION_STRING);
	ui->buttonUpdateCheck->setVisible(status == EGameUpdateStatus::NOT_CHECKED);
	ui->labelUpdateNotFound->setVisible(status == EGameUpdateStatus::NO_UPDATE);
	ui->labelUpdateAvailable->setVisible(status == EGameUpdateStatus::UPDATE_AVAILABLE);
	ui->buttonOpenChangelog->setVisible(status == EGameUpdateStatus::UPDATE_AVAILABLE);
	ui->buttonOpenDownloads->setVisible(status == EGameUpdateStatus::UPDATE_AVAILABLE);

	if (status == EGameUpdateStatus::UPDATE_AVAILABLE)
		ui->labelUpdateAvailable->setText(tr("Update to %1 available").arg(availableVersion));
}

void StartGameTab::on_buttonGameStart_clicked()
{
	getMainWindow()->hide();
	startGame({});
}

void StartGameTab::on_buttonOpenChangelog_clicked()
{
	QDesktopServices::openUrl(QUrl("https://vcmi.eu/ChangeLog/"));
}

void StartGameTab::on_buttonOpenDownloads_clicked()
{
	QDesktopServices::openUrl(QUrl("https://vcmi.eu/download/"));
}

void StartGameTab::on_buttonUpdateCheck_clicked()
{
	UpdateDialog::showUpdateDialog(true);
}

void StartGameTab::on_buttonGameEditor_clicked()
{
	getMainWindow()->hide();
	startEditor({});
}

void StartGameTab::on_buttonImportFiles_clicked()
{
	const auto & importFunctor = [this]
	{
#ifndef VCMI_MOBILE
		QString filter =
			tr("All supported files") + " (*.h3m *.vmap *.h3c *.vcmp *.zip *.json *.exe);;" +
			tr("Maps") + " (*.h3m *.vmap);;" +
			tr("Campaigns") + " (*.h3c *.vcmp);;" +
			tr("Configs") + " (*.json);;" +
			tr("Mods") + " (*.zip);;" +
			tr("Gog files") + " (*.exe)";
#else
		//Workaround for sometimes incorrect mime for some extensions (e.g. for exe)
		QString filter = tr("All files (*.*)");
#endif
		QStringList files = QFileDialog::getOpenFileNames(this, tr("Select files (configs, mods, maps, campaigns, gog files) to install..."), QDir::homePath(), filter);

		for(const auto & file : files)
			getMainWindow()->manualInstallFile(file);
	};

	// iOS can't display modal dialogs when called directly on button press
	// https://bugreports.qt.io/browse/QTBUG-98651
	QTimer::singleShot(0, this, importFunctor);
}

void StartGameTab::on_buttonInstallTranslation_clicked()
{
	if (getMainWindow()->getTranslationStatus() == ETranslationStatus::NOT_INSTALLLED)
	{
		QString preferredlanguage = QString::fromStdString(settings["general"]["language"].String());
		QString modName = getMainWindow()->getModView()->getTranslationModName(preferredlanguage);
		getMainWindow()->getModView()->doInstallMod(modName);
	}
}

void StartGameTab::on_buttonActivateTranslation_clicked()
{
	QString preferredlanguage = QString::fromStdString(settings["general"]["language"].String());
	QString modName = getMainWindow()->getModView()->getTranslationModName(preferredlanguage);
	getMainWindow()->getModView()->enableModByName(modName);
}

void StartGameTab::on_buttonUpdateMods_clicked()
{
	QStringList updateableMods = getMainWindow()->getModView()->getUpdateableMods();

	getMainWindow()->switchToModsTab();

	for (const auto & modName : updateableMods)
		getMainWindow()->getModView()->doUpdateMod(modName);
}

void StartGameTab::on_buttonHelpImportFiles_clicked()
{
	QString message = tr(
		"This option allows you to import additional data files into your VCMI installation. "
		"At the moment, following options are supported:\n\n"
		" - Heroes III Maps (.h3m or .vmap).\n"
		" - Heroes III Campaigns (.h3c or .vcmp).\n"
		" - Heroes III Chronicles using offline backup installer from GOG.com (.exe).\n"
		" - VCMI mods in zip format (.zip)\n"
		" - VCMI configuration files (.json)\n"
	);

	QMessageBox::information(this, ui->buttonImportFiles->text(), message);
}

void StartGameTab::on_buttonInstallTranslationHelp_clicked()
{
	QString message = tr(
		"Your Heroes III version uses different language. "
		"VCMI provides translations of the game into various languages that you can use. "
		"Use this option to automatically install such translation to your language."
	);
	QMessageBox::information(this, ui->buttonInstallTranslation->text(), message);
}

void StartGameTab::on_buttonActivateTranslationHelp_clicked()
{
	QString message = tr(
		"Translation of Heroes III into your language is installed, but has been turned off. "
		"Use this option to enable it."
	);

	QMessageBox::information(this, ui->buttonActivateTranslation->text(), message);
}

void StartGameTab::on_buttonUpdateModsHelp_clicked()
{
	QString message = tr(
		"A new version of some of the mods that you have installed is now available in mod repository. "
		"Use this option to automatically update all your mods to latest version.\n\n"
		"WARNING: In some cases, updated versions of mods may not be compatible with your existing saves. "
		"You many want to postpone mod update until you finish any of your ongoing games."
		);

	QMessageBox::information(this, ui->buttonUpdateMods->text(), message);
}

void StartGameTab::on_buttonChroniclesHelp_clicked()
{
	QString message = tr(
		"If you own Heroes Chronicles on gog.com, you can use offline backup installers provided by gog "
		"to import Heroes Chronicles data into VCMI as custom campaigns.\n"
		"To import Heroes Chronicles, download offline backup installer of each chronicle that you wish to install, "
		"select 'Import files' option and select downloaded file. "
		"This will generate and install mod for VCMI that contains imported chronicles"
	);

	QMessageBox::information(this, ui->labelChronicles->text(), message);
}

void StartGameTab::on_buttonMissingSoundtrackHelp_clicked()
{
	QString message = tr(
		"VCMI has detected that Heroes III music files are missing from your installation. "
		"VCMI will run, but in-game music will not be available.\n\n"
		"To resolve this problem, please copy missing mp3 files from Heroes III to VCMI data files directory manually "
		"or reinstall VCMI and re-import Heroes III data files"
	);
	QMessageBox::information(this, ui->labelMissingSoundtrack->text(), message);
}

void StartGameTab::on_buttonMissingVideoHelp_clicked()
{
	QString message = tr(
		"VCMI has detected that Heroes III video files are missing from your installation. "
		"VCMI will run, but in-game cutscenes will not be available.\n\n"
		"To resolve this problem, please copy VIDEO.VID file from Heroes III to VCMI data files directory manually "
		"or reinstall VCMI and re-import Heroes III data files"
		);
	QMessageBox::information(this, ui->labelMissingVideo->text(), message);
}

void StartGameTab::on_buttonMissingFilesHelp_clicked()
{
	QString message = tr(
		"VCMI has detected that some of Heroes III data files are missing from your installation. "
		"You may attempt to run VCMI, but game may not work as expected or crash.\n\n"
		"To resolve this problem, please reinstall game and reimport data files using supported version of Heroes III. "
		"VCMI requires Heroes III: Shadow of Death or Complete Edition to run, which you can get (for example) from gog.com"
	);
	QMessageBox::information(this, ui->labelMissingFiles->text(), message);
}

void StartGameTab::on_buttonMissingCampaignsHelp_clicked()
{
	QString message = tr(
		"VCMI has detected that some of Heroes III: Armageddon's Blade data files are missing from your installation. "
		"VCMI will work, but Armageddon's Blade campaigns will not be available.\n\n"
		"To resolve this problem, please copy missing data files from Heroes III to VCMI data files directory manually "
		"or reinstall VCMI and re-import Heroes III data files"
	);
	QMessageBox::information(this, ui->labelMissingCampaigns->text(), message);
}

void StartGameTab::on_buttonPresetExport_clicked()
{
	// TODO
}

void StartGameTab::on_buttonPresetImport_clicked()
{
	// TODO
}

void StartGameTab::on_buttonPresetNew_clicked()
{
	bool ok;
	QString presetName = QInputDialog::getText(
		this,
		ui->buttonPresetNew->text(),
		tr("Enter preset name:"),
		QLineEdit::Normal,
		QString(),
		&ok);

	if (ok && !presetName.isEmpty())
	{
		getMainWindow()->getModView()->createNewPreset(presetName);
		getMainWindow()->getModView()->activatePreset(presetName);
		refreshPresets();
	}
}

void StartGameTab::on_buttonPresetDelete_clicked()
{
	QString activePresetBefore = getMainWindow()->getModView()->getActivePreset();
	QStringList allPresets = getMainWindow()->getModView()->getAllPresets();

	allPresets.removeAll(activePresetBefore);
	if (!allPresets.empty())
	{
		getMainWindow()->getModView()->activatePreset(allPresets.front());
		getMainWindow()->getModView()->deletePreset(activePresetBefore);
		refreshPresets();
	}
}

void StartGameTab::on_comboBoxModPresets_currentTextChanged(const QString &presetName)
{
	getMainWindow()->getModView()->activatePreset(presetName);
}

void StartGameTab::on_buttonPresetRename_clicked()
{
	QString currentName = getMainWindow()->getModView()->getActivePreset();

	bool ok;
	QString newName = QInputDialog::getText(
		this,
		ui->buttonPresetNew->text(),
		tr("Rename preset '%1' to:").arg(currentName),
		QLineEdit::Normal,
		currentName,
		&ok);

	if (ok && !newName.isEmpty())
	{
		getMainWindow()->getModView()->renamePreset(currentName, newName);
		refreshPresets();
	}
}
