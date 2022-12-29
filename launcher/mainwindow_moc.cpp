/*
 * mainwindow_moc.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "mainwindow_moc.h"
#include "ui_mainwindow_moc.h"

#include <QDir>

#include "../lib/CConfigHandler.h"
#include "../lib/VCMIDirs.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/logging/CBasicLogConfigurator.h"

#include "updatedialog_moc.h"
#include "main.h"

void MainWindow::load()
{
	// Set current working dir to executable folder.
	// This is important on Mac for relative paths to work inside DMG.
	QDir::setCurrent(QApplication::applicationDirPath());

#ifndef VCMI_IOS
	console = new CConsoleHandler();
#endif
	CBasicLogConfigurator logConfig(VCMIDirs::get().userLogsPath() / "VCMI_Launcher_log.txt", console);
	logConfig.configureDefault();

	CResourceHandler::initialize();
	CResourceHandler::load("config/filesystem.json");

#ifdef Q_OS_IOS
	QDir::addSearchPath("icons", pathToQString(VCMIDirs::get().binaryPath() / "icons"));
#else
	for(auto & string : VCMIDirs::get().dataPaths())
		QDir::addSearchPath("icons", pathToQString(string / "launcher" / "icons"));
	QDir::addSearchPath("icons", pathToQString(VCMIDirs::get().userDataPath() / "launcher" / "icons"));
#endif

	settings.init();
}

MainWindow::MainWindow(QWidget * parent)
	: QMainWindow(parent), ui(new Ui::MainWindow)
{
	load(); // load FS before UI
	updateTranslation(); // load translation

	ui->setupUi(this);

	//load window settings
	QSettings s(Ui::teamName, Ui::appName);

	auto size = s.value("MainWindow/Size").toSize();
	if(size.isValid())
	{
		resize(size);
	}
	auto position = s.value("MainWindow/Position").toPoint();
	if(!position.isNull())
	{
		move(position);
	}

#ifndef ENABLE_EDITOR
	ui->startEditorButton->hide();
#endif

	ui->tabListWidget->setCurrentIndex(0);

	ui->settingsView->isExtraResolutionsModEnabled = ui->modlistView->isExtraResolutionsModEnabled();
	ui->settingsView->setDisplayList();
	connect(ui->modlistView, &CModListView::extraResolutionsEnabledChanged,
		ui->settingsView, &CSettingsView::fillValidResolutions);
	
	if(settings["launcher"]["updateOnStartup"].Bool())
		UpdateDialog::showUpdateDialog(false);
}

void MainWindow::changeEvent(QEvent *event)
{
	if(event->type() == QEvent::LanguageChange)
	{
		ui->retranslateUi(this);
	}
	QMainWindow::changeEvent(event);
}

MainWindow::~MainWindow()
{
	//save window settings
	QSettings s(Ui::teamName, Ui::appName);
	s.setValue("MainWindow/Size", size());
	s.setValue("MainWindow/Position", pos());

	delete ui;
}

void MainWindow::on_startGameButton_clicked()
{
	startGame({});
}

void MainWindow::on_startEditorButton_clicked()
{
	startEditor({});
}

const CModList & MainWindow::getModList() const
{
	return ui->modlistView->getModList();
}

void MainWindow::on_modslistButton_clicked()
{
	ui->startGameButton->setEnabled(true);
	ui->tabListWidget->setCurrentIndex(TabRows::MODS);
}

void MainWindow::on_settingsButton_clicked()
{
	ui->startGameButton->setEnabled(true);
	ui->tabListWidget->setCurrentIndex(TabRows::SETTINGS);
}

void MainWindow::on_lobbyButton_clicked()
{
	ui->startGameButton->setEnabled(false);
	ui->tabListWidget->setCurrentIndex(TabRows::LOBBY);
}

void MainWindow::updateTranslation()
{
#ifdef ENABLE_QT_TRANSLATIONS
	std::string translationFile = settings["general"]["language"].String()+ ".qm";

	QVector<QString> searchPaths;

	for(auto const & string : VCMIDirs::get().dataPaths())
		searchPaths.push_back(pathToQString(string / "launcher" / "translation" / translationFile));
	searchPaths.push_back(pathToQString(VCMIDirs::get().userDataPath() / "launcher" / "translation" / translationFile));

	for(auto const & string : boost::adaptors::reverse(searchPaths))
	{
		if (translator.load(string))
		{
			if (!qApp->installTranslator(&translator))
				logGlobal->error("Failed to install translator");
			return;
		}
	}

	logGlobal->error("Failed to find translation");

#endif
}
