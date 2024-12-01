#include "StartGameTab.h"
#include "ui_StartGameTab.h"

#include "../mainwindow_moc.h"
#include "../main.h"

#include "../../lib/filesystem/Filesystem.h"

StartGameTab::StartGameTab(QWidget * parent)
	: QWidget(parent)
	, ui(new Ui::StartGameTab)
{
	ui->setupUi(this);

	refreshState();
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

	bool updateAvailable = false;
	bool checkedForUpdate = false;

	bool missingSoundtrack = !CResourceHandler::get()->existsResource(AudioPath::builtin("Music/MainMenu"));
	bool missingVideoFiles = !CResourceHandler::get()->existsResource(VideoPath::builtin("Video/H3Intro"));
	bool missingGameFiles = false;
	bool missingCampaings = false;

	for (const auto & filename : potentiallyMissingFiles)
		missingGameFiles &= !CResourceHandler::get()->existsResource(ImagePath::builtin(filename));

	for (const auto & filename : armaggedonBladeCampaigns)
		missingCampaings &= !CResourceHandler::get()->existsResource(ResourcePath(filename, EResType::CAMPAIGN));

	ui->buttonEngine->setText("VCMI " VCMI_VERSION_STRING);
	ui->buttonUpdateCheck->setVisible(!checkedForUpdate);
	ui->labelUpdateAvailable->setVisible(checkedForUpdate && updateAvailable);
	ui->labelUpdateNotFound->setVisible(checkedForUpdate && !updateAvailable);
	ui->buttonOpenChangelog->setVisible(checkedForUpdate && updateAvailable);
	ui->buttonOpenDownloads->setVisible(checkedForUpdate && updateAvailable);

	ui->labelMissingCampaigns->setVisible(missingCampaings);
	ui->labelMissingFiles->setVisible(missingGameFiles);
	ui->labelMissingVideo->setVisible(missingVideoFiles);
	ui->labelMissingSoundtrack->setVisible(missingSoundtrack);

	ui->buttonMissingCampaignsHelp->setVisible(missingCampaings);
	ui->buttonMissingFilesHelp->setVisible(missingGameFiles);
	ui->buttonMissingVideoHelp->setVisible(missingVideoFiles);
	ui->buttonMissingSoundtrackHelp->setVisible(missingSoundtrack);
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

