#include "StdInc.h"
#include "mainwindow_moc.h"
#include "ui_mainwindow_moc.h"

#include <QProcess>
#include <QDir>

#include "../lib/CConfigHandler.h"
#include "../lib/VCMIDirs.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/logging/CBasicLogConfigurator.h"

void MainWindow::load()
{
	console = new CConsoleHandler;
	CBasicLogConfigurator logConfig(VCMIDirs::get().userCachePath() / "VCMI_Launcher_log.txt", console);
	logConfig.configureDefault();

	CResourceHandler::initialize();
	CResourceHandler::load("config/filesystem.json");

	for (auto & string : VCMIDirs::get().dataPaths())
		QDir::addSearchPath("icons", pathToQString(string / "launcher" / "icons"));
	QDir::addSearchPath("icons", pathToQString(VCMIDirs::get().userDataPath() / "launcher" / "icons"));

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
	startExecutable(pathToQString(VCMIDirs::get().clientPath()));
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
		                      "Failed to start " + name + "\n"
		                      "Reason: " + process.errorString(),
		                      QMessageBox::Ok,
		                      QMessageBox::Ok);
		return;
	}

}
