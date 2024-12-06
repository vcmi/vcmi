#include "StartGameTab.h"
#include "ui_StartGameTab.h"

#include "../mainwindow_moc.h"
#include "../main.h"

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
	refreshUpdateStatus(EGameUpdateStatus::NOT_CHECKED);//TODO
	refreshTranslation(getMainWindow()->getTranslationStatus());
	refreshMods();
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
	ui->buttonEngine->setText("VCMI " VCMI_VERSION_STRING);
	ui->buttonUpdateCheck->setVisible(status == EGameUpdateStatus::NOT_CHECKED);
	ui->labelUpdateNotFound->setVisible(status == EGameUpdateStatus::NO_UPDATE);
	ui->labelUpdateAvailable->setVisible(status == EGameUpdateStatus::UPDATE_AVAILABLE);
	ui->buttonOpenChangelog->setVisible(status == EGameUpdateStatus::UPDATE_AVAILABLE);
	ui->buttonOpenDownloads->setVisible(status == EGameUpdateStatus::UPDATE_AVAILABLE);
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
	// TODO: implement
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

	for (const auto & modName : updateableMods)
		getMainWindow()->getModView()->doInstallMod(modName);
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
	QMessageBox::information(this, ui->buttonInstallTranslation->text(), tr("TODO")); // TODO
}

void StartGameTab::on_buttonActivateTranslationHelp_clicked()
{
	QMessageBox::information(this, ui->buttonActivateTranslation->text(), tr("TODO")); // TODO
}

void StartGameTab::on_buttonUpdateModsHelp_clicked()
{
	QMessageBox::information(this, ui->buttonUpdateMods->text(), tr("TODO")); // TODO
}

void StartGameTab::on_buttonChroniclesHelp_clicked()
{
	QMessageBox::information(this, ui->labelChronicles->text(), tr("TODO")); // TODO
}

void StartGameTab::on_buttonMissingSoundtrackHelp_clicked()
{
	QMessageBox::information(this, ui->labelMissingSoundtrack->text(), tr("TODO")); // TODO
}

void StartGameTab::on_buttonMissingVideoHelp_clicked()
{
	QMessageBox::information(this, ui->labelMissingVideo->text(), tr("TODO")); // TODO
}

void StartGameTab::on_buttonMissingFilesHelp_clicked()
{
	QMessageBox::information(this, ui->labelMissingCampaigns->text(), tr("TODO")); // TODO
}

void StartGameTab::on_buttonMissingCampaignsHelp_clicked()
{
	QMessageBox::information(this, ui->labelMissingCampaigns->text(), tr("TODO")); // TODO
}
