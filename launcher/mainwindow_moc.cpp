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
#include "../lib/Languages.h"
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

void MainWindow::computeSidePanelSizes()
{
	QVector<QToolButton*> widgets = {
		ui->modslistButton,
		ui->settingsButton,
		ui->lobbyButton,
		ui->aboutButton,
		ui->startEditorButton,
		ui->startGameButton
	};

	for(auto & widget : widgets)
	{
		QFontMetrics metrics(widget->font());
		QSize iconSize = widget->iconSize();

		// this is minimal space that is needed for our button to avoid text clipping
		int buttonHeight = iconSize.height() + metrics.height() + 4;

		widget->setMinimumHeight(buttonHeight);
		widget->setMaximumHeight(buttonHeight * 1.2);
	}
}

MainWindow::MainWindow(QWidget * parent)
	: QMainWindow(parent), ui(new Ui::MainWindow)
{
	load(); // load FS before UI

	bool setupCompleted = settings["launcher"]["setupCompleted"].Bool();

	if (!setupCompleted)
		detectPreferredLanguage();

	updateTranslation(); // load translation

	ui->setupUi(this);
	
	connect(ui->lobbyView, &Lobby::enableMod, ui->modlistView, &CModListView::enableModByName);
	connect(ui->lobbyView, &Lobby::disableMod, ui->modlistView, &CModListView::disableModByName);
	connect(ui->modlistView, &CModListView::modsChanged, ui->lobbyView, &Lobby::updateMods);

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

	computeSidePanelSizes();

	bool h3DataFound = CResourceHandler::get()->existsResource(ResourceID("DATA/GENRLTXT.TXT"));

	if (h3DataFound && setupCompleted)
		ui->tabListWidget->setCurrentIndex(TabRows::MODS);
	else
		enterSetup();

	ui->settingsView->setDisplayList();
	
	if(settings["launcher"]["updateOnStartup"].Bool())
		UpdateDialog::showUpdateDialog(false);
}

void MainWindow::detectPreferredLanguage()
{
	auto preferredLanguages = QLocale::system().uiLanguages();

	std::string selectedLanguage;

	for (auto const & userLang : preferredLanguages)
	{
		logGlobal->info("Preferred language: %s", userLang.toStdString());

		for (auto const & vcmiLang : Languages::getLanguageList())
			if (vcmiLang.tagIETF == userLang.toStdString())
				selectedLanguage = vcmiLang.identifier;
	}

	logGlobal->info("Selected language: %s", selectedLanguage);

	if (!selectedLanguage.empty())
	{
		Settings node = settings.write["general"]["language"];
		node->String() = selectedLanguage;
	}
}

void MainWindow::enterSetup()
{
	ui->startGameButton->setEnabled(false);
	ui->startEditorButton->setEnabled(false);
	ui->lobbyButton->setEnabled(false);
	ui->settingsButton->setEnabled(false);
	ui->modslistButton->setEnabled(false);
	ui->tabListWidget->setCurrentIndex(TabRows::SETUP);
}

void MainWindow::exitSetup()
{
	Settings writer = settings.write["launcher"]["setupCompleted"];
	writer->Bool() = true;

	ui->startGameButton->setEnabled(true);
	ui->startEditorButton->setEnabled(true);
	ui->lobbyButton->setEnabled(true);
	ui->settingsButton->setEnabled(true);
	ui->modslistButton->setEnabled(true);
	ui->tabListWidget->setCurrentIndex(TabRows::MODS);
}

void MainWindow::switchToModsTab()
{
	ui->startGameButton->setEnabled(true);
	ui->tabListWidget->setCurrentIndex(TabRows::MODS);
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

CModListView * MainWindow::getModView()
{
	return ui->modlistView;
}

void MainWindow::on_modslistButton_clicked()
{
	switchToModsTab();
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

void MainWindow::on_aboutButton_clicked()
{
	ui->startGameButton->setEnabled(true);
	ui->tabListWidget->setCurrentIndex(TabRows::ABOUT);
}

void MainWindow::updateTranslation()
{
#ifdef ENABLE_QT_TRANSLATIONS
	std::string translationFile = settings["general"]["language"].String() + ".qm";
	logGlobal->info("Loading translation '%s'", translationFile);

	QVector<QString> searchPaths;

#ifdef Q_OS_IOS
	searchPaths.push_back(pathToQString(VCMIDirs::get().binaryPath() / "translation" / translationFile));
#else
	for(auto const & string : VCMIDirs::get().dataPaths())
		searchPaths.push_back(pathToQString(string / "launcher" / "translation" / translationFile));
	searchPaths.push_back(pathToQString(VCMIDirs::get().userDataPath() / "launcher" / "translation" / translationFile));
#endif

	for(auto const & string : boost::adaptors::reverse(searchPaths))
	{
		logGlobal->info("Searching for translation at '%s'", string.toStdString());
		if (translator.load(string))
		{
			logGlobal->info("Translation found");
			if (!qApp->installTranslator(&translator))
				logGlobal->error("Failed to install translator");
			return;
		}
	}

	logGlobal->error("Failed to find translation");

#endif
}
