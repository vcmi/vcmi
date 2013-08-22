#include "StdInc.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QProcess>
#include <QDir>

#include "../lib/CConfigHandler.h"
#include "../lib/VCMIDirs.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/logging/CBasicLogConfigurator.h"

void MainWindow::load()
{
	console = new CConsoleHandler;
	CBasicLogConfigurator logConfig(VCMIDirs::get().userCachePath() + "/VCMI_Launcher_log.txt", console);
	logConfig.configureDefault();

	CResourceHandler::initialize();
	CResourceHandler::loadMainFileSystem("config/filesystem.json");

	for (auto & string : VCMIDirs::get().dataPaths())
		QDir::addSearchPath("icons", QString::fromUtf8(string.c_str()) + "/launcher/icons");
	QDir::addSearchPath("icons", QString::fromUtf8(VCMIDirs::get().userDataPath().c_str()) + "/launcher/icons");

	settings.init();
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
	load(); // load FS before UI

	ui->setupUi(this);
	ui->tabListWidget->setCurrentIndex(0);

	connect(ui->tabSelectList, SIGNAL(currentRowChanged(int)),
	        ui->tabListWidget, SLOT(setCurrentIndex(int)));
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::on_startGameButon_clicked()
{
#if defined(Q_OS_WIN)
	QString clientName = "VCMI_Client.exe";
#else
	// TODO: Right now launcher will only start vcmi from system-default locations
	QString clientName = "vcmiclient";
#endif
	startExecutable(clientName);
}

void MainWindow::startExecutable(QString name)
{
	QProcess process;

	// Start the executable
	if (process.startDetached(name))
	{
		close(); // exit launcher
	}
	else
	{
		QMessageBox::critical(this,
		                      "Error starting executable",
		                      "Failed to start " + name + ": " + process.errorString(),
		                      QMessageBox::Ok,
		                      QMessageBox::Ok);
		return;
	}

}
